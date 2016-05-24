/*
 * RoundRobinBalance.cpp
 *
 *  Created on: 2010-4-12
 *      Author: Administrator
 */

#include "RoundRobinBalance.h"


namespace forum
{
	RoundRobinBalance::RoundRobinBalance() : _lock(), _lastId(-1)
	{
	}

	int RoundRobinBalance::balanceServer(request_t* request)
	{
		if (request == NULL || request->serverNum == 0)
		{
			UB_LOG_WARNING("balanceServer arg error[request=%p server_num=%u]",
					request, request ? request->serverNum : 0);
			return -1;
		}

		_lock.lock();
		int lastId = _lastId++;
		_lock.unlock();

		uint serverNum = request->serverNum;

//		printf("===============================> %d %u\n", lastId, serverNum);
		if (lastId < 0)
			lastId = -1;

		uint curId = (uint)(lastId + 1) % serverNum;

		for (uint i = 0; i < request->serverNum; i++)
		{
			if (i == curId)
			{
				request->serverRanks[i].balanceRank = serverNum + 1;
			}
			else
			{
				request->serverRanks[i].balanceRank = rand() % serverNum;
			}
		}

//		for (uint i = 0; i < request->serverNum; i++)
//		{
////			if (i <= curId)
////				request->serverRanks[i].balanceRank = i + serverNum;
////			else
////				request->serverRanks[i].balanceRank = i;
//			uint id = (i + curId) % serverNum;
//			request->serverRanks[id].balanceRank = serverNum - i;
//		}

		return 0;
	}
}
