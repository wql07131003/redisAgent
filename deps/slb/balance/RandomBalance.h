/*
 * RandomBalance.h
 *
 *  Created on: 2010-4-6
 *      Author: Administrator
 */

#ifndef RANDOMBALANCE_H_
#define RANDOMBALANCE_H_

#include "Balance.h"

namespace forum
{

	class RandomBalance : public Balance
	{
	public:
		RandomBalance();
		virtual ~RandomBalance() {}

		virtual int balanceServer(request_t* request);
	};

}

#endif /* RANDOMBALANCE_H_ */
