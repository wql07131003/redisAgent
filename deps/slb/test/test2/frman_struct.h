/*$Id: frman_struct.h,v 1.1 2010/04/13 04:29:03 wangbo3 Exp $*/
#ifndef __ECHO_DATA_H__
#define __ECHO_DATA_H__

#include "frman_include.h"

#define  SERV_SIGNATURE_LEN  (16)
#define  AUTH_LIST_LEN       (1024*4)

#ifndef PATH_SIZE
#define PATH_SIZE 256
#endif

/**
 * @brief the network interface info of srch, will be used in nshead
 */
#define srch_ID       0
#define srch_VERSION  0

#define DEFAULT_POST_PER_FILE   250000000

//获得删贴人id和时间的一级索引结构体
typedef struct _ind_delpost_t
{
     unsigned int fno;
     unsigned int offset;
}ind_delpost_t;

//获得删贴人信息的结构体
#ifndef NP_MAX_USERNAME_LEN
#define NP_MAX_USERNAME_LEN 64
#endif

#pragma pack(push,4)
typedef struct _delpost_record_t
{
   u_int64_t post_id;
   char username[NP_MAX_USERNAME_LEN];
   unsigned int del_time;
}delpost_record_t;
#pragma pack(pop)

typedef struct _srch_thread_data_t{
	static const int max_post_count = 1024;

	u_int64_t post_ids[max_post_count]; 
	delpost_record_t records[max_post_count];
}srch_thread_data_t;

///SERVER服务的配置信息
typedef struct _g_conf_t {

	char conf_dir[PATH_SIZE];			  /**<   配置文件的目录    */
	char conf_file[PATH_SIZE];			  /**<   配置文件名        */
	char data_dir[PATH_SIZE];			  /**<   数据文件的根目录  */

	char log_dir[PATH_SIZE];			  /**<   日志文件的根目录  */
	char log_file[PATH_SIZE];			  /**<   日志文件名        */
	int log_level;					  /**<   日志级别          */
	int log_size;					  /**<   日志回滚的大小(M) */

	ub_svr_t srch;					  /**<   srch的配置信息 */

	char auth_file[PATH_SIZE];			  /**<   认证文件路径           */

	/*************************************/
	char mask_path[PATH_SIZE];
	char index_file[PATH_SIZE];
	char mask_file[PATH_SIZE];
} g_conf_t;

///SERVER相关数据
typedef struct _frman_data_t {
	ub_auth_t   *auth;				      /**<   IP认证    */
	ub_client_t *client;		      /**<   客户端   */
	ub_server_t *srch_svr;	      /**<   srch服务       */

} frman_data_t;

extern g_conf_t g_conf;
extern frman_data_t g_frman;

#endif
