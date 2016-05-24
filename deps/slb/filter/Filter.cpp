/*
 * Filter.cpp
 *
 *  Created on: 2010-4-7
 *      Author: Administrator
 */

#include "Filter.h"

namespace forum
{
	int Filter::updateConnectStatus(slb_server_t * /*server*/, int /*errNo*/)
	{
		return 0;
	}

	int Filter::updateRequestStatus(slb_server_t * /*server*/,
			const slb_talk_returninfo_t* /*talk*/)
	{
		return 0;
	}

	int Filter::reload(const comcfg::ConfigUnit& /*conf*/) {
		return 0;
	}

	int Filter::doAfter(int /*selectId*/, request_t* /*request*/)
	{
		return 0;
	}

	void Filter::debug(uint /*num*/) {
	}

}
