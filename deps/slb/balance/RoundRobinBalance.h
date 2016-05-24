/*
 * RoundRobinBalance.h
 *
 *  Created on: 2010-4-12
 *      Author: Administrator
 */

#ifndef ROUNDROBINBALANCE_H_
#define ROUNDROBINBALANCE_H_

#include "Balance.h"
#include "Lock.h"

namespace forum
{
	class RoundRobinBalance: public forum::Balance
	{
	public:
		RoundRobinBalance();
		virtual ~RoundRobinBalance() {}

		virtual int balanceServer(request_t* request);

	private:
		Lock _lock;
		int _lastId;
	};
}

#endif /* ROUNDROBINBALANCE_H_ */
