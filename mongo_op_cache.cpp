#include <map>
#include <string>
#include <stdio.h>
#include <assert.h>
//#include "json.h"
#include "mongo_op_cache.h"
#include "json-c/json_object_private.h"
using namespace std;

/**
 * key=update key
 * value=update values
 **/
typedef map<string,json_object*> stat_map;

/**
 * 两个stat_map,当进行刷新使用，对换其中的两个。避免冲突
 * 程序运行过程中，总是一个在添加使用，另外一个在持久化到数据库
 **/
typedef struct stat_map_pair_t{
	stat_map_pair_t():firstUsed(true){}
	
	bool firstUsed; //true: frist正在使用，false second正在使用
	stat_map first;   
	stat_map second; 
	//获取正在使用的map.
	stat_map* getUsed(){return firstUsed?&this->first:&this->second;}
	//切换正在使用的map.
	void switchUsed(){ this->firstUsed = !this->firstUsed;}
}stat_map_pair_t;

 
/**
 key=collname("db.table")
 value=map
 **/
typedef map<string, stat_map_pair_t*> req_stat_cache;

stat_cache* req_stat_cache_new()
{
	req_stat_cache* cache = new req_stat_cache();
	return cache;
}

void req_stat_cache_delete(stat_cache* c)
{
	if(c == NULL){
		return ;
	}
	req_stat_cache* cache = (req_stat_cache*)c;
	req_stat_cache::iterator it;
	stat_map_pair_t* map_pair;
	
	for(it=cache->begin();it!=cache->end();it++){
		map_pair = it->second;
		if(map_pair==NULL){
			continue;
		}
	
		stat_map::iterator sit;
		stat_map* stats;
		stats = &map_pair->first;	
		for(sit=stats->begin();sit!=stats->end();sit++){
			json_object* jso = sit->second;
			if(jso != NULL){				
				json_object_put(jso);
				sit->second = NULL;
			}
		}
		stats->clear();
		
		stats = &map_pair->second;	
		for(sit=stats->begin();sit!=stats->end();sit++){
			json_object* jso = sit->second;
			if(jso != NULL){				
				json_object_put(jso);
				sit->second = NULL;
			}
		}
		stats->clear();
		delete map_pair;
	}
	delete cache;

}

/**
 * 将jso_src合并到jso_dst中
 * 合并规则：
 * jso_src中存在但jso_dst中不存在的属性，直接将将该属性复制到jso_dst中。
 * jso_src中存在，jso_dst中也存在的属性：
 * 元素的值是数字的，对值进行相加(包括double,int,long)
 * 元素值是字符串的，忽略。
 * 元素值是对象的，循环应用上面的规则处理。
 */
int json_object_merge(struct json_object* jso_dst, struct json_object* jso_src, const char* jso_name=NULL){
	json_type type_src = json_object_get_type(jso_src);
	json_type type_dst = json_object_get_type(jso_dst);
	
	if(type_src != type_dst){
		printf("jso_name [%s] src type [%d] != dst type [%d]\n", 
						jso_name==NULL?"":jso_name, type_src, type_dst);
		//可选的操作，删除dst中的元素，再添加新的。
		return -1;
	}
	
	switch(type_src){
	case json_type_double:
		jso_dst->o.c_double += json_object_get_double(jso_src);
	break;
	case json_type_int:
		jso_dst->o.c_int64 += json_object_get_int64(jso_src);
	break;
	case json_type_object:
	{	json_object_object_foreach(jso_src,k,v ) {
	        json_object* jso_sub = json_object_object_get(jso_dst, k);
	        if(jso_sub == NULL || json_object_get_type(jso_sub) == json_type_null){//
				json_object_object_add(jso_dst, k, json_object_get(v));
	        }else{
	        	//printf("%s jso_dst:%d\n", k, json_object_get_type(jso_sub));
				json_object_merge(jso_sub, v, k);
	        }
	    }
	    //printf("################################\n");
	}
	break;
	default://不处理。
	  	printf("Modifier $inc allowed for numbers only:%s=%d\n", jso_name?jso_name:"",type_src);
		return -1;
	}

	return 0;
}


int req_stat_cache_flush(stat_cache* c,flush_callback_f flush_callback, void* ctx){
	req_stat_cache* cache = (req_stat_cache*)c;
	req_stat_cache::iterator it;
	stat_map_pair_t* map_pair;
	
	for(it=cache->begin();it!=cache->end();it++){
		string collname = it->first;
		map_pair = it->second;
		if(map_pair==NULL){
			map_pair = new stat_map_pair_t();
			continue;
		}

		stat_map* stats = map_pair->getUsed();
		map_pair->switchUsed(); //切换使用的map.
		
		stat_map::iterator sit;
		for(sit=stats->begin();sit!=stats->end();sit++){
			string key = sit->first;
			json_object* jso = sit->second;
			if(jso != NULL){
				flush_callback(ctx, collname.c_str(),key.c_str(),jso);
				json_object_put(jso);
				sit->second = NULL;
			}
		}
		stats->clear();
		
	}

	return 0;
}

int req_stat_cache_add(stat_cache* c,const char*collname, u_char* key, size_t key_len,
							json_object* jso)
{
	assert(c != NULL);
	req_stat_cache* cache = (req_stat_cache*)c;
	int ret = 0;
	
	string scollname = string(collname);
	string skey((const char*)key, key_len);
	req_stat_cache::iterator it = cache->find(scollname);

	stat_map_pair_t* map_pair;
	stat_map* json_map = NULL;
	//printf("add db.%s.update(%s,%s,true)\n",collname,skey.c_str(),json_object_to_json_string(jso));

	if(it == cache->end()){//没找到。
		map_pair = new stat_map_pair_t();
		json_map = map_pair->getUsed();
		json_map->insert(make_pair(skey, json_object_get(jso)));
		//printf("add collname map [%s]\n", scollname.c_str());
		//printf("add jso [%s]=%s\n", skey.c_str(), json_object_to_json_string(jso));
		cache->insert(make_pair(scollname, map_pair));
	}else{
		map_pair = it->second;
		json_map = map_pair->getUsed();
		
		stat_map::iterator xit;
		xit = json_map->find(skey);
		if(xit == json_map->end()){//没找到对应的key. 添加	
			json_map->insert(make_pair(skey, json_object_get(jso)));
		}else{
			if(xit->second == NULL){
				xit->second = json_object_get(jso);
			}else{
				json_object* jso_exist = xit->second;
				//printf("############## merge element ##########\n");
				//printf("jso_new: %s\n", json_object_to_json_string(jso));
				//printf("jso_exist: %s\n", json_object_to_json_string(jso_exist));
				ret = json_object_merge(jso_exist, jso,NULL);
				if(ret != 0){
					printf("###### json_object_merge failed!\n");
					return -1;
				}
			}
		}
	}

	return 0;
}

#ifdef CACHE_TEST

void print_callback(void* ctx, const char* collname,const char* key, json_object* jso)
{
	printf("db.%s.update(%s,%s,true);\n",collname,key,json_object_to_json_string(jso));
}
			
int main(int argc, char* argv[])
{

	stat_cache* cache = req_stat_cache_new();
	
	int ret;
	/**
	const char* str = "{'$inc': {'count':1,'count_cli.tt':1,'hour_cnt.11':1, 'status.400':1,'req_time.all': 33.44}}";
	int len = strlen(str);
	json_object* jso = json_tokener_parse(str);
	if(is_error(jso)){
		printf("parse error!\n");
	} **/

#define s1 "{'$inc': {'count':1,'count_cli.tt':1,'hour_cnt.11':1, 'status.500':1,'req_time.all': 11.44}}"
#define s2 "{'$inc': {'count':2,'count_cli.radar':1,'hour_cnt.11':1, 'status.200':1,'req_time.all': 11.11}}"
#define s3 "{'$inc': {'count':1,'count_cli.tt':1,'hour_cnt.12':1, 'status.200':1,'req_time.all': 11.11}}"
#define s4 "{'$inc': {'count':1,'count_cli.tt':1,'hour_cnt.11':1, 'status.500':1,'req_time.all': 11.11}}"

#define NS_CNT 3
#define DATE_CNT 4
#define URL_CNT 2
#define STAT_CNT 4

	const char* stats[STAT_CNT] = {s1,s2,s3,s4};
	const char* collnames[NS_CNT] = {"ngx_stat.thumb", "ngx_stat.tfs", "ngx_stat.cloud"};
	const char* dates[DATE_CNT] = {"2013-11-25", "2013-11-26", "2013-11-27","2013-11-28"};
#define url1 "/test/login"
#define url2 "/web/post"

	const char* urls[URL_CNT] = {url1,url2};
	
	for(int n=0;n<NS_CNT;n++){
		for(int d=0;d<DATE_CNT;d++){
			for(int u=0;u<URL_CNT;u++){
				char key[256];
				memset(key,0,sizeof(key));
				sprintf(key,"{'date':'%s','url':'%s'}", dates[d], urls[u]);
				for(int s=0;s<STAT_CNT;s++){
					const char* stat = stats[s];
					json_object* jso = json_tokener_parse(stat);		
					req_stat_cache_add(cache, collnames[n], (u_char*)key, strlen(key),jso);
					json_object_put(jso);
				}
						
			}
		}
	}

	req_stat_cache_flush(cache, &print_callback,NULL);
	printf("##########################################\n");
	for(int n=0;n<NS_CNT;n++){
		for(int d=0;d<DATE_CNT;d++){
			for(int u=0;u<URL_CNT;u++){
				char key[256];
				memset(key,0,sizeof(key));
				sprintf(key,"{'date':'%s','url':'%s'}", dates[d], urls[u]);
				for(int s=0;s<STAT_CNT;s++){
					const char* stat = stats[s];
					json_object* jso = json_tokener_parse(stat);		
					req_stat_cache_add(cache, collnames[n], (u_char*)key, strlen(key),jso);
					json_object_put(jso);
				}
						
			}
		}
	}

	req_stat_cache_flush(cache, &print_callback,NULL);

	printf("##########################################\n");
	for(int n=0;n<NS_CNT;n++){
		for(int d=0;d<DATE_CNT;d++){
			for(int u=0;u<URL_CNT;u++){
				char key[256];
				memset(key,0,sizeof(key));
				sprintf(key,"{'date':'%s','url':'%s'}", dates[d], urls[u]);
				for(int s=0;s<STAT_CNT;s++){
					const char* stat = stats[s];
					json_object* jso = json_tokener_parse(stat);		
					req_stat_cache_add(cache, collnames[n], (u_char*)key, strlen(key),jso);
					json_object_put(jso);
				}
						
			}
		}
	}

	req_stat_cache_flush(cache, &print_callback,NULL);

	req_stat_cache_delete(cache);
	

	#if 0
	for(int i=0;i< STAT_CNT;i++){
		json_object* jso1 = json_tokener_parse(stats[i]);
		int ret;
		ret = json_object_merge(jso, jso1);
		if(ret != 0){
			printf("merge error!\n");
			exit(1);
		}
		//JSON_C_TO_STRING_PRETTY
		const char* js = json_object_to_json_string(jso);
		printf("js:%s\n", js);
		json_object_put(jso1);
	}
	#endif
	
	return 0;
}

#endif

