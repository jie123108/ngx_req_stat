/*************************************************
 * Author: jie123108@163.com
 * Copyright: jie123108
 *************************************************/
#ifndef __MONGO_OP_CACHE_H__
#define __MONGO_OP_CACHE_H__
#include "json-c/json.h"
//#include "bson.h"
//#include "mongoc.h"

#ifdef   __cplusplus
extern "C"{
#endif

typedef void stat_cache;

typedef void (*flush_callback_f)(void* c, const char* collname, const char* key, json_object* jso);
int req_stat_cache_flush(stat_cache* c,flush_callback_f flush_callback, void* ctx);
int req_stat_cache_add(stat_cache* c,const char*ns, u_char* key, size_t key_len,
							json_object* bso);

stat_cache* req_stat_cache_new();
void req_stat_cache_delete(stat_cache* cache);

#ifdef   __cplusplus
}
#endif

#endif 
