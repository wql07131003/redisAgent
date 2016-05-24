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

//�����ļ��У�service��Service_Partition�ֶΣ��ַ�������󳤶�
#define PARTITION_INFO_SIZE 5120
//hash�ֶε�mod_key��bitλ��
#define PARTITION_SHARDING_BIT 16
//hash�ֶε�mod_key
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
    //ʵ������
    static const int MAX_ADDR_LEN = 128;
    char addr[MAX_ADDR_LEN];
    unsigned int port;
    //���ڷ�Ƭ
    unsigned int partition;
    //�ɶ�
    bool can_read;
    //��д
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
    //Ȩ�� 1-��д�û���2-ֻ���û���
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
    //shard mode: 0-�ֶ�
    unsigned int shard_mode;
    //shard info

    //������
    unsigned int connect_per_service;
    //timeout
    unsigned int timeout;
    unsigned int timeout_sec;
    unsigned int timeout_usec;
    //��������
    bool long_connect;
    //��ȡ�������Դ���
    unsigned int get_connect_retry;
    // ��ֻ�߱�IDC
    int readIdcOnly;
    // ��ֻ�߱�����
    int readRoomOnly;
    // server idc & machineroom
    char serverIdcName[MAX_IDC_NAME_LEN];
    char serverMachineRoomName[MAX_MACHINE_ROOM_NAME_LEN];

    int service_num;
    meta_service_info_t **services;

    int user_num;
    meta_user_info_t **users;

    //partition����Ϣ
    unsigned int partition_num;
    meta_partition_info_t *partition;

    //��Ƭ�ϵ�service��Ϣ
    //һ��partition��Ӧ��һ��partition_service_w��һ��partition_service_r
    //һ��partition_service_w��Ӧ��һ����Ƭ�ϵ�����service��Դ
    //partition_service_wͨ��partition_id��partition����
    meta_partition_service_info_t *partition_service_w;
    meta_partition_service_info_t *partition_service_r;
};

struct meta_info {
    meta_db_info_t **dbs;
};

//meta��ʼ����������������ļ����µĶ�ʱʱ�䣬1��Ϊ����
int meta_init();

//���һ�����������Ϣ//1.0�汾ֻ֧��һ���⣬�̶�����0�ſ������
int meta_get_db_info(/*char* dbname,*/ meta_db_info_t* &db_info);
int meta_get_timeout (time_t &sec, time_t &usec);

//Ȩ����֤
int meta_auth_check(/*char *pid,*/ const char *uname, const char *tk, const char *ip);

//�������ļ��ж�ȡ������Ϣ
int _meta_load_conf(int i, meta_info_t *meta);

//����һ�����·�ɱ�//1.0�汾ֻ֧��һ����
int _meta_build_router();

//���¼�������
int _meta_conf_reload(void *param);

int _meta_info_clean(int i, meta_info_t *meta);
int _meta_load_partition(int id, char *partition_str, meta_partition_info_t &partition_info);

#endif  //__META_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
