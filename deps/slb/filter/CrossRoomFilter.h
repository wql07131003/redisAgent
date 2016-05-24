/*
 * CrossRoomFilter.h
 *
 *  Created on: 2010-4-11
 *      Author: wangbo
 */

#ifndef CROSSROOMFILTER_H_
#define CROSSROOMFILTER_H_

#include "Filter.h"
#include "Queue.h"

namespace forum
{

	class CrossRoomFilter : public Filter
	{
	public:
		CrossRoomFilter();
		virtual ~CrossRoomFilter();

		virtual int load(const comcfg::ConfigUnit& conf, SLB_Resource * resource);
		virtual int reload(const comcfg::ConfigUnit & conf);
		virtual int doFilter(request_t* request);
	private:
		bool _loadSucc; // 表示是否加载成功，加载成功才启用
	};

}

#endif /* CROSSROOMFILTER_H_ */
