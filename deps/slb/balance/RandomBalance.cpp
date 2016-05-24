/*
 * RandomBalance.cpp
 *
 *  Created on: 2010-4-6
 *      Author: Administrator
 */

#include <stdlib.h>

#include "RandomBalance.h"

namespace forum
{
	RandomBalance::RandomBalance()
	{
		srand(time(NULL));
	}

	int RandomBalance::balanceServer(request_t* request)
	{
		if (request == NULL || request->serverNum == 0)
		{
			UB_LOG_WARNING("balanceServer arg error[request=%p server_num=%u]",request, request ? request->serverNum : 0);
			return -1;
		}

		for (uint i = 0; i < request->serverNum; i++)
		{
			request->serverRanks[i].balanceRank = rand();
//			printf("server[%u]: %lu\n", i, request->serverRanks[i].balanceRank);
		}

		return 0;
	}

}
