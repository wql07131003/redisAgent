/*
 * BalanceFactory.h
 *
 *  Created on: 2010-4-12
 *      Author: Administrator
 */

#ifndef BALANCEFACTORY_H_
#define BALANCEFACTORY_H_

#include "Balance.h"

namespace forum
{

	class BalanceFactory
	{
	public:
		static Balance* getBalance(const comcfg::ConfigUnit & conf);
	};

}

#endif /* BALANCEFACTORY_H_ */
