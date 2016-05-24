/*
 * TbStrategy.h
 *
 *  Created on: 2010-9-24
 *      Author: Administrator
 */

#ifndef TBSTRATEGY_H_
#define TBSTRATEGY_H_

#include <request_t.h>

namespace forum {
	/**
	 * @brief 转换server信息的call back方法定义
	 */
	typedef void (* copy_server_call_t)(void * xxServer, slb_server_t * server);

	/**
	 * @brief 接口类
	 */
	class TbStrategy {
	protected:
		copy_server_call_t copyCall; /**< 转化server信息的call back方法 */
		bool _isDebug;
	public:
		/**
		 * @brief
		 */
		TbStrategy() {copyCall = NULL;}
		/**
		 * @brief
		 */
		virtual ~TbStrategy() {};
		/**
		 * @brief 接口
		 */
		virtual int load(const comcfg::ConfigUnit & conf) = 0;
		/**
		 * @brief 接口
		 */
		virtual int reload(const comcfg::ConfigUnit & conf) = 0;
		/**
		 * @brief 接口
		 */
		virtual int fetchServer(slb_request_t * req) = 0;
		/**
		 * @brief 接口
		 */
		virtual int setServerArgAfterConn(void *xxServer, int errNo) = 0;
		/**
		 * @brief 接口
		 */
		virtual int setServerArg(void * xxServer, const slb_talk_returninfo_t *talk) = 0;
		/**
		 * @brief 接口
		 */
		virtual void setCallBack(copy_server_call_t call) {
			copyCall = call;
		}
		virtual void setDebug(bool isDebug) {
			_isDebug = isDebug;
		}
	};
}

#endif /* TBSTRATEGY_H_ */
