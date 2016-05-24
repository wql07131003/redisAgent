/*
 * slb_define.h
 *
 *  Created on: 2010-9-29
 *      Author: Administrator
 */
#include <sys/types.h>

#ifndef SLB_DEFINE_H_
#define SLB_DEFINE_H_

namespace forum
{

	const char SLB_Balance[] = "Balance"; /**< 负载均衡算法，有默认值Random */
	// HealthyFilter，如果加载不成功，就不开启
	const char SLB_HealthyQueueSize[] = "HealthyQueueSize"; /**< 状态信息队列大小，是以s为单位，表示记过去100s的信息，有默认值100 */
	const char SLB_HealthyTimeout[] = "HealthyTimeout"; /**< client端读的超时时间 */
	const char SLB_HealthyCheckTime[] = "HealthyCheckTime"; /**< 计算选择概率的时间间隔，以s为单位 */
	const char SLB_HealthyMinRate[] = "HealthyMinRate"; /**< 选择概率的最小值，0.1表示最小概率为10% */
	const char SLB_HealthyBackupThreshold[] = "HealthyBackupThreshold"; /**< 速度大于这个倍数才能做流量均分，比如3倍 */
	// ConnectFilter，如果加载不成功，就不开启
	const char SLB_ConnectQueueSize[] = "ConnectQueueSize"; /**< 状态信息队列大小，表示记录过去100次的连接成功/失败情况，有默认值100 */
	const char SLB_ConnectX1[] = "ConnectX1"; /**< K点坐标 */
	const char SLB_ConnectY1[] = "ConnectY1"; /**< K点坐标 */
	const char SLB_ConnectX2[] = "ConnectX2"; /**< P点坐标 */
	const char SLB_ConnectY2[] = "ConnectY2"; /**< P点坐标 */

	const char SLB_CrossRoom[] = "CrossRoom"; /**< 是否跨机房 */

	const char SLB_MachineRoom[] = "MachineRoom"; /**< 机房信息 */

	/*
	 * @brief 一个Strytegy总共享内存数的计算公式：
	 *  Service支持的最大Server数 ×（8 × connect队列长度 + 2×8×healthy队列长度 + 280）
	 *
	 *  sizeof(node_t) = 8
	 *
	 *  sizeof(ConnectStatus) = 48
	 *  sizeof(HealthyStatus) = 96
	 *  siezof(pthread_mutex_t) = 40
	 *  xnofreepool放进ResourceRool之后，在create_array的时候，会从xnofreepool中申请一部分空间来维护info信息，目前的应用下是88
	 *  以上几项之和：272
	 */
	const uint MAX_SERVER_SIZE = 64; /**< 为了降低对共享内存的占用，减少了server数量，后续如不够用再考虑优化 */
	const int SLB_DEF_QUEUE_SIZE = 100; /**< ConnectStatus / HealthyStatus的默认队列长度，改大要小心，会占用更多内存 */
	const int SLB_DEF_BUF_SIZE = 200 * 1024; /**< 在默认配置下，一个Service所需的共享内存 170K，这个值也当成为Strategy分配内存的最小值 */

	const int SLB_MACHINE_ROOM_BUF_SIZE = 32; /**< 机房信息长度 */

	/**
	 * @brief 对外的请求结构体
	 */
	typedef struct _slb_request_t {
		int key;					/**< balanceServer使用的均衡因子 */
		int nthRetry;				/**< 第几次重试，0表示第一次选择 */
		uint serverNum;             /**< server 数目 */
		void * svr[MAX_SERVER_SIZE];  /**< 保存server的指针，类型由具体的实现识别 */
		char currentMachineRoom[SLB_MACHINE_ROOM_BUF_SIZE]; /**< 当前请求的机房信息 */
	} slb_request_t;

	/**
	 * @brief 对外的交互状态信息结构体
	 */
    typedef struct _slb_talk_returninfo_t {
        int realreadtime;        /**<  通信过程实际花费的读时间 */
        int realwritetime;       /**<  通信过程实际花费的写时间 */
        int reserve;             /**<  保留字段 */
    } slb_talk_returninfo_t;
}

#endif /* SLB_DEFINE_H_ */
