/*
 * BalanceFactory.cpp
 *
 *  Created on: 2010-4-12
 *      Author: Administrator
 */

#include "BalanceFactory.h"
#include "RandomBalance.h"
#include "RoundRobinBalance.h"
#include "ConsistencyBalance.h"

namespace forum
{

	Balance* BalanceFactory::getBalance(const comcfg::ConfigUnit & conf)
	{
		const char* balance = NULL;
		Balance* ret = NULL;

		try
		{
			balance = conf[SLB_Balance].to_cstr();
		}
		catch (...)
		{}

		if (balance == NULL)
		{
			ret = new RandomBalance();
			UB_LOG_WARNING("BalanceFactory::getBalance use default balance: RandomBalance");
		}
		else if (strcasecmp("Random", balance) == 0)
		{
			ret = new RandomBalance();
		}
		else if (strcasecmp("Consistency", balance) == 0)
		{
			ret = new ConsistencyBalance();
		}
		else if (strcasecmp("RoundRobin", balance) == 0)
		{
			ret = new RoundRobinBalance();
		}
		else
		{
			UB_LOG_FATAL("unknown Balance[%s]", balance);
		}

		return ret;
	}

}
