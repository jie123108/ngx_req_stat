# coding=utf8
'''
Created on 2013-2-28

@author: nic
'''
import time

from flask import Blueprint, request, render_template as render

import settings
from ngx_stat_dao import tables,tablesContent,mongo_query_stat

views = Blueprint('ngxstatquery', __name__,url_prefix=settings.URL_PREFIX)

@views.route('/', methods=['get'])
def home():
    return render('dwz_base.html')

def get_options(tables, selTable):
    arr = []
    if tables:
        for table in tables:
            if selTable == table:
                arr.append('''<option value='%s' selected='selected'>%s</option>''' % (table, table))
            else:
                arr.append('''<option value='%s'>%s</option>''' % (table, table))
        return ''.join(arr)
    else:
        return u'<option value="none">无</option>'

@views.route('/query', methods=['get', 'post'])
#@check_permission('PRIV_NGXSTAT_QUERY')
def main():
    args = request.args if request.method == 'GET' else request.form
    
    serverid = args.get('serverid',settings.servers.keys()[0])
    table = args.get('table', '')
    today = time.strftime('%Y-%m-%d',time.localtime(int(time.time())))
    logdate = args.get('logdate', today)
    url = args.get('url', '')
    if logdate == 'no':
        logdate = ''
    
    defTables = None
    if serverid:
        defTables = tables[serverid]
    #print "defTables:", defTables
    record_cnt = 0
    
    errmsg = None
    stats_cur = None
    while 1:
        if logdate is None and url is None:
            #errmsg = u"查询错误，日期或URL必须填写一项"
            break
        
        host = settings.servers[serverid]
        #print "serverid:", serverid
        #print "host:", host
        #print "table:", table
        if host and table:
            stats_cur = mongo_query_stat(host, table, logdate, url)
            record_cnt = stats_cur.count()
        #print "records:", record_cnt
        break
    
    server_names = sorted(settings.servers.keys())
    tcpstat = table and table.startswith('tcp_')
    return render('query.html', servers=settings.servers,server_names=server_names,
                  defTables=get_options(defTables,table),tablesContent=tablesContent,
                  logdate=logdate,url=url,record_cnt=record_cnt,records=stats_cur,
                  errmsg=errmsg,serverid=serverid,table=table,tcpstat=tcpstat)
    #return render('list_types.html', msg_types=msg_type_list)
