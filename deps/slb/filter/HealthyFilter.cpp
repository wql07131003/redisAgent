/*
 * HealthyFilter.cpp
 *
 *  Created on: 2010-4-8
 *      Author: Administrator
 */

#include <time.h>

#include "HealthyFilter.h"

namespace forum
{

	HealthyStatus::HealthyStatus(int timeWindow, SLB_Resource * resource) : _timeQueue(timeWindow, resource->rp),
	_countQueue(timeWindow, resource->rp)
	{
		_currentSecondTotalTime = 0;
		_currentSecondrequestCount = 0;

		_totalTime = 0;
		_totalCount = 0;
		_currentTime = 0;
		_lastSelectRate = 0.0;
		_lastCheckTime = 0;

		_mutex = resource->getMutex();
	}

	HealthyStatus::~HealthyStatus()
	{
		_currentSecondTotalTime = 0;
		_currentSecondrequestCount = 0;

		_totalTime = 0;
		_totalCount = 0;
		_currentTime = 0;
		_lastSelectRate = 0.0;
		_lastCheckTime = 0;
		
		_mutex = NULL;
	}


	double HealthyStatus::_getTotalAvgTime()
	{
		if (_totalCount == 0)
			return 0;

		double ret = 1.0 * _totalTime / _totalCount;

		return ret;
	}

	int HealthyStatus::check_all(void * mutex){
		static unsigned long first_mutex = (unsigned long)mutex;
		//排查对象异常
		if((_lastCheckTime < 0) || (_lastSelectRate < 0) || ((unsigned long)mutex < 0x65)){
			UB_LOG_WARNING("HealthyStatus illegal, mutex:%p, _lastCheckTime:%d, _lastSelectRate:%g, currentSecondTotalTime:%u, _currentSecondrequestCount:%u, 100sTotalRtime:%u, 100sTotalCount:%u _currentTime:%u", _mutex, _lastCheckTime, _lastSelectRate, _currentSecondTotalTime, _currentSecondrequestCount, _totalTime, _totalCount, _currentTime);
		}else{
			UB_LOG_DEBUG("first_mutex:%p, _mutex:%p, _lastCheckTime:%d, _lastSelectRate:%g, currentSecondTotalTime:%u, _currentSecondrequestCount:%u, 100sTotalRtime:%u, 100sTotalCount:%u _currentTime:%u", (void*)first_mutex, _mutex, _lastCheckTime, _lastSelectRate, _currentSecondTotalTime, _currentSecondrequestCount, _totalTime, _totalCount, _currentTime);	
		}
		return 0;
	}

	double HealthyStatus::getSelectRate(uint timeout, int checkTime, double minRate)
	{
		double rate = 0;
		int tm = time(NULL);

		//check this
		if( (unsigned long)_mutex < 0x65 ){
			UB_LOG_WARNING("Error,HealthyStatus::getSelectRate mutex illegal.mutex:%p.",_mutex);
			rate = _lastSelectRate >= minRate ? _lastSelectRate : minRate;
			return rate;
		}
		AutoLock _lock(_mutex);

		//check_all(_mutex);//delete this

		if (_lastCheckTime + checkTime/*_selectCheckTime*/ <= tm){	  /**< 需要重新计算健康选择概率   */
			double totalAvg = _getTotalAvgTime();
			double recentAvg = _getRecentAvgTime(checkTime, timeout);
			//UB_LOG_DEBUG("total:%g recent:%g timeout:%u", totalAvg, recentAvg, timeout);

			//计算健康状态
			if (totalAvg< 1e-6 || recentAvg < 1e-6) {
				rate = 1.0;
			}else if (totalAvg >= timeout){
				rate = 0;
			}else{
				rate = 1.0 * (timeout - recentAvg) / (timeout - totalAvg);
			}
			//计算选择概率
			if (rate < minRate){
				rate = minRate;
			}else if (rate > 1){
				rate = 1;
			}
			_lastSelectRate = rate;
			_lastCheckTime = tm;
		}else{
			rate = _lastSelectRate >= minRate ? _lastSelectRate : minRate;
		}
		return rate;
	}

	double HealthyStatus::getTotalAvgTime()
	{
		//_lock.lock();
		if( (unsigned long)_mutex < 0x65 ){
			UB_LOG_WARNING("Error,HealthyStatus::getTotalAvgTime mutex illegal.mutex:%p.",_mutex);
			return 0;
		}
		AutoLock _lock(_mutex);

		double ret = _getTotalAvgTime();

		//_lock.unlock();

		return ret;
	}

	double HealthyStatus::_getRecentAvgTime(int checkTime, uint timeout)  /**< 最近checkTime秒，平均处理时间 */
	{
		double ret = 0.0;

		uint totalTime = 0;
		uint totalCount = 0;

		for (int i = 0; i < checkTime && i < _timeQueue.size(); i++)
		{
			uint time = 0;
			uint count = 0;
			bool r1 = _timeQueue.getLast(i, &time);
			bool r2 = _countQueue.getLast(i, &count);

			if (r1 && r2)
			{
				totalTime += time;
				totalCount += count;
			}
		}

		if (totalCount)
			ret = 1.0 * totalTime / totalCount;
		else
			ret = timeout;

		return ret;
	}



	void HealthyStatus::add(uint proctime, uint currentTime)
	{
		if( (unsigned long)_mutex < 0x65 ){
			UB_LOG_WARNING("Error,HealthyStatus::add mutex illegal.mutex:%p.",_mutex);
			return;
		}
		AutoLock _lock(_mutex);

		if (currentTime != _currentTime)		  /**< add时的currentTime不是目前统计的这一秒(_currentTime)       */
		{
			uint oldTime = 0;
			uint oldCount = 0;
			bool r1 = _timeQueue.insert(_currentSecondTotalTime, &oldTime);		  /**< 入队前一秒统计的Readtime    */
			bool r2 = _countQueue.insert(_currentSecondrequestCount, &oldCount);  /**<     前一秒统计的 请求次数   */

			if (r1 && r2)
			{
				_totalTime -= oldTime;
				_totalCount -= oldCount;
			}

			_totalTime += _currentSecondTotalTime;
			_totalCount += _currentSecondrequestCount;

			_currentSecondTotalTime = 0;
			_currentSecondrequestCount = 0;

			_currentTime = currentTime;
		}

		_currentSecondTotalTime += proctime;
		_currentSecondrequestCount++;
	}

	void HealthyStatus::dump(uint index)
	{
		UB_LOG_TRACE("DUMP : HealthyStatus[%u] timeQueue", index);
		_timeQueue.dump();
		UB_LOG_TRACE("DUMP : HealthyStatus[%u] countQueue", index);
		_countQueue.dump();
	}

	HealthyFilter::HealthyFilter()
	{
		_timeout = 0;
		_checkTime = 0;
		_minRate = 0.0;
		_loadSucc = false;
		memset(_status, 0, sizeof (_status));
	}

	HealthyFilter::~HealthyFilter()
	{
		for (uint i = 0; i < MAX_SERVER_SIZE; i++)
		{
			// 使用resourcePool管理资源
			//delete _status[i];
			_status[i] = NULL;
		}
	}

	int HealthyFilter::doFilter(request_t* request)
	{
		//CHECK_POOL("HealthyFilter::doFilter ",resource->rp);
		if (!_loadSucc) { // 配置加载没有成功，表示不需要
			return 0;
		}
		if (!request->needHealthyFilter)
			return 0;

		uint serverNum = request->serverNum;
		HealthyStatus* hs = NULL;
		double rate = 0;
		//计算每台server的健康选择概率healthyscore,并进行淘汰
		for (uint i = 0; i < serverNum; i++)
		{
			server_rank_t* serverRanks = request->serverRanks;
			uint serverId = serverRanks[i].serverId;

			if (serverId >= MAX_SERVER_SIZE)
			{
				UB_LOG_WARNING("serverId[%u] error", serverId);
				continue;
			}

			hs = _status[serverId];

			//hs->dump(serverId);
			rate = hs->getSelectRate(_timeout, _checkTime, _minRate);

			serverRanks[i].healthyScore = rate;

			int rnd = rand() % 100;
			if (rnd > rate * 100){
				serverRanks[i].disabled = true;
			}
			//UB_LOG_DEBUG("HealthyFilter doFilter: server_id[%u] rate[%g] rand[%d] disabled[%d]", serverId, rate*100, rnd, serverRanks[i].disabled);
		}

		// 选择备机
		// 选择概率:((1/ti)/(sum(1/tj)))/(1/n)  tj 当前机房每台机器处理时间  该选择概率体现单机处理性能

		double sumSpeed = 0;		  /**< (sum(1/tj)       */
		int totalServerNum = 0; 		  /**< 当前机房的server的数目  */
		//计算当前机房(sum(1/tj) 每秒能处理的请求数
		for (uint i = 0; i < serverNum; i++)
		{
			server_rank_t* serverRanks = request->serverRanks;
			uint serverId = serverRanks[i].serverId;
			if (serverId >= MAX_SERVER_SIZE)
			{
				UB_LOG_WARNING("serverId[%u] error", serverId);
				continue;
			}

			if (serverRanks[i].crossRoom)
				continue;

			totalServerNum++;

			double avgTime = _status[serverId]->getTotalAvgTime();
			//UB_LOG_DEBUG("server:%d total avg Rtime: %g", serverId+1, avgTime);
			if (avgTime > 1e-6) // >0
			{
				sumSpeed += (1 / avgTime);
			}
		}

		// 计算healthySelectRate该机器的平均性能
		// 根据该值考虑切流量，小于1表示性能较低于平均水平要考虑切分
		for (uint i = 0; i < serverNum; i++)
		{
			double avgTime;
			server_rank_t* serverRanks = request->serverRanks;
			uint serverId = serverRanks[i].serverId;
			if (serverId >= MAX_SERVER_SIZE)
			{
				UB_LOG_WARNING("serverId[%u] error", serverId);
				continue;
			}

			if (serverRanks[i].crossRoom || serverRanks[i].disabled)
				continue;

			avgTime = _status[serverId]->getTotalAvgTime();
			serverRanks[i].healthySelectRate = 1;

			double rate = 1.0;
			if (avgTime < 1e-6 || sumSpeed < 1e-6) {
				continue;
			}else{
				rate = (1 / avgTime) / (sumSpeed / totalServerNum);
			}
			serverRanks[i].healthySelectRate = rate;		  /**< 该机器相对平均水平的性能 可能大于1 */
		}

		// 进行流量切分，选备机 ,大体思路是从速度大于_K倍的里面选均衡最近的，如没有找到则不分流量
		for (uint i = 0; i < serverNum; i++)		  /**< 记录HealtyFilter的Debug日志     */
		{
		    server_rank_t* serverRanks = request->serverRanks;
		    server_rank_t& sr = serverRanks[i];
		    uint serverId = sr.serverId;
		    double avgTime = _status[serverId]->getTotalAvgTime();

		    backup_machine_info_t backup[MAX_SERVER_SIZE];
		    uint backupCount = 0;
		    if (rand() % 100 > serverRanks[i].healthySelectRate * 100) {
		        double total = 0;
		        // 获取全部可用备机
		        for (uint j = 0; j < serverNum; j++) {
		            if (serverRanks[j].disabled || serverRanks[j].crossRoom|| i == j) {
		                continue;
		            }
		            uint backupId = serverRanks[j].serverId;
		            if (backupId >= MAX_SERVER_SIZE) {
		                UB_LOG_WARNING("serverId[%u] error", backupId);
		                continue;
		            }
		            double tj = _status[backupId]->getTotalAvgTime();
		            double nj = 1 / tj;
		            if (avgTime >= tj * _K)
		            {
		                backup_machine_info_t& bu = backup[backupCount++];
		                bu.balance = serverRanks[j].balanceRank;
		                bu.serverId = backupId;
		                bu.speed = nj;
		                total += nj;
		            }
		        }
		        if (backupCount > 0){
		            //对选择出来的备机，按均衡策略得分balanceRank优先选择
		            std::sort(backup, backup + backupCount, _backup_cmp);
		            backup_machine_info_t& bu = backup[0];
		            serverRanks[i].backupServerId = bu.serverId;
		            serverRanks[i].disabled = true;
		            serverRanks[i].useBackup = true;
		        }
		    }// 切流量结束
		    UB_LOG_TRACE("HealthyFilter doFilter: server_id[%u] sum_speed[%g] healthy_score[%g]"
		            " speed[%g] disabled[%d] use_backup[%d] backup_id[%u]",
		            serverId, sumSpeed, sr.healthyScore, sr.healthySelectRate, sr.disabled,
		            sr.useBackup, sr.backupServerId);
		}//end for

		//CHECK_POOL("HealthyFilter::doFilter ",resource->rp);
		return 0;
	}

	bool HealthyFilter::_backup_cmp(backup_machine_info_t a1, backup_machine_info_t a2)
	{
		return a1.balance > a2.balance;
	}

	int HealthyFilter::load(const comcfg::ConfigUnit & conf, SLB_Resource * resource)
	{
		int queueSize = SLB_DEF_QUEUE_SIZE;
		try
		{
			int loadErr; // int
			_timeout = conf[SLB_HealthyTimeout].to_uint32(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("HealthyFilter::load fail with no %s so HealthyFilter is OFF", SLB_HealthyTimeout);
				return 0;
			}
			_checkTime = conf[SLB_HealthyCheckTime].to_uint32(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("HealthyFilter::load fail with no %s so HealthyFilter is OFF", SLB_HealthyCheckTime);
				return 0;
			}
			_minRate = conf[SLB_HealthyMinRate].to_double(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("HealthyFilter::load fail with no %s so HealthyFilter is OFF", SLB_HealthyMinRate);
				return 0;
			}
			_K = conf[SLB_HealthyBackupThreshold].to_double(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("HealthyFilter::load fail with no %s so HealthyFilter is OFF", SLB_HealthyBackupThreshold);
				return 0;
			}
			queueSize = conf[SLB_HealthyQueueSize].to_int32(& loadErr);
			if (0 != loadErr) {
				queueSize = SLB_DEF_QUEUE_SIZE;
				UB_LOG_WARNING("HealthyFilter::load no HealthyQueueSize we use default:%d", queueSize);
			} else if (queueSize > SLB_DEF_QUEUE_SIZE || queueSize <= 0) {
				queueSize = SLB_DEF_QUEUE_SIZE;
			}
		}
		catch (...)
		{
			UB_LOG_WARNING("HealthyFilter::load conf with exception,so HealthyFilter is OFF");
			return 0;
		}

		try {
			long last = 0,curr = 0;
			bsl::ResourcePool * rp = resource->rp;
			//CHECK_POOL("HealthyFilter before create Status ",rp);
			for (uint i = 0; i < MAX_SERVER_SIZE; i++)
			{
				//_status[i] = new HealthyStatus(queueSize);
				_status[i] = rp->createp<HealthyStatus>(queueSize, resource);
				curr = (long)_status[i];
				last = last - curr;
				//UB_LOG_DEBUG("create HealthyStatus[%d] from pool[%p] dif[%ld]", i, _status[i], last);
				last = curr;
			}
		} catch (...) {
			UB_LOG_WARNING("HealthyFilter::load create HealthyStatus with exception,so HealthyFilter is OFF");
			return 0;
		}
		_loadSucc = true;
		return 0;
	}

	/**
	 * @note 此reload接口，不做互斥保证，有上层调用者保证
	 */
	int HealthyFilter::reload(const comcfg::ConfigUnit& conf) {
		if (!_loadSucc) { // 配置加载没有成功，表示不需要
			return 0;
		}

		uint timeout;
		uint checkTime;
		double minRate;
		double K;
		_loadSucc = false; // 默认关闭，配置项加载成功时才开启
		try
		{
			int loadErr; // int
			timeout = conf[SLB_HealthyTimeout].to_uint32(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("HealthyFilter::reload fail with no HealthyTimeout");
				return 0;
			}
			checkTime = conf[SLB_HealthyCheckTime].to_uint32(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("HealthyFilter::reload fail with no HealthyCheckTime");
				return 0;
			}
			minRate = conf[SLB_HealthyMinRate].to_double(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("HealthyFilter::reload fail with no HealthyMinRate");
				return 0;
			}
			K = conf[SLB_HealthyBackupThreshold].to_double(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("HealthyFilter::reload fail with no HealthyBackupThreshold");
				return 0;
			}
		} catch (...){
			UB_LOG_WARNING("HealthyFilter::reload with exception,so HealthyFilter is OFF");
			return 0;
		}
		_timeout = timeout;
		_checkTime = checkTime;
		_minRate = minRate;
		_K = K;
		_loadSucc = true;
		UB_LOG_TRACE("RELOAD HealthyFilter SUCC");
		return 0;
	}

	int HealthyFilter::updateRequestStatus(slb_server_t *server, const slb_talk_returninfo_t *talk)
	{
		if (!_loadSucc) { // 配置加载没有成功，表示不需要
			return 0;
		}

		int id = server->id;
		if ((uint)id >= MAX_SERVER_SIZE)
			return -1;

		uint proctime  = (uint)talk->realreadtime;
		uint currentTm = time(NULL);
		_status[id]->add(proctime, currentTm);

		return 0;
	}

	void HealthyFilter::debug(uint num) {
		if (!_loadSucc) {
			return;
		}
		for (uint i = 0; i < num && i < MAX_SERVER_SIZE; i++) {
			_status[i]->dump(i);
		}
	}

}



