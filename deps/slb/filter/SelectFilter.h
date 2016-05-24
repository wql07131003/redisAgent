/*
 * SelectFilter.h
 *
 *  Created on: 2010-4-11
 *      Author: wangbo
 */

#ifndef SELECTFILTER_H_
#define SELECTFILTER_H_

#include "Filter.h"
#include "Queue.h"

namespace forum
{

	class SelectFilter: public Filter
	{
	public:
		SelectFilter(){};
		virtual ~SelectFilter() {}

		virtual int doFilter(request_t* request);
		virtual int load(const comcfg::ConfigUnit& /*conf*/, SLB_Resource *  /*resource*/ ){
			return 0;
		}
	};

}

#endif /* SELECTFILTER_H_ */
