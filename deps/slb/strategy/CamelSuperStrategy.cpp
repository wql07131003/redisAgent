/*
 * CamelSuperStrategy.cpp
 *
 *  Created on: 2010-9-23
 *      Author: Administrator
 */

#include "CamelSuperStrategy.h"
#include "BalanceFilter.h"
#include "ConnectFilter.h"
#include "HealthyFilter.h"
#include "CrossRoomFilter.h"
#include "PrepareFilter.h"
#include "SelectFilter.h"

namespace forum
{
	// 这里extern出来，是为了在后面debug，分析是否buf不够引起问题
	// extern bsl::xnofreepool * debug_pool = NULL;
	// 有可能在多进程中，同时使用该buf创建对象，关于_buf _pool _rp在多进程间使用全部要加锁
	CamelSuperStrategy::CamelSuperStrategy(char * buf, int bufSize) {
		//UB_LOG_DEBUG("new CamelSuperStrategy, bufSize:%d ConnectStatus:%u HealthyStatus:%u pthread_mutex_t:%u",
		//		bufSize, sizeof(ConnectStatus), sizeof(HealthyStatus), sizeof(pthread_mutex_t));
		if (NULL == buf && bufSize <= 0) {
			UB_LOG_FATAL("CamelSuperStrategy get buf fail buf[%p],bufSize[%d]", buf, bufSize);
		}
		_buf = buf;
		_bufSize = bufSize;
		_pool.create(_buf, _bufSize);
		_rp = new bsl::ResourcePool(_pool);

		ChainFilter* filter = new ChainFilter();

		filter->addFilter(new PrepareFilter());
		filter->addFilter(new BalanceFilter());
		filter->addFilter(new ConnectFilter());
		filter->addFilter(new HealthyFilter());
		filter->addFilter(new CrossRoomFilter());
		filter->addFilter(new SelectFilter());

		_chain_filter = filter;
		_isDebug = false;
		//UB_LOG_DEBUG("CamelSuperStrategy(): pool[%p],buf_size[%lu],buf_left[%lu],buf_free[%lu]", _pool._buffer, _pool._bufcap, _pool._bufleft, _pool._free);
	}

	CamelSuperStrategy::~CamelSuperStrategy() {
		uint sz = _chain_filter->getFilterSize();
		for (uint i = 0; i < sz; i++)
		{
			Filter* filter = _chain_filter->getFilter(i);
			delete filter;
		}
		delete _chain_filter;

		//UB_LOG_DEBUG("~CamelSuperStrategy() RESET before: pool[%p],buf_size[%lu],buf_left[%lu],buf_free[%lu]", _pool._buffer, _pool._bufcap, _pool._bufleft, _pool._free);
		_rp->reset();
		delete _rp;
		//UB_LOG_DEBUG("~CamelSuperStrategy() RESET after: pool[%p],buf_size[%lu],buf_left[%lu],buf_free[%lu]", _pool._buffer, _pool._bufcap, _pool._bufleft, _pool._free);
	}

	int CamelSuperStrategy::load(const comcfg::ConfigUnit & conf) {
		//conf.print(4);
		int ret;
		SLB_Resource resource;
		resource.rp = _rp;
		resource.type = SLB_RESOURCE_TYPE_SHM;


		//UB_LOG_DEBUG("CamelSuperStrategy LOAD before: pool[%p],buf_size[%lu],buf_left[%lu],buf_free[%lu]", _pool._buffer, _pool._bufcap, _pool._bufleft, _pool._free);
		ret = _chain_filter->load(conf, &resource);
		//UB_LOG_DEBUG("CamelSuperStrategy LOAD after: pool[%p],buf_size[%lu],buf_left[%lu],buf_free[%lu]", _pool._buffer, _pool._bufcap, _pool._bufleft, _pool._free);

		return ret;
	}

	int CamelSuperStrategy::reload(const comcfg::ConfigUnit & conf) {
		int ret;
		//UB_LOG_DEBUG("CamelSuperStrategy RELOAD before : pool[%p],buf_size[%lu],buf_left[%lu],buf_free[%lu]", _pool._buffer, _pool._bufcap, _pool._bufleft, _pool._free);
		ret = _chain_filter->reload(conf);
		//UB_LOG_DEBUG("CamelSuperStrategy RELOAD after: pool[%p],buf_size[%lu],buf_left[%lu],buf_free[%lu]", _pool._buffer, _pool._bufcap, _pool._bufleft, _pool._free);
		return ret;
	}


	int CamelSuperStrategy::setServerArgAfterConn(void *xxServer, int errNo) {
		CHECK_POOL("BEFORE updateConnect",_rp);
		if (xxServer == NULL || copyCall == NULL) {
			return -1;
		}

		slb_server_t server;
		copyCall(xxServer, &server);		  /**< 将外部Server拷贝到server中  */
		int ret = _chain_filter->updateConnectStatus(&server, errNo);
		CHECK_POOL("AFTER updateConnect",_rp);
		return ret;
	}

	int CamelSuperStrategy::setServerArg(void *xxServer, const slb_talk_returninfo_t *talk) {
		CHECK_POOL("BEFORE updateRequest",_rp);
		if (xxServer == NULL || talk == NULL || copyCall == NULL) {
			return -1;
		}

		slb_server_t server;
		copyCall(xxServer, &server);
		int ret = _chain_filter->updateRequestStatus(&server, talk);
		CHECK_POOL("AFTER updateRequest",_rp);
		return ret;
	}

	int CamelSuperStrategy::fetchServer(slb_request_t * req) {
		CHECK_POOL("BEFORE doFilter ",_rp);
		if (req == NULL || copyCall == NULL) {
			return -1;
		}
		if (_isDebug) {
			_chain_filter->debug(req->serverNum);
		}
		const int serverNum = req->serverNum;
		server_rank_t serverRank[serverNum];
		memset(serverRank, 0, sizeof (serverRank));		  /**< disable=false  */
		request_t request;
		memset(&request, 0, sizeof (request));
		request.serverRanks        = serverRank;
		request.mustSelectOneServer= true;
		request.needHealthyFilter  = true;

		// get from req
		request.key = req->key;
		request.nthRetry = req->nthRetry;
		snprintf(request.currentMachineRoom, sizeof(request.currentMachineRoom), "%s", req->currentMachineRoom);
		request.serverNum = serverNum;

		if (serverNum > static_cast<int>(MAX_SERVER_SIZE)) {
			UB_LOG_WARNING("too many server while fetchServer, get %d, suppert %d", serverNum, MAX_SERVER_SIZE);
			return -1;
		}
		for (int i = 0; i < serverNum; i++) {
			copyCall(req->svr[i], request.server + i);
		}
		int ret = _chain_filter->doFilter(&request);		  /**< 返回找到的serverID  */
		server_rank_t* serv = NULL;

		for (int i = 0; ret >= 0 && i < serverNum; i++)		  /**< 找到该serverID 的server_rank_t  */
		{
			server_rank_t& sr = serverRank[i];
			if (sr.serverId == (uint)ret)
			{
				serv = &sr;
				break;
			}
		}

		if (serv == NULL){
			ret = -1;
			UB_LOG_WARNING("SuperStrategy<fetchServer: failed ret[%d]", ret);
		}
		/*else{
			UB_LOG_DEBUG("SuperStrategy<fetchServer: server_id[%u] machine_room[%s] cross_room[%d] balance[%lu] \
					connect[%g] healthy_score[%g] healthy_select[%g] disabled[%d] use_backup[%d] backup_server[%u] \
					retry[%d]  key[%d]",
					serv->serverId, serv->machineRoom, serv->crossRoom, serv->balanceRank, serv->connectScore,
					serv->healthyScore, serv->healthySelectRate, serv->disabled, serv->useBackup, 
					serv->backupServerId, request.nthRetry, request.key);
		}*/


		CHECK_POOL("AFTER doFilter ",_rp);
		return ret;
	}

}
