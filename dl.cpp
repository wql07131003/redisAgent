/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file dl.cpp
 * @author opeddm@gmail.com
 * @date 2012/06/05 23:18:59
 * @brief 
 *  
 **/

#include <Configure.h>
#include <dlfcn.h>

#include "dl.h"
#include "log.h"
#include "proxy.h"

extern comcfg::Configure *conf;

int load_dl(const char *dir, const char *filename) {
    char path[128];
    if (path == NULL) {
        strcpy(path, filename);
    } else {
        strcpy(path, dir);
        strcat(path, "/");
        strcat(path, filename);
    }
    void *dlhandler = dlopen(path, RTLD_NOW);
    if (dlhandler == NULL) {
        log_fatal("dlopen error! error: %s", dlerror());
        return -1;
    }
    dlerror();
    init_func_t func = (init_func_t) dlsym(dlhandler, "init");
    char *err;
    if ((err = dlerror()) != NULL) {
        log_fatal("%s is not a valid proxy plugin, get symbol `init' failed", filename);
        return -1;
    }
    if (func()) {
        log_fatal("Initialization of plugin %s failed", filename);
        return -1;
    }
    return 0;
}

int dl_init() {
    const char *dir;
    const char *file;
    int ret;
    try {
        dir = conf[0]["Plugin"]["Dir"].to_cstr();

        int count;
        count = conf[0]["Plugin"]["Lib"].size();
        log_debug("/Plugin/Lib . size() = %d", count);
        for (int i = 0; i < count; i++) {
            file = conf[0]["Plugin"]["Lib"][i].to_cstr();
            log_debug("loading plugin %s/%s", dir, file);
            ret = load_dl(dir, file);
            if (ret) return ret;
        }
    } catch (comcfg::ConfigException e) {
        log_fatal("Read conf error in dl, errmsg: %s", e.what());
        return -1;
    }

    return 0;
}

int dl_fini() {
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
