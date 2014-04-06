/*************************************************
 * Author: jie123108@163.com
 * Copyright: jie123108
 *************************************************/
#include "SimpleConnPool.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
//#include "Logger.h"

#define PCALLOC(size) calloc(1,size)
#define PFREE(ptr) if(ptr){free(ptr);ptr=NULL;}

//namespace redislib{

inline uint64_t uint64_inc(uint64_t* pcount){
	return __sync_add_and_fetch(pcount, 1);
}

#define sync_inc(p) __sync_add_and_fetch(p,1)
#define sync_dec(p) __sync_sub_and_fetch(p,1)
	

#define cpool_is_empty(pool) \
	(pool->start == pool->end && pool->curconns <= 0)

#define cpool_is_full(pool) \
	(pool->start == pool->end && pool->curconns > 0)

void* rs_new_and_connect(rs_conn_cb_t* cbs,void* args,rs_conn_statis_t* statis){
	assert(cbs != NULL);
	void* conn = cbs->conn_new(args);
	if(conn != NULL){
		if(cbs->connect(conn, args)!=0){
			//printf("conn fail \n");
			cbs->conn_free(conn);
			conn = NULL;
		}else{
			//printf("conn ok \n");
			if(statis!=NULL)uint64_inc(&statis->connect);
		}
	}
	return conn;
}
void rs_free_and_close(void* conn, rs_conn_cb_t* cbs,rs_conn_statis_t* statis){
	assert(cbs != NULL);
	if(conn != NULL){
		cbs->close(conn);
		cbs->conn_free(conn);
		if(statis!=NULL)uint64_inc(&statis->close);
	}
}

rs_conn_pool_t* rs_conn_pool_new(int size,int lazy_init,rs_conn_cb_t* cbs, void* args)
{
	int i;
	assert(size > 0);
	rs_conn_pool_t* pool = (rs_conn_pool_t*)PCALLOC(sizeof(rs_conn_pool_t));
	pool->conns = (void**)PCALLOC(sizeof(void*)*size);
	pool->size = size;
	pool->args = args;
	pool->curconns = 0;
	pool->reconn_interval = 1;
	pool->cbs = (rs_conn_cb_t*)PCALLOC(sizeof(rs_conn_cb_t));
	memcpy(pool->cbs, cbs, sizeof(rs_conn_cb_t));

	if(lazy_init == 0){
		pool->start = pool->end = 0;
		for(i=0;i<size; i++){
			pool->conns[i] = rs_new_and_connect(cbs, args,&pool->statis);
			pool->curconns++;
		}
	}else{
		pool->start = 0;
		pool->end = 0;
	}
	

	pthread_mutex_init(&pool->mutex,NULL);
	return pool;
}

void rs_conn_pool_free(rs_conn_pool_t* pool)
{
	rs_conn_cb_t* cbs = pool->cbs;
	int i;
	for(i=0;i<pool->size; i++){
		if(pool->conns[i] != NULL){
			rs_free_and_close(pool->conns[i], cbs,&pool->statis);
			pool->conns[i] = NULL;
		}
	}
	pthread_mutex_destroy(&pool->mutex);
	PFREE(pool->cbs);
	PFREE(pool->conns);
	PFREE(pool);
}

inline int rs_need_reconnect(rs_conn_pool_t* pool)
{
	if(pool->reconn_interval<1){
		return 1;
	}else{
		time_t now = time(NULL);
		return (now - pool->pre_err_time > pool->reconn_interval);
	}
}

void* rs_conn_pool_get(rs_conn_pool_t* pool)
{
	void* conn = NULL;
	rs_conn_cb_t* cbs =pool->cbs;
	int islocked = (pthread_mutex_lock(&pool->mutex)==0);
	//if(!islocked) LOG_ERROR("get pool[0x%08x] lock failed!", (long)pool);

	//LOG_DEBUG("pool get[0x%08x](%d/%d)", (long)pool, pool->curconns, pool->size);
	while(!cpool_is_empty(pool)){			
		if(pool->conns[pool->start] == NULL){
			pool->start = (pool->start+1)%pool->size;
			//LOG_DEBUG("######## conn is null##########");
			continue;
		}else{
			conn = pool->conns[pool->start];
			pool->conns[pool->start] = NULL;
			//sync_dec(&pool->curconns);
			//LOG_DEBUG("---- get conn[0x%08x] from pool[0x%08x](%d) ----", 
			//	(long)conn, (long)pool, pool->curconns);
			pool->start = (pool->start+1)%pool->size;
			if(conn != NULL)uint64_inc(&pool->statis.get);
			break;
		}
	}
	
	if(islocked)pthread_mutex_unlock(&pool->mutex);
	
	if(conn==NULL){		
		if(rs_need_reconnect(pool)){
			conn = rs_new_and_connect(cbs, pool->args, &pool->statis);
			if(conn != NULL){
				uint64_inc(&pool->statis.get_real);
			}else{
				pool->pre_err_time = time(NULL);
			}
		}
		//LOG_DEBUG("###### new conn [0x%08x] ######", (long)conn);
	}
	
	return conn;
	
}

int rs_conn_pool_put(rs_conn_pool_t* pool, void* conn)
{
	rs_conn_cb_t* cbs = pool->cbs;
	int islocked = (pthread_mutex_lock(&pool->mutex)==0);
	//if(!islocked) LOG_ERROR("get pool[0x%08x] lock failed!", (long)pool);
	
	if(!cpool_is_full(pool)){
		assert(pool->conns[pool->end] == NULL);
		if(cbs->test_close(conn)==0){
			//LOG_DEBUG("---- release conn[0x%08x]  to pool[0x%08x](%d)----", 
			//	(long)conn, (long)pool, pool->curconns+1);
			pool->conns[pool->end] = conn;
			conn = NULL;
			pool->end = (pool->end+1) % pool->size;
			//sync_inc(&pool->curconns);
			uint64_inc(&pool->statis.release);
		}else{//连接出错了，需要关闭连接。
			if(cbs->reconnect != NULL && cbs->reconnect(conn,pool->args)==0){//重新连接成功。
				pool->conns[pool->end] = conn;
				conn = NULL;
				pool->end = (pool->end+1) % pool->size;
				//sync_inc(&pool->curconns);
				uint64_inc(&pool->statis.release);
			} 
		}
	} 
	
	if(islocked)pthread_mutex_unlock(&pool->mutex);

	//LOG_DEBUG("pool put[0x%08x](%d/%d)", (long)pool, pool->curconns, pool->size);

	if(conn != NULL){
		//LOG_DEBUG("###### free conn [0x%08x] ######", (long)conn);
		rs_free_and_close(conn,cbs,&pool->statis);
		uint64_inc(&pool->statis.release_real);
		conn = NULL;
	}
	
	return 0;
}

//}
