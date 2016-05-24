/*
 * BalanceFilter.h
 *
 *  Created on: 2010-4-7
 *      Author: Administrator
 */

#ifndef BALANCEFILTER_H_
#define BALANCEFILTER_H_

#include "Filter.h"
#include "Balance.h"
#include "Queue.h"

namespace forum
{

	class BalanceFilter : public Filter
	{
	public:
		BalanceFilter();
		virtual ~BalanceFilter();

		virtual int doFilter(request_t* request);  /**<根据均衡策略计算balanceRank，据此对request_t的serverRanks进行排序 */
		virtual int doAfter(int selectId, request_t* request);
		virtual int load(const comcfg::ConfigUnit & conf, SLB_Resource * resource);
		virtual int reload(const comcfg::ConfigUnit& conf);

	private:
		static bool _cmp_rank(server_rank_t s1, server_rank_t s2);

		Balance* _balance;
	};

}

#endif /* BALANCEFILTER_H_ */
