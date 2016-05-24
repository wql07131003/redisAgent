/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file event.cpp
 * @author opeddm@gmail.com
 * @date 2012/06/04 02:58:40
 * @brief 
 *  
 **/

#include <pthread.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_struct.h>
#include <event2/listener.h>
#include <event2/util.h>

#include <bsl/containers/hash/bsl_hashmap.h>

#include "event.h"
#include "log.h"
#include "hash.h"

struct _event_t {
    struct bufferevent buf_event;
};

struct _event_timer_t {
    struct event *ev;
    timer_cb_t cb;
    void *ctx;
};

#define BE(x) ((struct bufferevent*)(x))

struct event_base *base;
static struct event *custom_event;

static bsl::hashmap <unsigned long, custom_cb_t> custom_event_map;

// multiple producer single comsumer FIFO list
typedef struct _ts_list_node_t {
    struct _ts_list_node_t *next;
    unsigned long key;
    void *data;
    bool is_placeholder;
} ts_list_node_t;

typedef struct _ts_list_t {
    ts_list_node_t *head;
    ts_list_node_t *tail;
} ts_list_t;

ts_list_t ts_list;
pthread_mutex_t list_wlock;

int list_produce(unsigned long key, void *data) {
    ts_list_node_t *cur_tail, *new_tail;
    new_tail = (ts_list_node_t*) malloc(sizeof(ts_list_node_t));
    if (new_tail == NULL) return -1;
    
    //start insert to list
    pthread_mutex_lock(&list_wlock);

    cur_tail = ts_list.tail;
    cur_tail->key = key;
    cur_tail->data = data;
    cur_tail->next = new_tail;

    new_tail->next = NULL;
    new_tail->is_placeholder = true;

    cur_tail->is_placeholder = false;

    ts_list.tail = new_tail;

    pthread_mutex_unlock(&list_wlock);
    return 0;
}

int list_consume(unsigned long *key, void **data) {
    if (ts_list.head->is_placeholder) return -1;
    ts_list_node_t *node = ts_list.head;

    *key = node->key;
    *data = node->data;

    ts_list.head = node->next;
    free(node);
    return 0;
}

int list_init() {
    ts_list_node_t *node = (ts_list_node_t*) malloc(sizeof(ts_list_node_t));
    if (node == NULL) return -1;
    node->is_placeholder = true;
    node->next = NULL;
    ts_list.head = node;
    ts_list.tail = node;

    pthread_mutex_init(&list_wlock, NULL);
    return 0;
}
// multiple producer single comsumer FIFO list END

static void custom_event_cb(evutil_socket_t sock, short res, void *ctx);
static void my_timer_cb(evutil_socket_t fd, short flags, void *ctx);

int event_init() {
	base = event_base_new();
	if (!base) {
		perror("event_base_new()");
		return 1;
	}
    custom_event = event_new(base, -1, EV_READ, custom_event_cb, NULL);

    if (list_init()) {
        return -1;
    }

    if (custom_event_map.create(1024) != 0) {
        log_fatal("create hashmap error");
        return -1;
    }

    return 0;
}

int event_loop() {
	event_base_dispatch(base);
    return 0;
}

int event_set_cb(event_t *e, read_cb_t rcb, write_cb_t wcb, event_cb_t ecb, void *ctx) {
    bufferevent_setcb(BE(e), (bufferevent_data_cb) rcb, (bufferevent_data_cb) wcb, (bufferevent_event_cb) ecb, ctx);
    return 0;
}

event_t* event_new(event_socket_t fd) {
    return (event_t*) bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
}

int event_free(event_t *e) {
    bufferevent_free(BE(e));
    return 0;
}

//server

event_t* event_listen(accept_cb_t cb, struct sockaddr *addr, int socklen) {
    struct evconnlistener *listener;
    listener = evconnlistener_new_bind(base, (evconnlistener_cb)cb, NULL,
	    LEV_OPT_CLOSE_ON_FREE|LEV_OPT_CLOSE_ON_EXEC|LEV_OPT_REUSEABLE,
	    -1, addr, socklen);
    return (event_t*) listener;
}

int event_listen_free(event_t *e) {
    evconnlistener_free((struct evconnlistener*) e);
    return 0;
}


int event_close_socket(event_socket_t fd) {
    evutil_closesocket(fd);
    return 0;
}

//client

int event_connect(event_t *e, struct sockaddr *addr, int socklen) {
    return bufferevent_socket_connect(BE(e), addr, socklen);
}

int event_connect(event_t *e, const char *addr, int port) {
    char port_buf[8];
    struct addrinfo hints, *servinfo;
    snprintf(port_buf, sizeof(port_buf), "%d", port);
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(addr, port_buf, &hints, &servinfo);
    if (ret) {
        log_warning("getaddrinfo failed. ret=%d", ret);
        return -1;
    }

    ret = event_connect(e, servinfo->ai_addr, servinfo->ai_addrlen);
    if (ret) {
        log_warning("event_connect failed");
        freeaddrinfo(servinfo);
        return -1;
    }
    freeaddrinfo(servinfo);

    return 0;
}

//read & write

int event_read(event_t *e, void *buf, int size) {
    return bufferevent_read(BE(e), buf, size);
}

int event_write(event_t *e, const void *buf, int size) {
    return bufferevent_write(BE(e), buf, size);
}

int event_get_input_len(event_t *e) {
    return evbuffer_get_length(bufferevent_get_input(BE(e)));
}

int event_disable_read(event_t *e) {
    bufferevent_disable(BE(e), EV_READ);
    return 0;
}

int event_enable_read(event_t *e) {
    bufferevent_enable(BE(e), EV_READ);
    return 0;
}

int event_enable_write_callback(event_t *e) {
    bufferevent_enable(BE(e), EV_WRITE);
    return 0;
}

int event_set_read_watermark(event_t *e, int low, int high) {
    bufferevent_setwatermark(BE(e), EV_READ, low, high);
    return 0;
}

int event_set_write_watermark(event_t *e, int low, int high) {
    bufferevent_setwatermark(BE(e), EV_WRITE, low, high);
    return 0;
}

//timer

static void my_timer_cb(evutil_socket_t fd, short flags, void *ctx) {
    event_timer_t *evt = (event_timer_t*) ctx;
    evt->cb(evt->ctx);
}

event_timer_t* event_timer_new(timer_cb_t cb, unsigned long interval_usec, bool repeating, void *ctx) {
    event_timer_t *evt = (event_timer_t*) malloc(sizeof(event_timer_t));
    evt->cb = cb;
    evt->ctx = ctx;
    struct event* ev = event_new(base, -1, (repeating? EV_PERSIST : EV_TIMEOUT), my_timer_cb, evt);
    struct timeval timeout;
    timeout.tv_sec = interval_usec/1000000;
    timeout.tv_usec = interval_usec%1000000;
    event_add(ev, &timeout);
    evt->ev = ev;
    return evt;
}

int event_timer_free(event_timer_t *evt) {
    event_free(evt->ev);
    free(evt);
    return 0;
}

//custom event

static void custom_event_cb(evutil_socket_t sock, short res, void *ctx) {
    unsigned long key;
    void *value;
    custom_cb_t cb;
    log_debug("i'm in custom event!");
    while (list_consume(&key, &value) == 0) {
        if (custom_event_map.get(key, &cb) == bsl::HASH_EXIST) {
            cb(value);
        }
    }
}

int event_listen(const char* event, custom_cb_t cb) {
    custom_event_map.set(strhash(event), cb, 1);
    return 0;
}

int event_trigger(const char *event, void *param) {
    if (list_produce(strhash(event), param)) {
        return -1;
    }
    event_active(custom_event, EV_READ, 0);
    return 0;
}

//time

int event_gettime(struct timeval *tv) {
    return evutil_gettimeofday(tv, NULL);
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
