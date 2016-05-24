/*
 * HealthyFilter.h
 *
 *  Created on: 2010-4-8
 *      Author: Administrator
 */

#ifndef HEALTHYFILTER_H_
#define HEALTHYFILTER_H_



#include "Filter.h"
#include "Lock.h"
#include "Queue.h"

namespace forum
{
	class HealthyStatus
	{
	public:
		HealthyStatus(int timeWindow, SLB_Resource * resource);
		virtual ~HealthyStatus();

		/**
		 * @brief 返回上一次计算的健康选择概率
		 * @param timeout  最大读超时时间 默认100ms
		 * @param checkTime 计算的间隔时间 默认3s 
		 * @param minRate   最小的选择概率 默认0.1
		 * @retval 健康选择概率 (minRate <= retval <= 1) 
		**/
		double getSelectRate(uint timeout, int checkTime, double minRate);
		double getTotalAvgTime();

		/**
		 * @brief 添加一次交互处理时间(目前实际是读时间)
		 * @param proctime 一次交互的处理时间 
		 * @param currentTime  交互时当前时间
		**/
		void add(uint proctime, uint currentTime);
		/**
		 * @brief dump接口，index为server在service中的索引
		 */
		void dump(uint index);

		/**
		 * @brief debug对象异常问题
		**/
		int check_all(void *mutex);

	private:
		double _getRecentAvgTime(int checkTime, uint timeout = 0); // 获取一段时间内的平均处理时间
		double _getTotalAvgTime();

		uint _currentSecondTotalTime;    // 当前秒的总处理时间
		uint _currentSecondrequestCount; // 当前秒总的请求次数

		Queue<uint> _timeQueue;		  /**< 最近100s，每秒的Read时间    */
		Queue<uint> _countQueue;	  /**< 最近100s，每秒的交互次数    */

		uint _totalTime;		  /**< 最近100s总共的Readtime    */
		uint _totalCount;		  /**< 最近100s总共的交互次数    */

		uint _currentTime;		  /**< 当前统计的时间 统计1s内交互时间与次数  */

		int _lastCheckTime;		  /**< 上一次统计的时间  统计1s内交互时间与次数 */
		double _lastSelectRate;		  /**< 上一次统计的 健康选择概率 */

		//Lock _lock;
		pthread_mutex_t * _mutex;
	};

	/*
	 * @brief 健康状态
	 * @note 占用共享内存数计算，
	 *   先计算每个Server占用的内存（考虑对齐因素）：
	 *   只有：HealthyStatus 和 锁占用共享内存 = 56 + 2*800 + 40 = 1696 取整到2K
	 *   HealthyStatus占用内存 = sizeof(HealthyStatus) + 2 * Queue上node_t占用的内存
	 *   sizeof(HealthyStatus) = 8 + 16 + 16 + 8 + 8 = 56
	 *   Queue上node_t占用的内存 = queueSize * 8 = 100 * 8 = 800
	 *   锁占用共享内存 = 40
	 *
	 *   目前一个Service最多支持32个Server，那么：
	 *   1个Service的HealthyFilter所需的总共享内存大小：32 * 2K = 64 K
	 */
	class HealthyFilter : public Filter
	{
	public:
		HealthyFilter();
		virtual ~HealthyFilter();

		/**
		 * @brief 计算健康选择概率 与备机切流量
		**/
		virtual int doFilter(request_t* request);
		virtual int load(const comcfg::ConfigUnit& conf, SLB_Resource * resource);
		virtual int reload(const comcfg::ConfigUnit& conf);

		virtual int updateRequestStatus(slb_server_t *server, const slb_talk_returninfo_t *talk);

		virtual void debug(uint num);

	private:
		struct backup_machine_info_t
		{
			uint serverId;
			u_int64_t balance;
			double speed;
		};

		static bool _backup_cmp(backup_machine_info_t a1, backup_machine_info_t a2);

		//static const uint MAX_SERVER_COUNT = 4096;

		uint _timeout;
		double _minRate;
		uint _checkTime;
		HealthyStatus* _status[MAX_SERVER_SIZE];
		double _K; // 速度大于K倍的才能使用

		bool _loadSucc; // 表示是否加载成功，加载成功才启用
	};

}

#endif /* HEALTHYFILTER_H_ */
