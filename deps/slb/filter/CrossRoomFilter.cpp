/*
 * CrossRoomFilter.cpp
 *
 *  Created on: 2010-4-11
 *      Author: wangbo
 */

#include "CrossRoomFilter.h"

namespace forum
{
	CrossRoomFilter::CrossRoomFilter() {
		_loadSucc = false;
	}

	CrossRoomFilter::~CrossRoomFilter() {
	}

	int CrossRoomFilter::load(const comcfg::ConfigUnit & conf, SLB_Resource * /*resource*/)
	{
		_loadSucc = false;
		const char* cross = NULL;
		//CHECK_POOL("CrossRoomFilter before ",resource->rp);
		try {
			cross = conf[SLB_CrossRoom].to_cstr();
		} catch (...) {
			UB_LOG_WARNING("CrossRoomFilter::load fail with no CrossRoom so CrossRoomFilter is OFF");
			return 0;
		}
		if (cross != NULL && strncasecmp(cross, "On", sizeof("On")) == 0) {
			_loadSucc = true;
		}
		//CHECK_POOL("CrossRoomFilter after ",resource->rp);
		return 0;
	}
	int CrossRoomFilter::reload(const comcfg::ConfigUnit & conf) {
		const char* cross = NULL;
		try {
			cross = conf[SLB_CrossRoom].to_cstr();
		} catch (...) {
			UB_LOG_WARNING("CrossRoomFilter::reload fail with no CrossRoom so CrossRoomFilter is OFF");
			return 0;
		}
		if (cross != NULL && strncasecmp(cross, "On", sizeof("On")) == 0) {
			_loadSucc = true;
		} else {
			_loadSucc = false;
		}
		UB_LOG_TRACE("RELOAD CrossRoomFilter[%s]", _loadSucc ? "On" : "Off");
		return 0;
	}

	/**
	 * @brief crossRoom的策略描述：
	 *  只有第一次访问才考虑cross（重试不考虑）；
	 *  如果本机房能找到enable的server；
	 *      如果这个server，被负载均衡设置为disable；
	 *          去其他机房找第一个机器；
	 *
	 *  可以清晰看到：enable的server，因为负载均衡打分低被负载均衡disable，可以crossRoom；
	 *  那么，问题来了，对于外部disable的server（标记不可用的），是否需要crossRoom ？
	 *
	 *  从需求上看，对于外部标记为disable的server（不可用，不存活），分两种情况：
	 *  1. 偶尔不稳定，可以cross room;
	 *  2. 长期摘除；不能cross room;
	 *  对于这两种情况，服务端在没有重启的情况下，对于server都是采取标记为disable
	 *
	 *  这里，目前的问题是外部标记为disable的server要不要cross room？
	 *  根据上述分析，暂没有完美的方案，这块目前需求不大，后续有需求了再行处理
	 */
	int CrossRoomFilter::doFilter(request_t* request)
	{
		//CHECK_POOL("CrossRoomFilter::doFilter ",resource->rp);
		if (!_loadSucc) { // 配置加载没有成功，表示不需要
			return 0;
		}
		int retry = request->nthRetry;
		if (retry > 0)	// 只有第一次访问才考虑跨机房.
			return 0;

		server_rank_t* sr = NULL; // 根据serverRanks顺序找当前机房第一台可用机器
		for (uint i = 0; i < request->serverNum; i++)
		{
			server_rank_t& r = request->serverRanks[i];
			if (!r.crossRoom && r.server->enable)
			{
				sr = &r;
				break;
			}
		}

		if (sr == NULL){
			return 0;
		}
		if (!sr->disabled){ // 如果第一台可用的机器健康 则不用跨机房
			return 0;
		}
		server_rank_t* cross = NULL;
		for (uint i = 0; i < request->serverNum; i++){	  /**< 第一台机器不健康，找跨机房的第一台机器 */
			server_rank_t& r = request->serverRanks[i];
			if (r.crossRoom && r.server->enable &&!r.disabled){
				cross = &r;
				break;
			}
		}

		if (cross != NULL){
			sr->backupServerId = cross->serverId;
			sr->useBackup = true;
			//UB_LOG_DEBUG("CrossRoom serverId %d",cross->serverId);
		}

		// 把不是本机房的都关闭掉(除了选中的跨机房机器)
		for (uint i = 0; i < request->serverNum; i++){
			server_rank_t& r = request->serverRanks[i];
			if (cross != NULL && r.serverId == cross->serverId){		  /**< 跨机房生效，该server不淘汰   */
				continue;
			}

			if (r.crossRoom && !r.disabled){		  /**< 淘汰掉跨机房的其它机器   */
				r.disabled = true;
			}

			UB_LOG_TRACE("CrossRoomFilter doFilter: server_id[%u] cross_room[%d] "
					"disabled[%d] use_backup[%d] backup_id[%u]",
					r.serverId, r.crossRoom, r.disabled, r.useBackup, r.backupServerId);
		}

		//CHECK_POOL("CrossRoomFilter::doFilter ",resource->rp);
		return 0;
	}
}
