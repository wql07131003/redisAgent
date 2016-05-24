/*
 * SelectFilter.cpp
 *
 *  Created on: 2010-4-11
 *      Author: wangbo
 */

#include "SelectFilter.h"

namespace forum
{
	int SelectFilter::doFilter(request_t* request)
	{
		//CHECK_POOL("SelectFilter::doFilter ",resource->rp);
		int ret = -1;

		int retry = request->nthRetry;

		for (uint i = 0, count = 0; i < request->serverNum; i++)
		{
			int serverId = -1;
			server_rank_t& sr = request->serverRanks[i];

			if (sr.crossRoom) // 只要是跨机房, 就不选择
				continue;

			if (!sr.disabled){
				serverId = sr.serverId;
			}else if (sr.useBackup){
				serverId = sr.backupServerId;
			}
			//@TODO check这个retry作用
			if (serverId >= 0 && (int)count == retry){
				ret = serverId;
				//UB_LOG_DEBUG("SelectFilter server_id[%u] serverNum[%d ] retry[%d]", ret, request->serverNum, retry);
				break;
			}

			if (serverId >= 0){
				count++;
			}
		}

		if (ret < 0){
			// 如果没选到, 则随机选一个
			UB_LOG_TRACE("SelectFilter random select_server_id[%u] serverNum[%d ] retry[%d]", ret, request->serverNum, retry);
			uint rd = rand() % request->serverNum;
			for (uint i = 0; i < request->serverNum; i++){
				uint id = (i + rd) % request->serverNum;
				server_rank_t& sr = request->serverRanks[id];
				if (request->mustSelectOneServer){
					if (sr.server != NULL && sr.server->enable && //必须选中一个，忽略是否淘汰，在本机房随机选择
							sr.server->isHealthy && !sr.crossRoom){
						ret = sr.serverId;
						//UB_LOG_DEBUG("slb select failed,mustSelectOneServer random %d",ret);
						break;
					}
				}else{
					if (!sr.disabled){
						ret = sr.serverId;
						//UB_LOG_DEBUG("slb select failed,SelectOne random %d",ret);
						break;
					}
				}
			}
		}
		//CHECK_POOL("SelectFilter::doFilter ",resource->rp);
		UB_LOG_TRACE("SelectFilter::doFilter: select_server_id[%u] retry[%d]", ret, retry);
		return ret;
	}
}
