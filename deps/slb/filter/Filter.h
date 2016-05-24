/*
 * Filter.h
 *
 *  Created on: 2010-4-7
 *      Author: Administrator
 */

#ifndef FILTER_H_
#define FILTER_H_

#include "request_t.h"

namespace forum
{
	class Filter
	{
	public:
		virtual ~Filter() {}

		/*
		 * @brief 注意：所有回调函数，返回失败都会导致chain退出
		 *   因此，除非是严重错误，否则不能返回失败
		 */
		virtual int doFilter(request_t* request) = 0;

		virtual int doAfter(int selectId, request_t* request);

		virtual int load(const comcfg::ConfigUnit& conf, SLB_Resource * resource) = 0;

		/*
		 * @brief 在PHP中间层中新增reload接口，由于C中reload需求不大，暂时不做线程安全，C中不能用
		 */
		virtual int reload(const comcfg::ConfigUnit& conf);

		virtual int updateConnectStatus(slb_server_t *server, int errNo);

		virtual int updateRequestStatus(slb_server_t *server, const slb_talk_returninfo_t *talk);

		/**
		 * @brief dump接口
		 * @param num: server数目
		 */
		virtual void debug(uint num);
	};
}

#endif /* FILTER_H_ */
