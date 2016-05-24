/*
 * ConnectFilter.cpp
 *
 *  Created on: 2010-4-7
 *      Author: Administrator
 */

#include "ConnectFilter.h"
#include "server_rank_t.h"

namespace forum
{
	ConnectStatus::ConnectStatus(int queueSize, SLB_Resource * resource) : _queue(queueSize, resource->rp)
	{
		_failedCount = 0;
		_mutex = resource->getMutex();
	}

	ConnectStatus::~ConnectStatus()
	{
		_failedCount = 0;
	}

	void ConnectStatus::insert(int status)
	{
		AutoLock _lock(_mutex);
		int lastStatus = 0;
		//0 succ 1 fail
		bool r = _queue.insert(status, &lastStatus);

		if (r && lastStatus != 0) {	  /**< 队列满且获取到队头数据为交互失败 */
			_failedCount--;
		}

		if (status != 0) {	  /**< 本次交互失败       */
			_failedCount++;
		}
	}

	int ConnectStatus::getFailedCount() const
	{
		return _failedCount;
	}

	void ConnectStatus::dump(uint index)
	{
		UB_LOG_TRACE("DUMP : ConnectStatus[%d]", index);
		_queue.dump();
	}

	ConnectFilter::ConnectFilter() 
	{
		memset(_connectStatus, 0, sizeof (_connectStatus));
		_x1 = _y1 = 0;
		_x2 = _y2 = 0;
		_loadSucc = false;
	}

	ConnectFilter::~ConnectFilter()
	{
		for (uint i = 0; i < MAX_SERVER_SIZE; i++)
		{
			//使用resourcePool管理资源
			//delete _connectStatus[i];
			_connectStatus[i] = NULL;
		}
	}

	int ConnectFilter::doFilter(request_t* request)
	{
		//CHECK_POOL("ConnectFilter::doFilter ",resource->rp);
		if (!_loadSucc) { // 配置加载没有成功，表示不需要
			return 0;
		}

		uint serverNum = request->serverNum;

		for (uint i = 0; i < serverNum; i++)
		{
			server_rank_t* serverRanks = request->serverRanks;
			server_rank_t& serverRank = serverRanks[i];
			uint serverId = serverRank.serverId;

			if (serverId >= MAX_SERVER_SIZE)
			{
				UB_LOG_WARNING("serverId[%u] error", serverId);
				continue;
			}

			ConnectStatus* status = _connectStatus[serverId];

			int failedCount = status->getFailedCount();

			double rate = 0;
			if (0 <= failedCount && failedCount < _x1)
			{
				rate = 1.0 * failedCount * (_y1 - 100) / _x1 + 100;
			}
			else if (_x1 <= failedCount && failedCount < _x2)
			{
				rate = 1.0 * failedCount * (_y2 - _y1) / (_x2 - _x1) +
					_y1 - (_y2 - _y1) / (_x2 - _x1) * _x1;
			}
			else
			{
				rate = _y2;
			}

			serverRank.connectScore = rate / 100;

			int rnd = rand() % 100;
			if (rnd > rate)
				serverRank.disabled = true;

			UB_LOG_TRACE("ConnectFilter doFilter: server_id[%u] rate[%g] rand[%d] disabled[%d]",
					serverId, rate, rnd, serverRank.disabled);
		}

		//CHECK_POOL("ConnectFilter::doFilter ",resource->rp);
		return 0;
	}

	int ConnectFilter::load(const comcfg::ConfigUnit & conf, SLB_Resource * resource)
	{
		int queueSize = 100;

		try
		{
			int loadErr; // int
			_x1 = conf[SLB_ConnectX1].to_double(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("ConnectFilter::load fail with no ConnectX1 so ConnectFilter is OFF");
				return 0;
			}
			_y1 = conf[SLB_ConnectY1].to_double(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("ConnectFilter::load fail with no ConnectY1 so ConnectFilter is OFF");
				return 0;
			}
			_x2 = conf[SLB_ConnectX2].to_double(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("ConnectFilter::load fail with no ConnectX2 so ConnectFilter is OFF");
				return 0;
			}
			_y2 = conf[SLB_ConnectY2].to_double(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("ConnectFilter::load fail with no ConnectY2 so ConnectFilter is OFF");
				return 0;
			}

			queueSize = conf[SLB_ConnectQueueSize].to_int32(& loadErr);
			if (0 != loadErr) {
				queueSize = 100;
				UB_LOG_WARNING("queueSize::load no ConnectQueueSize we use default:%d", queueSize);
			} else if (queueSize > SLB_DEF_QUEUE_SIZE || queueSize <= 0) {
				queueSize = SLB_DEF_QUEUE_SIZE;
			}
		}
		catch (...)
		{
			UB_LOG_WARNING("ConnectFilter::load conf with exception,so ConnectFilter is OFF");
			return 0;
		}

		bsl::ResourcePool * rp = resource->rp;
		//CHECK_POOL("ConnectFilter before",rp);
		try {
			for (uint i = 0; i < MAX_SERVER_SIZE; i++)
			{
				//_connectStatus[i] = new ConnectStatus(queueSize);
				_connectStatus[i] = rp->createp<ConnectStatus>(queueSize, resource);
			}
		} catch (...) {
			UB_LOG_WARNING("ConnectFilter::load create ConnectStatus with exception,so ConnectFilter is OFF");
			return 0;
		}
		//CHECK_POOL("ConnectFilter after",rp);
		_loadSucc = true;
		return 0;
	}

	/**
	 * @note 此reload接口，不做互斥保证，有上层调用者保证
	 */
	int ConnectFilter::reload(const comcfg::ConfigUnit & conf)
	{
		if (!_loadSucc) { // 配置加载没有成功，表示不需要
			return 0;
		}
		double x1, y1, x2, y2;
		_loadSucc = false; // 默认关闭，当配置项都加载成功后再开启
		try
		{
			int loadErr; // int
			x1 = conf[SLB_ConnectX1].to_double(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("ConnectFilter::reload fail with no ConnectX1");
				return 0;
			}
			y1 = conf[SLB_ConnectY1].to_double(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("ConnectFilter::reload fail with no ConnectY1");
				return 0;
			}
			x2 = conf[SLB_ConnectX2].to_double(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("ConnectFilter::reload fail with no ConnectX2");
				return 0;
			}
			y2 = conf[SLB_ConnectY2].to_double(& loadErr);
			if (0 != loadErr) {
				UB_LOG_WARNING("ConnectFilter::reload fail with no ConnectY2");
				return 0;
			}
		} catch (...) {
			UB_LOG_WARNING("ConnectFilter::reload conf with exception,so ConnectFilter is OFF");
			return 0;
		}
		_x1 = x1;
		_y1 = y1;
		_x2 = x2;
		_y2 = y2;
		_loadSucc = true;
		UB_LOG_TRACE("RELOAD ConnectFilter SUCC");
		return 0;
	}

	int ConnectFilter::updateConnectStatus(slb_server_t *server, int errNo)
	{
		if (!_loadSucc) { // 配置加载没有成功，表示不需要
			return 0;
		}

		int id = server->id;
		if (id >= (int)MAX_SERVER_SIZE)
			return -1;

		_connectStatus[id]->insert(errNo != 0 ? 1 : 0); // succ:0 err:1

		UB_LOG_TRACE("ConnectFilter updateConnectStatus: server_id[%d] err_no[%d]",
				id, errNo);

		return 0;
	}

	void ConnectFilter::debug(uint num) {
		if (!_loadSucc) {
			return;
		}
		for (uint i = 0; i < num && i < MAX_SERVER_SIZE; i++) {
			_connectStatus[i]->dump(i);
		}
	}

}
