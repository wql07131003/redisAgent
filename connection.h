/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file connection.h
 * @author opeddm@gmail.com
 * @date 2012/04/25 23:34:29
 * @brief 
 *  
 **/

#ifndef  __PROTOCOL_H_
#define  __PROTOCOL_H_

#include "event.h"

typedef struct _connection_t connection_t;
typedef void (*connection_cb_t) (connection_t *c);

typedef struct _connection_t {
    event_t *ev;

    char *readbuf;
    int readbuf_size;

    bool is_processing;
    bool has_error;

    char reqip[16];
    char provider[16];
    unsigned int log_id;

    connection_cb_t connect_cb;
    connection_cb_t error_cb;
    connection_cb_t read_done_cb;
    connection_cb_t write_done_cb;

    struct timeval accept_time;
    struct timeval read_begin_time;
    struct timeval read_done_time;
    struct timeval backend_request_time;
    struct timeval backend_reply_time;
    struct timeval write_done_time;

    void *priv; // user private data
} connection_t;

connection_t* connection_new(event_socket_t fd);
void connection_free(connection_t *c);

void connection_set_cb(connection_t *c,
    connection_cb_t connect_cb, connection_cb_t error_cb,
    connection_cb_t read_done_cb, connection_cb_t write_done_cb);
void connection_set_reqip(connection_t *c, struct in_addr addr);
void connection_set_provider(connection_t *c, const char *provider);
void connection_set_log_id(connection_t *c, unsigned int log_id);
void connection_set_processing(connection_t *c, bool is_proccessing);
void connection_update_time(struct timeval*);
void connection_reset_time(struct timeval*);

void connection_prepare_read(connection_t *c);
void connection_disable_read(connection_t *c);
void connection_write(connection_t *c, const void *buffer, int size);

#endif  //__PROTOCOL_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
