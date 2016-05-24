/*
 * ConsistencyBalance.cpp
 *
 *  Created on: 2010-4-13
 *      Author: Administrator
 */

#include "ConsistencyBalance.h"
#include <openssl/md5.h>

namespace forum
{
	int ConsistencyBalance::balanceServer(request_t* request)
	{
		if (request == NULL || request->serverNum == 0)
		{
			UB_LOG_WARNING("balanceServer arg error[request=%p server_num=%u]",
					request, request ? request->serverNum : 0);
			return -1;
		}

		for (uint i = 0; i < request->serverNum; i++)
		{
			slb_server_t* svr = request->serverRanks[i].server;
			char buff[1024];
			int len = snprintf(buff, sizeof(buff), "%s#%u#%d", svr->ip, svr->port, request->key);
			unsigned int md5res[4];
			unsigned char *md5 = (unsigned char*)(md5res);
			//算签名, 直接MD5就好了
			MD5((unsigned char*)buff,len, md5);
			request->serverRanks[i].balanceRank = md5res[0] + md5res[1] + md5res[2] + md5res[3];
		}

		return 0;
	}
}
