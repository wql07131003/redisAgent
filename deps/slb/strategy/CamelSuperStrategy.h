/*
 * CamelSuperStrategy.h
 *
 *  Created on: 2010-9-23
 *      Author: Administrator
 */

#ifndef CAMELSUPERSTRATEGY_H_
#define CAMELSUPERSTRATEGY_H_

#include "TbStrategy.h"
#include "ChainFilter.h"

namespace forum
{
	/**
	 * @brief 多进程模式下的负载均衡类
	 */
	class CamelSuperStrategy : public TbStrategy
	{
	public:
		/**
		 * @brief 构造函数，从外部传入buf，一般是共享内存
		 */
		CamelSuperStrategy(char * buf, int _bufSize);
		/**
		 * @brief 析构函数
		 */
		virtual ~CamelSuperStrategy();

		/**
		 * @brief load
		 */
		virtual int load(const comcfg::ConfigUnit & conf);
		/**
		 * @brief reload
		 */
		virtual int reload(const comcfg::ConfigUnit & conf);
		/**
		 * @brief fetchServer
		 */
		virtual int fetchServer(slb_request_t * req);
		/**
		 * @brief 连接状态信息
		 */
		virtual int setServerArgAfterConn(void *xxServer, int errNo);
		/**
		 * @brief 交互状态信息 realreadtime
		 */
		virtual int setServerArg(void *xxServer, const slb_talk_returninfo_t *talk);

	private:
		ChainFilter* _chain_filter; /**< filter chain */

		// 用于资源管理
		char * _buf; /**< 负载均衡所需的bug，多进程模式就是共享内存 */
		int _bufSize; /**< buf大小 */
		bsl::xnofreepool _pool; /**< 用于buf管理 */
		bsl::ResourcePool * _rp; /**< 用于buf管理 */
	};
}

#endif /* CAMELSUPERSTRATEGY_H_ */
