/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file event.h
 * @author opeddm@gmail.com
 * @date 2012/06/04 02:47:49
 * @brief 
 *  
 **/

#ifndef  __EVENT_H_
#define  __EVENT_H_

#define EVENT_READING	0x01	/**< error encountered while reading */
#define EVENT_WRITING	0x02	/**< error encountered while writing */
#define EVENT_EOF		0x10	/**< eof file reached */
#define EVENT_ERROR		0x20	/**< unrecoverable error encountered */
#define EVENT_TIMEOUT	0x40	/**< user specified timeout reached */
#define EVENT_CONNECTED	0x80	/**< connect operation finished. */

typedef struct _event_t event_t;
typedef struct _event_timer_t event_timer_t;
typedef int event_socket_t;

extern struct event_base *base;
//basic
int event_init();
int event_loop();

typedef void (*read_cb_t)(event_t *e, void *ctx);
typedef void (*write_cb_t)(event_t *e, void *ctx);
typedef void (*event_cb_t)(event_t *e, short what, void *ctx);
typedef void (*accept_cb_t)(event_t *e, event_socket_t fd, struct sockaddr *a, int slen, void *ctx);
int event_set_cb(event_t *e, read_cb_t rcb, write_cb_t wcb, event_cb_t ecb, void *ctx);

event_t* event_new(event_socket_t fd);
int event_free(event_t *e);

//server
event_t* event_listen(accept_cb_t cb, struct sockaddr *addr, int socklen);
int event_listen_free(event_t *e);
int event_close_socket(event_socket_t fd);

//client
int event_connect(event_t *e, struct sockaddr *addr, int socklen);
int event_connect(event_t *e, const char *addr, int port);

//read & write
int event_read(event_t *e, void *buf, int size);
int event_write(event_t *e, const void *buf, int size);
int event_get_input_len(event_t *e);
int event_disable_read(event_t *e);
int event_enable_read(event_t *e);
int event_enable_write_callback(event_t *e);

int event_set_read_watermark(event_t *e, int low, int high);
int event_set_write_watermark(event_t *e, int low, int high);

//timer
typedef void (*timer_cb_t)(void *ctx);
event_timer_t* event_timer_new(timer_cb_t cb, unsigned long interval_usec, bool repeating, void *ctx);
int event_timer_free(event_timer_t *evt);

//custom event
typedef void (*custom_cb_t)(void *value);
int event_listen(const char *event, custom_cb_t cb);
int event_trigger(const char *event, void *param);

//time
int event_gettime(struct timeval *tv);

#endif  //__PROXY_EVENT_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
