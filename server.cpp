/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file server.cpp
 * @author opeddm@gmail.com
 * @date 2012/04/25 18:21:24
 * @brief 
 *  
 **/

#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <event2/listener.h>

#include "server.h"
#include "connection.h"
#include "log.h"
#include "meta.h"
#include "pool.h"
#include "req.h"
#include "event.h"
#include "ip_whitelist.h"

//static void accept_cb(event_t *listener, event_socket_t fd,
//    struct sockaddr *a, int slen, void *p);
static void accept_cb(evutil_socket_t, short revents, void *p);
static void error_cb(connection_t *c);
static void read_done_cb(connection_t *c);

extern comcfg::Configure *conf;
static int port;
static const char *whitelist_file = NULL;
static int check_interval;

// for log
char *pid;
char *omp_product;
char *omp_subsys; // equal to pid
char *omp_module;

static event_timer_t *enable_timer;
static event *listen_ev;

static struct sockaddr_storage listen_on_addr;
//static struct sockaddr_storage connect_to_addr;

whitelist_t *whitelist = NULL;
static void assign_whitelist(whitelist_t *w) {
    whitelist_t *tmp = whitelist;
    whitelist = w;
    free_whitelist(tmp);
}

static void whitelist_changed_event(void *data) {
    log_notice("authip updated");
    whitelist_t *w = (whitelist_t*) data;
    assign_whitelist(w);
}

int server_read_conf() {
    try {
        port = conf[0]["Server"]["Port"].to_int32();
        whitelist_file = strdup(conf[0]["Server"]["Whitelist"].to_cstr());
        check_interval = conf[0]["Server"]["WhitelistCheckInterval"].to_int32();
        pid = strdup(conf[0]["Server"]["Pid"].to_cstr());
        int err = 0;
        omp_product = strdup(conf[0]["Server"]["Product"].to_cstr(&err, "ksarch-redis"));
        omp_subsys = strdup(conf[0]["Server"]["Subsys"].to_cstr(&err, pid));
        omp_module = strdup(conf[0]["Server"]["Module"].to_cstr(&err, "redisproxy"));
    } catch (comcfg::ConfigException e) {
        log_fatal("Read conf error, errmsg: %s", e.what());
        return -1;
    }
    return 0;
}

int server_init() {
    int ret;
    log_debug("in server_init");
    if (server_read_conf()) {
        return -1;
    }

	memset(&listen_on_addr, 0, sizeof(listen_on_addr));
	//socklen = sizeof(listen_on_addr);
    struct sockaddr_in *sin = (struct sockaddr_in*)&listen_on_addr;
    sin->sin_port = htons(port);
    sin->sin_family = AF_INET;

    ret = whitelist_init(whitelist_file, check_interval);
    if (ret) {
        return -1;
    }
    assign_whitelist(load_whitelist());
    event_listen("whitelist_changed", whitelist_changed_event);

	return 0;
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx) {
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    log_warning("accept failed, error: %d, %s", err, evutil_socket_error_to_string(err));
}

void enable_accept(void *p) {
    int listenfd = (int)p;
    if (listen(listenfd, -1) == -1) {
        log_fatal("fail to listen: %s", strerror(errno));
        exit(-1);
    }
    if (-1 == event_add(listen_ev, NULL)) {
        log_fatal("fail to event_add");
        exit(-1);
    }
    if (enable_timer) {
        event_timer_free(enable_timer);
        enable_timer = NULL;
    }
}

int server_run() {
    log_trace("server_run");
//	event_t *listener;
//	listener = event_listen(accept_cb, (struct sockaddr*)&listen_on_addr, sizeof(listen_on_addr));
//
//    if (!listener) {
//        log_fatal("could not create listener, error: %s", strerror(errno));
//        return -1;
//    }
//    evconnlistener_set_error_cb((evconnlistener*)listener, accept_error_cb);
    // modified by cdz
    int listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd == -1) {
        log_fatal("fail to create listnen socket: %s", strerror(errno));
        exit(-1);
    }
    // set ADDRREUSE
    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        log_fatal("fail to set addrresule: %s", strerror(errno));
        exit(-1);
    }
    // set nonblock
    if ((optval = fcntl(listenfd, F_GETFL)) == -1) {
        log_fatal("fail to fcntl: %s", strerror(errno));
        exit(-1);
    }
    optval |= O_NONBLOCK;
    if (fcntl(listenfd, F_SETFL, optval) == -1) {
        log_fatal("fail to fcntl: %s", strerror(errno));
        exit(-1);
    }
    // bind
    if (bind(listenfd, (struct sockaddr*)&listen_on_addr, sizeof(listen_on_addr)) == -1) {
        log_fatal("fail to bind: %s", strerror(errno));
        exit(-1);
    }
    // add to ev_loop
    listen_ev = event_new(base, listenfd, EV_READ|EV_PERSIST, accept_cb, NULL);
    if (!listen_ev) {
        log_fatal("fail to event_new");
        exit(-1);
    }
    enable_accept((void*)listenfd);
	event_loop();
	//event_listen_free(listener);
	//event_base_free(base);
    return 0;
}

//static void accept_cb(event_t *listener, event_socket_t fd,
//    struct sockaddr *addr, int slen, void *p)
static void accept_cb(evutil_socket_t listenfd, short revents, void *p)
{
    log_debug("accept cb");
    struct sockaddr addr;
    socklen_t len = sizeof(addr);
    int fd = accept(listenfd, &addr, &len);
    if (fd == -1) {
        if (errno == EINTR) {
            return;
        } else if (errno == ENFILE || errno == EMFILE) {
            log_warning("proxy reach the limit: %s", strerror(errno));
            if (listen(listenfd, 0) == -1) {
                log_warning("fail to listen: %s", strerror(errno));
                exit(-1);
            }
            event_del(listen_ev);
            enable_timer = event_timer_new(enable_accept, 10000, false, (void*)listenfd);
            return;
        } else {
            log_fatal("accept error: %s", strerror(errno));
            exit(-1);
        }
    }

    if (!in_whitelist(((struct sockaddr_in*)&addr)->sin_addr)) {
        log_warning("Unauthorized access from %s", inet_ntoa(((struct sockaddr_in*)&addr)->sin_addr));
        close(fd);
        return;
    }

    connection_t *c = connection_new(fd);
    connection_update_time(&(c->accept_time));
    connection_set_cb(c, NULL, error_cb, read_done_cb, NULL);
    connection_set_reqip(c, ((struct sockaddr_in*)&addr)->sin_addr);
    connection_prepare_read(c);
}

static void error_cb(connection_t *c) {
    if (!c->is_processing) {
        log_debug("connection error when is not processing");
        connection_free(c);
    } else {
        log_warning("connection error when is processing");
    }
}

static void read_done_cb(connection_t *c) {
    connection_set_processing(c, true);
    connection_disable_read(c); // TODO will lost EVENT_EOF until process done
    connection_update_time(&(c->read_done_time));

    if (strcmp(c->provider, "__MONITOR__") == 0) {
        // just return empty data
        server_write(c, NULL, 0);
    } else {
        req_ready(c); // make this as callback. NEXT
    }
}

void server_write(connection_t *c, const void* buffer, int size) {
    connection_write(c, buffer, size);
}

void server_process_done(connection_t *c) {
    connection_set_processing(c, false);
    connection_reset_time(&(c->read_begin_time));
    connection_reset_time(&(c->read_done_time));
    connection_reset_time(&(c->backend_request_time));
    connection_reset_time(&(c->backend_reply_time));
    connection_reset_time(&(c->write_done_time));
    if (c->has_error) {
        log_warning("process done with connection error");
        connection_free(c);
    } else {
        // continue to process next request
        log_debug("process done normally");
        connection_prepare_read(c);
    }
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
