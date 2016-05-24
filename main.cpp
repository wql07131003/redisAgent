/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file main.cpp
 * @author opeddm@gmail.com
 * @date 2012/04/25 17:30:21
 * @brief 
 *  
 **/

#include <Configure.h>
#include <stdio.h>
#include <signal.h>

#include "proxy.h"
#include "event.h"
#include "server.h"
#include "pool.h"
#include "router.h"
#include "meta.h"
#include "log.h"
#include "dl.h"
#include "file.h"

#include "req.h"

std::string conf_dir = "";
std::string conf_name = "";
comcfg::Configure *conf;

#define ENSURE(x, msg) do { if(x) { log_fatal(msg); return 1; } } while(0)

int signal_handler_init() {
    struct sigaction sa;

    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGPIPE, &sa, NULL))
        return -1;
    return 0;
}

#ifndef UNIT_TEST
int main(int argc, char **argv) {
    if (argc == 1) {
        conf_dir = "./conf";
        conf_name = "proxy.conf";
    } else if (argc == 2) {
        conf_dir = dirname(argv[1]);
        conf_name = basename(argv[1]);
    } else if (argc == 3) {
        conf_dir = argv[1];
        conf_name = argv[2];
    } else {
        fprintf(stderr, "usage: %s path_to_conf_file\n", argv[0]);
        return 1;
    }

    if (signal_handler_init()) {
        fprintf(stderr, "Initialize signal handler failed!\n");
        return 1;
    }
    conf = new comcfg::Configure();
    if (conf->load(conf_dir.c_str(), conf_name.c_str())) {
        fprintf(stderr, "Load conf fail. dir:%s, name:%s\n", conf_dir.c_str(), conf_name.c_str());
        return 1;
    }
    if (log_init() != 0) {
        fprintf(stderr, "log init failed\n");
        return 1;
    }
    ENSURE(event_init(), "event init failed");
    ENSURE(server_init(), "server init failed");
    ENSURE(router_init(), "router init failed"); //必须在meta的初始化之前
    ENSURE(meta_init(), "meta init failed");
    ENSURE(dl_init(), "dl init failed");

    ENSURE(check_all_funcdefs(), "initialization not complete, not all plugin required functions defined");

    return server_run();
}
#endif
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
