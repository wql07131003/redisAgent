/***************************************************************************
 * 
 * Copyright (c) 2008 Baidu.com, Inc. All Rights Reserved
 * $Id: frman_imp_srch.cpp,v 1.1 2010/04/13 04:29:03 wangbo3 Exp $ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file 
 * @author 
 * @version $Revision: 1.1 $ 
 * @brief 处理交互的逻辑 
 *  
 **/
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "frman_include.h"
#include "service.h"

extern frman_data_t g_frman;
extern g_conf_t g_conf;

STATIC int srch_process(int cmd_no, nshead_t * req_head, ub_buff_t * req_buf,
            nshead_t * res_head, ub_buff_t * res_buf);
//static int _do_query(int count, const u_int64_t* pids, _delpost_record_t* records,
//		const char* mask_dir, const char* index_file_path, const char* mask_file_path);
static bool unpack_data(const char* src_buf, int src_buf_len, int& post_count, u_int64_t* pids, int max_post_count);

static int pack_data(char* dest_buf, int dest_buf_len, int post_count, const delpost_record_t* records);


//srch处理的命令表
static const ub_cmd_map_t srch_CMD_MAP[] = {
    {C_srch_ECHO_REQ, srch_process},
    {-1, NULL}    //must be the last one
};

/**
 *
 * @brief srch的命令处理回调函数
 *
 * @return
 *       0  : 成功
 *       -1 : 失败,socket将直接关闭,不给client返回错误信息
 **/
int
srch_cmdproc_callback()
{
    nshead_t *req_head;
    nshead_t *res_head;
    ub_buff_t req_buf;
    ub_buff_t res_buf;
    int cmd_no = -1;
    int ret = 0;

    //获取请求和回复buffer
    req_head = (nshead_t *) ub_server_get_read_buf();
    res_head = (nshead_t *) ub_server_get_write_buf();
    if(NULL == req_head || NULL == res_head) {
        UB_LOG_FATAL("srch process callback get buffer error.");
        return -1;
    }

    //获取请求和回复实际数据
    req_buf.buf = (char *) (req_head + 1);
    req_buf.size = ub_server_get_read_size() - sizeof (nshead_t);
    res_buf.buf = (char *) (res_head + 1);
    res_buf.size = ub_server_get_write_size() - sizeof (nshead_t);

    //取得命令号
    cmd_no = ((srch_req_t *) (req_buf.buf))->cmd_no;

    //ub_server中设置了: UB_LOG_REQIP, UB_LOGID, UB_LOG_PROCTIME, UB_LOG_ERRNO
    ub_log_setbasic(UB_LOG_REQSVR, "%s", req_head->provider);
    ub_log_setbasic(UB_LOG_SVRNAME, "%s", g_conf.srch.svr_name);
    ub_log_setbasic(UB_LOG_CMDNO, "%d", cmd_no);

    //ret = ub_process_cmdmap(srch_CMD_MAP, cmd_no, req_head, &req_buf, res_head, &res_buf);
	ret = srch_process(cmd_no, req_head, &req_buf, res_head, &res_buf);

    return ret;
}

/**
 *
 * @brief 重置应答数据
 *
 * @param[in] req_head 请求数据的nshead_t*
 * @param[out] res_head 应答数据的nshead_t*
 * @param[out] res_buf  应答数据的ub_buff_t*
 *
 * return
 *        nothing
 **/
STATIC void
srch_reset_res(nshead_t * req_head, nshead_t * res_head, ub_buff_t * res_buf)
{
    res_head->id = srch_ID;
    res_head->version = srch_VERSION;
    res_head->log_id = req_head->log_id;
    strlcpy(res_head->provider, g_conf.srch.svr_name, sizeof (res_head->provider));
    res_head->magic_num = NSHEAD_MAGICNUM;
    res_head->body_len = 0;
    res_buf->buf[0] = '\0';
}

/**
 *
 * @brief 处理srch命令的函数
 *
 * @param[in] cmd_no 命令号,一个函数可能处理多条命令,可以用命令号来区分
 * @param[in] req_head 请求数据的nshead_t*
 * @param[in] req_buf 请求数据的buffer,本buffer不包含nshead_t
 * @param[out] res_head 应答数据的nshead_t*
 * @param[out] res_buf 应答数据的buffer,本buffer不包含nshead_t
 * @return
 *       0  : 成功
 *       -1 : 失败,socket将直接关闭,不给client返回错误信息
 **/
STATIC int
srch_process(int cmd_no, nshead_t * req_head, ub_buff_t * req_buf,
         nshead_t * res_head, ub_buff_t * res_buf)
{
    /**
     *在打NOTICE日志时,请使用 ub_log_pushnotice 加入日志信息
     *如果返回值不为0,将导致socket被直接关闭而不给客户返回任何信息
     **/
    srch_thread_data_t *p_thread_data;

    if(NULL == req_head || NULL == req_buf || NULL == res_head || NULL == res_buf) {
        UB_LOG_FATAL("parameter error in srch process.");
        return -1;
    }

    srch_reset_res(req_head, res_head, res_buf);

    char* req = req_buf->buf;
    char* res = res_buf->buf;

    res_head->body_len = 0;
	int response_buffer_size = ub_server_get_write_size() - sizeof (nshead_t);

    p_thread_data = (srch_thread_data_t* )ub_server_get_user_data();
    if(NULL == p_thread_data) {
        UB_LOG_FATAL("thead_data null");
        return -1;
    }

	int post_count            = 0;
	int max_post_count        = p_thread_data->max_post_count;
	u_int64_t* pids           = p_thread_data->post_ids;
	delpost_record_t* records = p_thread_data->records;

	if (!unpack_data(req, req_head->body_len, post_count, pids, max_post_count))
	{
		UB_LOG_WARNING("unpack mcpack error");
		return -1;
	}

	int ret = query(post_count, pids, records, 
			g_conf.mask_path, g_conf.index_file, g_conf.mask_file);

	if (ret < 0)
	{
		UB_LOG_WARNING("query[ret=%d] error", ret);
		return -1;
	}
	
	ret = pack_data(res, response_buffer_size, post_count, records);
	if (ret < 0)
	{
		UB_LOG_WARNING("pack mcpack error");
		return -1;
	}

	res_head->body_len = ret;

    return 0;
}

// mc_pack 解包
static bool unpack_data(const char* src_buf, int src_buf_len, int& post_count, u_int64_t* pids, int max_post_count)
{
	const int TMP_BUF_SIZE = 1024 * 1024;
	char tmp_buffer[TMP_BUF_SIZE];
	// mc_pack 处理
	mc_pack_t* pack = mc_pack_open_r(src_buf, src_buf_len, tmp_buffer, sizeof (tmp_buffer));
	if (pack == NULL || MC_PACK_PTR_ERR(pack) != 0)
	{
		UB_LOG_WARNING("mc_pack_open_r error.");
		return false;
	}

	int ret = 0;
	ret = mc_pack_get_int32(pack, "post_count", &post_count);
	if(0 != ret || post_count <= 0)
	{
		UB_LOG_WARNING("post_count[ret=%d, post_count=%d] get error.", ret, post_count);
		return false;
	}

	if (post_count > max_post_count)
	{
		UB_LOG_WARNING("query post_count[%d] is too large[max=%d].", post_count, max_post_count);
		return false;
	}

	mc_pack_t* pid_array = mc_pack_get_array(pack, "postids");
	if (pid_array == NULL || MC_PACK_PTR_ERR(pid_array) != 0)
	{
		UB_LOG_WARNING("recv postids pack[pid_array=%p, PTR_ERR=%d] is not valid.", pid_array, MC_PACK_PTR_ERR(pid_array));
		return false;
	}
	int c = mc_pack_get_item_count(pid_array);
	if (c != post_count)
	{
		UB_LOG_WARNING("mcpack_pid_count[ret=%d, post_count=%d] get error.", c, post_count);
		return false;
	}

	for (int i = 0; i < post_count; i++)
	{
		u_int64_t pid = 0;
		ret = mc_pack_get_uint64_arr(pid_array, i, (mc_uint64_t*)&pid);
		if(0 != ret)
		{
			UB_LOG_WARNING("get the %d(th) pid[ret = %d, pid=%lu] error. ", i, ret, pid);
			return false;
		}
		pids[i] = pid;
	}
	return true;
}

static int pack_data(char* dest_buf, int dest_buf_len, int post_count, const delpost_record_t* records)
{
	const int TMP_BUF_SIZE = 1024 * 1024;
	char tmp_buffer[TMP_BUF_SIZE];
	
	mc_pack_t* res_pack = mc_pack_open_w(1, dest_buf, dest_buf_len, tmp_buffer, sizeof (tmp_buffer));
	if(MC_PACK_PTR_ERR(res_pack) != 0)
	{
		UB_LOG_WARNING("mc_pack_create error");
		return -1;
	}

	int ret = 0;

	ret = mc_pack_put_int32(res_pack, "post_count", post_count);
	if (ret != 0)
	{
		UB_LOG_WARNING("post_count[ret=%d, post_count=%d] put error.", ret, post_count);
		return -1;
	}

	mc_pack_t* res_array = mc_pack_put_array(res_pack, "del_post_info");
	if (res_array == NULL || MC_PACK_PTR_ERR(res_array) != 0)
	{
		UB_LOG_WARNING("mc_pack_put_array del_post_info[ret=%p, MC_PACK_PTR_ERR=%d] error", res_array, MC_PACK_PTR_ERR(res_array));
		return -1;
	}

	for (int i = 0; i < post_count; i++)
	{
		mc_pack_t* obj = mc_pack_put_object(res_array, NULL);
		if (obj == NULL || MC_PACK_PTR_ERR(obj) != 0)
		{
			UB_LOG_WARNING("mc_pack_put_object res_array[ret=%p, MC_PACK_PTR_ERR=%d] error", obj, MC_PACK_PTR_ERR(obj));
			return -1;
		}

		int r = 0;
		r = mc_pack_put_uint64(obj, "post_id", (mc_uint64_t)records[i].post_id);
		if (r != 0)
		{
			UB_LOG_WARNING("mc_pack_put_uint64 post_id[pid=%lu, ret=%d] error", records[i].post_id, r);
			return -1;
		}

		r = mc_pack_put_nstr(obj, "user_name", records[i].username, strlen(records[i].username) + 1);
		if (r != 0)
		{
			UB_LOG_WARNING("mc_pack_put_uint64 user_name[username=%s, ret=%d] error", records[i].username, r);
			return -1;
		}

		r = mc_pack_put_uint32(obj, "del_time", records[i].del_time);
		if (r != 0)
		{
			UB_LOG_WARNING("mc_pack_put_uint64 del_time[del_time=%u, ret=%d] error", records[i].del_time, r);
			return -1;
		}
	}

	mc_pack_close(res_pack);

	return mc_pack_get_size(res_pack);
}

/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */

