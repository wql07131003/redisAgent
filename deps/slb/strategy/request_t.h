/*
 * RandomBalance.h
 *
 *  Created on: 2010-4-6
 *      Author: Administrator
 */

#ifndef REQUEST_H_
#define REQUEST_H_

#include <slb_define.h>
#include <pthread.h>
#include "server_rank_t.h"
#include <Configure.h> // include bsl/var/IVar.h &  bsl/ResourcePool.h
#include <proxy_log.h>
#include <bsl/pool.h>
#include <bsl/ResourcePool.h>

namespace forum
{
	const int SLB_RESOURCE_TYPE_NORMAL = 0; /**< 普通内存，线程模型 */
	const int SLB_RESOURCE_TYPE_SHM = 1; /**< 共享内存，进程模型 */
	/**
	 * @brief 内部request_t，保存每次请求的相关信息
	 */
	typedef struct _request_t{
		int key; /**< 负载均衡key */
		int nthRetry; /**< retry数，0表示第一次 */
		uint serverNum; /**< 全部server num */
		//comcm::Server * _svr[MAX_SERVER_SIZE];
		slb_server_t server[MAX_SERVER_SIZE]; /**< server信息数组 */
		char currentMachineRoom[SLB_MACHINE_ROOM_BUF_SIZE]; /**< 机房信息 */

		server_rank_t* serverRanks; /**< server打分数组 */
		bool mustSelectOneServer;	/**< 是否必须选择一台机器 */
		bool needHealthyFilter;		/**< 是否需要进行HealthyFilter */
	}request_t;


	/**
	 * @brief 用于管理负载均衡内部所需的资源
	 */
	class SLB_Resource {
public:
		SLB_Resource(){
			type = SLB_RESOURCE_TYPE_SHM;
			rp = NULL;
		}
		~SLB_Resource(){
		}
		/**
		 * @brief 获取锁，区分进程锁 / 线程锁
		 */
		pthread_mutex_t * getMutex() {
			int ret = 0;
			pthread_mutex_t * mutex = rp->createp<pthread_mutex_t>(/*rp->get_allocator()*/);
			if( (unsigned long)mutex < 0x65 ){
				UB_LOG_WARNING("SLB_Resource create pthread_mutex_t failed!!! mutex:[%p]",mutex);
			}
			if (SLB_RESOURCE_TYPE_SHM == type) {
				pthread_mutexattr_t mat;
				pthread_mutexattr_init(&mat);
				pthread_mutexattr_setpshared(&mat, PTHREAD_PROCESS_SHARED);
				ret = pthread_mutex_init(mutex, &mat);
				if(ret != 0){
					UB_LOG_WARNING("SLB_Resource pthread_mutex_init failed!!! ret:[%d] mutex:[%p]",ret,mutex);
				}
			} else {
				pthread_mutex_init(mutex, NULL);
			}
			return mutex;
		}
		int type; /**< 多进程/多线程 模式 */
		bsl::ResourcePool * rp; /**< 资源池 */
	};
}

#endif
