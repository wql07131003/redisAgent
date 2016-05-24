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

//��Ƭ���Ժ���
typedef int (*router_shard_func_t) (int partition_key); 
typedef int (*router_getshards_cb_t) (backend_req_t **backend_req, int num, void *user_data, pool_info_t* &pool);

//��Ƭ���Բ����ṹ��
typedef struct router_shard_parms {
    int router_map[PARTITION_MOD_KEY];
}router_shard_parms_t;

//router��ʼ��
int router_init(); 

//router���٣������ȼ���ʱ�Ծɵ�router��Ϣ������Ӧ���ӳء����ӽ����ͷ�
int router_destroy(); 
//
//���ӳ����ٻص�
void _router_on_destroy(pool_info_t *pool);

//��ǰrouter�Ƿ��ڽ����л�
bool router_is_building();

//router������������Ӧ���ӳصĴ���
int router_build(); 

//���������ڣ������������󣩸�������ķ�Ƭ��Ϣ�������з�Ƭ�ϲ�
int router_acquire_shards(backend_req_t **backend_req, int key_num, bool read, void *user_data, router_getshards_cb_t cb);


/**
 * @brief ���Ѳ�ֵ�������Ƭ���������·��
 *        �������ȵ���backend_parse��������ԭʼ������ݷ�Ƭ����д��ͬ�����Ϊ�������
 *        �ֶ�ν�����router_get_shard���󣬶�λ���ض����ӳ�
 *        �����ض���pool�ṹ�����
 *
 * @return =0��ʾ�ɹ�������ʧ��
 *
 */
int _get_shard(int partition_key, bool is_read, pool_info_t* &pool);

//��Χ�ķ�Ƭ��ʽ
int _shard_init_part(meta_db_info_t *db);
int _shard_func_part(int partition_key);
//���ӳصĴ���
int _shard_build_pool(meta_db_info_t *db);
int _quicksort(backend_req_t **backend_req, int start, int end);






#endif  //__ROUTER_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
