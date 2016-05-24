/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file ip_whitelist.cpp
 * @author opeddm@gmail.com
 * @date 2012/06/15 03:05:26
 * @brief 
 *  
 **/

#include <set>
#include <bsl/containers/hash/bsl_readset.h>
#include <Configure.h>

#include <pthread.h>

#include <arpa/inet.h>

#include "log.h"
#include "event.h"
#include "ip_whitelist.h"

static const char *whitelist_file = NULL;
static int check_interval;

typedef bsl::readset<unsigned int> whitelist_set_t;
typedef struct _whitelist_t {
    whitelist_set_t *set;
    long mtime;
} whitelist_t;
pthread_t whitelist_thread;

static whitelist_t *whitelist = NULL;

#define in_addr_to_int(x) (*((unsigned int*)&(x)))
static long get_mtime(const char* filename)
{
    struct stat buf;
    if(lstat(filename, &buf)<0){
        return -1;
    }
    return (long)buf.st_mtime;
}
whitelist_t* load_whitelist() {
    FILE *f;
    char buf[128];
    char *line;
    char *end;
    struct in_addr addr;
    int lineno;
    int ret;
    unsigned int ip;
    long mtime;
    mtime = get_mtime(whitelist_file);
    if (mtime < 0) {
        log_warning("Get mtime of whitelist file failed, possibly file does not exist");
        return NULL;
    }
    f = fopen(whitelist_file, "r");
    if (f == NULL) {
        log_warning("Open whitelist file %s error, errmsg: %s", whitelist_file, strerror(errno));
        return NULL;
    }
    lineno = 0;
    std::set <unsigned int> ip_set;
    while(fgets(buf, sizeof(buf), f) != NULL) {
        lineno ++;
        line = buf;
        //trim leading whitespace
        while (*line == ' ' || *line == '\t') line++;
        //skip empty line or comments
        if (buf[0] == '#' || buf[0] == '\r' || buf[0] == '\n' || buf[0] == 0) continue;
        end = line;
        //trim trailing characters
        while ((*end >= '0' && *end <= '9') || *end == '.') end++;
        *end = 0;

        //convert to struct in_addr
        ret = inet_pton(AF_INET, line, &addr);
        if (ret != 1) {
            log_warning("Error while reading whitelist file: %s:%d", whitelist_file, lineno);
        }

        //add to map
        ip_set.insert(in_addr_to_int(addr));
        log_debug("whitelist added for %s", inet_ntoa(addr));
    }
    fclose(f);

    //build bsl::readmap from ip_map
    whitelist_set_t *rst = new whitelist_set_t();
    ret = rst->assign(ip_set.begin(), ip_set.end());
    if (ret) {
        log_warning("error while assigning map from std::map to bsl::readmap");
        return NULL;
    }
    whitelist_t *w = (whitelist_t*) malloc(sizeof(whitelist_t));
    w->set = rst;
    w->mtime = mtime;
    whitelist = w;
    return w;
}

bool is_whitelist_changed() {
    if (whitelist == NULL) return false;
    if (get_mtime(whitelist_file) > whitelist->mtime) {
        return true;
    }
    return false;
}

void free_whitelist(whitelist_t *w) {
    if (!w) return;
    delete w->set;
    free(w);
}

bool in_whitelist_u(unsigned int ip) {
    if (whitelist == NULL) return true;
    if (whitelist->set->get(ip) == bsl::HASH_EXIST) {
        return true;
    }
    return false;
}

bool in_whitelist(struct in_addr addr) {
    return in_whitelist_u(in_addr_to_int(addr));
}

void *whitelist_loop(void*) {
    log_debug("whitelist_loop_started");
    while(true) {
        //log_debug("checking whitelist change");
        if (is_whitelist_changed()) {
            log_debug("whitelist changed detected");
            whitelist_t *w = load_whitelist();
            event_trigger("whitelist_changed", w);
        }
        sleep(check_interval);
    }
    return NULL;
}

int whitelist_init(const char *filename, int interval) {
    int ret;
    whitelist_file = filename;
    check_interval = interval;
    ret = pthread_create(&whitelist_thread, NULL, whitelist_loop, NULL);
    if (ret) {
        log_fatal("Error create whitelist check loop thread, errstr: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
