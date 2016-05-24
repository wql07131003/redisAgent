/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file proxy_log.h
 * @author cao_yu(com@baidu.com)
 * @date 2012/05/29 22:13:19
 * @brief 
 *  
 **/




#ifndef  __PROXY_LOG_H_
#define  __PROXY_LOG_H_

#include "log.h"

#define UB_LOG_DEBUG(...) log_debug(__VA_ARGS__)
#define UB_LOG_TRACE(...) log_trace(__VA_ARGS__)
#define UB_LOG_NOTICE(...) log_notice(__VA_ARGS__)
#define UB_LOG_WARNING(...) log_warning(__VA_ARGS__)
#define UB_LOG_FATAL(...) log_fatal(__VA_ARGS__)

#endif  //__PROXY_LOG_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
