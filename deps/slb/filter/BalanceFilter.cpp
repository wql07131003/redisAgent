/*
 * BalanceFilter.cpp
 *
 *  Created on: 2010-4-7
 *      Author: Administrator
 */

#include "BalanceFilter.h"
#include "RandomBalance.h"
#include "BalanceFactory.h"

namespace forum
{
	BalanceFilter::BalanceFilter() : _balance(NULL)
	{}

	BalanceFilter::~BalanceFilter()
	{
		delete _balance;
		_balance = NULL;
	}

	int BalanceFilter::doFilter(request_t* request)
	{
		//CHECK_POOL("BalanceFilter::doFilter ",resource->rp);
		if (_balance == NULL) {
			return -1;
		}

		int ret = _balance->balanceServer(request);
		if (ret < 0) {
			return ret;
		}

		std::sort(request->serverRanks, request->serverRanks + request->serverNum, _cmp_rank);

		for (uint i = 0; i < request->serverNum; i++)
		{
			server_rank_t& r = request->serverRanks[i];

			UB_LOG_TRACE("BalanceFilter doFilter: server_id[%u] balance[%lu]",
							r.serverId, r.balanceRank);
		}
		//CHECK_POOL("BalanceFilter::doFilter ",resource->rp);
        return 0;
	}

	int BalanceFilter::doAfter(int selectId, request_t* request)
	{
		if (_balance == NULL) {
			return -1;
		}

		int ret = _balance->afterSelect(selectId, request);
        return ret;
	}

	int BalanceFilter::load(const comcfg::ConfigUnit & conf, SLB_Resource *  /*resource*/)
	{
		//CHECK_POOL("BalanceFilter before ",resource->rp);
		_balance = BalanceFactory::getBalance(conf);

		if (_balance == NULL) {
			UB_LOG_WARNING("BalanceFilter::load fail");
			return -1;
		}
		//CHECK_POOL("BalanceFilter after ",resource->rp);
        return 0;
	}

	/**
	 * @note 由上层调用者保证互斥
	 */
	int BalanceFilter::reload(const comcfg::ConfigUnit& conf) {
		Balance * balance = BalanceFactory::getBalance(conf);
		if (balance == NULL) {
			UB_LOG_WARNING("BalanceFilter::reload fail");
			return 0;
		}
		if (_balance != NULL) {
			delete _balance;
			_balance = NULL;
		}
		_balance = balance;
		UB_LOG_TRACE("RELOAD BalanceFilter SUCC");
		return 0;
	}

	bool BalanceFilter::_cmp_rank(server_rank_t s1, server_rank_t s2)
	{
		return s1.balanceRank > s2.balanceRank;
	}

}
