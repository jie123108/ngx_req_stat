/*************************************************
 * Author: jie123108@163.com
 * Copyright: jie123108
 *************************************************/

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "json_bson.h"
#include "mongo_op_cache.h"
#include "ngx_http_req_stat_module.h"

typedef struct {
	ngx_array_t             keys;    /* array of ngx_http_req_stat_key_t */
	ngx_uint_t              def_used; /* unsigned  def_used:1 */
	//ngx_str_t 				mongo_host;
	//ngx_uint_t				mongo_port;
	ngx_str_t				mongo_db;
	char 					mongo_db_str[64];
	//ngx_uint_t				mongo_conns;
	ngx_msec_t				mongo_op_timeout;
	ngx_msec_t				mongo_flush_interval;
	time_t 					last_error_time;
	//rs_conn_pool_t* 			mongo_pool;
	ngx_str_t					mongo_uri;
	mongoc_uri_t*                          mongoc_uri_inst;
	mongoc_client_pool_t*         mongo_pool;
	stat_cache* 			cache; //统计对象缓存。
	ngx_event_t* 			flush_event;
	
} ngx_http_req_stat_main_conf_t;

typedef struct {
    ngx_array_t                *stats;       /* array of ngx_http_req_stat_t */

    ngx_uint_t                  off;        /* unsigned  off:1 */
} ngx_http_req_stat_loc_conf_t;



static int ngx_http_req_stat_update(mongoc_client_t* mongoc, 
		const char* db, const char* collname, 
		json_object* query, json_object* update,
		ngx_log_t* log);
static ngx_int_t  ngx_http_req_stat_init_process(ngx_cycle_t *cycle);
static void ngx_http_req_stat_exit_process(ngx_cycle_t *cycle);

static ngx_int_t ngx_http_req_stat_variable_compile(ngx_conf_t *cf,
    ngx_http_req_stat_op_t *op, ngx_str_t *value);
static size_t ngx_http_req_stat_variable_getlen(ngx_http_request_t *r,
    uintptr_t data);
static u_char *ngx_http_req_stat_variable(ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op);
static uintptr_t ngx_http_req_stat_escape(u_char *dst, u_char *src, size_t size);


static void *ngx_http_req_stat_create_main_conf(ngx_conf_t *cf);
static char* ngx_http_req_stat_init_main_conf(ngx_conf_t *cf, void *conf);
static void *ngx_http_req_stat_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_req_stat_merge_loc_conf(ngx_conf_t *cf, void *parent,
    void *child);
static char *ngx_http_req_stat_set_req_stat(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_req_stat_set_key(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_req_stat_compile_key(ngx_conf_t *cf,
    ngx_array_t *flushes, ngx_array_t *ops, ngx_array_t *args, ngx_uint_t s);
static ngx_int_t ngx_http_req_stat_init(ngx_conf_t *cf);


static ngx_command_t  ngx_http_req_stat_commands[] = {

    { ngx_string("stat_key"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_2MORE,
      ngx_http_req_stat_set_key,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("mongo_uri"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_req_stat_main_conf_t, mongo_uri),
      NULL },
    { ngx_string("mongo_db"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_req_stat_main_conf_t, mongo_db),
      NULL },

    { ngx_string("mongo_op_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_req_stat_main_conf_t, mongo_op_timeout),
      NULL },
    { ngx_string("mongo_flush_interval"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_req_stat_main_conf_t, mongo_flush_interval),
      NULL },


    { ngx_string("req_stat"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_HTTP_LMT_CONF|NGX_CONF_TAKE13,
      ngx_http_req_stat_set_req_stat,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_req_stat_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_req_stat_init,                     /* postconfiguration */

    ngx_http_req_stat_create_main_conf,         /* create main configuration */
    ngx_http_req_stat_init_main_conf,         /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_req_stat_create_loc_conf,          /* create location configuration */
    ngx_http_req_stat_merge_loc_conf            /* merge location configuration */
};


ngx_module_t  ngx_http_req_stat_module = {
    NGX_MODULE_V1,
    &ngx_http_req_stat_module_ctx,              /* module context */
    ngx_http_req_stat_commands,                 /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    &ngx_http_req_stat_init_process,       /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    &ngx_http_req_stat_exit_process,       /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_str_t  ngx_http_req_stat_def_key_fmt = ngx_string("{'date':'$date','url':'$uri'}");

ngx_http_req_stat_var_t  ngx_http_req_stat_vars[] = {
    { ngx_string("uri_full"), 0, ngx_http_req_stat_uri_full,ngx_http_req_stat_uri_full_getlen },
    //{ ngx_string("json_status"), 0, ngx_http_req_stat_json_status,ngx_http_req_stat_json_status_getlen},
    //{ ngx_string("json_reason"), 0, ngx_http_req_stat_json_reason,ngx_http_req_stat_json_reason_getlen},

	{ ngx_string("msec"), NGX_TIME_T_LEN + 4, ngx_http_req_stat_msec,NULL },
    { ngx_string("request_time"), NGX_TIME_T_LEN + 4,ngx_http_req_stat_request_time,NULL },
    { ngx_string("status"), 3, ngx_http_req_stat_status,NULL },
    { ngx_string("date"), 10, ngx_http_req_stat_date,NULL }, //yyyy-MM-dd
    { ngx_string("time"), 8, ngx_http_req_stat_time,NULL }, //hh:mm:ss
    { ngx_string("year"), 4, ngx_http_req_stat_year,NULL }, //yyyy
    { ngx_string("month"), 2, ngx_http_req_stat_month,NULL }, //MM
    { ngx_string("day"), 2, ngx_http_req_stat_day,NULL }, //dd
    { ngx_string("hour"), 2, ngx_http_req_stat_hour,NULL }, //hh
    { ngx_string("minute"), 2, ngx_http_req_stat_minute,NULL }, //mm
    { ngx_string("second"), 2, ngx_http_req_stat_second,NULL }, //ss
    { ngx_string("bytes_sent"), NGX_OFF_T_LEN, ngx_http_req_stat_bytes_sent,NULL },
    { ngx_string("body_bytes_sent"), NGX_OFF_T_LEN,
                          ngx_http_req_stat_body_bytes_sent,NULL },
    { ngx_string("request_length"), NGX_SIZE_T_LEN,
                          ngx_http_req_stat_request_length,NULL },

    { ngx_null_string, 0, NULL,NULL }
};

typedef struct {
	ngx_log_t* log;
	ngx_http_req_stat_main_conf_t* conf;
}ngx_http_flush_callback_ctx_t;

static void ngx_http_req_stat_flush(void* c, const char* collname, const char* key, json_object* jso)
{
	ngx_http_flush_callback_ctx_t* ctx = c;
    	ngx_http_req_stat_main_conf_t   *lmcf = ctx->conf;
	json_object* jso_query = json_tokener_parse(key);
	if(is_error(jso_query)){
		time_t now = ngx_time();
	    	if (now - lmcf->last_error_time > 59) {
	        	ngx_log_error(NGX_LOG_ALERT, ctx->log, 0,
						"parse query json(%s) failed!",&key);
				lmcf->last_error_time = now;
	    	}
		return;
	}

	
	mongoc_client_t* mongoc = mongoc_client_pool_pop(lmcf->mongo_pool);
	if(mongoc == NULL){
		//记录错误时间。。
		time_t now = ngx_time();
	    	if (now - lmcf->last_error_time > 59) {
	        		ngx_log_error(NGX_LOG_ALERT, ctx->log, 0,
		                      "get mongo conn failed! mongo_uri: %V", &lmcf->mongo_uri);
			lmcf->last_error_time = now;
	    	}
		json_object_put(jso_query);
		return;
	}
	
	int ret = ngx_http_req_stat_update(mongoc, lmcf->mongo_db_str, collname,
						jso_query, jso,ctx->log);
	if(ret != 0){
		time_t now = ngx_time();
	    	if (now - lmcf->last_error_time > 59) {
	        	ngx_log_error(NGX_LOG_ALERT, ctx->log, 0,"mongo update failed! ret=%d",ret);
				lmcf->last_error_time = now;
	    	}
	}else{
		//printf("########## upsert ngx stat #####\n");
	}
	mongoc_client_pool_push(lmcf->mongo_pool, mongoc);
	json_object_put(jso_query);
}

static void ngx_http_req_stat_flush_handler(ngx_event_t *ev)
{
	ngx_http_req_stat_main_conf_t   *lmcf;
	
	lmcf = (ngx_http_req_stat_main_conf_t*)ev->data;
	if(lmcf == NULL){
		return;
	}

	//printf("pid[%d]########## flush, interval [%d]#######\n", 
	//		ngx_getpid(), (int)lmcf->mongo_flush_interval);
	ngx_http_flush_callback_ctx_t ctx;
	ctx.log = ev->log;
	ctx.conf = lmcf;
	req_stat_cache_flush(lmcf->cache, &ngx_http_req_stat_flush, &ctx);

	
	ngx_add_timer(lmcf->flush_event,lmcf->mongo_flush_interval);

}

static ngx_int_t  ngx_http_req_stat_init_process(ngx_cycle_t *cycle)
{
    ngx_http_req_stat_main_conf_t   *lmcf;

	lmcf = (ngx_http_req_stat_main_conf_t*)ngx_get_conf(cycle->conf_ctx, ngx_http_req_stat_module);
	if(lmcf == NULL){
		printf("#### get req_stat_main conf failed!\n########");
		return 0;
	}

	if(lmcf->mongo_flush_interval > 0){
		//printf("#############mongo_flush_interval[%d]###############\n",(int)lmcf->mongo_flush_interval);;
		ngx_event_t *fev = (ngx_event_t*)ngx_pcalloc(cycle->pool, sizeof(ngx_event_t));
		lmcf->flush_event = fev;
		fev->data = lmcf;
		fev->handler = &ngx_http_req_stat_flush_handler;
		fev->timer_set = 0;
		fev->log = cycle->log;
		lmcf->cache = req_stat_cache_new();
		srandom(ngx_getpid());
		ngx_msec_t first_time = ngx_random()%lmcf->mongo_flush_interval+100;
		//printf("first_time:%d\n", (int)first_time);
		ngx_add_timer(fev, first_time);
	}

	return NGX_OK;
}


static void ngx_http_req_stat_exit_process(ngx_cycle_t *cycle)
{
    ngx_http_req_stat_main_conf_t   *lmcf;

	lmcf = (ngx_http_req_stat_main_conf_t*)ngx_get_conf(cycle->conf_ctx, ngx_http_req_stat_module);
	if(lmcf == NULL){
		printf("#### get req_stat_main conf failed!\n########");
		return;
	}
	
	if(lmcf->cache!=NULL){
		ngx_http_flush_callback_ctx_t ctx;
		ctx.log = cycle->log;
		ctx.conf = lmcf;
		req_stat_cache_flush(lmcf->cache, &ngx_http_req_stat_flush, &ctx);

		req_stat_cache_delete(lmcf->cache);
		lmcf->cache = NULL;
	}	
}


static int ngx_http_req_stat_update(mongoc_client_t* mongoc, 
		const char* db, const char* collname, 
		json_object* query, json_object* update,
		ngx_log_t* log){
	int ret = 0;	

	bson_t * bso_query = json_to_bson(query);	 
	bson_t* bso_update = json_to_bson(update);
	//ngx_log_error(NGX_LOG_ALERT, log, 0," db.test.update(%s,%s,true)",
	//			json_object_to_json_string(query),
	//			json_object_to_json_string(update));
	
	mongoc_collection_t* collection = mongoc_client_get_collection(mongoc, db, collname);
	if(collection != NULL){		
		bson_error_t err;
		memset(&err,0,sizeof(err));
		bool ok = mongoc_collection_update(collection, MONGOC_UPDATE_UPSERT,
					bso_query, bso_update, NULL, &err);
		if(!ok){
			ngx_log_error(NGX_LOG_ALERT, log, 0,"mongo update failed! err: %d: %s", 
					err.code, err.message);
		}
		mongoc_collection_destroy(collection);
	}else{
		ngx_log_error(NGX_LOG_ALERT, log, 0,"get_collection(%s.%d) failed!", db,collname);
	}
	
	bson_destroy(bso_query);
	bson_destroy(bso_update);
	
	return ret;	
}

void ngx_http_req_stat_write_stat(ngx_http_request_t *r, const char* collname,
		ngx_str_t* query,ngx_str_t* update)
{
	ngx_http_req_stat_main_conf_t   *lmcf;
	lmcf = ngx_http_get_module_main_conf(r, ngx_http_req_stat_module);

	int i;
	for(i=0;i<(int)query->len;i++) {
		if(query->data[i] == '%') query->data[i] = '$';
	}
	for(i=0;i<(int)update->len;i++) {
		if(update->data[i] == '%') update->data[i] = '$';
	}
	json_object* jso_update = json_tokener_parse2((const char*)update->data, (int)update->len);
	if(is_error(jso_update)){
		time_t now = ngx_time();
	    	if (now - lmcf->last_error_time > 59) {
	        	ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
						"parse update json(%V) failed!",&update);
				lmcf->last_error_time = now;
	    	}
		return;
	}

	req_stat_cache_add(lmcf->cache, collname, query->data,query->len, jso_update);
	
	json_object_put(jso_update);
}

ngx_int_t
ngx_http_req_stat_handler(ngx_http_request_t *r)
{
    u_char                   *buf_key, *p_key;
    u_char                   *buf_value, *p_value;
    size_t                    len_key,len_value;
    ngx_uint_t                i, l;
    ngx_http_req_stat_t           *stat;
    //ngx_open_file_t          *file;
    ngx_http_req_stat_op_t        *op_key,*op_value;
    ngx_http_req_stat_loc_conf_t  *lcf;
	ngx_http_req_stat_main_conf_t   *lmcf;


	
    lmcf = ngx_http_get_module_main_conf(r, ngx_http_req_stat_module);

    //ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->stat, 0,
    //               "http req stat handler");

    lcf = ngx_http_get_module_loc_conf(r, ngx_http_req_stat_module);
	if (lcf->off || lcf->stats == NULL) {
        return NGX_OK;
    }
	
    stat = lcf->stats->elts;
    for (l = 0; l < lcf->stats->nelts; l++) {
		//############# key ###################
        ngx_http_script_flush_no_cacheable_variables(r, stat[l].key->flushes);

        len_key = 0;
		
        op_key = stat[l].key->ops->elts;
		
        for (i = 0; i < stat[l].key->ops->nelts; i++) {
            if (op_key[i].len == 0) {
                len_key += op_key[i].getlen(r, op_key[i].data);
            } else {
                len_key += op_key[i].len;
            }
        }

        buf_key = ngx_pcalloc(r->pool, len_key+1);
        if (buf_key == NULL) {
            return NGX_ERROR;
        }

        p_key = buf_key;

        for (i = 0; i < stat[l].key->ops->nelts; i++) {
            p_key = op_key[i].run(r, p_key, &op_key[i]);
        }

		//############# value ###################
        ngx_http_script_flush_no_cacheable_variables(r, stat[l].value->flushes);

	len_value = 0;
	op_value = stat[l].value->ops->elts;
		
        for (i = 0; i < stat[l].value->ops->nelts; i++) {
            if (op_value[i].len == 0) {
                len_value += op_value[i].getlen(r, op_value[i].data);
            } else {
                len_value += op_value[i].len;
            }
        }

        buf_value = ngx_pcalloc(r->pool, len_value+1);
        if (buf_value == NULL) {
            return NGX_ERROR;
        }

        p_value = buf_value;

        for (i = 0; i < stat[l].value->ops->nelts; i++) {
            p_value = op_value[i].run(r, p_value, &op_value[i]);
        }

	ngx_str_t query = {(p_key - buf_key), buf_key};
	ngx_str_t update = {(p_value-buf_value), buf_value};
	
	ngx_http_req_stat_write_stat(r, stat->table, &query, &update);
        //ngx_http_req_stat_write(r, &stat[l], buf_key, p - buf_key);
    }

    return NGX_OK;
}


static u_char *
ngx_http_req_stat_copy_short(ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op)
{
    size_t     len;
    uintptr_t  data;

    len = op->len;
    data = op->data;

    while (len--) {
        *buf++ = (u_char) (data & 0xff);
        data >>= 8;
    }

    return buf;
}


static u_char *
ngx_http_req_stat_copy_long(ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op)
{
    return ngx_cpymem(buf, (u_char *) op->data, op->len);
}



static ngx_int_t
ngx_http_req_stat_variable_compile(ngx_conf_t *cf, ngx_http_req_stat_op_t *op,
    ngx_str_t *value)
{
    ngx_int_t  index;

    index = ngx_http_get_variable_index(cf, value);
    if (index == NGX_ERROR) {
        return NGX_ERROR;
    }

    op->len = 0;
    op->getlen = ngx_http_req_stat_variable_getlen;
    op->run = ngx_http_req_stat_variable;
    op->data = index;

    return NGX_OK;
}


static size_t
ngx_http_req_stat_variable_getlen(ngx_http_request_t *r, uintptr_t data)
{
    uintptr_t                   len;
    ngx_http_variable_value_t  *value;

    value = ngx_http_get_indexed_variable(r, data);

    if (value == NULL || value->not_found) {
        return 1;
    }

    len = ngx_http_req_stat_escape(NULL, value->data, value->len);

    value->escape = len ? 1 : 0;

    return value->len + len * 3;
}


static u_char *
ngx_http_req_stat_variable(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
    ngx_http_variable_value_t  *value;

    value = ngx_http_get_indexed_variable(r, op->data);

    if (value == NULL || value->not_found) {
        *buf = '-';
        return buf + 1;
    }

    if (value->escape == 0) {
        return ngx_cpymem(buf, value->data, value->len);

    } else {
        return (u_char *) ngx_http_req_stat_escape(buf, value->data, value->len);
    }
}


static uintptr_t
ngx_http_req_stat_escape(u_char *dst, u_char *src, size_t size)
{
    ngx_uint_t      n;
    static u_char   hex[] = "0123456789ABCDEF";

    static uint32_t   escape[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00000004, /* 0000 0000 0000 0000  0000 0000 0000 0100 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x10000000, /* 0001 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };


    if (dst == NULL) {

        /* find the number of the characters to be escaped */

        n = 0;

        while (size) {
            if (escape[*src >> 5] & (1 << (*src & 0x1f))) {
                n++;
            }
            src++;
            size--;
        }

        return (uintptr_t) n;
    }

    while (size) {
        if (escape[*src >> 5] & (1 << (*src & 0x1f))) {
            *dst++ = '\\';
            *dst++ = 'x';
            *dst++ = hex[*src >> 4];
            *dst++ = hex[*src & 0xf];
            src++;

        } else {
            *dst++ = *src++;
        }
        size--;
    }

    return (uintptr_t) dst;
}


static void *
ngx_http_req_stat_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_req_stat_main_conf_t  *conf;

    ngx_http_req_stat_key_t  *fmt;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_req_stat_main_conf_t));
    if (conf == NULL) {
        return NULL;
    }

	conf->mongo_op_timeout = NGX_CONF_UNSET_UINT;
	conf->mongo_flush_interval = NGX_CONF_UNSET_UINT;
	conf->flush_event = NULL;
	conf->last_error_time = 0;
	//static ngx_str_t mongo_def_db = ngx_string("ngx_stat");

    if (ngx_array_init(&conf->keys, cf->pool, 4, sizeof(ngx_http_req_stat_key_t))
        != NGX_OK)
    {
        return NULL;
    }

    fmt = ngx_array_push(&conf->keys);
    if (fmt == NULL) {
        return NULL;
    }

    ngx_str_set(&fmt->name, "def");

    fmt->flushes = NULL;

    fmt->ops = ngx_array_create(cf->pool, 16, sizeof(ngx_http_req_stat_op_t));
    if (fmt->ops == NULL) {
        return NULL;
    }

    return conf;
}

static char*   ngx_http_req_stat_init_main_conf(ngx_conf_t *cf, void *conf)
{
	ngx_http_req_stat_main_conf_t  *rscf = conf;
	static ngx_str_t mongo_def_uri = ngx_string("mongodb://127.0.0.1:27017");
	static ngx_str_t mongo_def_db = ngx_string("ngx_stat");
	if(rscf->mongo_uri.len == 0)rscf->mongo_uri = mongo_def_uri;
	if(rscf->mongo_db.len == 0)rscf->mongo_db = mongo_def_db;
	if(rscf->mongo_op_timeout==NGX_CONF_UNSET_UINT)rscf->mongo_op_timeout = 200;
	if(rscf->mongo_flush_interval==NGX_CONF_UNSET_UINT)rscf->mongo_flush_interval = 1000*5;
	rscf->last_error_time = 0;
	if(rscf->mongo_flush_interval < 1000){
		rscf->mongo_flush_interval = 1000;
	}
	return NGX_CONF_OK;
}

static void *
ngx_http_req_stat_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_req_stat_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_req_stat_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }
	//conf->off = 0;

    return conf;
}


static char *
ngx_http_req_stat_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_req_stat_loc_conf_t *prev = parent;
    ngx_http_req_stat_loc_conf_t *conf = child;

    //ngx_http_req_stat_t            *stat;
    //ngx_http_req_stat_key_t        *fmt;
    ngx_http_req_stat_main_conf_t  *lmcf;

    if (conf->stats || conf->off) {
       return NGX_CONF_OK;
    }

    conf->stats = prev->stats;
    conf->off = prev->off;
 
    if (conf->stats || conf->off) {
        return NGX_CONF_OK;
    }

    lmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_req_stat_module);
    lmcf->def_used = 1;

	//ngx_snprintf((u_char*)stat->ns, sizeof(stat->ns)-1, "%V.%V", 
	//			&lmcf->mongo_db, &table);
	//printf("#########db:%.*s\n", (int)lmcf->mongo_db.len, lmcf->mongo_db.data);


    return NGX_CONF_OK;
}


static char *
ngx_http_req_stat_set_req_stat(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_req_stat_loc_conf_t *llcf = conf;

    ngx_uint_t                  i ;//, n;
    ngx_str_t                  *value,name,table, val;
    ngx_http_req_stat_t             *stat;
    ngx_http_req_stat_key_t         *key;
    ngx_http_req_stat_main_conf_t   *lmcf;
    //ngx_http_script_compile_t   sc;
	//printf("##############3 set_req_stat ######\n");
    value = cf->args->elts;
	//printf("args: %d\n", (int)cf->args->nelts);
    if (ngx_strcmp(value[1].data, "off") == 0) {
        llcf->off = 1;
        if (cf->args->nelts == 2) {
            return NGX_CONF_OK;
        }

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid parameter \"%V\"", &value[2]);
        return NGX_CONF_ERROR;
    }else if(cf->args->nelts != 4) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid config \"%V\", missing param!", &value[0]);
        return NGX_CONF_ERROR;
    }

    if (llcf->stats == NULL) {
        llcf->stats = ngx_array_create(cf->pool, 2, sizeof(ngx_http_req_stat_t));
        if (llcf->stats == NULL) {
            return NGX_CONF_ERROR;
        }
    }

	lmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_req_stat_module);

	stat = ngx_array_push(llcf->stats);
	if (stat == NULL) {
		return NGX_CONF_ERROR;
	}

	ngx_memzero(stat, sizeof(ngx_http_req_stat_t));

	table = value[1];
	ngx_snprintf((u_char*)stat->table, sizeof(stat->table)-1, "%V", &table);
	//ngx_snprintf((u_char*)stat->ns, sizeof(stat->ns)-1, "%V.%V", 
	//			&lmcf->mongo_db, &table);
	//printf("#########db:%.*s\n", (int)lmcf->mongo_db.len, lmcf->mongo_db.data);
	
	name = value[2];
    if (ngx_strcmp(name.data, "def") == 0) {
        lmcf->def_used = 1;
    }
	
    key = lmcf->keys.elts;
    for (i = 0; i < lmcf->keys.nelts; i++) {
        if (key[i].name.len == name.len
            && ngx_strcasecmp(key[i].name.data, name.data) == 0)
        {
            stat->key = &key[i];
            break;
        }
    }

	if(stat->key == NULL){
	    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
	                       "unknown stat key \"%V\"", &name);
	    return NGX_CONF_ERROR;
	}

	stat->value = ngx_palloc(cf->pool, sizeof(ngx_http_req_stat_value_t));
	ngx_memzero(stat->value,  sizeof(ngx_http_req_stat_value_t));
	
	val = value[3];
	
    stat->value->name = val;

    stat->value->flushes = ngx_array_create(cf->pool, 2, sizeof(ngx_int_t));
    if (stat->value->flushes == NULL) {
        return NGX_CONF_ERROR;
    }

    stat->value->ops = ngx_array_create(cf->pool, 2, sizeof(ngx_http_req_stat_op_t));
    if (stat->value->ops == NULL) {
        return NGX_CONF_ERROR;
    }

    return ngx_http_req_stat_compile_key(cf, stat->value->flushes, stat->value->ops, cf->args, 3);
}


static char *
ngx_http_req_stat_set_key(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_req_stat_main_conf_t *lmcf = conf;

    ngx_str_t           *value;
    ngx_uint_t           i;
    ngx_http_req_stat_key_t  *fmt;

    value = cf->args->elts;

    fmt = lmcf->keys.elts;
    for (i = 0; i < lmcf->keys.nelts; i++) {
        if (fmt[i].name.len == value[1].len
            && ngx_strcmp(fmt[i].name.data, value[1].data) == 0)
        {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "duplicate \"stat_key\" name \"%V\"",
                               &value[1]);
            return NGX_CONF_ERROR;
        }
    }

    fmt = ngx_array_push(&lmcf->keys);
    if (fmt == NULL) {
        return NGX_CONF_ERROR;
    }

    fmt->name = value[1];

    fmt->flushes = ngx_array_create(cf->pool, 4, sizeof(ngx_int_t));
    if (fmt->flushes == NULL) {
        return NGX_CONF_ERROR;
    }

    fmt->ops = ngx_array_create(cf->pool, 16, sizeof(ngx_http_req_stat_op_t));
    if (fmt->ops == NULL) {
        return NGX_CONF_ERROR;
    }

    return ngx_http_req_stat_compile_key(cf, fmt->flushes, fmt->ops, cf->args, 2);
}


static char *
ngx_http_req_stat_compile_key(ngx_conf_t *cf, ngx_array_t *flushes,
    ngx_array_t *ops, ngx_array_t *args, ngx_uint_t s)
{
    u_char              *data, *p, ch;
    size_t               i, len;
    ngx_str_t           *value, var;
    ngx_int_t           *flush;
    ngx_uint_t           bracket;
    ngx_http_req_stat_op_t   *op;
    ngx_http_req_stat_var_t  *v;

    value = args->elts;

    for ( /* void */ ; s < args->nelts; s++) {

        i = 0;

        while (i < value[s].len) {

            op = ngx_array_push(ops);
            if (op == NULL) {
                return NGX_CONF_ERROR;
            }

            data = &value[s].data[i];

            if (value[s].data[i] == '$') {

                if (++i == value[s].len) {
                    goto invalid;
                }

                if (value[s].data[i] == '{') {
                    bracket = 1;

                    if (++i == value[s].len) {
                        goto invalid;
                    }

                    var.data = &value[s].data[i];

                } else {
                    bracket = 0;
                    var.data = &value[s].data[i];
                }

                for (var.len = 0; i < value[s].len; i++, var.len++) {
                    ch = value[s].data[i];

                    if (ch == '}' && bracket) {
                        i++;
                        bracket = 0;
                        break;
                    }

                    if ((ch >= 'A' && ch <= 'Z')
                        || (ch >= 'a' && ch <= 'z')
                        || (ch >= '0' && ch <= '9')
                        || ch == '_')
                    {
                        continue;
                    }

                    break;
                }

                if (bracket) {
                    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                       "the closing bracket in \"%V\" "
                                       "variable is missing", &var);
                    return NGX_CONF_ERROR;
                }

                if (var.len == 0) {
                    goto invalid;
                }

                for (v = ngx_http_req_stat_vars; v->name.len; v++) {

                    if (v->name.len == var.len
                        && ngx_strncmp(v->name.data, var.data, var.len) == 0)
                    {
                        op->len = v->len;
                        op->getlen = v->getlen;
                        op->run = v->run;
                        op->data = 0;

                        goto found;
                    }
                }

                if (ngx_http_req_stat_variable_compile(cf, op, &var) != NGX_OK) {
                    return NGX_CONF_ERROR;
                }

                if (flushes) {

                    flush = ngx_array_push(flushes);
                    if (flush == NULL) {
                        return NGX_CONF_ERROR;
                    }

                    *flush = op->data; /* variable index */
                }

            found:

                continue;
            }

            i++;

            while (i < value[s].len && value[s].data[i] != '$') {
                i++;
            }

            len = &value[s].data[i] - data;

            if (len) {

                op->len = len;
                op->getlen = NULL;

                if (len <= sizeof(uintptr_t)) {
                    op->run = ngx_http_req_stat_copy_short;
                    op->data = 0;

                    while (len--) {
                        op->data <<= 8;
                        op->data |= data[len];
                    }

                } else {
                    op->run = ngx_http_req_stat_copy_long;

                    p = ngx_pnalloc(cf->pool, len);
                    if (p == NULL) {
                        return NGX_CONF_ERROR;
                    }

                    ngx_memcpy(p, data, len);
                    op->data = (uintptr_t) p;
                }
            }
        }
    }

    return NGX_CONF_OK;

invalid:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid parameter \"%s\"", data);

    return NGX_CONF_ERROR;
}


static ngx_int_t
ngx_http_req_stat_init(ngx_conf_t *cf)
{
	ngx_str_t                  *value;
	ngx_array_t                 a;
	ngx_http_handler_pt        *h;
	ngx_http_req_stat_key_t         *fmt;
	ngx_http_req_stat_main_conf_t   *lmcf;
	ngx_http_core_main_conf_t  *cmcf;

	lmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_req_stat_module);
	cf->cycle->conf_ctx[ngx_http_req_stat_module.index] = (void***)lmcf;

	char mongo_uri[128];
	memset(mongo_uri,0,sizeof(mongo_uri));
	ngx_sprintf((u_char*)mongo_uri, "%V", &lmcf->mongo_uri);
	memset(lmcf->mongo_db_str, 0, sizeof(lmcf->mongo_db_str));
	ngx_sprintf((u_char*)lmcf->mongo_db_str, "%V", &lmcf->mongo_db);
	
	lmcf->mongoc_uri_inst = mongoc_uri_new(mongo_uri);
	if(lmcf->mongoc_uri_inst == NULL){
		printf("mongo_uri [%s] invalid!\n", mongo_uri);
		return NGX_ERROR;
	}	
	
	lmcf->mongo_pool = mongoc_client_pool_new(lmcf->mongoc_uri_inst);
	if(lmcf->mongo_pool == NULL){
		printf("mongoc_client_pool_new failed!\n");
		return NGX_ERROR;
	}
	//printf("###### mongo pool init ############\n");

    if (lmcf->def_used) {
        if (ngx_array_init(&a, cf->pool, 1, sizeof(ngx_str_t)) != NGX_OK) {
            return NGX_ERROR;
        }

        value = ngx_array_push(&a);
        if (value == NULL) {
            return NGX_ERROR;
        }

        *value = ngx_http_req_stat_def_key_fmt;
        fmt = lmcf->keys.elts;

        if (ngx_http_req_stat_compile_key(cf, NULL, fmt->ops, &a, 0)
            != NGX_CONF_OK)
        {
            return NGX_ERROR;
        }
    }

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_LOG_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_req_stat_handler;

    return NGX_OK;
}
