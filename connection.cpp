/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file connection.cpp
 * @author opeddm@gmail.com
 * @date 2012/04/25 20:26:06
 * @brief 
 *  
 **/

#include <nshead.h>
#include <errno.h>

//#include <sys/socket.h>
//#include <netinet/in.h>
#include <arpa/inet.h>

#include "event.h"
#include "connection.h"
#include "log.h"

// --------------------------------------------------------
// nshead_connection

#define CCAST(x) ((nshead_conn_t*)(x))

enum nshead_state_t {
    NSHEAD_STATE_IDLE       = 0,
    NSHEAD_STATE_HEAD       = 1,
    NSHEAD_STATE_BODY       = 2,
    NSHEAD_STATE_PROCESSING = 3,
};

typedef struct {
    connection_t conn;
    nshead_state_t state;
    int expected_input_len;
} nshead_conn_t;

static void nshead_conn_set_expected_input_len(connection_t *c, int size) {
    CCAST(c)->expected_input_len = size;
    event_set_read_watermark(c->ev, size, size * 10);
}

void nshead_conn_prepare_read(connection_t *c) {
    CCAST(c)->state = NSHEAD_STATE_HEAD;
    nshead_conn_set_expected_input_len(c, sizeof(nshead_t));
}

int nshead_conn_read(connection_t *c) {
    log_debug("in nshead_conn_read");
    while (event_get_input_len(c->ev) >= CCAST(c)->expected_input_len) {
        if (CCAST(c)->state == NSHEAD_STATE_HEAD) {
            //read nshead here
            int len;
            nshead_t head;

            len = event_read(c->ev, &head, sizeof(nshead_t));
            if (len < (int)sizeof(nshead_t)) {
                return -1;
            }
            if (head.magic_num != NSHEAD_MAGICNUM) {
                log_warning("nshead magic_num not match. expected:%x, actual:%x",
                    NSHEAD_MAGICNUM, head.magic_num);
                return -1;
            }

            CCAST(c)->state = NSHEAD_STATE_BODY;
            snprintf(c->provider, sizeof(c->provider), "%s", head.provider);
            c->log_id = head.log_id;

            len = head.body_len;
            nshead_conn_set_expected_input_len(c, len);

            //make sure buffer is enough
            if (!c->readbuf) {
                c->readbuf = (char*)malloc(len);
                c->readbuf_size = len;
            } else if (c->readbuf && c->readbuf_size < len) {
                free(c->readbuf);
                c->readbuf = (char*)malloc(len);
                c->readbuf_size = len;
            }

            log_debug("done head");
            log_debug("input_len: %d", event_get_input_len(c->ev));
        } else {
            //read body here
            int len;
            len = event_read(c->ev, c->readbuf, c->readbuf_size);
            if (len < (int)CCAST(c)->expected_input_len) {
                return -1;
            }

            CCAST(c)->state = NSHEAD_STATE_PROCESSING;
            return 1; //read done
        }
    }
    return 0; //not fininshed
}

void nshead_conn_write(connection_t *c, const void* buffer, int size) {
    log_debug("in nshead_conn_write");

    nshead_t head;
    memset(&head, 0, sizeof(head));
    head.log_id = c->log_id;
    snprintf(head.provider, sizeof(head.provider), "%s", c->provider);
    head.magic_num = NSHEAD_MAGICNUM;
    head.body_len = size;

    event_write(c->ev, &head, sizeof(head));
    event_write(c->ev, buffer, size);
}

// --------------------------------------------------------
// connection

static void connection_read_cb(event_t *ev, void *ctx);
static void connection_write_cb(event_t *ev, void *ctx);
static void connection_event_cb(event_t *ev, short what, void *ctx);

connection_t* connection_new(event_socket_t fd) {
    connection_t *c = (connection_t*)calloc(1,sizeof(nshead_conn_t));

	c->ev = event_new(fd);
	event_set_cb(c->ev, connection_read_cb, connection_write_cb, connection_event_cb, c);

    return c;
}

void connection_free(connection_t *c) {
    log_debug("in connection_free");
    event_free(c->ev);

    if (c->readbuf) {
        free(c->readbuf);
        c->readbuf = NULL;
        c->readbuf_size = 0;
    }
    free(c);
}

void connection_update_time(struct timeval *needupdate) {
    event_gettime(needupdate);
} 

void connection_reset_time(struct timeval *needupdate) {
    needupdate->tv_sec = 0;
    needupdate->tv_usec = 0;
} 

void connection_set_cb(connection_t *c,
    connection_cb_t connect_cb, connection_cb_t error_cb,
    connection_cb_t read_done_cb, connection_cb_t write_done_cb)
{
    c->connect_cb       = connect_cb;
    c->error_cb         = error_cb;
    c->read_done_cb     = read_done_cb;
    c->write_done_cb    = write_done_cb;
}

void connection_set_reqip(connection_t *c, struct in_addr addr) {
    inet_ntop(AF_INET, &addr, c->reqip, sizeof(c->reqip));
    log_debug("repip: %s", c->reqip);
}

void connection_set_provider(connection_t *c, const char *provider) {
    snprintf(c->provider, sizeof(c->provider), "%s", provider);
}

void connection_set_log_id(connection_t *c, unsigned int log_id) {
    c->log_id = log_id;
}

void connection_set_processing(connection_t *c, bool is_proccessing) {
    c->is_processing = is_proccessing;
}

void connection_prepare_read(connection_t *c) {
    log_debug("in connection_prepare_read");
    nshead_conn_prepare_read(c);
    event_enable_read(c->ev);
}

void connection_disable_read(connection_t *c) {
    event_disable_read(c->ev);
}

void connection_write(connection_t *c, const void *buffer, int size) {
    if (c->has_error) {
        return;
    }
    nshead_conn_write(c, buffer, size);
}

static void connection_call_error_cb(connection_t *c) {
    c->has_error = true;
    if (c->error_cb) {
        c->error_cb(c);
    }
}

static void connection_read_cb(event_t *ev, void *ctx)
{
    connection_t *c = (connection_t*) ctx;
    if (c->read_begin_time.tv_sec == 0 && c->read_begin_time.tv_usec == 0) {
        connection_update_time(&(c->read_begin_time));
    }
    int ret = nshead_conn_read(c);
    if (ret > 0) {
        if (c->read_done_cb) {
            c->read_done_cb(c);
        }
    } else if (ret < 0) {
        connection_call_error_cb(c);
    }
}

static void connection_write_cb(event_t *ev, void *ctx)
{
    connection_t *c = (connection_t*) ctx;
    if (c->write_done_cb) {
        c->write_done_cb(c);
    }
}

static void connection_event_cb(event_t *ev, short what, void *ctx)
{
    log_debug("in event_cb. what:0x%x", what);
    connection_t *c = (connection_t*) ctx;

    if (what & EVENT_CONNECTED) {
        log_debug("socket connected");
        if (c->connect_cb) {
            c->connect_cb(c);
        }
    } else if (what & EVENT_EOF) {
        log_trace("socket closed. eof");
        connection_call_error_cb(c);
    } else if (what & EVENT_ERROR) {
        log_warning("socket closed. errno:%d,%s", errno, strerror(errno));
        connection_call_error_cb(c);
    }
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
