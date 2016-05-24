/*
 * ChainFilter.h
 *
 *  Created on: 2010-4-7
 *      Author: Administrator
 */

#ifndef CHAINFILTER_H_
#define CHAINFILTER_H_

#include <vector>
#include "Filter.h"
#include "request_t.h"

namespace forum
{
	class ChainFilter
	{
	public:
        virtual ~ChainFilter() {}
		virtual int doFilter(request_t* request);
		virtual int load(const comcfg::ConfigUnit & conf, SLB_Resource * resource);
		virtual int reload(const comcfg::ConfigUnit & conf);
		/**
		 * @brief 更新连接信息 0 成功 1失败
		 *
		 * @param server  更新指定server信息
		 * @param errNo  0 成功 1 失败
		 * @retval 0成功 <0失败
		**/
		virtual int updateConnectStatus(slb_server_t *server, int errNo);
		/**
		 * @brief 更新交互信息(读时间)
		 *
		 * @param server 更新指定server信息 
		 * @param talk  交互信息 实际读时间生效 
		 * @retval  0成功 <0失败
		**/
		virtual int updateRequestStatus(slb_server_t *server, const slb_talk_returninfo_t *talk);
		/**
		 * @brief debug接口
		 * @param num : server数目
		 */
		virtual void debug(uint num);

		void addFilter(Filter* filter);
		uint getFilterSize() const;
		Filter* getFilter(uint index);

	private:
		std::vector<Filter*> _filters;
	};
}

#endif /* CHAINFILTER_H_ */
