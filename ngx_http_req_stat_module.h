/*************************************************
 * Author: jie123108@163.com
 * Copyright: jie123108
 *************************************************/
#ifndef __NGX_HTTP_REQ_STAT_PUB_H__
#define __NGX_HTTP_REQ_STAT_PUB_H__
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_string.h>


typedef struct ngx_http_req_stat_op_s  ngx_http_req_stat_op_t;

typedef u_char *(*ngx_http_req_stat_op_run_pt) (ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op);

typedef size_t (*ngx_http_req_stat_op_getlen_pt) (ngx_http_request_t *r,
    uintptr_t data);


struct ngx_http_req_stat_op_s {
    size_t                      len;
    ngx_http_req_stat_op_getlen_pt   getlen;
    ngx_http_req_stat_op_run_pt      run;
    uintptr_t                   data;
};


typedef struct {
    ngx_str_t                   name;
    ngx_array_t                *flushes;
    ngx_array_t                *ops;        /* array of ngx_http_req_stat_op_t */
} ngx_http_req_stat_key_t;

typedef struct {
    ngx_str_t                   name;
    ngx_array_t                *flushes;
    ngx_array_t                *ops;        /* array of ngx_http_req_stat_op_t */
} ngx_http_req_stat_value_t;



typedef struct {
    ngx_array_t                *lengths;
    ngx_array_t                *values;
} ngx_http_req_stat_script_t;


typedef struct {
    //ngx_http_req_stat_script_t      *script;
    time_t                      	error_req_stat_time; //出错时间处理。
    char 	table[64];
	//char    ns[256];  // 格式为 db.table
    ngx_http_req_stat_key_t         *key;
	ngx_http_req_stat_value_t		*value;
} ngx_http_req_stat_t;

typedef struct {
    ngx_str_t                   name;
    size_t                      len;
    ngx_http_req_stat_op_run_pt      run;
	ngx_http_req_stat_op_getlen_pt   getlen;
} ngx_http_req_stat_var_t;

u_char *ngx_http_req_stat_uri_full(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op);
size_t ngx_http_req_stat_uri_full_getlen(ngx_http_request_t *r, uintptr_t data);
u_char* ngx_http_req_stat_date(ngx_http_request_t *r, u_char *buf, 
	ngx_http_req_stat_op_t *op);
u_char* ngx_http_req_stat_time(ngx_http_request_t *r, u_char *buf, 
	ngx_http_req_stat_op_t *op);
u_char* ngx_http_req_stat_year(ngx_http_request_t *r, u_char *buf, 
	ngx_http_req_stat_op_t *op);
u_char* ngx_http_req_stat_month(ngx_http_request_t *r, u_char *buf, 
	ngx_http_req_stat_op_t *op);
u_char* ngx_http_req_stat_day(ngx_http_request_t *r, u_char *buf, 
	ngx_http_req_stat_op_t *op);
u_char* ngx_http_req_stat_hour(ngx_http_request_t *r, u_char *buf, 
	ngx_http_req_stat_op_t *op);
u_char* ngx_http_req_stat_minute(ngx_http_request_t *r, u_char *buf, 
	ngx_http_req_stat_op_t *op);
u_char* ngx_http_req_stat_second(ngx_http_request_t *r, u_char *buf, 
	ngx_http_req_stat_op_t *op);
u_char *ngx_http_req_stat_msec(ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op);
u_char *ngx_http_req_stat_request_time(ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op);
u_char *ngx_http_req_stat_status(ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op);
u_char *ngx_http_req_stat_bytes_sent(ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op);
u_char *ngx_http_req_stat_body_bytes_sent(ngx_http_request_t *r,
    u_char *buf, ngx_http_req_stat_op_t *op);
u_char *ngx_http_req_stat_request_length(ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op);

u_char* ngx_http_req_stat_json_status(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op);
size_t ngx_http_req_stat_json_status_getlen(ngx_http_request_t *r, uintptr_t data);
u_char* ngx_http_req_stat_json_reason(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op);
size_t ngx_http_req_stat_json_reason_getlen(ngx_http_request_t *r, uintptr_t data);


#endif
