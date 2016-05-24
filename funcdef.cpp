/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file funcdef.cpp
 * @author opeddm@gmail.com
 * @date 2012/06/05 15:00:37
 * @brief 
 *  
 **/

#include <stdlib.h>
#include "proxy.h"

int (*backend_call)(req_t* r, backend_req_t **backend_req, int num, handle_t* handle, backend_cb_t cb, void* priv) = NULL;
int (*backend_timeout)(req_t* r, backend_req_t **backend_req, int num, handle_t* handle, backend_cb_t cb, void* priv) = NULL;
int (*backend_parse)(req_t* r) = NULL;
int (*backend_req_done)(req_t* r) = NULL;
int (*backend_req_init)(req_t* r) = NULL;

int check_all_funcdefs() {
    return !(
            backend_call
            && backend_timeout
            && backend_parse
            && backend_req_done
            && backend_req_init
            );
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
