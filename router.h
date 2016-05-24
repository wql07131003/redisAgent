/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file router.h
 * @author opeddm@gmail.com
 * @date 2012/05/09 13:27:32
 * @brief 
 *  
 **/


#ifndef  __ROUTER_H_
#define  __ROUTER_H_

#include "proxy.h"
#include "pool.h"
#include "meta.h"
#include "server.h"
#include "hash.h"

#include <map>

//分片策略函数
typedef int (*router_shard_func_t) (int partition_key); 
typedef int (*router_getshards_cb_t) (backend_req_t **backend_req, int num, void *user_data, pool_info_t* &pool);

//分片策略参数结构体
typedef struct router_shard_parms {
    int router_map[PARTITION_MOD_KEY];
}router_shard_parms_t;

//router初始化
int router_init(); 

//router销毁，用于热加载时对旧的router信息及其相应连接池、连接进行释放
int router_destroy(); 
//
//连接池销毁回调
void _router_on_destroy(pool_info_t *pool);

//当前router是否在进行切换
bool router_is_building();

//router创建，包括相应连接池的创建
int router_build(); 

//计算请求内（包括批量请求）各个请求的分片信息，并进行分片合并
int router_acquire_shards(backend_req_t **backend_req, int key_num, bool read, void *user_data, router_getshards_cb_t cb);


/**
 * @brief 对已拆分到单个分片的请求进行路由
 *        上游首先调用backend_parse函数，将原始请求根据分片、读写不同，拆分为多个请求
 *        分多次将发起router_get_shard请求，定位至特定连接池
 *        返回特定的pool结构体对象
 *
 * @return =0表示成功，否则失败
 *
 */
int _get_shard(int partition_key, bool is_read, pool_info_t* &pool);

//范围的分片方式
int _shard_init_part(meta_db_info_t *db);
int _shard_func_part(int partition_key);
//连接池的创建
int _shard_build_pool(meta_db_info_t *db);
int _quicksort(backend_req_t **backend_req, int start, int end);






#endif  //__ROUTER_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
