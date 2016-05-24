/*
 * Balance.h
 *
 *  Created on: 2010-4-6
 *      Author: Administrator
 */

#ifndef BALANCE_H_
#define BALANCE_H_

#include "request_t.h"

namespace forum
{

	class Balance
	{
	public:
		virtual ~Balance() { }

		virtual int balanceServer(request_t* request) = 0;

		virtual int afterSelect(int selectId, request_t* request);
	};

}

#endif /* BALANCE_H_ */
