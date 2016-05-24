/***************************************************************************
 * 
 * Copyright (c) 2008 Baidu.com, Inc. All Rights Reserved
 * $Id: frman_if.h,v 1.1 2010/04/13 04:29:03 wangbo3 Exp $ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file 
 * @author 
 * @version $Revision: 1.1 $ 
 * @brief 
 *  
 **/
#ifndef  __frman_IF_H__
#define  __frman_IF_H__

/**
 * @brief the network interface of srch server is defined here
 */
//srch�����
enum srch_cmd_t {
	C_srch_CMD_BEGIN = 0,
	C_srch_ECHO_REQ = 1,
	C_srch_RECHO_REQ = 2,
	C_srch_CMD_END
};

//srch�����
enum srch_error_t {
	E_srch_SUCCESS = 0,
	E_srch_REJECT = 1,
	E_srch_ERROR = 2
};

//srch����ṹ��
typedef struct _srch_req_t {
	int cmd_no;		      /**< �����        */
	char str[20];		      /**< �����ַ���    */
} srch_req_t;

//srchӦ��ṹ��
typedef struct _srch_res_t {
	int err_no;		     /**< �����         */
	char str[20];		     /**< �����ַ���     */
} srch_res_t;


#endif
/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
