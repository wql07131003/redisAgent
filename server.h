/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file server.h
 * @author liuqingjun(com@baidu.com)
 * @date 2012/04/25 20:41:25
 * @brief 
 *  
 **/

#ifndef  __SERVER_H_
#define  __SERVER_H_

#include "connection.h"

int server_init();

int server_run();

void server_write(connection_t *c, const void* buffer, int size);

void server_process_done(connection_t *c);

extern char *pid;
extern char *omp_product;
extern char *omp_subsys;
extern char *omp_module;

#endif  //__SERVER_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
