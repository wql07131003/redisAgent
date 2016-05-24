/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file pool.h
 * @author opeddm@gmail.com
 * @date 2012/05/02 16:38:52
 * @brief 
 *  
 **/


#ifndef  __PROXY_POOL_H_
#define  __PROXY_POOL_H_

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include "proxy.h"
#include "event.h"
#include "server.h"

#include "log.h"
#include "CamelSuperStrategy.h"

typedef struct connect_queue connect_queue_t;
typedef struct connect_queue unconnect_queue_t;
typedef struct waiting_queue waiting_queue_t;
typedef struct connect_info connect_info_t;
typedef struct server_info server_info_t;
typedef struct pool_service_conf pool_service_conf_t;
typedef struct pool_info pool_info_t;
typedef struct pool_cb pool_cb_t;

//提供给backend的回调，用于连接创建和销毁完成时的处理
//typedef void (*pool_onconnect_cb_t) (pool_info_t *pool, connect_info_t *ci, const void *conn, int status);
//typedef void (*pool_ondestroy_cb_t) (pool_info_t *pool, connect_info_t *ci);

//backend提供的回调，用于执行请求、创建和销毁连接，由主程序初始化Pool时进行注册
typedef int (*pool_call_cb_t) (handle_t *handle, void *sub_req);

//router提供的回调，用于完成pool destroy后通知上游
typedef void (*pool_on_destroy_cb_t) (pool_info_t *pool);

struct connect_queue {
    int connect_id;
    struct connect_queue *next;
    struct connect_queue *prev;
};

struct waiting_queue {
    struct waiting_queue *last;
    struct waiting_queue *next;
    void *sub_req;
    pool_call_cb_t cb;
    server_info_t *server;
};

struct connect_info {
    server_info_t *m_serverInfo;

    int m_serverIdx;
    int m_connectIdx;
    int m_fd;

    connect_queue_t *m_connectQueueNode;

    struct timeval timestamp;

    pool_info_t *pool;

    handle_t *handle;
    bool in_use;
    bool to_be_destroyed;
};

struct server_info {
    int m_serverIdx;
    pool_info_t *pool;

    static const int MAX_ADDR_LEN = 128;
    char addr[MAX_ADDR_LEN];
    int port;

    connect_info_t** m_connectArr;
    connect_queue_t* m_connectQueueHead;
    connect_queue_t* m_connectQueueTail;
    waiting_queue_t* m_waitingQueueHead;
    waiting_queue_t* m_waitingQueueTail;
    unconnect_queue_t* m_unconnectQueueHead;
    unconnect_queue_t* m_unconnectQueueTail;

    int m_connectCount;
    int m_unconnectCount;
    int m_freeConnectCount;
    int m_waitingCount;

    bool m_destroyFlag;

    event_timer_t* connect_monitor_event;

};

struct pool_info {
    int m_poolIdx;
    server_info_t* m_serverArr;

    char *m_machineRoom;

    int m_serverCount;
    int m_lowConnectPerServer;
    int m_connectPerServer;
    int m_connectRetry;
    bool m_longConnect;
    int m_cTimeOutMs;

    char *m_strategyBuf;
    forum::TbStrategy * m_strategy;
    pool_on_destroy_cb_t m_destroy_cb;
};


/**
* @brief 服务器配置信息，由调用者填写
*
* @param addr              服务器地址
* @param port              服务器端口
*/
struct pool_service_conf {
    static const int MAX_ADDR_LEN = 128;
    char addr[MAX_ADDR_LEN];
    int port;
};

struct pool_cb {
    pool_create_cb_t m_createCb;
    pool_destroy_cb_t m_destroyCb;
};

enum ErrorType {
    ERR_SUCCESS = 0,  //成功返回
    ERR_FAILED = -1,  //一般错误
    ERR_CONN = -2,    //连接失败
    ERR_MEM = -3,     //内存分配错误
    ERR_RETRY = -4,   //所有重试失败
    ERR_PARAM = -5,   //传入参数异常
    ERR_SOCK = -6,    //传入sock fd有问题，可能不存在
    ERR_ASSERT = -7   //原来的assert，严重数据错误
};


/**
* @brief 创建一个连接池
*
* @param pool               连接池的指针，空间由上层分配和释放
* @param pool_idx           连接池id，提供读服务的连接池id>=0，提供写服务的连接池id<0
* @param serverConfs        每个服务器的配置
* @param serverCount        服务器数量
* @param connectPerServer   每个服务器允许的连接数
* @param cTimeOutMs         连接超时（MS）
* @param connectRetry       连接重试次数
* @param longConnect        是否长连接
*
* @return >=0表示成功，否则失败
*
* @see pool_service_conf_t
*/
int pool_build(pool_info_t *pool, int pool_idx, const pool_service_conf_t serverConfs[], 
        int serverCount, int connectPerServer, int cTimeOutMs, 
        char *machine_room, int connectRetry=1, 
        bool longConnect=true);

//连接池的销毁，供上游热加载时对旧配置生成的连接池进行销毁和释放
int pool_destroy(pool_info_t *pool, pool_on_destroy_cb_t cb);

/**
* @brief 获取一个可用的连接
*
* @param pool               连接池
* @param user_data          和req相关的信息，pool层不关注
* @param backend_req        和backend相关的信息，pool层不关注
* @param num                backend_req的数量，pool层不关注
* @param call_cb            获取到连接后的处理回调，req.cpp::req_on_acquired
* @param balanceKey         负载均衡参数，<0表示不使用balanceKey
*
* @return 0-成功；-1-失败
*
*/
int pool_fetch_connect(pool_info_t *pool, void *sub_req, void* &pool_waiting_node, pool_call_cb_t cb, int balanceKey = -1);

/**
* @brief 释放一个连接
*
* @param pool               连接所在连接池
* @param conn               连接（上游不关注连接具体信息，使用void*方式）
* @param errclose           是否请求失败:0-success, others-failed
*
* @return 0-success, others-failed
*/
int pool_release_connect(handle_t *handle, bool errclose);




//server的销毁，由连接池销毁函数调用，对本server的所有连接发起销毁操作
void _pool_server_destroy(server_info_t &server);

//server销毁完成后的处理，向pool注销本连接池，并检查属于pool的所有server是否销毁完成，若完成则向上游回调，通报pool销毁完成
void _pool_server_destroyed(server_info_t *server);

//负载均衡初始化
int _pool_strategy_init(pool_info_t *pool);

//提供给superStrategy的回调，用于将本地的server信息赋给superStrategy的结构体
void _pool_strategy_copy_serv(void *xxServer, forum::slb_server_t *server) ;

//将新创建或使用后的连接放回连接池
int _pool_put_back_connect(pool_info_t *pool, connect_info_t *conn);

/**
* @brief [so]连接创建回调
*
* @param pool               连接所在连接池
* @param ci                 pool的连接结构体
* @param conn               具体存储引擎直接相关的连接信息
* @param status             连接创建状态
*
* @return >=0表示成功，否则失败
*/
static void _pool_on_connect_created(handle_t *handle, int status);

/**
* @brief [so]连接销毁回调
*
* @param pool               连接所在连接池
* @param ci                 pool的连接结构体
*
* @return >=0表示成功，否则失败
*/
static void _pool_on_connect_destroyed(handle_t *handle);

//超级负载均衡失败时，用于随机获取一台server
int _pool_get_random_server(int server_num, int balanceKey);

/**
* @brief 均衡策略：获取一台最合适的server
*
* @param pool               连接所在连接池
* @param balanceKey         负载均衡参数，<0表示不使用balanceKey
*
* @return int              server_id，<0表示失败
*/
int _pool_get_balance_server(pool_info_t *pool, int balanceKey);

int _pool_unconnect_queue_get(server_info_t &server);
int _pool_unconnect_queue_put(server_info_t &server, connect_queue_t *cq);
int _pool_connect_queue_get(server_info_t &server);
int _pool_connect_queue_put(server_info_t &server, connect_queue_t *cq);
void* _pool_waiting_queue_get(server_info_t &server, pool_call_cb_t &cb);
int _pool_waiting_queue_rm(void* wq);
int _pool_waiting_queue_put(server_info_t &server, waiting_queue_t *wq);
int _pool_fetch_connect_from_connectqueue(server_info_t& server,
        void *sub_req, pool_call_cb_t cb);

int _pool_create_new_connect(server_info_t& server);


#endif  //__POOL_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
