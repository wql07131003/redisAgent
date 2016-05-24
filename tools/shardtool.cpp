/***************************************************************************
 * 
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file shardtool.cpp
 * @author chendazhuang(com@baidu.com)
 * @date 2013/01/07 16:39:38
 * @brief 
 *  
 **/

#include "Configure.h"
#include "hash.h"

int main(int argc, char *argv[]) {
    
    if (argc != 2) {
        printf("No Key \n");
        return -1;
    }

    comcfg::Configure *conf = new comcfg::Configure();
    if (conf->load(".", "proxy.conf")) {
        printf("fail to load proxy.conf\n");
        return -1;
    }
    //printf("%s\n", conf[0]["Galileo"]["ZkHost"].to_cstr());
    //printf("%u\n", conf[0]["Db"].size());
    //printf("%s\n", conf[0]["Log"]["Name"].to_cstr());
    //printf("%s\n", conf[0]["Db"][0]["Db_Name"].to_cstr());
    //printf("总分片数：%s\n", conf[0]["Db"][0]["DbShard"]["Db_Shard_PartitionNum"].to_cstr());

    int sum_shards = conf[0]["Db"][0]["DbShard"]["Db_Shard_PartitionNum"].to_int32();
    int *shard_table = (int*)calloc(sizeof(int), sum_shards);
    if (!shard_table) {
        printf("calloc fail\n");
        return -1;
    }

    for(int i = 0; i < sum_shards; i ++ ) {
        int start, end;
        char buf[64] = {0};
        snprintf(buf, sizeof(buf), "Db_Shard_Partition%d", i);
        sscanf(conf[0]["Db"][0]["DbShard"][buf].to_cstr(), "%d-%d", &start, &end);
        //printf("shard%d, start %d, end %d\n", i, start, end);
        shard_table[i] = end;
    }

    char *shard_ip = (char*)malloc(sizeof(char)*32*sum_shards);
    unsigned short *shard_port = (unsigned short*)malloc(sizeof(unsigned short)*sum_shards);

    int sum_servers = conf[0]["Db"][0]["Service"].size();
    for(int i = 0; i < sum_servers; i ++ ) {
        if (conf[0]["Db"][0]["Service"][i]["Service_CanWrite"].to_int32() == 1) {
                int this_shard = conf[0]["Db"][0]["Service"][i]["Service_Partition"].to_int32();
                conf[0]["Db"][0]["Service"][i]["Service_Addr"].get_cstr((char*)shard_ip+(this_shard*32), 32);
                conf[0]["Db"][0]["Service"][i]["Service_Port"].get_uint16((unsigned short*)shard_port+this_shard);

                //printf("shard %d, ip: %s, port: %u\n", this_shard, shard_ip+this_shard*32, *(shard_port+this_shard));
        }
    }

    for (int i = 1; i < argc; i ++) {
       int shard_value = strhash(argv[i]) & 0xFFFF;
       int shard_num = 0;
       for (; shard_num < sum_shards; shard_num ++) {
           if (shard_table[shard_num] >= shard_value)
               break;
       }

       //printf("key: %s, shard_value: %d, ip: %s, port: %u\n", argv[i], shard_value,  shard_ip + shard_num * 32, shard_port[shard_num]);
       printf("%s %d %d %s %u\n", argv[i], shard_value, shard_num, shard_ip + shard_num * 32, shard_port[shard_num]);
    }


    return 0;
}





















/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
