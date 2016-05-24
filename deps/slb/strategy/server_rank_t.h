/*
 * RandomBalance.h
 *
 *  Created on: 2010-4-6
 *      Author: Administrator
 */

#ifndef SERVER_BALANCE_RANK_H_
#define SERVER_BALANCE_RANK_H_
#include <sys/types.h>

namespace forum
{

	/**
	 * @brief 负载均衡内部使用的server结构体
	 */
	struct slb_server_t {
		bool isHealthy; /**< 健康状况 */
		bool enable; /**< 是否存活 */
		int id; /**< 在Service中分配的id */
		int port; /**< 端口 */
		const char * ip; /**< ip */
		const char * tag; /**< tag 机房信息 */
	};

	/**
	 * @brief 负载均衡内部使用的server打分结构体
	 */
	struct server_rank_t
	{
		uint serverId; /**< id */
		const char* machineRoom; /**< 机房 */
		bool crossRoom; /**< 是否跨机房 */
		//comcm::Server* server; // server
		slb_server_t * server; /**< server信息数组 */

		u_int64_t balanceRank; /**< 均衡策略选取得分 */
		double connectScore;   /**< 连接得分 */
		double healthyScore;   /**< 健康得分 */
		double healthySelectRate; /**< 健康选择概率 */

		bool disabled;         /**< 是否被禁止 */
		bool useBackup;        /**< 是否启用备份server */
		uint backupServerId;   /**< 备份server-id */

	};
}

#endif
