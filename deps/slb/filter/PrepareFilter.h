/*
 * PrepareFilter.h
 *
 *  Created on: 2010-4-11
 *      Author: wangbo
 */

#ifndef PREPAREFILTER_H_
#define PREPAREFILTER_H_

#include "Filter.h"
#include "Queue.h"

namespace forum
{

	class PrepareFilter: public forum::Filter
	{
	public:
		virtual ~PrepareFilter() {}

		virtual int doFilter(request_t* request);
		virtual int load(const comcfg::ConfigUnit& /*conf*/, SLB_Resource * /*resource*/)
		{
			return 0;
		}
	};

}

#endif /* PREPAREFILTER_H_ */
