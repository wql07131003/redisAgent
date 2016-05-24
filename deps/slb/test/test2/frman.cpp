/***************************************************************************
 * 
 * Copyright (c) 2008 Baidu.com, Inc. All Rights Reserved
 * $Id: frman.cpp,v 1.2 2010/04/18 04:16:05 wangbo3 Exp $ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file 
 * @author 
 * @version $Revision: 1.2 $ 
 * @brief 主程序 
 *  
 **/
#include "frman_include.h"
#include <stdio.h>

#include <ub_log.h>
#include <ubclient_include.h>

#include "SuperStrategy.h"

using namespace forum;

UBCLIENT_GEN_IOC_CREATE(SuperStrategy);
ub::UbClientManager ubmgr;


///全局变量
g_conf_t g_conf;        //全局配置
frman_data_t g_frman;        //全局数据
/**
 *
 * 打印帮助信息
**/
STATIC void
print_help()
{
    printf("\t-d:     conf_dir\n");
    printf("\t-f:     conf_file\n");
    printf("\t-h:     show this help page\n");
    printf("\t-t:     check config file syntax\n");
    printf("\t-g:     generate config sample file\n");
    printf("\t-v:     show version infomation\n");
    return;
}

/**
 *
 * 打印模块的版本信息
**/
STATIC void
print_version()
{
    printf("Project    :%s\n", PROJECT_NAME);
    printf("Version    :%s\n", VERSION);
    printf("CVSTag     :%s\n", CVSTAG);
    printf("BuildDate  :%s\n", BUILD_DATE);
    return;
}


/**
 *
 * @brief 读取配置文件,同时初始化log信息
 *
 * @param[in] dir 配置的目录
 * @param[in] file 配置的文件名
 * @param[in] build 工作方式: 0表示读取配置;1表示生成配置文件
 *
 * @return
 *       0  : 成功
 *       -1 : 失败
**/
STATIC int
serv_loadconf(const char *dir, const char *file, int build)
{
    /** 读取配置文件
     *  一些值的范围建议配置在.range文件中
     **/
    int ret = 0;

    //parameter check
    if(NULL == dir || NULL == file){
        printf("input parameter error in function:%s", __func__);
        return -1;
    }

    //初始化配置库，打开配置信息
    ub_conf_data_t *conf = ub_conf_init(dir, file, build);
    if (NULL == conf) {
        printf("load conf file [dir:%s file:%s build:%d ] failed!", dir, file, build);
        return -1;
    }

    //获得日志相关信息
    UB_CONF_GETNSTR(conf, "log_dir", g_conf.log_dir, sizeof(g_conf.log_dir), "日志目录");
    UB_CONF_GETNSTR(conf, "log_file", g_conf.log_file, sizeof(g_conf.log_file), "日志文件名");
    UB_CONF_GETINT(conf, "log_size", &g_conf.log_size, "日志文件刷新大小(M)");
    UB_CONF_GETINT(conf, "log_level", &g_conf.log_level, "日志级别");

    //打开日志
    if (0==build) {
        //初始化日志库
        ret = ub_log_init(g_conf.log_dir, g_conf.log_file, g_conf.log_size, g_conf.log_level);
        if (0 != ret) {
            printf("ub_log_init [dir:%s file:%s size:%d level:%d ] failed!",
                    g_conf.log_dir, g_conf.log_file, g_conf.log_size, g_conf.log_level);
            goto out;
        }
    }

    //其它配置信息
    UB_CONF_GETSVR(conf, "tieba", "srch", &g_conf.srch, "srch服务");
    UB_CONF_GETNSTR(conf, "auth_file", g_conf.auth_file, sizeof(g_conf.auth_file), "认证IP文件路径");
    
//	UB_CONF_GETNSTR(conf, "mask_path", g_conf.mask_path, sizeof(g_conf.mask_path), "mask文件的路径");
//    UB_CONF_GETNSTR(conf, "index_file", g_conf.index_file, sizeof(g_conf.index_file), "索引文件名");
//    UB_CONF_GETNSTR(conf, "mask_file", g_conf.mask_file, sizeof(g_conf.mask_file), "数据文件名");

out:
    if (conf) {
        ub_conf_close(conf);
        conf = NULL;
    }

    return ret;
}

/**
 *
 * @brief 配置项的合法性检查(在.range文件之外的检查)
 *
 * @return
 *       0  : 成功
 *       -1 : 失败
 **/

STATIC int
serv_validateconf()
{
    return 0;
}


/**
 *
 * @brief 测试load配置文件是否成功
 *
 * @param[in] dir 配置文件的目录
 * @param[in] file 配置文件名
 *
 * @return
 *       0  : 成功
 *       -1 : 失败
 **/
STATIC int
serv_checkconf(const char *dir, const char *file)
{
    if (0!= serv_loadconf(dir, file, 0)) {
        return -1;
    }
    return  serv_validateconf();
}

/**
 *
 * @brief 服务初始化
 *
 * @return
 *       0  : 成功
 *       -1 : 失败
 **/
STATIC int
serv_init()
{
    /**服务相关初始化**/
    int ret;

    signal(SIGPIPE, SIG_IGN);

    //服务相关初使化工作
    g_frman.client = ub_client_init();//client的初始化
    if(NULL == g_frman.client) {
        UB_LOG_FATAL("ub_client init error, return NULL");
        return -1;
    }

    //认证
    if (g_conf.auth_file[0]) {
        g_frman.auth = ub_auth_create();
        if (NULL==g_frman.auth) {
            UB_LOG_FATAL("auth_create failed!!");
            ret = -1;
            goto out;
        }
        if (0!=ub_auth_load_ip(g_frman.auth, g_conf.auth_file)) {
            UB_LOG_WARNING("auth_load_ip failed [file:%s]!", g_conf.auth_file );
            ret = -1;
            goto out;
        }
    }

    //服务相关
    g_frman.srch_svr = ub_server_create();
    if (NULL == g_frman.srch_svr) {
        UB_LOG_FATAL("ub_server_create failed!");
        ret = -1;
        goto out;
    }
    ret = ub_server_load(g_frman.srch_svr, &g_conf.srch);
    if (0 != ret) {
        UB_LOG_FATAL("load srch server failed!");
        ret = -1;
        goto out;
    }
    //设置NODELAY模式，使服务端的数据可以尽快发出
    ret = ub_server_setoptsock(g_frman.srch_svr, UBSVR_NODELAY);
    if (0 != ret) {
        UB_LOG_FATAL("set opt socket UBSVR_NODELAY srch fail!");
        ret = -1;
        goto out;
    }
    

    //设置认证
    if (g_frman.auth) {
        ub_server_set_ip_auth(g_frman.srch_svr, g_frman.auth);
    }

    //设置信息buffer
    ub_server_set_buffer(g_frman.srch_svr,
        NULL,  1000000, NULL, 1000000);
    //设置线程数据
    ub_server_set_user_data(g_frman.srch_svr,
        NULL,  sizeof(srch_thread_data_t));
    //使用回调函数
    ub_server_set_callback(g_frman.srch_svr,
        srch_cmdproc_callback);

	//ub_log_init("./log", "test2");
	ub::strategy_ioc *strategyIoc = ub::ubclient_get_strategyioc();
	UBCLIENT_ADD_IOC_CLASS(strategyIoc, SuperStrategy);

	ret = ubmgr.init();
	if (0 != ret) {
		printf("%s\n", "init error");
		return -1;
	}
	ubmgr.startHealthyCheck(10);
	printf("%s\n", "init OK");
	UB_LOG_WARNING("%s", "init OK");


    //TODO:其它初始化工作

out:
    if (0 != ret) {
        if (g_frman.auth) {
            ub_auth_destroy(g_frman.auth);
            g_frman.auth = NULL;
        }

        if (g_frman.srch_svr) {
            ub_server_destroy(g_frman.srch_svr);
            g_frman.srch_svr = NULL;
        }

    }

    return ret;
}

/**
 *
 * @brief 初始化配置
 *
 * @param[in] conf_check 配置检查
 * @param[in] build 生成配置文件
 *
 * @return
 *       0  : 成功
 *       1 : 应该退出
 **/
STATIC int
init_config(int conf_check, int conf_build)
{
    int ret;
    //读取配置文件
    if (conf_check) {
        //only check config
        ret = serv_checkconf(g_conf.conf_dir, g_conf.conf_file);
        if (0 != ret) {
            fprintf(stderr,    "CHECK CONFIG FILE [dir:%s file:%s ret:%d] FAILED!\n",
                    g_conf.conf_dir, g_conf.conf_file, ret);
        } else {
            fprintf(stderr,    "CHECK CONFIG FILE SUCCESS!\n");
        }
        return 1;
    } else {
        //load config
        ret = serv_loadconf(g_conf.conf_dir, g_conf.conf_file, conf_build);
        if (0 == ret) {
            UB_LOG_TRACE("load config success!");
        } else {
            UB_LOG_FATAL("load config [dir:%s file:%s ret:%d] failed!",
                    g_conf.conf_dir, g_conf.conf_file, ret);
            return 1;
        }
        if (1 == conf_build) {
            return 1;
        }
    }
    return 0;

}

/**
 *@brief 运行服务
 *
 * @return
 *       0  : 成功
 *       -1 : 失败
 **/
STATIC int
serv_run()
{
    int ret;

    //srch服务
    ret = ub_server_run(g_frman.srch_svr);
    if (0 != ret) {
        UB_LOG_FATAL("run srch server failed!");
        return -1;
    }


    return 0;
}

/**
 *@brief 线程join
 *
 * @return
 *       0  : 成功
 *       -1 : 失败
 **/
STATIC int
serv_join()
{

    //srch服务
    ub_server_join(g_frman.srch_svr);


    return 0;
}

/**
 *
 *服务的清理工作
 *
 **/
STATIC void
serv_cleanup()
{
    if (g_frman.auth) {
        ub_auth_destroy(g_frman.auth);
        g_frman.auth = NULL;
    }

    if (g_frman.srch_svr) {
        ub_server_destroy(g_frman.srch_svr);
        g_frman.srch_svr = NULL;
    }


    return;
}

#ifndef frman_TEST /**!! 在测试宏_TEST被定义时,main将不被编译,防止和test程序的main函数冲突 **/

int
main(int argc, char **argv)
{
    int ret = 0;
    int c = 0;
    int conf_build = 0;    //读取配置还是生成配置文件
    int conf_check = 0;    //是否只是进行配置文件的语法检查

    strlcpy(g_conf.conf_dir, DEF_CONF_DIR, PATH_SIZE);
    strlcpy(g_conf.conf_file, DEF_CONF_FILE, PATH_SIZE);

    while (-1 != (c = getopt(argc, argv, "d:f:gthv"))) {
        switch (c) {
            case 'd':
                snprintf(g_conf.conf_dir, PATH_SIZE, "%s/", optarg);
                break;
            case 'f':
                snprintf(g_conf.conf_file, PATH_SIZE, "%s", optarg);
                break;
            case 'v':
                print_version();
                return 0;
            case 'h':
                print_help();
                return 0;
            case 'g':
                conf_build = 1;
                break;
            case 't':
                conf_check = 1;
                break;
            default:
                print_help();
                return 0;
        }
    }

    if(0 != init_config(conf_check, conf_build))
        goto OUT_PROJECT;


    //初始化服务
    ret = serv_init();
    if (0 != ret) {
        UB_LOG_FATAL("serv_init failed! [ret:%d]", ret);
        goto OUT_PROJECT;
    } else {
        UB_LOG_FATAL("serv_init success!");
    }

    //开始服务
    ret = serv_run();
    if (0 != ret) {
        UB_LOG_FATAL("serv_start failed! [ret:%d]", ret);
        goto OUT_PROJECT;
    } else {
        UB_LOG_FATAL("serv_start success!");
    }

    //join
    ret = serv_join();
    if (0 != ret) {
        UB_LOG_FATAL("serv_join failed! [ret:%d]", ret);
        goto OUT_PROJECT;
    } else {
        UB_LOG_FATAL("serv_join success!");
    }


OUT_PROJECT:
    //清除工作
    serv_cleanup();

    return ret;
}

#endif
/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
