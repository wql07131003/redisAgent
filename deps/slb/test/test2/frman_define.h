/***************************************************************************
 * 
 * Copyright (c) 2008 Baidu.com, Inc. All Rights Reserved
 * $Id: frman_define.h,v 1.1 2010/04/13 04:29:03 wangbo3 Exp $ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file 
 * @author 
 * @version $Revision: 1.1 $ 
 * @brief 
 *  
 **/
#ifndef  __frman_DEFINE_H__
#define  __frman_DEFINE_H__

#if defined(__DATE__) && defined(__TIME__)
	#define BUILD_DATE  (__DATE__ " " __TIME__)
#else
	#define BUILD_DATE  "unknown"
#endif

#define  DEF_CONF_DIR   "./conf/"
#define  DEF_CONF_FILE  (PROJECT_NAME ".conf")

#endif
/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
