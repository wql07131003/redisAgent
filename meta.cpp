/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file meta.cpp
 * @author opeddm@gmail.com
 * @date 2012/05/10 17:21:54
 * @brief 
 *  
 **/

#include <Configure.h>
#include "meta.h"
#include "router.h"


extern comcfg::Configure *conf;
extern std::string conf_dir;
extern std::string conf_name;

time_t conf_lastmodify;
event_timer_t *conf_reload_event;

meta_info_t *meta;
hash_map_auth meta_auth;

int meta_init() {
    meta = (meta_info_t*)calloc(1, sizeof(meta_info_t));
    //1.0仅支持1个db
    meta->dbs = (meta_db_info_t**)calloc(1, sizeof(meta_db_info_t*));

    conf_lastmodify = 0;

    //register conf load/reload event
    //struct timeval one_sec;
    //one_sec.tv_sec = 1;
    //one_sec.tv_usec = 0;
    //conf_reload_event = event_timer_new(_meta_conf_reload, 1000000, true, NULL);
    //int ret = event_add(conf_reload_event, &one_sec);
    //if (ret == 0) {
    //    log_debug("conf_reload_event added success");
    //}
    //else {
    //    log_fatal("conf_reload_event added failed");
    //    return -1;
    //}

    //auth init
    if (meta_auth.create(MAX_USER_NUM) != 0) {
        log_fatal("failed to create bsl::hashmap with size %d\n", MAX_USER_NUM);
        return -1;
    }
    int ret = _meta_conf_reload(NULL);
    if (ret) {
        log_fatal("meta init failed");
        return ret;
    }

    log_notice("meta init done! timestamp=%d", conf_lastmodify);

    return 0;
}

int _meta_info_clean(int i, meta_info_t *meta) {
    meta_db_info_t *db = meta->dbs[i];
    if (db == NULL) {
        return 0;
    }
    log_debug("_meta_info_clean");

    for (int i=0; i<db->service_num; i++) {
        free(db->services[i]);
    }
    free(db->services);
    db->service_num = 0;
    db->services = NULL;

    for (int i=0; i<db->user_num; i++) {
        free(db->users[i]);
    }
    free(db->users);
    db->user_num = 0;
    db->users = NULL;

    free(db->partition);

    free(db->partition_service_w);

    free(db->partition_service_r);

    free(db);
    return 0;
}

int _meta_build_router() {
    int ret = 0;
    log_debug("in meta build router");

    ret = _meta_info_clean(0, meta);
    if (ret) {
        log_fatal("meta_info_clean failed");
        return ret;
    }
    ret = _meta_load_conf(0, meta);
    if (ret) {
        log_fatal("meta_load_conf failed");
        return ret;
    }

    ret = router_build();
    if (ret) {
        log_fatal("router_build failed");
        return ret;
    }

    log_notice("meta build router done! timestamp=%d", conf_lastmodify);
    return ret;
}

int _meta_auth_load() {
    meta_auth.clear();

    for (int i=0; i<meta->dbs[0]->user_num; i++) {
        meta_user_info_t *user = meta->dbs[0]->users[i];
        int ret = meta_auth.set(key(user->uname), i);
        log_debug("_meta_auth_load, add user, ret=%d uid=%d uname=%s", ret, i, user->uname);
        if (ret == bsl::HASH_EXIST) {
            log_warning("_meta_auth_load, uname key existed, uid=%d uname=%s", i, user->uname);
        }
    }

    log_debug("_meta_auth_load done!");
    return 0;
}

int meta_auth_check(/*char *pid,*/ const char *uname, const char *tk, const char *ip) {
    if (uname == NULL || tk == NULL || ip == NULL) {
        log_warning("meta_auth_check input validation failed");
        return -1;
    }
    /*
    //pid check
    if (strhash(pid) != strhash(meta->dbs[0]->pid)) {
        log_warning("meta_auth_check failed, pid error, pid:%s uname:%s tk:%s fromip:%s", pid, uname, tk, ip);
        return -1;
    }
    */

    //uname check
    value id;
    int ret = meta_auth.get(key(uname), &id);
    if (ret != bsl::HASH_EXIST) {
        //log_warning("meta_auth_check failed, uname unauth, pid:%s uname:%s tk:%s fromip:%s", pid, uname, tk, ip);
        log_warning("meta_auth_check failed, uname unauth, uname=%s tk=%s fromip=%s", uname, tk, ip);
        return -1;
    }

    //tk check
    meta_user_info_t *user = meta->dbs[0]->users[id];
    if (strhash(tk) != strhash(user->tk)) {
        //log_warning("meta_auth_check failed, tk unauth, pid:%s uname:%s tk:%s fromip:%s", pid, uname, tk, ip);
        log_warning("meta_auth_check failed, tk unauth, uname=%s tk=%s fromip=%s", uname, tk, ip);
        return -2;
    }

    //log_debug("meta_auth_check pass, pid:%s uname:%s tk:%s fromip:%s power:%d", pid, uname, tk, ip, user->power);
    log_debug("meta_auth_check pass, uname=%s fromip=%s power=%d", uname, ip, user->power);
    return user->power;
}


int _meta_conf_reload(void *param) {
    time_t t = conf->lastConfigModify();

    if (conf_lastmodify < t && !router_is_building()) {
        comcfg::Configure * p = new comcfg::Configure();
        if (p->load(conf_dir.c_str(), conf_name.c_str())) {
            log_fatal("bad conf. will not reload it");
            return -1;
        }

        comcfg::Configure *old = conf;
        conf = p;
        delete old;
        conf_lastmodify = t;

        //db init
        int db_num_count;//配置中[@db]的数量

        try {
            db_num_count = conf[0]["Db"].size();
        } catch (comcfg::ConfigException e) {
            log_fatal("Read conf error=%s", e.what());
            return -1;
        }

        if (db_num_count!=1) {
            log_fatal("meta init conf failed! db_num wrong! timestamp=%d", conf_lastmodify);
            //配置的db数量异常
            return -1;
        }

        int ret = _meta_build_router();
        if (ret) {
            log_fatal("meta_build_router failed");
            return -1;
        }
        ret = _meta_auth_load();
        if (ret) {
            log_fatal("meta_auth_load failed");
            return -1;
        }
    }
    return 0;
}

int meta_get_db_info(/*char* dbname,*/ meta_db_info_t *&db_info) {
    db_info = meta->dbs[0];
    return 0;
}

int meta_get_timeout (time_t &sec, time_t &usec) {
    sec = (time_t)meta->dbs[0]->timeout_sec;
    usec = (time_t)meta->dbs[0]->timeout_usec;
    return 0;
}

int _meta_load_partition(int id, char *partition_str, meta_partition_info_t &partition_info) {

    log_debug("_meta_load_partition, partid=%d str=%s", id, partition_str);
    char comma = ',';
    char range_sign = '-';
    char * pinfo= NULL;
    char * word_begin= NULL;
    char * line_end= NULL;
    char * p= NULL;
    unsigned int group_begin = 0;
    unsigned int group_end = 0;
    unsigned int group = 0;
    int len = 0;

    partition_info.num = 0;
    partition_info.id = id;

    len = strlen(partition_str);
    if (len == 0)
    {
        return 0;
    }

    pinfo = partition_str;
    line_end = pinfo + len;
    while (pinfo < line_end) {
        word_begin = pinfo;
        p = strchr(pinfo, comma);

        if (p!=NULL)
        {
            *p = '\0';
            pinfo = p+1;
        }
        else
        {
            pinfo = line_end;
        }

        p = strchr(word_begin, range_sign);
        if (p == NULL) {
            group = atoi(word_begin);
            if (group >= PARTITION_MOD_KEY) {
                log_fatal("Service_Partition conf error, group=%d", group);
                return -1;
            }
            //记录group
            partition_info.partitions[partition_info.num] = group;
            partition_info.num ++;
        }
        else {
            *p = '\0';
            group_begin = atoi(word_begin);
            group_end = atoi(p+1);
            if  (group_begin>group_end || group_end>PARTITION_MOD_KEY) {
                log_fatal("Service_Partition conf error, group_begin=%d group_end=%d", group_begin, group_end);
                return -1;
            }
            for (unsigned int j=group_begin; j<=group_end; j++) {
                //记录group
                partition_info.partitions[partition_info.num] = j;
                partition_info.num ++;
            }
        }
    }
 
    log_debug("_meta_load_partition, partid=%d partnum=%d", partition_info.id, partition_info.num);
    return partition_info.num;
}

int _meta_load_conf(int i, meta_info_t *meta) {
    //basic config
    meta_db_info_t *db = (meta_db_info_t*)calloc(1, sizeof(meta_db_info_t));
    int partition_num_r = 0;
    int partition_num_w = 0;

    meta->dbs[i]=db;
    db->id = i;
    char isLong[5] = {0};

    try {
        //machine room
        conf[0]["SuperStrategy"]["Machine_Room"].get_cstr(db->machine_room, MAX_DB_NAME_LEN);

        conf[0]["Db"][i]["Db_Pid"].get_cstr(db->pid, MAX_PID_LEN);
        conf[0]["Db"][i]["Db_Name"].get_cstr(db->db_name, MAX_DB_NAME_LEN);

        //connect  config
        conf[0]["Db"][i]["DbConnect"]["Db_Connect_type"].get_cstr(isLong, 5);
        db->connect_per_service = conf[0]["Db"][i]["DbConnect"]["Db_Connect_Num"].to_uint32();
        db->timeout = conf[0]["Db"][i]["DbConnect"]["Db_Connect_ProcessTimeoutMs"].to_uint32();
        db->get_connect_retry = conf[0]["Db"][i]["DbConnect"]["Db_Connect_GetConnectRetryNum"].to_uint32();

        //shard config
        db->shard_mode = conf[0]["Db"][i]["DbShard"]["Db_Shard_Mode"].to_uint32();
        db->partition_num = conf[0]["Db"][i]["DbShard"]["Db_Shard_PartitionNum"].to_uint32();
    } catch (comcfg::ConfigException e) {
        log_fatal("Read conf error: %s", e.what());
        return -1;
    }

    // support for idc & machineroom, by cdz
    comcfg::ErrCode err;
    db->readRoomOnly = conf[0]["Db"][i]["DbConnect"]["Db_Connect_ReadRoomOnly"].to_int32(&err);
    if (err != 0) {
        log_warning("No Db_Connect_ReadRoomOnly config or get error: error[%d], use 0 for default(cross_room mode)", err);
        db->readRoomOnly = 0;
    }
    if (db->readRoomOnly) {
        if (err=conf[0]["Server"]["MachineRoom"].get_cstr(db->serverMachineRoomName, MAX_MACHINE_ROOM_NAME_LEN)) {
            log_fatal("Db_Connect_ReadRoomOnly is not 0, but get MachineRoom error: error[%d], connectRoomOnly[%d] MachineRoom[%s]", err, db->readRoomOnly, db->serverMachineRoomName);
            return -1;
        }
    }

    db->readIdcOnly = conf[0]["Db"][i]["DbConnect"]["Db_Connect_ReadIdcOnly"].to_int32(&err);
    if (err != 0) {
        log_warning("No Db_Connect_ReadIdcOnly config or get error: error[%d], use 1 for default(slave_idc_only mode)", err);
        db->readIdcOnly = 1;
    }
    if (db->readIdcOnly) {
        if (err=conf[0]["Server"]["Idc"].get_cstr(db->serverIdcName, MAX_IDC_NAME_LEN)) {
            log_fatal("Db_Connect_ReadIdcOnly is not 0, but get Idc error: error[%d], connectIdcOnly[%d] Idc[%s]", err, db->readIdcOnly, db->serverIdcName);
            return -1;
        }
    }
    // add done

    db->timeout_sec = (unsigned int)(db->timeout / 1000);
    db->timeout_usec = (unsigned int)((db->timeout % 1000) * 1000);
    db->long_connect = strcmp(isLong, "LONG") ? true : false;
    db->partition = (meta_partition_info_t*)calloc(db->partition_num, sizeof(meta_partition_info_t));
    db->partition_service_w = (meta_partition_service_info_t*)calloc(db->partition_num, sizeof(meta_partition_service_info_t));
    db->partition_service_r = (meta_partition_service_info_t*)calloc(db->partition_num, sizeof(meta_partition_service_info_t));

    for (unsigned int j=0; j<db->partition_num; j++) {
        char cname[25];
        snprintf(cname, sizeof(cname), "Db_Shard_Partition%d", j);
        //sprintf(cname, "Db_Shard_Partition%d", j);
        char partition_str[PARTITION_INFO_SIZE];
        try {
            conf[0]["Db"][i]["DbShard"][cname].get_cstr(partition_str, PARTITION_INFO_SIZE);
        } catch (comcfg::ConfigException e) {
            log_fatal("Read conf error: %s", e.what());
            return -1;
        }

        int ret = _meta_load_partition(j, partition_str, db->partition[j]);

        log_debug("_meta_load_conf, ret=%d partid=%d part_num=%d", ret, db->partition[j].id, db->partition[j].num);
    }

    //service config
    int service_num_r = 0;
    int service_num_w = 0;
    try {
        db->service_num = conf[0]["Db"][i]["Service"].size();
    } catch (comcfg::ConfigException e) {
        log_fatal("Read conf error: %s", e.what());
        return -1;
    }

    db->services = (meta_service_info_t**)calloc(db->service_num, sizeof(meta_service_info_t*));

    for (int j=0; j<db->service_num; j++) {
        meta_service_info_t *service = (meta_service_info_t*)calloc(1, sizeof(meta_service_info_t));
        db->services[j] = service;

        service->id = j;

        try {
            conf[0]["Db"][i]["Service"][j]["Service_Name"].get_cstr(service->service_name, 32);
            conf[0]["Db"][i]["Service"][j]["Service_Addr"].get_cstr(service->addr, 128);
            service->port = conf[0]["Db"][i]["Service"][j]["Service_Port"].to_uint32();
            service->partition = conf[0]["Db"][i]["Service"][j]["Service_Partition"].to_uint32();

            service->can_read = conf[0]["Db"][i]["Service"][j]["Service_CanRead"].to_int32() == 0 ? false:true;
            service->can_write = conf[0]["Db"][i]["Service"][j]["Service_CanWrite"].to_int32() == 0 ? false:true;
        } catch (comcfg::ConfigException e) {
            log_fatal("Read conf error: %s", e.what());
            return -1;
        }
        // support for idc & machineroom, by cdz
        comcfg::ErrCode err;
        if (db->readRoomOnly) {
            if (err=conf[0]["Db"][i]["Service"][j]["Service_MachineRoom"].get_cstr(service->machineRoomName, MAX_MACHINE_ROOM_NAME_LEN)) {
                log_fatal("Db_Connect_ReadRoomOnly is not 0, but get serviceMachineRoom error: error[%d], connectRoomOnly[%d] MachineRoom[%s]", err, db->readRoomOnly, service->machineRoomName);
                return -1;
            }
        }

        if (db->readIdcOnly) {
            if (err=conf[0]["Db"][i]["Service"][j]["Service_Idc"].get_cstr(service->idcName, MAX_IDC_NAME_LEN)) {
                log_fatal("Db_Connect_ReadIdcOnly is not 0, but get serviceIdc error: error[%d], connectIdcOnly[%d] Idc[%s]", err, db->readIdcOnly, service->idcName);
                return -1;
            }
        }

        if (service->can_read) {
            if (db->readIdcOnly && strncmp(service->idcName, db->serverIdcName, MAX_IDC_NAME_LEN)) {
                service->can_read = false;
            }
            if (db->readRoomOnly && strncmp(service->machineRoomName, db->serverMachineRoomName, MAX_MACHINE_ROOM_NAME_LEN)) {
                service->can_read = false;
            }
        }
        // add done

        if (service->partition >= db->partition_num) {
            log_fatal("_meta_load_conf, partition conf error for service=%d partition=%d partition_num=%d", j, service->partition, db->partition_num);
            return -1;
        }

        if (service->can_read == true) {
            //读库允许一个分片包括多台机器
            //获取机器所在分片的id = service->partition;
            
            //本机器落在该分片的service_id = 当前该分片已有的service数量
            int service_id = db->partition_service_r[service->partition].service_num;
            db->partition_service_r[service->partition].service[service_id] = service;
            db->partition_service_r[service->partition].service_num ++;
            service_num_r ++;
            log_debug("_meta_load_conf, service=%d add_read_partition=%d", j, service->partition);
        }

        if (service->can_write == true) {
            //ver1.0不允许一个写分片包括多台机器
            int service_id = db->partition_service_w[service->partition].service_num;
            db->partition_service_w[service->partition].service[service_id] = service;
            db->partition_service_w[service->partition].service_num ++;
            service_num_w ++;
            log_debug("_meta_load_conf, service=%d add_write_partition=%d", j, service->partition);
        }
    }
 
    //user config
    try {
        db->user_num = conf[0]["Db"][i]["User"].size();
    } catch (comcfg::ConfigException e) {
        log_fatal("Read conf error: %s", e.what());
        return -1;
    }

    db->users = (meta_user_info_t**)calloc(db->user_num, sizeof(meta_user_info_t*));
    for (int j=0; j<db->user_num; j++) {
        meta_user_info_t *user = (meta_user_info_t*)calloc(1, sizeof(meta_user_info_t));
        db->users[j] = user;
        
        try {
            conf[0]["Db"][i]["User"][j]["User_Name"].get_cstr(user->uname, MAX_UNAME_LEN);
            conf[0]["Db"][i]["User"][j]["User_Tk"].get_cstr(user->tk, MAX_TK_LEN);
            user->power = conf[0]["Db"][i]["User"][j]["User_Power"].to_int32();
        } catch (comcfg::ConfigException e) {
            log_fatal("Read conf error: %s", e.what());
            return -1;
        }
    }

    log_notice("db get meta done, pid=%s mode=%d pnum_r=%d pnum_w=%d snum_r=%d snum_w=%d snum=%d unum=%d",
            db->pid, db->shard_mode, partition_num_r, partition_num_w, service_num_r, service_num_w, db->service_num, db->user_num);

    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
