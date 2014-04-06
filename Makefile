
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

LDFLAGS= `pkg-config --libs libmongoc-1.0`
CFLAGS=`pkg-config --cflags libmongoc-1.0` -DMONGO_HAVE_STDINT

all:  json_merge

mongo_test: mongo_test.c mongo_c_pool.h ../BaseLib/ConnPool.h ../BaseLib/ConnPool.c
	gcc -std=c99 -g  $^ -o $@  $(CFLAGS) $(LDFLAGS)
	

json_merge: mongo_op_cache.cpp
	g++ -g $^ -o $@ $(CFLAGS) $(LDFLAGS) -DCACHE_TEST

clean:
	rm -f mongo_test json_merge

