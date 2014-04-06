/*************************************************
 * Author: jie123108@163.com
 * Copyright: jie123108
 *************************************************/
#ifndef __JSONLIB_CONN_POOL_H__
#define __JSONLIB_CONN_POOL_H__
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

uint64_t uint64_inc(uint64_t* pcount);

typedef void* (*conn_new_cb)(void* args);
typedef int (*conn_connect_cb)(void* conn,void* args);
typedef int (*conn_reconnect_cb)(void* conn,void* args);
typedef int (*conn_close_cb)(void* conn);
typedef int (*conn_test_close_cb)(void* conn); //测试是否需要关闭连接(在连接出错的情况下) 0:不需要关闭，1:需要关闭
typedef void (*conn_free_cb)(void* conn);

typedef struct rs_conn_cb_t{
	conn_new_cb conn_new;
	conn_free_cb conn_free;
	conn_connect_cb connect;
	conn_reconnect_cb reconnect;
	conn_close_cb close;
	conn_test_close_cb test_close;
}rs_conn_cb_t;

typedef struct rs_conn_statis_t{
	uint64_t connect; 	//连接总数(包括池内及池外)
	uint64_t close; 	//关闭连接总数
	uint64_t get;//获取连接总数
	uint64_t get_real; //从池外连接的总数
	uint64_t release;//池内释放连接数
	uint64_t release_real;//池外释放连接数。
}rs_conn_statis_t;

typedef struct rs_conn_pool_t{
	volatile int curconns; 
	void* args; //连接参数
	void** conns; //连接
	//char* status; //是否连接上。'y':已经连接上，'n':未连接上。'0':表示无连接。
	rs_conn_cb_t* cbs; //连接池相关回调。
	int size;
	int start; //当start==end时，有可能是满了，也可能是空了。用curconns判断。curconns > 0 满了，<=0 空了。
	int end;
	time_t pre_err_time; //上次连接出错的时间。
	int reconn_interval; 	  //出错连接最小时间间隔(秒)， 就是如果连接出错后，在该单位时间不再进行新连接。
	pthread_mutex_t mutex;
	rs_conn_statis_t statis;
}rs_conn_pool_t;

/**
 * size 连接池大大小
 * lazy_init 表示在new时不进行实际的连接
 */
rs_conn_pool_t* rs_conn_pool_new(int size,int lazy_init,rs_conn_cb_t* cbs, void* args);
void rs_conn_pool_free(rs_conn_pool_t* pool);
void* rs_conn_pool_get(rs_conn_pool_t* pool);
int rs_conn_pool_put(rs_conn_pool_t* pool, void* conn);
int rs_conn_pool_put2(rs_conn_pool_t* pool, void* conn);


#ifdef __cplusplus
}
#endif

//}
#endif

