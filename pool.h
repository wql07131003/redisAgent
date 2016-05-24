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

//�ṩ��backend�Ļص����������Ӵ������������ʱ�Ĵ���
//typedef void (*pool_onconnect_cb_t) (pool_info_t *pool, connect_info_t *ci, const void *conn, int status);
//typedef void (*pool_ondestroy_cb_t) (pool_info_t *pool, connect_info_t *ci);

//backend�ṩ�Ļص�������ִ�����󡢴������������ӣ����������ʼ��Poolʱ����ע��
typedef int (*pool_call_cb_t) (handle_t *handle, void *sub_req);

//router�ṩ�Ļص����������pool destroy��֪ͨ����
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
* @brief ������������Ϣ���ɵ�������д
*
* @param addr              ��������ַ
* @param port              �������˿�
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
    ERR_SUCCESS = 0,  //�ɹ�����
    ERR_FAILED = -1,  //һ�����
    ERR_CONN = -2,    //����ʧ��
    ERR_MEM = -3,     //�ڴ�������
    ERR_RETRY = -4,   //��������ʧ��
    ERR_PARAM = -5,   //��������쳣
    ERR_SOCK = -6,    //����sock fd�����⣬���ܲ�����
    ERR_ASSERT = -7   //ԭ����assert���������ݴ���
};


/**
* @brief ����һ�����ӳ�
*
* @param pool               ���ӳص�ָ�룬�ռ����ϲ������ͷ�
* @param pool_idx           ���ӳ�id���ṩ����������ӳ�id>=0���ṩд��������ӳ�id<0
* @param serverConfs        ÿ��������������
* @param serverCount        ����������
* @param connectPerServer   ÿ�������������������
* @param cTimeOutMs         ���ӳ�ʱ��MS��
* @param connectRetry       �������Դ���
* @param longConnect        �Ƿ�����
*
* @return >=0��ʾ�ɹ�������ʧ��
*
* @see pool_service_conf_t
*/
int pool_build(pool_info_t *pool, int pool_idx, const pool_service_conf_t serverConfs[], 
        int serverCount, int connectPerServer, int cTimeOutMs, 
        char *machine_room, int connectRetry=1, 
        bool longConnect=true);

//���ӳص����٣��������ȼ���ʱ�Ծ��������ɵ����ӳؽ������ٺ��ͷ�
int pool_destroy(pool_info_t *pool, pool_on_destroy_cb_t cb);

/**
* @brief ��ȡһ�����õ�����
*
* @param pool               ���ӳ�
* @param user_data          ��req��ص���Ϣ��pool�㲻��ע
* @param backend_req        ��backend��ص���Ϣ��pool�㲻��ע
* @param num                backend_req��������pool�㲻��ע
* @param call_cb            ��ȡ�����Ӻ�Ĵ���ص���req.cpp::req_on_acquired
* @param balanceKey         ���ؾ��������<0��ʾ��ʹ��balanceKey
*
* @return 0-�ɹ���-1-ʧ��
*
*/
int pool_fetch_connect(pool_info_t *pool, void *sub_req, void* &pool_waiting_node, pool_call_cb_t cb, int balanceKey = -1);

/**
* @brief �ͷ�һ������
*
* @param pool               �����������ӳ�
* @param conn               ���ӣ����β���ע���Ӿ�����Ϣ��ʹ��void*��ʽ��
* @param errclose           �Ƿ�����ʧ��:0-success, others-failed
*
* @return 0-success, others-failed
*/
int pool_release_connect(handle_t *handle, bool errclose);




//server�����٣������ӳ����ٺ������ã��Ա�server���������ӷ������ٲ���
void _pool_server_destroy(server_info_t &server);

//server������ɺ�Ĵ�����poolע�������ӳأ����������pool������server�Ƿ�������ɣ�������������λص���ͨ��pool�������
void _pool_server_destroyed(server_info_t *server);

//���ؾ����ʼ��
int _pool_strategy_init(pool_info_t *pool);

//�ṩ��superStrategy�Ļص������ڽ����ص�server��Ϣ����superStrategy�Ľṹ��
void _pool_strategy_copy_serv(void *xxServer, forum::slb_server_t *server) ;

//���´�����ʹ�ú�����ӷŻ����ӳ�
int _pool_put_back_connect(pool_info_t *pool, connect_info_t *conn);

/**
* @brief [so]���Ӵ����ص�
*
* @param pool               �����������ӳ�
* @param ci                 pool�����ӽṹ��
* @param conn               ����洢����ֱ����ص�������Ϣ
* @param status             ���Ӵ���״̬
*
* @return >=0��ʾ�ɹ�������ʧ��
*/
static void _pool_on_connect_created(handle_t *handle, int status);

/**
* @brief [so]�������ٻص�
*
* @param pool               �����������ӳ�
* @param ci                 pool�����ӽṹ��
*
* @return >=0��ʾ�ɹ�������ʧ��
*/
static void _pool_on_connect_destroyed(handle_t *handle);

//�������ؾ���ʧ��ʱ�����������ȡһ̨server
int _pool_get_random_server(int server_num, int balanceKey);

/**
* @brief ������ԣ���ȡһ̨����ʵ�server
*
* @param pool               �����������ӳ�
* @param balanceKey         ���ؾ��������<0��ʾ��ʹ��balanceKey
*
* @return int              server_id��<0��ʾʧ��
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
