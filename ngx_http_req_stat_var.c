/*************************************************
 * Author: jie123108@163.com
 * Copyright: jie123108
 *************************************************/
#include "ngx_http_req_stat_module.h"


u_char *
ngx_http_req_stat_msec(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
    ngx_time_t  *tp;

    tp = ngx_timeofday();

    return ngx_sprintf(buf, "%T.%03M", tp->sec, tp->msec);
}

int ngx_http_req_stat_uri_parse(ngx_http_request_t *r,size_t* begin, size_t* end)
{
	size_t len = 0;
	size_t url_pos = 0;
	int parse_step = 0;
	int for_break = 0;
	for(len=0;len<r->request_line.len;len++){
		switch(parse_step){
		case 0://解析method
			if(r->request_line.data[len] == ' '){
				parse_step++;
			}
		break;
		case 1://解析url
		if(r->request_line.data[len] != ' ')
		{
			if(url_pos == 0){
				url_pos = len;
				parse_step++;
			}
		}
		break;
		case 2:
		if(r->request_line.data[len] == '?' 
			|| r->request_line.data[len] == ' '
			|| r->request_line.data[len] == '\t'){
			parse_step++;
			for_break = 1;
			break;
		}
		break;
		}
		if(for_break) break;
	}

	if(url_pos == 0){
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                           "invalid uri \"%V\" not found url!", &r->request_line);
		return -1;
	}
	*begin = url_pos;
	*end = len;

	return 0;
}

size_t ngx_http_req_stat_uri_full_getlen(ngx_http_request_t *r, uintptr_t data)
{
	if(r->request_line.len == 0){
		return 1;
	}	
	size_t begin=0;
	size_t end = 0;
	int ret = ngx_http_req_stat_uri_parse(r, &begin, &end);
	if(ret == -1){
		return 1;
	}	
	return end-begin;
}

u_char *
ngx_http_req_stat_uri_full(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
	if(r->request_line.len == 0){
		return ngx_sprintf(buf, "-");
	}
	
	size_t begin=0;
	size_t end = 0;
	int ret = ngx_http_req_stat_uri_parse(r, &begin, &end);
	if(ret == -1){
		return ngx_sprintf(buf, "-");
	}	


	ngx_str_t tmp = {end-begin, r->request_line.data+begin};
    return ngx_sprintf(buf, "%V", &tmp);
}

#if 0
uint8_t valid_value[256] =
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

typedef enum {
	NORESP,
	NOJSON,
	OK_NOTFOUND,
	OK_VALUE_INVALID,
	OK_TRUE,
	OK_FALSE,
	OK_CNT
}JSON_STATUS;

const char* get_str_status(JSON_STATUS status){
	switch(status){
	case NORESP: return "noresp";
	case NOJSON: return "nojson";
	case OK_NOTFOUND: return "ok_notfound";
	case OK_VALUE_INVALID: return "ok_invalid";
	case OK_TRUE: return "ok_true";
	case OK_FALSE: return "ok_false";
	case OK_CNT:
	default:
		return "ok_err";
	}
}

int get_str_status_len(JSON_STATUS status){
	switch(status){
	case NORESP: return sizeof("noresp")-1;
	case NOJSON: return sizeof("nojson")-1;
	case OK_NOTFOUND: return sizeof("ok_notfound")-1;
	case OK_VALUE_INVALID: return sizeof("ok_invalid")-1;
	case OK_TRUE: return sizeof("ok_true")-1;
	case OK_FALSE: return sizeof("ok_false")-1;
	case OK_CNT:
	default:
		return sizeof("ok_err")-1;
	}
}

JSON_STATUS ngx_http_req_stat_find_ok_status(const u_char* json, size_t len)
{
	printf("resp: %.*s\n", (int)len, json);
#define IS_YH(c) (c=='"'||c=='\'')
	unsigned i;
	int step = 0;
	int stop = 0;
	int begin = 0;
	JSON_STATUS status = OK_NOTFOUND; 
	
	for(i=0;i<len; i++){
		switch(step){
		case 0: //find "ok"
		if(i<len-4 && IS_YH(json[i]) && json[++i] == 'o' 
			&& json[++i]=='k' && IS_YH(json[++i]))
		{
			step++;
		}
		break;
		case 1:
		if(valid_value[(uint8_t)json[i]]){//find f or t.
			begin = i;
			step++;
		}
		break;
		case 2:
		if(!valid_value[(uint8_t)json[i]]){
			step++;
			stop = 1;			
			//printf("ok_val: %.*s\n", i-begin, json+begin);
			if(i - begin == 4 && strncasecmp((const char*)json+begin, "true", 4)==0){
				status = OK_TRUE;
			}else if(i-begin == 5 && strncasecmp((const char*)json+begin, "false", 5)==0){
				status = OK_FALSE;
			}else{
				status = OK_VALUE_INVALID;
			}
		}
		break;
		}
		if(stop) break;
	}

	return status;
}

ngx_str_t ngx_http_req_stat_find_reason(const u_char* json, size_t len)
{
#define IS_YH(c) (c=='"'||c=='\'')
	unsigned i;
	int step = 0;
	int stop = 0;
	int begin = 0;
	ngx_str_t reason = ngx_string("-");
	
	for(i=0;i<len; i++){
		switch(step){
		case 0: //find "reason"
		if(i<len-8 && IS_YH(json[i]) 
			&&json[++i] == 'r' && json[++i]=='e' && json[++i]=='a' 
			&&json[++i] == 's' && json[++i]=='o' && json[++i]=='n'
			&&IS_YH(json[++i]))
		{
			step++;
		}
		break;
		case 1://查找到reason 值的左边引号
		if(i<len-1 && IS_YH(json[i]) ){//find "
			begin = i+1;
			step++;
		}
		break;
		case 2://查找到reason值的右边引号
		if(IS_YH(json[i])){
			step++;
			stop = 1;
			if(i-begin > 0){
				reason.len = i-begin;
				reason.data = (u_char*)json+begin;
			}
		}
		break;
		}
		if(stop) break;
	}

	return reason;
}

inline JSON_STATUS ngx_http_req_stat_get_json_status(ngx_http_request_t *r)
{
	if(r->out != NULL && r->out->buf != NULL){
		ngx_buf_t* buf = r->out->buf;
		if(ngx_buf_size(buf)> 5 && ngx_buf_in_memory_only(buf)){
			return ngx_http_req_stat_find_ok_status(buf->pos, buf->end-buf->pos);
		}else{
			//printf("#######ngx_buf_size(buf)> 5 && ngx_buf_in_memory_only(buf)###\n");
			return NOJSON;
		}
	}else{
		//printf("r->out != NULL && r->out->buf != NULL\n");
		return NORESP;
	}
}

size_t ngx_http_req_stat_json_status_getlen(ngx_http_request_t *r, uintptr_t data)
{
	if(r->request_line.len == 0){
		return 1;
	}	
	JSON_STATUS status = ngx_http_req_stat_get_json_status(r);
	
	return get_str_status_len(status);
}

u_char *
ngx_http_req_stat_json_status(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
	if(r->request_line.len == 0){
		return ngx_sprintf(buf, "-");
	}

	JSON_STATUS status = ngx_http_req_stat_get_json_status(r);
	
    return ngx_sprintf(buf, "%s", get_str_status(status));
}

inline ngx_str_t ngx_http_req_stat_get_json_reason(ngx_http_request_t *r)
{
	static ngx_str_t empty_reason = ngx_string("-");
	
	if(r->out != NULL && r->out->buf != NULL && ngx_buf_size(r->out->buf)> 8){
		ngx_buf_t* buf = r->out->buf;
		if(ngx_buf_in_memory_only(buf)){
			return ngx_http_req_stat_find_reason(buf->pos, buf->end-buf->pos);
		}else{
			return empty_reason;
		}
	}else{
		return empty_reason;
	}
}

size_t ngx_http_req_stat_json_reason_getlen(ngx_http_request_t *r, uintptr_t data)
{
	if(r->request_line.len == 0){
		return 1;
	}	
	ngx_str_t reason = ngx_http_req_stat_get_json_reason(r);
	
	return reason.len;
}

u_char *
ngx_http_req_stat_json_reason(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
	if(r->request_line.len == 0){
		return ngx_sprintf(buf, "-");
	}

	ngx_str_t reason = ngx_http_req_stat_get_json_reason(r);
	
    return ngx_sprintf(buf, "%V", &reason);
}
#endif

u_char *
ngx_http_req_stat_request_time(ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op)
{
    ngx_time_t      *tp;
    ngx_msec_int_t   ms;

    tp = ngx_timeofday();

    ms = (ngx_msec_int_t)
             ((tp->sec - r->start_sec) * 1000 + (tp->msec - r->start_msec));
    ms = ngx_max(ms, 0);

    return ngx_sprintf(buf, "%T.%04M", ms / 1000, ms % 1000);
}


u_char *
ngx_http_req_stat_status(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
    ngx_uint_t  status;

    if (r->err_status) {
        status = r->err_status;

    } else if (r->headers_out.status) {
        status = r->headers_out.status;

    } else if (r->http_version == NGX_HTTP_VERSION_9) {
        *buf++ = '0';
        *buf++ = '0';
        *buf++ = '9';
        return buf;

    } else {
        status = 0;
    }

    return ngx_sprintf(buf, "%ui", status);
}

u_char* ngx_http_req_stat_date(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
	u_char* p = ngx_cpymem(buf, ngx_cached_http_log_iso8601.data, 10);
	//buf[4] = '-';
	//buf[7] = '-';
	
    return p;
}


u_char* ngx_http_req_stat_time(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
	u_char* p = ngx_cpymem(buf, ngx_cached_err_log_time.data+11, 8);
    return p;
}


u_char* ngx_http_req_stat_year(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
	u_char* p = ngx_cpymem(buf, ngx_cached_err_log_time.data, 4);
    return p;
}

u_char* ngx_http_req_stat_month(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
	u_char* p = ngx_cpymem(buf, ngx_cached_err_log_time.data+5, 2);
    return p;
}

u_char* ngx_http_req_stat_day(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
	u_char* p = ngx_cpymem(buf, ngx_cached_err_log_time.data+8, 2);
    return p;
}

u_char* ngx_http_req_stat_hour(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
	u_char* p = ngx_cpymem(buf, ngx_cached_err_log_time.data+11, 2);
    return p;
}

u_char* ngx_http_req_stat_minute(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
	u_char* p = ngx_cpymem(buf, ngx_cached_err_log_time.data+14, 2);
    return p;
}

u_char* ngx_http_req_stat_second(ngx_http_request_t *r, u_char *buf, ngx_http_req_stat_op_t *op)
{
	u_char* p = ngx_cpymem(buf, ngx_cached_err_log_time.data+17, 2);
    return p;
}



u_char *
ngx_http_req_stat_bytes_sent(ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op)
{
    return ngx_sprintf(buf, "%O", r->connection->sent);
}


/*
 * although there is a real $body_bytes_sent variable,
 * this log operation code function is more optimized for logging
 */

u_char *
ngx_http_req_stat_body_bytes_sent(ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op)
{
    off_t  length;

    length = r->connection->sent - r->header_size;

    if (length > 0) {
        return ngx_sprintf(buf, "%O", length);
    }

    *buf = '0';

    return buf + 1;
}


u_char *
ngx_http_req_stat_request_length(ngx_http_request_t *r, u_char *buf,
    ngx_http_req_stat_op_t *op)
{
    return ngx_sprintf(buf, "%O", r->request_length);
}


