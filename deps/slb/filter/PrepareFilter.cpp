/*
 * PrepareFilter.cpp
 *
 *  Created on: 2010-4-11
 *      Author: wangbo
 */

#include "PrepareFilter.h"

namespace forum
{
	int PrepareFilter::doFilter(request_t* request)
	{
		for (uint i = 0; i < request->serverNum; i++)
		{
			server_rank_t& sr = request->serverRanks[i];

			sr.serverId    = (uint)i;                          // 设置机器ID
			//sr.server      = request->connectManager->getServerInfo(sr.serverId); // 设置机器信息
			sr.server = request->server + i;
			sr.machineRoom = sr.server->tag;        // 设置机房信息

			if (sr.server == NULL || !sr.server->enable || // 设置机器健康状态
					!sr.server->isHealthy)
			{
				sr.disabled = true;
			}

			// 都存在，且不相等就认为跨机房！
			if (sr.machineRoom == NULL || request->currentMachineRoom == NULL ||
					sr.machineRoom[0] == 0 || request->currentMachineRoom[0] == 0 ||
					strcmp(sr.machineRoom, request->currentMachineRoom) == 0)
			{
				sr.crossRoom = false;
			}
			else
			{
				sr.crossRoom = true;
			}

			UB_LOG_TRACE("PrepareFilter doFilter: server_id[%u] enable[%d] isHealthy[%d] "
					"cross_room[%d] disabled[%d]",
			         sr.serverId, sr.server->enable, sr.server->isHealthy, sr.crossRoom,
			         sr.disabled);
		}
		return 0;
	}
}
