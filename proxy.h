/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file proxy.h
 * @author opeddm@gmail.com
 * @date 2012/06/04 02:49:14
 * @brief 
 *  
 **/




#ifndef  __PROXY_H_
#define  __PROXY_H_

#include "event.h"

#include "log.h"
#include "mem.h"
#include "store_error.h"

#include <time.h>

#define LD(x) ((log_data_t*) e_get_data(x))
#define ADD_LOG_DATA(base, key, value) log_data_push(LD(base), key, value)
#define ADD_LOG_DATA_INT(base, key, value) log_data_push_int(LD(base), key, value)
#define ADD_LOG_DATA_UINT(base, key, value) log_data_push_uint(LD(base), key, value)
#define ADD_LOG_DATA_DOUBLE(base, key, value) log_data_push_double(LD(base), key, value)

#define MAX_COPY_NUM        64      //ÿ����Ƭ֧�����64������
#define MAX_PID_LEN         32      //pid��󳤶�
#define MAX_UNAME_LEN       32      //�û�����󳤶�
#define MAX_TK_LEN          32      //token��󳤶�
#define MAX_DB_NAME_LEN     32      //db_name��󳤶�
#define MAX_USER_NUM        1024    //���֧��1024���û�
#define MACHINE_ROOM_SIZE   32      //��������//��Ҫ��slb::SLB_MACHINE_ROOM_BUF_SIZE����һ��
#define LOW_CONNECT_THRESHOLD  10   //����������ֵ��Ϊ��������������10��֮һ����ʾ����ǰ������С�������ֵʱ���ᶨʱ�Զ������µ�����

typedef struct backend_req backend_req_t;
typedef struct _connection_t connection_t;
typedef struct _sub_req_t sub_req_t;

typedef struct _req_t {
    connection_t *conn;

    //req data
    unsigned int log_id; //mirror from conn
    char *readbuf; //mirror from conn
    char *writebuf;
    int readbuf_size; //mirror from conn
    int writebuf_size;

    bool is_write;
    int key_num;

    void *backend_data;

    backend_req_t **backend_req;
    int shard_count;

    const char *uname;
    const char *tk;

    event_timer_t *timeout_event;
    bool timeout;
    unsigned long req_id;
    struct timeval start_time;

    int err_no;
    char err_msg[STORE_ERR_MSG_LEN];

    sub_req_t *sub_req_not_done;

    bool req_done_called;
} req_t;

struct backend_req {
    const char *key;
    void *value;
    int shard_key;
};

typedef struct _handle_t handle_t;

typedef void (*backend_connect_cb_t) (handle_t *h, int status);
typedef void (*backend_destroy_cb_t) (handle_t *h);

typedef struct _handle_t {
    const char *addr;
    int port;
    void *backend_conn;
    backend_connect_cb_t connect_callback;
    backend_destroy_cb_t destroy_callback;
    void *pool_data;
} handle_t;

typedef int (*backend_cb_t) (req_t* r, int err_no, void *priv);

typedef int (*pool_create_cb_t) (handle_t *handle, backend_connect_cb_t connect_callback, backend_destroy_cb_t destroy_callback);
typedef int (*pool_destroy_cb_t) (handle_t *handle, backend_destroy_cb_t callback);
//���ӳ�ע�ắ������backend��poolע�����ڴ������Ӻ��������ӵķ���
int pool_register(pool_create_cb_t create_cb, pool_destroy_cb_t destroy_cb); 

typedef int (*init_func_t)();

#include "funcdef.h"

#endif  //__PROXY_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
