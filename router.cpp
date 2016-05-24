/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/



/**
 * @file router.cpp
 * @author opeddm@gmail.com
 * @date 2012/05/09 13:27:37
 * @brief 
 *  
 **/

#include "router.h"

extern meta_info_t* meta;

//两套连接池，相互切换，用于热加载
int router_flag;
int router_next_flag;
//连接池切换中
bool router_switching;

//连接池map 
pool_info_t** _pool_map_r;
pool_info_t** _pool_map_w;

//连接池计数
int _pool_num_r[2];
int _pool_num_w[2];

//分片计算函数
router_shard_func_t _shard_func[2];

//分片参数
router_shard_parms_t _shard_params[2];

//初始化
int router_init() {
    router_flag = 1;
    router_next_flag = 0;
    router_switching = false;

    _pool_map_r = (pool_info_t**)calloc(PARTITION_MOD_KEY*2, sizeof(pool_info_t*));
    _pool_map_w = (pool_info_t**)calloc(PARTITION_MOD_KEY*2, sizeof(pool_info_t*));

    _pool_num_r[0] = 0;
    _pool_num_r[1] = 0;
    _pool_num_w[0] = 0;
    _pool_num_w[1] = 0;

    return 0;
}

bool router_is_building() {
    return router_switching;
}

int router_build() {
    log_debug("in router_build");
    //set switching flag
    router_switching = true;

    //从meta获取db信息
    meta_db_info_t *db;
    int ret = meta_get_db_info(db);

    //分片策略init
    ret = 0;
    switch(db->shard_mode) {
        case 0:
            ret = _shard_init_part(db);
            if (ret != 0) {
                router_switching = false;
                return ret;
            }
            break;
        default:
            log_fatal("router_build, shard_mode not support");
            return -1;
            break;
    }

    //连接池初始化
    _shard_build_pool(db);

    //do switching
    router_flag = router_next_flag;;
    router_next_flag = router_flag==0 ? 1:0;
    //destroy old pools
    router_destroy();
    return 0;
}

void _router_on_destroy(pool_info_t *pool) {
    log_debug("_router_on_destroy");

    if (pool->m_poolIdx >= 0){ //read pool
        _pool_map_r[pool->m_poolIdx] = NULL;
        _pool_num_r[router_next_flag] --;
        log_debug("_router_on_destroy, _pool_num_r[%d]--, =%d", router_next_flag, _pool_num_r[router_next_flag]);
        free (pool);
    }
    else { //write pool
        _pool_map_w[-pool->m_poolIdx-1] = NULL;
        _pool_num_w[router_next_flag] --;
        log_debug("_router_on_destroy, _pool_num_w[%d]--, =%d", router_next_flag, _pool_num_w[router_next_flag]);
        free (pool);
    }

    if (_pool_num_r[router_next_flag]==0 && _pool_num_w[router_next_flag]==0) {
        //switching done!
        router_switching = false;
        log_notice("old pools had all destroyed, conf reload done!");
    }
}

int router_destroy() {
    int pool_num_r = _pool_num_r[router_next_flag];
    int pool_num_w = _pool_num_w[router_next_flag];
    log_debug("router_destroy, pool_num_r:%d, pool_num_w:%d", pool_num_r, pool_num_w);

    //如果目前没有连接池，则不用进行destroy操作
    if (pool_num_r == 0 && pool_num_w == 0) {
        router_switching = false;
        return 0;
    }

    for (int i=0; i<pool_num_r; i++) {
        pool_destroy(_pool_map_r[PARTITION_MOD_KEY*router_next_flag + i], _router_on_destroy);
    }
    for (int i=0; i<pool_num_w; i++) {
        pool_destroy(_pool_map_w[PARTITION_MOD_KEY*router_next_flag + i], _router_on_destroy);
    }

    return 0;
}

int router_acquire_shards(backend_req_t **backend_req, int key_num, bool is_read, void *user_data,  
        router_getshards_cb_t cb) {
    log_debug("in router_acquire_shards, key_num=%d", key_num);

    //calculate shard_key
    for (int i=0; i<key_num; i++) {
        backend_req[i]->shard_key = _shard_func[router_flag](strhash(backend_req[i]->key));
        log_debug("router_acquire_shards, hash, skey:%s sr:%d sid:%d", backend_req[i]->key, strhash(backend_req[i]->key), backend_req[i]->shard_key);
    }

    //sort backend_req by shard_key
    _quicksort(backend_req, 0, key_num-1);

    //cb
    req_t *r = (req_t*)user_data;
    r->shard_count = key_num;
    int last_partition = -1;
    int start = 0;
    log_debug("router_acquire_shards, key_num = %d", key_num);
    for (int i=0; i<key_num; i++) {
	if (backend_req[i]->shard_key == last_partition) {
	    r->shard_count --; // fix core
	}
        if (backend_req[i]->shard_key != last_partition && last_partition != -1) {
            pool_info_t *pool;
            _get_shard(last_partition, is_read, pool);
            log_debug("router_acquire_shards, cb, sid:%d knum:%d tnum:%d", last_partition, i-start, key_num);
            cb(backend_req + start, i-start, user_data, pool);
            start = i;
        }
        last_partition = backend_req[i]->shard_key;
    }
    pool_info_t *pool;
    _get_shard(last_partition, is_read, pool);
    cb(backend_req + start, key_num-start, user_data, pool);
    log_debug("router_acquire_shards, cb, sid:%d knum:%d tnum:%d", last_partition, key_num-start, key_num);

    return 0;
}

int _get_shard(int shard_key, bool is_read, pool_info_t* &pool) {

    if (is_read) {
        pool = _pool_map_r[PARTITION_MOD_KEY*router_flag + shard_key];
    }
    else {
        pool = _pool_map_w[PARTITION_MOD_KEY*router_flag + shard_key];
    }

    return 0;
}

int _shard_init_part(meta_db_info_t *db) {
    //mem reset
    memset(_shard_params[router_next_flag].router_map, 0, sizeof(int)*PARTITION_MOD_KEY);
    //get partition num
    int partition_num = db->partition_num;

    int partition_check[PARTITION_MOD_KEY];
    memset (partition_check, 0, sizeof(int)*PARTITION_MOD_KEY);

    for (int i=0; i<partition_num; i++) {
        meta_partition_info partition_info = db->partition[i];
        int num = partition_info.num;
        for (int j=0; j<num; j++) {
            int p = partition_info.partitions[j];
            if (partition_check[p] != 0) {
                log_fatal ("_shard_init_part, partition conf error, partition reallocate:%d", p);
                return -1;
            }

            _shard_params[router_next_flag].router_map[p] = i;
            partition_check[p] = 1;
        }
    }

    //check partition 
    for (int i=0; i<PARTITION_MOD_KEY; i++) {
        if (partition_check[i] == 0) {
            log_fatal ("_shard_init_part, partition conf error, partition no allocate:%d", i);
            return -1;
        }
    }
  
    _shard_func[router_next_flag] = _shard_func_part;
    return 0;
}

int _shard_func_part(int partition_key) {
    //根据系统配置的PARTITION_SHARDING_BIT截取有效的partition_key
    int key = partition_key & 0xFFFF;
    int pid = _shard_params[router_flag].router_map[key];
    log_debug("_shard_func_part, pkey:%d key:%d poolid:%d", partition_key, key, pid);
    return pid;
}

int _shard_build_pool(meta_db_info_t *db) {
    int partition_num = db->partition_num;
    meta_partition_service_info_t *partition_service_r = db->partition_service_r;
    meta_partition_service_info_t *partition_service_w = db->partition_service_w;

    //read pool
    for (int i=0; i<partition_num; i++) {
        //拿到partition info
        meta_partition_service_info partition = partition_service_r[i];
        int service_num = partition.service_num;
        if (service_num == 0) {
            log_fatal("_shard_build_pool, partition_r with no service for partition:%d", i);
        }
        
        //构造service_conf结构体
        pool_service_conf_t *sc = (pool_service_conf_t*)calloc(service_num, sizeof(pool_service_conf_t));
        for (int j=0; j<service_num; j++) {
            strncpy(sc[j].addr, partition.service[j]->addr, sc[j].MAX_ADDR_LEN);
            sc[j].port = partition.service[j]->port;
        }

        //创建pool
        pool_info_t *pool = (pool_info_t*)calloc(1, sizeof(pool_info_t));
        _pool_map_r[PARTITION_MOD_KEY*router_next_flag + i] = pool;
        _pool_num_r[router_next_flag] ++;

        int ret = pool_build(pool, PARTITION_MOD_KEY*router_next_flag + i, sc, service_num, 
                db->connect_per_service, db->timeout, 
                db->machine_room, db->get_connect_retry, 
                db->long_connect);

        log_debug("_shard_build_pool, _pool_num_r[%d]=%d poolIdx=%d", router_next_flag, _pool_num_r[router_next_flag], pool->m_poolIdx);
        free(sc);
    }

    //write pool
    for (int i=0; i<partition_num; i++) {
        //拿到partition info
        meta_partition_service_info partition = partition_service_w[i];
        int service_num = partition.service_num;
        
        if (service_num == 0) {
            log_fatal("_shard_build_pool, partition_w with no service for partition:%d", i);
        }
        
        //构造service_conf结构体
        pool_service_conf_t *sc = (pool_service_conf_t*)calloc(service_num, sizeof(pool_service_conf_t));
        for (int j=0; j<service_num; j++) {
            strncpy(sc[j].addr, partition.service[j]->addr, sc[j].MAX_ADDR_LEN);
            sc[j].port = partition.service[j]->port;
        }

        //创建pool
        pool_info_t *pool = (pool_info_t*)calloc(1, sizeof(pool_info_t));
        _pool_map_w[PARTITION_MOD_KEY*router_next_flag + i] = pool;
        _pool_num_w[router_next_flag] ++;

        //写pool的id为-id-1
        int ret = pool_build(pool, -PARTITION_MOD_KEY*router_next_flag-i-1, sc, service_num, 
                db->connect_per_service, db->timeout, db->machine_room, 
                db->get_connect_retry, db->long_connect);

        log_debug("_shard_build_pool, _pool_num_w[%d]++, =%d", router_next_flag, _pool_num_w[router_next_flag]);
        free(sc);
        
    }
    return 0;
}

int _quicksort(backend_req_t **backend_req, int start, int end) {
    if (start >= end) {
        return 0;
    }

    backend_req_t *tmp = backend_req[start];
    int l = start; //left ptr
    int r = end; //right ptr

    while(l < r) {
        while(l<r && tmp->shard_key<=backend_req[r]->shard_key) {
            r--;
        }
        backend_req[l] = backend_req[r];

        while(l<r && tmp->shard_key>=backend_req[l]->shard_key) {
            l++;
        }
        backend_req[r] = backend_req[l];
    }
    backend_req[r] = tmp;
    _quicksort(backend_req, start, r-1);
    _quicksort(backend_req, r+1, end);
    return 0;
}

