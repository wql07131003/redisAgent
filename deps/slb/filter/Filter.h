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
		 * @brief ע�⣺���лص�����������ʧ�ܶ��ᵼ��chain�˳�
		 *   ��ˣ����������ش��󣬷����ܷ���ʧ��
		 */
		virtual int doFilter(request_t* request) = 0;

		virtual int doAfter(int selectId, request_t* request);

		virtual int load(const comcfg::ConfigUnit& conf, SLB_Resource * resource) = 0;

		/*
		 * @brief ��PHP�м��������reload�ӿڣ�����C��reload���󲻴���ʱ�����̰߳�ȫ��C�в�����
		 */
		virtual int reload(const comcfg::ConfigUnit& conf);

		virtual int updateConnectStatus(slb_server_t *server, int errNo);

		virtual int updateRequestStatus(slb_server_t *server, const slb_talk_returninfo_t *talk);

		/**
		 * @brief dump�ӿ�
		 * @param num: server��Ŀ
		 */
		virtual void debug(uint num);
	};
}

#endif /* FILTER_H_ */
