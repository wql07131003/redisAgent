/*
 * ChainFilter.cpp
 *
 *  Created on: 2010-4-7
 *      Author: Administrator
 */

#include "ChainFilter.h"

namespace forum
{
	int ChainFilter::doFilter(request_t* request)
	{
		int ret = 0;
		for (std::vector<Filter*>::iterator it = _filters.begin();
				it != _filters.end(); ++it)
		{
			Filter* filter = *it;
			ret = filter->doFilter(request);
			if (ret < 0) {
				//return ret;
				break;
			}
		}

		for (std::vector<Filter*>::iterator it = _filters.begin();
				it != _filters.end(); ++it)
		{
			Filter* filter = *it;
			filter->doAfter(ret, request);
		}

		return ret;
	}

	int ChainFilter::load(const comcfg::ConfigUnit & conf, SLB_Resource * resource)
	{
		int ret = 0;
		for (std::vector<Filter*>::iterator it = _filters.begin();
						it != _filters.end(); ++it)
		{
			Filter* filter = *it;
			ret = filter->load(conf, resource);
			if (ret < 0) {
				return ret;
			}
		}
		return 0;
	}

	int ChainFilter::reload(const comcfg::ConfigUnit & conf) {
		int ret = 0;
		for (std::vector<Filter*>::iterator it = _filters.begin();
						it != _filters.end(); ++it)
		{
			Filter* filter = *it;
			ret = filter->reload(conf);
			if (ret < 0) {
				return ret;
			}
		}
		return 0;
	}

	int ChainFilter::updateConnectStatus(slb_server_t *server, int errNo)
	{
		int ret = 0;
		for (std::vector<Filter*>::iterator it = _filters.begin();
						it != _filters.end(); ++it)
		{
			Filter* filter = *it;
			ret = filter->updateConnectStatus(server, errNo);
			if (ret < 0) {
				return ret;
			}
		}

		return 0;
	}

	int ChainFilter::updateRequestStatus(slb_server_t *server, const slb_talk_returninfo_t *talk)
	{
		int ret = 0;
		for (std::vector<Filter*>::iterator it = _filters.begin();
						it != _filters.end(); ++it)
		{
			Filter* filter = *it;
			ret = filter->updateRequestStatus(server, talk);
			if (ret < 0) {
				return ret;
			}
		}

		return 0;
	}

	void ChainFilter::debug(uint num)
	{
		//UB_LOG_DEBUG("ChainFilter DEBUG START,server num:%d",num);
		for (std::vector<Filter*>::iterator it = _filters.begin();
						it != _filters.end(); ++it)
		{
			Filter* filter = *it;
			filter->debug(num);
		}
		//UB_LOG_DEBUG("ChainFilter DEBUG END,server num:%d",num);
	}

	void ChainFilter::addFilter(Filter* filter)
	{
		if (filter) {
			_filters.push_back(filter);
		}
	}

	uint ChainFilter::getFilterSize() const
	{
		return _filters.size();
	}

	Filter* ChainFilter::getFilter(uint index)
	{
		return index < (uint)_filters.size() ? _filters[index] : NULL;
	}

}
