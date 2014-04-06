#ifndef __MSGCENTER_MONGO_POOL_H__
#define __MSGCENTER_MONGO_POOL_H__

#include "SimpleConnPool.h"
#include <assert.h>
#include "json-c/json.h"
#include "mongoc.h"
#include "json/json.h"

void json_to_bson_append_element( bson_t *bb , const char *k , struct json_object *v );


#define json_object_object_foreach2(obj,key,val) \
 char *key; struct json_object *val; struct lh_entry *entry; \
 for(entry = json_object_get_object(obj)->head; (entry ? (key = (char*)entry->k, val = (struct json_object*)entry->v, entry) : 0); entry = entry->next)

void json_to_bson_append( bson_t *bb , struct json_object *o ) {
    json_object_object_foreach2( o,k,v ) {
        json_to_bson_append_element( bb , k , v );
    }
}

void json_to_bson_append_element( bson_t *bb , const char *k , struct json_object *v ) {
	if ( ! v ) {
	bson_append_null( bb , k,-1);
	return;
	}

	json_type type =  json_object_get_type( v ) ;
	switch (type) {
	case json_type_int:
		bson_append_int32( bb , k ,-1, json_object_get_int( v ) );
	break;
	case json_type_boolean:
		bson_append_bool( bb , k ,-1, json_object_get_boolean( v ) );
	break;
	case json_type_double:
		bson_append_double( bb , k ,-1, json_object_get_double( v ) );
	break;
	case json_type_string:
		bson_append_utf8( bb , k , -1, json_object_get_string( v ) , -1);
	break;
	case json_type_object:
	{
		bson_t* child = bson_new();
		bson_append_document_begin(bb, k,  -1, child);
		json_to_bson_append(child, v);
		bson_append_document_end(bb, child);
	}
	break;
	default:
	fprintf( stderr , "can't handle type(%d) for : %s\n" , type, json_object_to_json_string( v ) );
	}
}

static struct json_object* json_tokener_parse2(const char *str, int len)
{
    struct json_tokener* tok;
    struct json_object* obj;

    tok = json_tokener_new();
    if (!tok)
      return NULL;
    obj = json_tokener_parse_ex(tok, str, len);
    
    if(tok->err != json_tokener_success) {
		if (obj != NULL)
			json_object_put(obj);
        obj = NULL;
    }

    json_tokener_free(tok);
    return obj;
}

inline bson_t* json_to_bson(json_object* jso) {	
	bson_t* bso = bson_new();
	bson_init(bso );
	json_to_bson_append(bso ,jso);
    	
	return bso;
}


#endif


