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
 * @brief ������ 
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


///ȫ�ֱ���
g_conf_t g_conf;        //ȫ������
frman_data_t g_frman;        //ȫ������
/**
 *
 * ��ӡ������Ϣ
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
 * ��ӡģ��İ汾��Ϣ
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
 * @brief ��ȡ�����ļ�,ͬʱ��ʼ��log��Ϣ
 *
 * @param[in] dir ���õ�Ŀ¼
 * @param[in] file ���õ��ļ���
 * @param[in] build ������ʽ: 0��ʾ��ȡ����;1��ʾ���������ļ�
 *
 * @return
 *       0  : �ɹ�
 *       -1 : ʧ��
**/
STATIC int
serv_loadconf(const char *dir, const char *file, int build)
{
    /** ��ȡ�����ļ�
     *  һЩֵ�ķ�Χ����������.range�ļ���
     **/
    int ret = 0;

    //parameter check
    if(NULL == dir || NULL == file){
        printf("input parameter error in function:%s", __func__);
        return -1;
    }

    //��ʼ�����ÿ⣬��������Ϣ
    ub_conf_data_t *conf = ub_conf_init(dir, file, build);
    if (NULL == conf) {
        printf("load conf file [dir:%s file:%s build:%d ] failed!", dir, file, build);
        return -1;
    }

    //�����־�����Ϣ
    UB_CONF_GETNSTR(conf, "log_dir", g_conf.log_dir, sizeof(g_conf.log_dir), "��־Ŀ¼");
    UB_CONF_GETNSTR(conf, "log_file", g_conf.log_file, sizeof(g_conf.log_file), "��־�ļ���");
    UB_CONF_GETINT(conf, "log_size", &g_conf.log_size, "��־�ļ�ˢ�´�С(M)");
    UB_CONF_GETINT(conf, "log_level", &g_conf.log_level, "��־����");

    //����־
    if (0==build) {
        //��ʼ����־��
        ret = ub_log_init(g_conf.log_dir, g_conf.log_file, g_conf.log_size, g_conf.log_level);
        if (0 != ret) {
            printf("ub_log_init [dir:%s file:%s size:%d level:%d ] failed!",
                    g_conf.log_dir, g_conf.log_file, g_conf.log_size, g_conf.log_level);
            goto out;
        }
    }

    //����������Ϣ
    UB_CONF_GETSVR(conf, "tieba", "srch", &g_conf.srch, "srch����");
    UB_CONF_GETNSTR(conf, "auth_file", g_conf.auth_file, sizeof(g_conf.auth_file), "��֤IP�ļ�·��");
    
//	UB_CONF_GETNSTR(conf, "mask_path", g_conf.mask_path, sizeof(g_conf.mask_path), "mask�ļ���·��");
//    UB_CONF_GETNSTR(conf, "index_file", g_conf.index_file, sizeof(g_conf.index_file), "�����ļ���");
//    UB_CONF_GETNSTR(conf, "mask_file", g_conf.mask_file, sizeof(g_conf.mask_file), "�����ļ���");

out:
    if (conf) {
        ub_conf_close(conf);
        conf = NULL;
    }

    return ret;
}

/**
 *
 * @brief ������ĺϷ��Լ��(��.range�ļ�֮��ļ��)
 *
 * @return
 *       0  : �ɹ�
 *       -1 : ʧ��
 **/

STATIC int
serv_validateconf()
{
    return 0;
}


/**
 *
 * @brief ����load�����ļ��Ƿ�ɹ�
 *
 * @param[in] dir �����ļ���Ŀ¼
 * @param[in] file �����ļ���
 *
 * @return
 *       0  : �ɹ�
 *       -1 : ʧ��
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
 * @brief �����ʼ��
 *
 * @return
 *       0  : �ɹ�
 *       -1 : ʧ��
 **/
STATIC int
serv_init()
{
    /**������س�ʼ��**/
    int ret;

    signal(SIGPIPE, SIG_IGN);

    //������س�ʹ������
    g_frman.client = ub_client_init();//client�ĳ�ʼ��
    if(NULL == g_frman.client) {
        UB_LOG_FATAL("ub_client init error, return NULL");
        return -1;
    }

    //��֤
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

    //�������
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
    //����NODELAYģʽ��ʹ����˵����ݿ��Ծ��췢��
    ret = ub_server_setoptsock(g_frman.srch_svr, UBSVR_NODELAY);
    if (0 != ret) {
        UB_LOG_FATAL("set opt socket UBSVR_NODELAY srch fail!");
        ret = -1;
        goto out;
    }
    

    //������֤
    if (g_frman.auth) {
        ub_server_set_ip_auth(g_frman.srch_svr, g_frman.auth);
    }

    //������Ϣbuffer
    ub_server_set_buffer(g_frman.srch_svr,
        NULL,  1000000, NULL, 1000000);
    //�����߳�����
    ub_server_set_user_data(g_frman.srch_svr,
        NULL,  sizeof(srch_thread_data_t));
    //ʹ�ûص�����
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


    //TODO:������ʼ������

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
 * @brief ��ʼ������
 *
 * @param[in] conf_check ���ü��
 * @param[in] build ���������ļ�
 *
 * @return
 *       0  : �ɹ�
 *       1 : Ӧ���˳�
 **/
STATIC int
init_config(int conf_check, int conf_build)
{
    int ret;
    //��ȡ�����ļ�
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
 *@brief ���з���
 *
 * @return
 *       0  : �ɹ�
 *       -1 : ʧ��
 **/
STATIC int
serv_run()
{
    int ret;

    //srch����
    ret = ub_server_run(g_frman.srch_svr);
    if (0 != ret) {
        UB_LOG_FATAL("run srch server failed!");
        return -1;
    }


    return 0;
}

/**
 *@brief �߳�join
 *
 * @return
 *       0  : �ɹ�
 *       -1 : ʧ��
 **/
STATIC int
serv_join()
{

    //srch����
    ub_server_join(g_frman.srch_svr);


    return 0;
}

/**
 *
 *�����������
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

#ifndef frman_TEST /**!! �ڲ��Ժ�_TEST������ʱ,main����������,��ֹ��test�����main������ͻ **/

int
main(int argc, char **argv)
{
    int ret = 0;
    int c = 0;
    int conf_build = 0;    //��ȡ���û������������ļ�
    int conf_check = 0;    //�Ƿ�ֻ�ǽ��������ļ����﷨���

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


    //��ʼ������
    ret = serv_init();
    if (0 != ret) {
        UB_LOG_FATAL("serv_init failed! [ret:%d]", ret);
        goto OUT_PROJECT;
    } else {
        UB_LOG_FATAL("serv_init success!");
    }

    //��ʼ����
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
    //�������
    serv_cleanup();

    return ret;
}

#endif
/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
