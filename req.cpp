/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file req.cpp
 * @author opeddm@gmail.com
 * @date 2012/05/07 15:19:21
 * @brief 
 *  
 **/

#include <time.h>
#include <stdlib.h>

#include "req.h"
#include "server.h"
#include "log.h"
#include "meta.h"
#include "hash.h"
#include "router.h"
#include "mem.h"
#include "event.h"
#include "connection.h"

//start sub_req
enum sub_req_state_t {
    BEFORE_REQUEST_POOL,
    POOL_RETURNED,
    BACKEND_RETURNED
};
typedef struct _sub_req_t {
    backend_req_t **backend_req;
    int num;
    req_t *r;//req_t
    handle_t *handle;

    sub_req_state_t state;

    //del the node when sub_req is sent, so, only the sub_reqs not yet send are in the sub_req_queue
    struct _sub_req_t *prev;
    struct _sub_req_t *next;

    void *pool_waiting_node;
} sub_req_t;

void _req_subreq_queue_push(sub_req_t **list, sub_req_t *sr) {
    if (*list == NULL) {
        *list = sr;
    } else {
        sr->prev = NULL;
        sr->next = *list;
        (*list)->prev = sr;
        *list = sr;
    }
}

sub_req_t* _req_subreq_queue_pop(sub_req_t **list) {
    if (*list == NULL) {
        return NULL;
    } else {
        sub_req_t *temp = *list;
        *list = (*list)->next;
        if (*list) {
            (*list)->prev = NULL;
        }
        return temp;
    }
}

void _req_subreq_queue_rm(sub_req_t **list, sub_req_t *sr) {
    if (sr->prev) sr->prev->next = sr->next;
    if (sr->next) sr->next->prev = sr->prev;
    if (sr == *list) *list = sr->next;
}
//end sub_req

int req_on_shard_done (req_t* r, int err_no, void *priv);
int req_on_getshards (backend_req_t **backend_req, int num, void *user_data, pool_info_t* &pool);
void req_timeout(void *param);

req_t *req_create() {
    static unsigned long req_id = 0;
    req_t *r = (req_t*) ecalloc_new(sizeof(req_t));
    if (r) {
        r->req_id = req_id++;
    }
    return r;
}

int req_ready(connection_t *c) {
    log_debug("in req_ready");
    //create req_t
    req_t *r = req_create();
    r->conn = c;
    //c->data = r;

    event_gettime(&r->start_time);

    log_data_t *ld = log_data_new();
    if (r == NULL || ld == NULL) {
        log_fatal("create object failed");
        return 0;
    }
    e_set_data(r, ld);

    ADD_LOG_DATA(r, "reqip", c->reqip);
    ADD_LOG_DATA(r, "reqsvr", c->provider);
    ADD_LOG_DATA_UINT(r, "logid", c->log_id);
    ADD_LOG_DATA_UINT(r, "reqid", r->req_id);

    // for log report
    ADD_LOG_DATA(r, "pid", pid);

    // for OMP
    ADD_LOG_DATA(r, "product", omp_product);
    ADD_LOG_DATA(r, "subsys", omp_subsys);
    ADD_LOG_DATA(r, "module", omp_module);

    //initialize req_t
    r->log_id = c->log_id;
    r->readbuf = r->conn->readbuf;
    r->readbuf_size = r->conn->readbuf_size;
    r->timeout = false;
    STORE_CLEAR_ERR(r);
    r->req_done_called = false;
    r->timeout_event = NULL;
    if (req_process(r)) {
        req_fail(r);
    }
    return 0;
}

int req_process(req_t *r) {
    rlog_debug(LD(r), "in server process");

    //register timeout event
    struct timeval timeout;
    int ret;
    meta_get_timeout(timeout.tv_sec, timeout.tv_usec);
    r->timeout_event = event_timer_new(req_timeout, timeout.tv_sec * 1000000 + timeout.tv_usec, false, r);

    ret = backend_req_init(r);
    if (ret < 0) {
        rlog_warning(LD(r), "backend_req_init error");
        return -1;
    }
    ret = backend_parse(r);
    rlog_debug(LD(r), "backend_parse return %d", ret);
    if (ret < 0) {
        rlog_warning(LD(r), "backend_parse error");
        return -1;
    }

    rlog_debug(LD(r), "after backend parse, keynum=%d is_write=%d, key0=%s", r->key_num, r->is_write, (*r->backend_req)->key);

    //authenticate
    ret = meta_auth_check(r->uname, r->tk, r->conn->reqip);
    if (ret <= 0 || (ret == AUTH_READ_ONLY && r->is_write)) {
        rlog_warning(LD(r), "Unauthorized access!");
        STORE_SET_ERR_UNAUTHORIZED(r, "invalid uname(%s) or tk(%s)", r->uname, r->tk);
        return -1;
    }
    //query meta
    r->shard_count = 0;
    r->sub_req_not_done = NULL;
    ret = router_acquire_shards(
            r->backend_req,
            r->key_num,
            !r->is_write,
            r, req_on_getshards);
    rlog_debug(LD(r), "return from router_acquire_shards, %d", ret);
    if (ret) {
        rlog_warning(LD(r), "router_acquire_shards failed");
        STORE_SET_ERR_SERVICE_UNAVAILABLE(r, "cannot fetch shard");
        return -1;
    }
    return 0;
}

int req_on_getshards (backend_req_t **backend_req, int num, void *user_data, pool_info_t* &pool) {
    req_t *r = (req_t*) user_data;
    rlog_debug(LD(r), "req_on_getshards");
    int balanceCode;
    int ret;
    
    // r->shard_count++;
    // now , in router.cpp: router_acquire_shards

    //form sub_req_t
    sub_req_t *sr = (sub_req_t*)emalloc(r, sizeof(sub_req_t));
    sr->backend_req = backend_req;
    sr->num = num;
    sr->r = r;
    sr->prev = NULL;
    sr->next = NULL;
    sr->pool_waiting_node = NULL;
    sr->state = BEFORE_REQUEST_POOL;

    _req_subreq_queue_push(&(r->sub_req_not_done), sr);

    ret = pool_fetch_connect(pool, sr, sr->pool_waiting_node, req_on_acquired, balanceCode);
    if (ret) {
        //fail this subreq
        STORE_SET_ERR_SERVICE_UNAVAILABLE(r, "cannot fetch connection");
        req_on_shard_done(r, 1, NULL);
        return -1;
    }

    return 0;
}

int req_on_acquired(handle_t *handle, void *user_data) {
    sub_req_t *sr = (sub_req_t*)user_data;

    sr->handle = handle;
    req_t *r = sr->r;
    backend_req_t **breq = sr->backend_req;
    int num = sr->num;
    sr->state = POOL_RETURNED;

    //profile code, trace the time here
    if (log_has_trace()) {
        struct timeval get_conn_time;
        event_gettime(&get_conn_time);
        long cur_proctime;
        cur_proctime = (get_conn_time.tv_sec - r->start_time.tv_sec) * 1000000 + (get_conn_time.tv_usec - r->start_time.tv_usec);
        rlog_trace(LD(r), "got_connection_for_shard,send_request shardid=%d addr=%s port=%d cur_proctime_us=%ld key_num=%d", (*sr->backend_req)->shard_key, handle->addr, handle->port, cur_proctime, num);
    }

    //print the first key addr and port 
    ADD_LOG_DATA(r, "addr", handle->addr);
    ADD_LOG_DATA_INT(r, "port", handle->port);

    if (r->conn->backend_request_time.tv_sec == 0 && r->conn->backend_request_time.tv_usec == 0) {
        // mark the first shard's processing time
        connection_update_time(&(r->conn->backend_request_time));
    }
    //_req_subreq_queue_pop(r, sr);
    if (backend_call(r, breq, num, handle, req_on_shard_done, sr)) {
        //fail this subreq
        sr->state = BACKEND_RETURNED;
        return -1;
    }

    return 0;
}

int req_cleanup(req_t *r) {
    rlog_debug(LD(r), "req_cleanup");
    if (r->timeout_event) {
        event_timer_free(r->timeout_event);
        r->timeout_event = NULL;
    }

    log_data_free((log_data_t*)e_get_data(r));
    efree_all(r);
    return 0;
}

int req_timeout_connect_cleanup (req_t *r) {
    sub_req_t *sr;

    while (sr = _req_subreq_queue_pop(&(r->sub_req_not_done))) {
        if (sr->state == BEFORE_REQUEST_POOL) {
            rlog_debug(LD(r), "req_timeout_connect_cleanup, key:%s", sr->backend_req[0]->key);
            _pool_waiting_queue_rm (sr->pool_waiting_node);
            {
                server_info_t *server = ((waiting_queue_t*)sr->pool_waiting_node)->server;
                pool_info_t *pool = server->pool;
                //setServerArg
                forum::slb_talk_returninfo_t *talk = 
                    (forum::slb_talk_returninfo_t*)calloc(1, sizeof(forum::slb_talk_returninfo_t));
                struct timeval timeout;
                meta_get_timeout(timeout.tv_sec, timeout.tv_usec);
                int timecost = timeout.tv_sec * 1000000 + timeout.tv_usec;
                talk->realreadtime = timecost;
                talk->realwritetime = timecost;
            
                pool->m_strategy->setServerArg(server, talk);
                free(talk);
            }
            efree(sr);
            sr = NULL;
            r->shard_count--;
        } else if (sr->state == POOL_RETURNED) {
            backend_timeout(r, sr->backend_req, sr->num, sr->handle, req_on_shard_done, sr);
            rlog_debug(LD(r), "backend_timeout");
            r->shard_count--;
        } else {
            rlog_warning(LD(r), "req_timeout_connect_cleanup, state = BACKEND_RETURNED");
        }

    }
    return 0;
}

int req_on_shard_done (req_t *r, int err_no, void *priv) {
    sub_req_t *sr = NULL;
    handle_t *handle = NULL;
    if (priv != NULL) {
        sr = (sub_req_t *)priv;
        if (sr->handle != NULL) {
            handle = sr->handle;
        }

        //profile code, trace the time here
        if (log_has_trace()) {
            struct timeval shard_done_time;
            event_gettime(&shard_done_time);
            long cur_proctime;
            cur_proctime = (shard_done_time.tv_sec - r->start_time.tv_sec) * 1000000 + (shard_done_time.tv_usec - r->start_time.tv_usec);
            rlog_trace(LD(r), "shard_request_done shardid=%d cur_proctime_us=%ld", (*sr->backend_req)->shard_key, cur_proctime);
        }

        sr->state = BACKEND_RETURNED;
        _req_subreq_queue_rm(&(r->sub_req_not_done), sr);
    }

    //release_connect
    if (handle != NULL) {
        pool_release_connect(handle, err_no);
    }

    //del subreq node in subreq queue
    if (sr != NULL) {
        efree(sr);
        sr = NULL;
    }

    r->shard_count--;
    if (r->shard_count == 0) {
        if (r->timeout) {
            //timeout process
            //req_cleanup(r);
        } else {
            req_done(r);
        }
    }
    return 0;
}

void req_timeout(void *param) {
    req_t *r = (req_t*)param;
    r->timeout = true;
    rlog_warning(LD(r), "req_timeout");
    STORE_SET_ERR_SERVICE_TIMEOUT(r, "service(%s) timeout", "backend");
    req_timeout_connect_cleanup(r);
    req_fail(r);
}

int req_fail(req_t *r) {
    if (!r->err_no) {
        STORE_SET_ERR_INTERNAL_ERROR(r, "unknown error");
    }
    req_done(r);
    return 0;
}

int req_done(req_t *r) {
    if (r->req_done_called) {
        rlog_trace(LD(r), "req_done called twice!");
        return 0;
    }
    r->req_done_called = true;

    connection_update_time(&(r->conn->backend_reply_time));

    backend_req_done(r);
    if (r->writebuf) {
        server_write(r->conn, r->writebuf, r->writebuf_size);
    }
    ADD_LOG_DATA_UINT(r, "accept_time", r->conn->accept_time.tv_sec * 1000000 + r->conn->accept_time.tv_usec);
    ADD_LOG_DATA_UINT(r, "read_begin_time", r->conn->read_begin_time.tv_sec * 1000000 + r->conn->read_begin_time.tv_usec);
    ADD_LOG_DATA_UINT(r, "read_done_time", r->conn->read_done_time.tv_sec * 1000000 + r->conn->read_done_time.tv_usec);
    ADD_LOG_DATA_UINT(r, "backend_request_time", r->conn->backend_request_time.tv_sec * 1000000 +
                                                 r->conn->backend_request_time.tv_usec);
    ADD_LOG_DATA_UINT(r, "backend_reply_time", r->conn->backend_reply_time.tv_sec * 1000000 +
                                               r->conn->backend_reply_time.tv_usec);
    struct timeval end_time;
    event_gettime(&end_time);
    ADD_LOG_DATA_UINT(r, "write_time", end_time.tv_sec * 1000000 + end_time.tv_usec);
    int64_t short_connection_cost_us = (end_time.tv_sec - r->conn->accept_time.tv_sec) * 1000000 +
                                       (end_time.tv_usec - r->conn->accept_time.tv_usec);
    if (short_connection_cost_us < 0) short_connection_cost_us = 0;
    ADD_LOG_DATA_INT(r, "short_connection_cost_us", short_connection_cost_us);

    int64_t long_connection_cost_us = (end_time.tv_sec - r->conn->read_begin_time.tv_sec) * 1000000 + 
                                      (end_time.tv_usec - r->conn->read_begin_time.tv_usec);
    if (long_connection_cost_us < 0) long_connection_cost_us = 0;
    ADD_LOG_DATA_INT(r, "long_connection_cost_us", long_connection_cost_us);

    ADD_LOG_DATA_UINT(r, "start_time", r->start_time.tv_sec * 1000000 + r->start_time.tv_usec);
    int64_t cost_us = (end_time.tv_sec - r->start_time.tv_sec) * 1000000 + (end_time.tv_usec - r->start_time.tv_usec);
    if (cost_us < 0) cost_us = 0;
    ADD_LOG_DATA_INT(r, "proctime_us", cost_us); // temporary when omp test, will be deleted in future
    ADD_LOG_DATA_INT(r, "cost", cost_us);

    ADD_LOG_DATA_UINT(r, "writebuf_size", r->writebuf_size);
    if (r->is_write) {
        ADD_LOG_DATA_UINT(r, "is_write", 1);
    } else {
        ADD_LOG_DATA_UINT(r, "is_write", 0);
    }
    ADD_LOG_DATA_UINT(r, "err_no", r->err_no);
    ADD_LOG_DATA(r, "err_msg", r->err_msg);
    if (r->err_no) {
        rlog_notice(LD(r), "request_done_with_error");
    } else {
        rlog_notice(LD(r), "request_done");
    }

    //r->conn->data = NULL;
    server_process_done(r->conn);

    req_cleanup(r);
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
