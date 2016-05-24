/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file funcdef.h
 * @author opeddm@gmail.com
 * @date 2012/06/05 15:00:35
 * @brief 
 *  
 **/




#ifndef  __FUNCDEF_H_
#define  __FUNCDEF_H_

//functions that can be overwritten
extern int (*backend_req_init)(req_t* r);
//向后端发起请求
extern int (*backend_call)(req_t* r, backend_req_t **backend_req, int num, handle_t* handle, backend_cb_t cb, void* priv);
//超时
extern int (*backend_timeout)(req_t* r, backend_req_t **backend_req, int num, handle_t* handle, backend_cb_t cb, void* priv);

//解析后端协议
extern int (*backend_parse)(req_t* r);
extern int (*backend_req_done)(req_t* r);

int check_all_funcdefs();

#endif  //__FUNCDEF_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
