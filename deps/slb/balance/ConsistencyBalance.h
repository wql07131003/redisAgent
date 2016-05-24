/*
 * ConsistencyBalance.h
 *
 *  Created on: 2010-4-13
 *      Author: Administrator
 */

#ifndef CONSISTENCYBALANCE_H_
#define CONSISTENCYBALANCE_H_

#include "Balance.h"

namespace forum
{

	class ConsistencyBalance : public forum::Balance		  /**< 一致hash */
	{
	public:
		virtual ~ConsistencyBalance() {}
		virtual int balanceServer(request_t* request);
	};

}

#endif /* CONSISTENCYBALANCE_H_ */
