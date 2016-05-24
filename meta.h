/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file meta.h
 * @author opeddm@gmail.com
 * @date 2012/05/10 15:35:26
 * @brief 
 *  
 **/

#ifndef  __META_H_
#define  __META_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "proxy.h"
#include "log.h"

#include "string.h"
#include <map>
#include "bsl/containers/hash/bsl_hashmap.h"

//配置文件中，service的Service_Partition字段，字符串的最大长度
#define PARTITION_INFO_SIZE 5120
//hash分段的mod_key的bit位数
#define PARTITION_SHARDING_BIT 16
//hash分段的mod_key
#define PARTITION_MOD_KEY 65536 

#define AUTH_READ_ONLY 2
#define AUTH_READ_WRITE 1

#define MAX_IDC_NAME_LEN 32
#define MAX_MACHINE_ROOM_NAME_LEN 32

typedef struct meta_db_info meta_db_info_t;
typedef struct meta_service_info meta_service_info_t;
typedef struct meta_user_info meta_user_info_t;
typedef struct meta_partition_info meta_partition_info_t;
typedef struct meta_partition_service_info meta_partition_service_info_t;

typedef struct meta_info meta_info_t;

typedef std::string key;
typedef int value;
typedef bsl::hashmap<key, value> hash_map_auth;

struct meta_service_info {
    //service_id
    int id;
    //servicename
    static const int MAX_SERVICE_NAME_LEN = 32;
    char service_name[MAX_SERVICE_NAME_LEN];
    //实例配置
    static const int MAX_ADDR_LEN = 128;
    char addr[MAX_ADDR_LEN];
    unsigned int port;
    //所在分片
    unsigned int partition;
    //可读
    bool can_read;
    //可写
    bool can_write;
    // idc
    char idcName[MAX_IDC_NAME_LEN];
    // machine room
    char machineRoomName[MAX_MACHINE_ROOM_NAME_LEN];
};

struct meta_user_info {
    //tk
    char uname[MAX_UNAME_LEN];
    char tk[MAX_TK_LEN];
    //权限 1-读写用户；2-只读用户；
    int power;
};

struct meta_partition_info {
    int id;
    int num;
    int partitions[PARTITION_MOD_KEY];
};

struct meta_partition_service_info {
    int partition_id;
    int service_num;
    meta_service_info_t *service[MAX_COPY_NUM];
};

struct meta_db_info {
    int id;
    char pid[MAX_PID_LEN];
    char db_name[MAX_DB_NAME_LEN];
    char machine_room[MACHINE_ROOM_SIZE];
    //shard mode: 0-分段
    unsigned int shard_mode;
    //shard info

    //连接数
    unsigned int connect_per_service;
    //timeout
    unsigned int timeout;
    unsigned int timeout_sec;
    unsigned int timeout_usec;
    //长短连接
    bool long_connect;
    //获取连接重试次数
    unsigned int get_connect_retry;
    // 读只走本IDC
    int readIdcOnly;
    // 读只走本机房
    int readRoomOnly;
    // server idc & machineroom
    char serverIdcName[MAX_IDC_NAME_LEN];
    char serverMachineRoomName[MAX_MACHINE_ROOM_NAME_LEN];

    int service_num;
    meta_service_info_t **services;

    int user_num;
    meta_user_info_t **users;

    //partition的信息
    unsigned int partition_num;
    meta_partition_info_t *partition;

    //分片上的service信息
    //一个partition对应于一个partition_service_w和一个partition_service_r
    //一个partition_service_w对应于一个分片上的所以service资源
    //partition_service_w通过partition_id与partition关联
    meta_partition_service_info_t *partition_service_w;
    meta_partition_service_info_t *partition_service_r;
};

struct meta_info {
    meta_db_info_t **dbs;
};

//meta初始化，创建检查配置文件更新的定时时间，1秒为周期
int meta_init();

//获得一个库的配置信息//1.0版本只支持一个库，固定返回0号库的配置
int meta_get_db_info(/*char* dbname,*/ meta_db_info_t* &db_info);
int meta_get_timeout (time_t &sec, time_t &usec);

//权限认证
int meta_auth_check(/*char *pid,*/ const char *uname, const char *tk, const char *ip);

//从配置文件中读取配置信息
int _meta_load_conf(int i, meta_info_t *meta);

//创建一个库的路由表//1.0版本只支持一个库
int _meta_build_router();

//重新加载配置
int _meta_conf_reload(void *param);

int _meta_info_clean(int i, meta_info_t *meta);
int _meta_load_partition(int id, char *partition_str, meta_partition_info_t &partition_info);

#endif  //__META_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
