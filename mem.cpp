/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file mem.cpp
 * @author opeddm@gmail.com
 * @date 2012/06/18 10:38:45
 * @brief 
 *  
 **/
#include <stdlib.h>
#include <assert.h>

#define SAFE
#ifdef SAFE
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

#define _SIG 0xba1da5b3
struct _mem_pre {
    struct _mem_pre *prev;
    struct _mem_pre *next;
    void *data;
    unsigned int sig;
};

void* emalloc(void* base, size_t size) {
    struct _mem_pre *oldmem = ((struct _mem_pre*) base) - 1;
    ASSERT(oldmem->sig == _SIG);
    struct _mem_pre *newmem = (struct _mem_pre*) malloc(size + sizeof(struct _mem_pre));
    if (newmem == NULL) return NULL;
    newmem->next = oldmem->next;
    oldmem->next = newmem;
    newmem->prev = oldmem;
    if (newmem->next) newmem->next->prev = newmem;
    newmem->sig = _SIG;
    newmem->data = oldmem->data;
    return (void*) (newmem + 1);
}

void* ecalloc(void* base, size_t size) {
    struct _mem_pre *oldmem = ((struct _mem_pre*) base) - 1;
    ASSERT(oldmem->sig == _SIG);
    struct _mem_pre *newmem = (struct _mem_pre*) calloc(1, size + sizeof(struct _mem_pre));
    if (newmem == NULL) return NULL;
    newmem->next = oldmem->next;
    oldmem->next = newmem;
    newmem->prev = oldmem;
    if (newmem->next) newmem->next->prev = newmem;
    newmem->sig = _SIG;
    newmem->data = oldmem->data;
    return (void*) (newmem + 1);
}

void* erealloc(void* base, size_t size) {
    struct _mem_pre *mem = ((struct _mem_pre*) base) - 1;
    ASSERT(mem->sig == _SIG);
    mem = (struct _mem_pre*) realloc(mem, size + sizeof(struct _mem_pre));
    if (mem == NULL) return NULL;
    if (mem->next) mem->next->prev = mem;
    if (mem->prev) mem->prev->next = mem;
    return (void*) (mem + 1);
}

void* emalloc_new(size_t size) {
    struct _mem_pre *newmem = (struct _mem_pre*) malloc(size + sizeof(struct _mem_pre));
    if (newmem == NULL) return NULL;
    newmem->next = NULL;
    newmem->prev = NULL;
    newmem->sig = _SIG;
    return (void*) (newmem + 1);
}

void* ecalloc_new(size_t size) {
    struct _mem_pre *newmem = (struct _mem_pre*) calloc(1, size + sizeof(struct _mem_pre));
    if (newmem == NULL) return NULL;
    newmem->next = NULL;
    newmem->prev = NULL;
    newmem->sig = _SIG;
    return (void*) (newmem + 1);
}

void* emalloc_safe(void* base, size_t size) {
    if (base == NULL) return emalloc_new(size);
    else return emalloc(base, size);
}

void* ecalloc_safe(void* base, size_t size) {
    if (base == NULL) return ecalloc_new(size);
    else return ecalloc(base, size);
}

void efree(void* mem) {
    struct _mem_pre *cur = ((struct _mem_pre*) mem) - 1;
    ASSERT(cur->sig == _SIG);
    if (cur->next) cur->next->prev = cur->prev;
    if (cur->prev) cur->prev->next = cur->next;
    free(cur);
}

void efree_all(void* mem) {
    struct _mem_pre *cur = ((struct _mem_pre*) mem) - 1;
    ASSERT(cur->sig == _SIG);
    struct _mem_pre *n, *next;
    for(n = cur->next; n; n = next) {
        ASSERT(n->sig == _SIG);
        next = n->next;
        free(n);
    }
    for(n = cur->prev; n; n = next) {
        ASSERT(n->sig == _SIG);
        next = n->prev;
        free(n);
    }
    free(cur);
}

void e_set_data(void* base, void* data) {
    struct _mem_pre *mem = ((struct _mem_pre*) base) - 1;
    mem->data = data;
}

void* e_get_data(void* base) {
    struct _mem_pre *mem = ((struct _mem_pre*) base) - 1;
    return mem->data;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
