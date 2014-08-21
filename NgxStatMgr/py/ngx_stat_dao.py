# coding=utf8
'''
Created on 2014-05-25

'''
import time
import pymongo
import simplejson
import traceback

tables = {}
tablesContent = {}

def mongo_get_db(HOST,PORT,DB='ngx_stat'):
    conn = pymongo.Connection(HOST, PORT)
    db = conn[DB]
    #print "db:" , dir(db)
    return db

def mongo_get_tables(db):
    names = db.collection_names()
    names = [name for name in names if name != 'system.indexes']
    
    indexes = []
    for name in names:
        #print dir(db[name])
        idx = db[name].index_information()
        if u'date_1_url_1' in idx:
            indexes.append(name) 
    return names,tuple(indexes)
    #for name in names:
    
def mongo_get_all_tables(servers):
    global tables
    global tablesContent
    for key,val in servers.iteritems():
        #print key,'=',val
        try:
            arr = val.split(':')
            HOST = arr[0]
            PORT = int(arr[1])
            db =  mongo_get_db(HOST, PORT)
            names,indexes = mongo_get_tables(db)
            tables[key] = names
            tablesContent[key] = ''.join([u'''<option value='%s'>%s%s</option>''' % (name, name, '' if name in indexes else u'*') for name in names])
        except:
            print "get (key=%s,val=%s) " % (key, val)
            print traceback.print_exc()
        #print tablesContent[key]
    #print tablesContent

#得到所有count 小于10的记录条数 
def mongo_get_lt_10_totals(servers):
    for key,val in servers.iteritems():
        try:
            arr = val.split(':')
            HOST = arr[0]
            PORT = int(arr[1])
            db =  mongo_get_db(HOST, PORT)
            names,indexes = mongo_get_tables(db)
            for name in names:
                total = db[name].find().count()
                total_lt_10 = db[name].find({'count':{'$lt': 10}}).count()
                total_gt_10 = db[name].find({'count':{'$gte': 10}}).count()
                print "%s:%s table=%s all:%d, \t%d < 10 <= %d" % (key,val,name, total,total_lt_10,total_gt_10)
        except:
            print "get (key=%s,val=%s) " % (key, val)
            print traceback.print_exc()

#清除所有count 小于10的记录条数 
def mongo_clr_lt_10_totals(servers):
    for key,val in servers.iteritems():
        try:
            arr = val.split(':')
            HOST = arr[0]
            PORT = int(arr[1])
            db =  mongo_get_db(HOST, PORT)
            names,indexes = mongo_get_tables(db)
            for name in names:
                db[name].remove({'count':{'$lt': 10}})
                print "%s:%s table=%s clean" % (key,val,name)
        except:
            print "get (key=%s,val=%s) " % (key, val)
            print traceback.print_exc()       

def mongo_query_stat(host, table, date,url):
    arr = host.split(':')
    HOST = arr[0]
    PORT = int(arr[1])
    db =  mongo_get_db(HOST, PORT)
    table = db[table]
    query = {}
    if date:
        query['date'] = date
    if url:
        query['url'] = url
    #print "query:", query
    if date:
        cur = table.find(query).sort('count',pymongo.DESCENDING).limit(100)
    else: #没有时间，按时间排序。
        cur = table.find(query).sort('date',pymongo.DESCENDING).limit(100)
    #for item in cur:
    #    print item
    return cur

    
    