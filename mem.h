/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file mem.h
 * @author opeddm@gmail.com
 * @date 2012/05/23 02:29:34
 * @brief 
 *  
 **/




#ifndef  __MEM_H_
#define  __MEM_H_
void* emalloc(void* base, size_t size);
void* ecalloc(void* base, size_t size);
void* erealloc(void* base, size_t size);
void* emalloc_new(size_t size);
void* ecalloc_new(size_t size);
void* emalloc_safe(void* base, size_t size);
void* ecalloc_safe(void* base, size_t size);
void efree(void* mem);
void efree_all(void* mem);

void e_set_data(void* base, void* data);
void* e_get_data(void* base);
#endif  //__MEM_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
