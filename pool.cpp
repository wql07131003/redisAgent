/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file pool.cpp
 * @author opeddm@gmail.com
 * @date 2012/05/02 17:49:32
 * @brief 
 *  
 **/

#include "pool.h"
#include <bsl/var/Dict.h>
#include <bsl/var/Array.h>
#include <bsl/var/String.h>

extern comcfg::Configure *conf;
pool_cb_t m_poolCb;

void _pool_strategy_copy_serv(void *xxServer, forum::slb_server_t *server) {
    server_info_t * serv = static_cast<server_info_t *>(xxServer);
    server->enable = true;
    server->isHealthy = true;
    server->id = serv->m_serverIdx;
    server->port = serv->port;
    server->ip = serv->addr;
    server->tag = NULL;
    log_debug("in _pool_strategy_copy_serv, id=%d", server->id);
}

int pool_register(pool_create_cb_t create_cb, 
        pool_destroy_cb_t destroy_cb) {
    //callback func ptr
    m_poolCb.m_createCb = create_cb;
    m_poolCb.m_destroyCb = destroy_cb;

    return 0;
}

int _pool_strategy_init(pool_info_t *pool) {
    //strategy init
    pool->m_strategyBuf = (char*)calloc(1, forum::SLB_DEF_BUF_SIZE);
    pool->m_strategy = new forum::CamelSuperStrategy(pool->m_strategyBuf, forum::SLB_DEF_BUF_SIZE);

    if (NULL == pool->m_strategy) {
        log_warning("initStrategy fail service");
        return -1;
    }
    pool->m_strategy->setCallBack(_pool_strategy_copy_serv);

    int ret;
    if (conf[0]["SuperStrategy"].selfType() 
            == comcfg::CONFIG_ERROR_TYPE) {
        log_warning("conf not exist");
        // empty configure
        bsl::var::Dict emptyDict;
        comcfg::Configure tempConf;
        tempConf.loadIVar(emptyDict);
        ret = pool->m_strategy->load(tempConf);
    } else {
        ret = pool->m_strategy->load(conf[0]["SuperStrategy"]);
        log_debug("conf exist");
    }

    return ret;
}

int _pool_unconnect_queue_get(server_info_t &server) {
 
    if (server.m_unconnectQueueHead == NULL) {
        return -1;
    }

    unconnect_queue_t *ucq = server.m_unconnectQueueHead;
    int connectIdx = ucq->connect_id;
    if (server.m_unconnectQueueHead == 
            server.m_unconnectQueueTail) {
        server.m_unconnectQueueTail = NULL;
    }
    server.m_unconnectQueueHead = 
            server.m_unconnectQueueHead->next; 
    if (server.m_unconnectQueueHead) {
        server.m_unconnectQueueHead->prev = NULL;
    }
    server.m_unconnectCount --;
    ucq->next = NULL;
    ucq->prev = NULL;
    return connectIdx;
}

int _pool_unconnect_queue_put(server_info_t &server, connect_queue_t *cq) {
    cq->next = NULL;
    cq->prev = NULL;
    if (server.m_unconnectQueueHead == NULL) {
        server.m_unconnectQueueHead = cq;
        server.m_unconnectQueueTail = cq;
    }
    else {
        server.m_unconnectQueueTail->next = cq;
        cq->prev = server.m_unconnectQueueTail;
        server.m_unconnectQueueTail = cq;
    }
    server.m_unconnectCount ++;

    return 0;
}

int _pool_connect_queue_get(server_info_t &server) {
    if (server.m_connectQueueHead == NULL) {
        return -1;
    }
    connect_queue_t *cq = server.m_connectQueueHead;
    int connectIdx = cq->connect_id;

    if (server.m_connectQueueHead == 
            server.m_connectQueueTail) {
        server.m_connectQueueTail = NULL;
    }
    server.m_connectQueueHead = cq->next; 
    if (server.m_connectQueueHead) {
        server.m_connectQueueHead->prev = NULL;
    }
    server.m_freeConnectCount --;
    cq->next = NULL;
    cq->prev = NULL;
    return connectIdx;
}

int _pool_connect_queue_put(server_info_t &server, connect_queue_t *cq) {
    cq->next = NULL;
    cq->prev = NULL;
    if (server.m_connectQueueHead == NULL) {
        server.m_connectQueueHead = cq;
        server.m_connectQueueTail = cq;
    }
    else {
        server.m_connectQueueTail->next = cq;
        cq->prev = server.m_connectQueueTail;
        server.m_connectQueueTail = cq;
    }

    server.m_freeConnectCount ++;

    return 0;
}

int _pool_connect_queue_rm(server_info_t &server, connect_queue_t *cq) {
    if (server.m_connectQueueHead == cq) {
        server.m_connectQueueHead = cq->next;
    }
    if (server.m_connectQueueTail == cq) {
        server.m_connectQueueTail = cq->prev;
    }
    if (cq->next != NULL) {
        cq->next->prev = cq->prev;
    }
    if (cq->prev != NULL) {
        cq->prev->next = cq->next;
    }

    cq->next = NULL;
    cq->prev = NULL;

    server.m_freeConnectCount --;

    return 0;
}

void* _pool_waiting_queue_get(server_info_t &server, pool_call_cb_t &cb) {
    waiting_queue_t* wq = server.m_waitingQueueHead;
    //waiting queue empty
    if (wq == NULL) {
        log_debug("_pool_waiting_queue_get, pid=%d sid=%d w_count=0", server.pool->m_poolIdx, server.m_serverIdx);
        return NULL;
    }

    //queue process
    if (server.m_waitingQueueHead == server.m_waitingQueueTail) {
        //only one waiting in the queue
        server.m_waitingQueueHead = NULL;
        server.m_waitingQueueTail = NULL;
    }
    else {
        server.m_waitingQueueHead = server.m_waitingQueueHead->next;
        server.m_waitingQueueHead->last = NULL;
    }
    log_debug("_pool_waiting_queue_get, pid=%d sid=%d w_count=%d->%d", server.pool->m_poolIdx, server.m_serverIdx, server.m_waitingCount, server.m_waitingCount-1);
    server.m_waitingCount --;

    //get sub req
    void *sub_req = wq->sub_req;
    cb = wq->cb;
    free(wq);

    return sub_req;
}

int _pool_waiting_queue_rm(void* waiting_queue) {
    if (waiting_queue == NULL) return 0;
    waiting_queue_t *wq = (waiting_queue_t *)waiting_queue;

    server_info_t *server = wq->server;

    log_debug("_pool_waiting_queue_rm, pid=%d sid=%d w_count=%d->%d", server->pool->m_poolIdx, server->m_serverIdx, server->m_waitingCount, server->m_waitingCount-1);

    if (wq->last != NULL) {
        wq->last->next = wq->next;
    }
    if (wq->next != NULL) {
        wq->next->last = wq->last;
    }
    if (wq == server->m_waitingQueueHead) {
        server->m_waitingQueueHead = wq->next;
    }
    if (wq == server->m_waitingQueueTail) {
        server->m_waitingQueueTail = wq->last;
    }

    free(wq);
    server->m_waitingCount --;

    //waitingQueue为空时，判断当前pool是否处于destroy状态
    if (server->m_destroyFlag && server->m_waitingCount == 0 &&
            server->m_freeConnectCount + server->m_unconnectCount == server->pool->m_connectPerServer) {
        //若waitingQueue为空且处于destroy状态，则可以进行当前free连接的销毁工作；
        //当前还在服务的连接，会在交互完成后再次进入此判断
        //销毁回调会对当前server的连接数量进行计数和判断
        //若当前server所有连接均已完成销毁，则调用_pool_server_destroy
        log_debug("_pool_waiting_queue_rm, call _pool_server_destroy, sid=%d  fc=%d uc=%d", server->m_serverIdx, server->m_freeConnectCount, server->m_unconnectCount);
        _pool_server_destroy(*server);
    }

    return server->m_waitingCount;
}

int _pool_waiting_queue_put(server_info_t &server, waiting_queue_t *wq) {

    log_debug("_pool_waiting_queue_put, pid=%d sid=%d w_count=%d->%d", server.pool->m_poolIdx, server.m_serverIdx, server.m_waitingCount, server.m_waitingCount+1);
    if (server.m_waitingQueueHead == NULL) {
        server.m_waitingQueueHead = wq;
        server.m_waitingQueueTail = wq;
        wq->next = NULL;
        wq->last = NULL;
    }
    else {
        wq->last = server.m_waitingQueueTail;
        wq->next = NULL;

        wq->last->next = wq;

        server.m_waitingQueueTail = wq;
    }
    server.m_waitingCount ++;

    return 0;
}

void _server_connect_monitor (void *param) {
    server_info_t *server = (server_info_t*)param;
    pool_info_t *pool = server->pool;
    if (server->m_connectCount < pool->m_lowConnectPerServer) {
        _pool_create_new_connect(*server);
    }
}

int pool_build(pool_info_t *pool, int pool_idx, 
        const pool_service_conf_t serverConfs[], 
        int serverCount, int connectPerServer, int cTimeOutMs, 
        char *machine_room, int connectRetry, bool longConnect) {

    int ret = _pool_strategy_init(pool);

    //pool init
    //record info
    pool->m_poolIdx = pool_idx;
    pool->m_serverCount = serverCount;
    pool->m_machineRoom = machine_room;
    pool->m_connectPerServer = connectPerServer;
    pool->m_lowConnectPerServer = connectPerServer/LOW_CONNECT_THRESHOLD>0 ? connectPerServer/LOW_CONNECT_THRESHOLD : 1;
    pool->m_connectRetry = connectRetry;
    pool->m_longConnect = longConnect;
    pool->m_cTimeOutMs = cTimeOutMs;
    pool->m_destroy_cb = NULL;

    pool->m_serverArr = (server_info_t*)calloc(serverCount, sizeof(server_info_t));

    for (int i=0; i<serverCount; i++) {
        const pool_service_conf_t conf = serverConfs[i];
        server_info_t& server = pool->m_serverArr[i];

        server.m_serverIdx = i;
        server.pool = pool;

        strncpy(server.addr, conf.addr, sizeof(server.addr));
        server.port = conf.port;

        log_debug("add a server port:%d", server.port);

        server.m_connectQueueHead = NULL;
        server.m_connectQueueTail = NULL;
        server.m_waitingQueueHead = NULL;
        server.m_waitingQueueTail = NULL;
        server.m_unconnectQueueHead = NULL;
        server.m_unconnectQueueTail = NULL;

        server.m_connectCount = 0;
        server.m_unconnectCount = 0;
        server.m_freeConnectCount = 0;
        server.m_waitingCount = 0;
        server.m_destroyFlag = false;

        //register connect monitor event
        struct timeval one_sec;
        one_sec.tv_sec = 1;
        one_sec.tv_usec = 0;
        server.connect_monitor_event = event_timer_new(_server_connect_monitor, 1000000, true, &server);
        
        server.m_connectArr = (connect_info_t**)calloc(connectPerServer, sizeof(connect_info_t*));
        for (int j=0; j<connectPerServer; j++) {
            server.m_connectArr[j] = (connect_info_t*)calloc(1, sizeof(connect_info_t));
            server.m_connectArr[j]->m_serverInfo = &server;
            server.m_connectArr[j]->m_serverIdx = i;
            server.m_connectArr[j]->m_connectIdx = j;
            server.m_connectArr[j]->m_fd = 0;
            server.m_connectArr[j]->pool = pool;

            connect_queue_t *cq = (connect_queue_t*)calloc(1, sizeof(connect_queue_t));
            cq->connect_id = j;
            cq->next = NULL;
            server.m_connectArr[j]->m_connectQueueNode = cq;

            _pool_unconnect_queue_put(server, cq);

        }
    }

    log_debug("pool_build done! poolIdx=%d", pool->m_poolIdx);
    return 0;
}

int pool_destroy(pool_info_t *pool, pool_on_destroy_cb_t cb) {
    pool->m_destroy_cb = cb;
    int m_serverCount = pool->m_serverCount;
    for (int i=0; i<m_serverCount; i++) {
        server_info_t& server = pool->m_serverArr[i];
        log_debug("pool_destroy, pid:%d, scount:%d", pool->m_poolIdx, pool->m_serverCount);

        //free connect monitor event
        event_timer_free(server.connect_monitor_event);

        //判断当前server是否有未处理的请求
        if(server.m_connectCount == server.m_freeConnectCount 
                && server.m_freeConnectCount + server.m_unconnectCount == pool->m_connectPerServer
                && server.m_waitingCount == 0) {
            //若没有，则直接destroy
            log_debug("pool_destroy, destroy server now, sid:%d", server.m_serverIdx);
            server.m_destroyFlag = true;
            _pool_server_destroy(server);
        }
        else {
            //若有，等待处理完成后再触发destroy
            //set destroy flag
            log_debug("pool_destroy, set destroy_flag, destroy server later, sid:%d", server.m_serverIdx);
            server.m_destroyFlag = true;
        }
    }

    return 0;
}

void _pool_server_destroy(server_info_t &server) {
    int connectIdx = _pool_connect_queue_get(server);

    if (connectIdx < 0) {
        //没有创建过连接或当前无有效连接，直接销毁
        log_debug("_pool_server_destroy, destroy now");
        _pool_server_destroyed(&server);
        return;
    }
    while(connectIdx >= 0) {
        log_debug("_pool_server_destroy, server_id:%d connect_id:%d connect_count:%d", 
                server.m_serverIdx, connectIdx, server.m_connectCount);
        m_poolCb.m_destroyCb(server.m_connectArr[connectIdx]->handle, _pool_on_connect_destroyed);
        connectIdx = _pool_connect_queue_get(server);
    }
}

static void _pool_on_connect_destroyed(handle_t *handle) {
    connect_info_t *ci = (connect_info_t*) handle->pool_data;
    pool_info_t *pool = ci->pool;
    server_info_t *server = ci->m_serverInfo;

    //setServerArgAfterConn
    pool->m_strategy->setServerArgAfterConn(server, -1);

    if (!ci->in_use) {
        _pool_connect_queue_rm(*server, ci->m_connectQueueNode);
    } else if (!ci->to_be_destroyed) {
        ci->to_be_destroyed = true;
        log_warning("handle destroyed when should not yet be destroyed");
        return;
    }
    ci->handle->backend_conn = NULL;
    ci->handle = NULL;
    free(handle);

    log_debug("_pool_on_connect_destroyed pid:%d sid:%d cid:%d flag:%d count:%d", 
            pool->m_poolIdx, ci->m_serverIdx, ci->m_connectIdx,
            (int)server->m_destroyFlag, server->m_connectCount);
    _pool_unconnect_queue_put(*server, ci->m_connectQueueNode);
    server->m_connectCount --;

    if (server->m_destroyFlag && server->m_connectCount==0) {
        log_debug("server destroy done! pid:%d sid:%d cid:%d", 
                pool->m_poolIdx, ci->m_serverIdx, ci->m_connectIdx);
        _pool_server_destroyed(server);
    }
}
//static void _pool_on_connect_destroyed_outside_request(handle_t *handle) {
//    connect_info_t *ci = (connect_info_t*) handle->pool_data;
//    pool_info_t *pool = ci->pool;
//    server_info_t *server = ci->m_serverInfo;
//    _pool_on_connect_destroyed(handle);
//}


void _pool_server_destroyed(server_info_t *server) {
    pool_info_t *pool = server->pool;

    log_debug("_pool_server_destroyed, server_id:%d, current serverCount:%d", server->m_serverIdx, pool->m_serverCount);

    for (int i=0; i<server->pool->m_connectPerServer; i++) {
        free(server->m_connectArr[i]->m_connectQueueNode);
        free(server->m_connectArr[i]);
    }
    free(server->m_connectArr);
    pool->m_serverCount --;
    free(server);

    if (pool->m_serverCount == 0) {
        //连接池所有server均被销毁
        delete(pool->m_strategy);
        pool->m_strategy = NULL;
        free(pool->m_strategyBuf);
        pool->m_destroy_cb(pool);
    }
}

int _pool_fetch_connect_from_connectqueue(server_info_t& server,
        void *sub_req, pool_call_cb_t cb) {

    int connectIdx = _pool_connect_queue_get(server);

    connect_info_t *ci = server.m_connectArr[connectIdx];
    //reset timestamp
    gettimeofday(&(ci->timestamp), NULL);

    log_debug("从balance到的server获取一个free的连接, server_id:%d, connect_id:%d, fd:%d", 
            server.m_serverIdx, connectIdx, 
            server.m_connectArr[connectIdx]->m_fd);

    ci->in_use = true;
    cb(ci->handle, sub_req);

    return 0;
}

int _pool_add_req_into_waitingqueue(server_info_t& server, void *sub_req, void* &pool_waiting_node, pool_call_cb_t cb) {

    waiting_queue_t *wq = (waiting_queue_t*)malloc(sizeof(waiting_queue_t));
    pool_waiting_node = (void*) wq;

    wq->sub_req = sub_req;
    wq->cb = cb;
    wq->last = NULL;
    wq->next = NULL;
    wq->server = &server;

    _pool_waiting_queue_put(server, wq);

    log_debug("_pool_add_req_into_waitingqueue, pid:%d sid:%d waitingCount:%d server:%d", server.pool->m_poolIdx, server.m_serverIdx, server.m_waitingCount, &server);

    return 0;
}

static void _pool_on_connect_created(handle_t *handle, int status) {
    connect_info_t *ci = (connect_info_t*) handle->pool_data;
    pool_info_t *pool = ci->pool;
    server_info_t& server = pool->m_serverArr[ci->m_serverIdx];
    ci->in_use = false;
    ci->to_be_destroyed = false;
    if (status == 0) {

        log_debug("_pool_on_connect_created, m_connectCount=%d", 
                server.m_connectCount);

        server.m_connectCount ++;
        _pool_put_back_connect(pool, ci);
    }
    else {
        //connect create failed
        _pool_unconnect_queue_put(server, ci->m_connectQueueNode);

        log_warning("_pool_on_connect_created failed addr=%s port=%d", handle->addr, handle->port);
        ci->handle = NULL;
        free(handle);

        if (server.m_destroyFlag &&
                server.m_freeConnectCount + server.m_unconnectCount == pool->m_connectPerServer) {
            log_debug("_pool_on_connect_created, call _pool_server_destroy, sid=%d cid=%d fc=%d uc=%d", server.m_serverIdx, ci->m_connectIdx, server.m_freeConnectCount, server.m_unconnectCount);
            _pool_server_destroy(server);
        }
 
    }
}

int _pool_create_new_connect(server_info_t& server) {
    //find connect_idx
    int connectIdx = _pool_unconnect_queue_get(server);
    if (connectIdx == -1) {
        //没有待创建的连接
        //无所谓，不做额外异常处理
        return 0;
    }
   
    //获取到一个当前无效的连接位，异步创建连接
    log_debug("serverid=%d, connectid=%d", server.m_serverIdx, connectIdx);

    handle_t *handle = (handle_t*)calloc(1, sizeof(handle_t));
    connect_info_t *ci = server.m_connectArr[connectIdx];
    handle->addr = ci->m_serverInfo->addr;
    handle->port = ci->m_serverInfo->port;
    handle->pool_data = ci;
    ci->handle = handle;

    m_poolCb.m_createCb(handle, _pool_on_connect_created, _pool_on_connect_destroyed);
    //创建连接不成功
    //无所谓，不做额外异常处理

    return 0;
}

int pool_fetch_connect(pool_info_t *pool, void *sub_req, void* &pool_waiting_node, pool_call_cb_t cb, int balanceKey) {
    log_debug("in pool_fetch_connect, pid:%d", pool->m_poolIdx);

    //当前无server
    if (pool->m_serverCount <= 0) {
        return -1;
    }

    int retry = pool->m_connectRetry;
    int serverid;

    //balancing
    while (retry >= 0) {
        serverid = _pool_get_balance_server(pool, balanceKey);
        if (serverid >= 0) {
            break;
        }
        //当前无可用server//balance模块异常
        retry --;
    }

    if (serverid < 0) {
        //当前无可用server//balance模块异常
        //failed
        log_warning("pool_fetch_connect, balance gets no server");
        return -1;
    }

    //balance到一台server
    server_info_t& server = pool->m_serverArr[serverid];

    //free connect?
    //从balance到的server获取一个free的连接
    if (server.m_connectQueueHead != NULL) {
        //get a connect from connectqueue
        _pool_fetch_connect_from_connectqueue(server,
                sub_req, cb);
        //异常处理？
        return 0;
    }

    //create new connect?
    //当前server无free的连接，但是连接数未达到上限
    if (server.m_connectCount < pool->m_connectPerServer) {
        //create new connect
        int ret = _pool_create_new_connect(server);
    }
    else {
        log_warning("pool_fetch_connect, connect full, connectPerServer=%d curConnectCount=%d queueLength=%d", 
                pool->m_connectPerServer,
                server.m_connectCount,
                server.m_waitingCount
            );
    }

    //add into waiting queue
    _pool_add_req_into_waitingqueue(server, sub_req, pool_waiting_node, cb);
    log_debug("pool_fetch_connect, put into queue, pid:%d sid:%d waitingCount:%d server:%d", server.pool->m_poolIdx, server.m_serverIdx, server.m_waitingCount, &server);

    return 0;
}

int _pool_put_back_connect(pool_info_t *pool, connect_info_t *ci) {
    //setServerArgAfterConn
    pool->m_strategy->setServerArgAfterConn(ci->m_serverInfo, 0);

    //call success, put back connection
    server_info_t& server = pool->m_serverArr[ci->m_serverIdx];

    //获取一个waiting queue中待执行的请求
    pool_call_cb_t cb;
    void *sub_req = _pool_waiting_queue_get(server, cb);

    if (sub_req != NULL) {
        //waiting queue not empty
        log_debug("_pool_put_back_connect, get a waiting req, pid:%d sid:%d waitingCount:%d", 
                ci->pool->m_poolIdx, ci->m_serverIdx, server.m_waitingCount);

        //reset timestamp
        gettimeofday(&(ci->timestamp), NULL);

        ci->in_use = true;
        cb(ci->handle, sub_req);
    }
    else {
        //waiting queue empty
        //put back into connect queue
        
        _pool_connect_queue_put(server, ci->m_connectQueueNode);

        //waitingQueue为空时，判断当前pool是否处于destroy状态
        if (server.m_destroyFlag &&
                server.m_freeConnectCount + server.m_unconnectCount == pool->m_connectPerServer) {
            //若waitingQueue为空且处于destroy状态，则可以进行当前free连接的销毁工作；
            //当前还在服务的连接，会在交互完成后再次进入此判断
            //销毁回调会对当前server的连接数量进行计数和判断
            //若当前server所有连接均已完成销毁，则调用_pool_server_destroy
            log_debug("FreeConnect, call _pool_server_destroy, sid=%d cid=%d fc=%d uc=%d", server.m_serverIdx, ci->m_connectIdx, server.m_freeConnectCount, server.m_unconnectCount);
            _pool_server_destroy(server);
        }

        log_debug("FreeConnect, put connect into connect queue, freeConnectCount=%d", 
                server.m_freeConnectCount);
    }
    return 0;

}

int pool_release_connect(handle_t *handle, bool errclose) {
    pool_info_t *pool = ((connect_info_t*)handle->pool_data)->pool;
    connect_info_t * ci = (connect_info_t*) handle->pool_data;
    log_debug("pool_release_connect, pid:%d sid:%d cid:%d", pool->m_poolIdx, ci->m_serverIdx, ci->m_connectIdx);
    server_info_t& server = pool->m_serverArr[ci->m_serverIdx];
    struct timeval timenow;

    ci->in_use = false;
    //setServerArg
    forum::slb_talk_returninfo_t *talk = 
        (forum::slb_talk_returninfo_t*)calloc(1, sizeof(forum::slb_talk_returninfo_t));
    gettimeofday(&timenow, NULL);
    int timecost = (timenow.tv_sec-ci->timestamp.tv_sec)*1000000 + 
        (timenow.tv_usec-ci->timestamp.tv_usec);

    talk->realreadtime = timecost;
    talk->realwritetime = timecost;

    pool->m_strategy->setServerArg(ci->m_serverInfo, talk);
    log_trace("pool_free_connect, pool_idx:%d, server_idx:%d, connect_idx:%d, timecost=%d", 
            pool->m_poolIdx, ci->m_serverIdx, 
            ci->m_connectIdx, timecost);

    free(talk);
    if (errclose == 0 && !ci->to_be_destroyed) {
        _pool_put_back_connect(pool, ci);
    }
    else {
        if (NULL == ci->handle || NULL == ci->handle->addr) {
            log_warning("pool_release_connect, err_no=%d handle=%d ci=%d", errclose, ci->handle, ci);
        } else {
            log_warning("pool_release_connect, err_no=%d handle=%d ci=%d addr=%s port=%d", errclose, ci->handle, ci, ci->handle->addr, ci->handle->port);
        }
        //call failed, destroy connection
        m_poolCb.m_destroyCb(ci->handle, _pool_on_connect_destroyed);
    }
    return 0;
}

int _pool_get_random_server(int server_num, int balanceKey) {
    if (balanceKey > 0 && server_num > 0) {
        return (balanceKey % server_num);
    }
    return 0;
}

int _pool_get_balance_server(pool_info_t *pool, int balanceKey) {
    if (NULL == pool->m_strategy ) {
        log_warning("service strategy is NULL");
    }

    // use super strategy
    forum::slb_request_t slbReq;
    slbReq.key = balanceKey;
    slbReq.nthRetry = 0;
    slbReq.serverNum = 0;
    server_info_t *serverTemp;
    int index = 0;
    for (int i = 0; i < pool->m_serverCount; i++) {
        // only for available servers, add by cdz
        serverTemp = pool->m_serverArr + i;
        if (serverTemp->m_connectCount == 0 || serverTemp->m_connectCount == pool->m_connectPerServer) {
            continue;
        }
        //slbReq最多支持64个server
        slbReq.svr[index++] = (void*)(serverTemp);
    }
    slbReq.serverNum = index;
    if (index == 0) {
        // random
        index = rand()%pool->m_serverCount;
        // try use connected server, by cdz
        for (int i = 0; i < pool->m_serverCount; i ++) {
            if ((pool->m_serverArr + index)->m_connectCount == 0) {
                index = (index + 1)%pool->m_serverCount;
                continue;
            } else {
                break;
            }
        }
        return index;
    }

    snprintf(slbReq.currentMachineRoom, MACHINE_ROOM_SIZE, "%s", pool->m_machineRoom);

    int id = pool->m_strategy->fetchServer(&slbReq);
    if (id >= 0 && id < slbReq.serverNum) {
        id = ((server_info_t*)slbReq.svr[id])->m_serverIdx;
        log_debug("SuperStrategy fetchServer: %d of %d", id, pool->m_serverCount);

        if (id < 0 || id >= pool->m_serverCount || (pool->m_serverArr+id) == NULL) {
            // 负载均衡返回有误，使用罴蚵膔andom
            log_warning("Attention,SuperLoadBalance get server err. Use Random");
            id = _pool_get_random_server(pool->m_serverCount, balanceKey);
            //return fetchServerRandom(req->machineRoom);
        }
    } else {
        // 负载均衡返回有误，使用最简陋的random
        log_warning("Attention,SuperLoadBalance get server err. Use Random");
        id = _pool_get_random_server(pool->m_serverCount, balanceKey);
        //return fetchServerRandom(req->machineRoom);
    }


    return id;
}



/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
