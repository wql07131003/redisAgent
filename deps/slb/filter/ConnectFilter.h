/*
 * ConnectFilter.h
 *
 *  Created on: 2010-4-7
 *      Author: Administrator
 */

#ifndef CONNECTFILTER_H_
#define CONNECTFILTER_H_

#include "Filter.h"
#include "Lock.h"
#include "Queue.h"

namespace forum
{
	class ConnectStatus
	{
	public:
		ConnectStatus(int queueSize, SLB_Resource * resource);
		virtual ~ConnectStatus();

		void insert(int status);		  /**< 入队连接状态   */
		int getFailedCount() const;		  /**< 当前失败情况   */
		/**
		 * @brief dump接口，index为server在service中的索引
		 */
		void dump(uint index);

	private:
		int _failedCount;
		pthread_mutex_t * _mutex;
		//Lock _lock;
		Queue<int> _queue;		  /**< 最近100次连接情况队列 0成功 1失败     */
	};

	/*
	 * @brief 连接状态
	 * @note 占用共享内存数计算，
	 *   先计算每个Server占用的内存（考虑对齐因素）：
	 *   只有：ConnectStatus 和 锁占用共享内存 = 32 + 800 + 40 = 872 取整到1K
	 *   ConnectStatus占用内存 = sizeof(ConnectStatus) + 1 * Queue上node_t占用的内存
	 *   sizeof(HealthyStatus) = 8 + 8 + 16 = 32
	 *   Queue上node_t占用的内存 = queueSize * 8 = 100 * 8 = 800
	 *   锁占用共享内存 = 40
	 *
	 *   目前一个Service最多支持32个Server，那么：
	 *   1个Service的ConnectFilter所需的总共享内存大小：32 * 1K = 32 K
	 */
	class ConnectFilter : public Filter
	{
	public:
		ConnectFilter();
		virtual ~ConnectFilter();

		virtual int doFilter(request_t* request);		  /**< 根据K点(x1,y1) P点(x2,y2)计算连接选择概率   */
		virtual int load(const comcfg::ConfigUnit & conf, SLB_Resource * resource);
		virtual int reload(const comcfg::ConfigUnit & conf);

		virtual int updateConnectStatus(slb_server_t *server, int errNo);

		virtual void debug(uint num);

	private:
		//static const uint MAX_SERVER_SIZE = 4096;
		ConnectStatus* _connectStatus[MAX_SERVER_SIZE];
		double _x1, _y1;
		double _x2, _y2;

		bool _loadSucc; // 表示是否加载成功，加载成功才启用
	};

}

#endif /* CONNECTFILTER_H_ */
