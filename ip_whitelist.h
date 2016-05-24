/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file ip_whitelist.h
 * @author opeddm@gmail.com
 * @date 2012/06/15 03:05:31
 * @brief 
 *  
 **/




#ifndef  __IP_WHITELIST_H_
#define  __IP_WHITELIST_H_

#include <sys/socket.h>
#include <netinet/in.h>
typedef struct _whitelist_t whitelist_t;
whitelist_t* load_whitelist();
bool is_whitelist_changed();
void free_whitelist(whitelist_t *w);
bool in_whitelist_u(unsigned int ip);
bool in_whitelist(struct in_addr addr);
int whitelist_init(const char *filename, int interval);


#endif  //__IP_WHITELIST_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
