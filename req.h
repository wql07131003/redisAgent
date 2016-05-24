/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file req.h
 * @author opeddm@gmail.com
 * @date 2012/05/07 15:19:53
 * @brief 
 *  
 **/




#ifndef  __REQ_H_
#define  __REQ_H_

#include "proxy.h"
#include "server.h"

req_t *req_create();
int req_ready(connection_t *c);
int req_process(req_t *r);
int req_done(req_t *r);
int req_fail(req_t *r);


int _req_subreq_queue_get(req_t *r, sub_req_t* &sr);
int _req_subreq_queue_push(req_t *r, sub_req_t *sr);
int _req_subreq_queue_pop(req_t *r, sub_req_t *sr);
int req_on_acquired(handle_t *handle, void *sub_req);
int req_timeout_connect_cleanup (req_t *r);


#endif  //__REQ_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
