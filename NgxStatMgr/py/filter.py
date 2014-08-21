#coding=utf8
import datetime
import base64
import traceback
#import traceback

def boolean_value(content):
    if type(content) is int:
        if content > 0:
            return u'是'
        return u'否'
    return ''

def timestamp2date(t, fmt='%Y-%m-%d %H:%M:%S'):
    if type(t) is int:
        _t = datetime.datetime.fromtimestamp(t)
        return _t.strftime(fmt)
    return t

def ok_times(row):
    status = row['status']
    count = row['count']
    if status:
        ok_count = 0
        for key,value in status.iteritems():
            try:
                ikey = int(key)
                if 400 >= ikey >=200:
                    ok_count += value
            except:
                pass
        if count > 0 and ok_count > 0:
            percent = ok_count*100.0/count
            if percent >= 99.0:
                return "%d (%.2f%%)" % (ok_count, percent)
            else:
                return "<font color='red'>%d (%.2f%%)</font>" % (ok_count, percent)
        else:
            return "<font color='red'>%d</font>" % ok_count
    else:
        return 0

def err_times(row):
    status = row['status']
    if status:
        err_count = 0
        for key,value in status.iteritems():
            try:
                ikey = int(key)
                if not 400 >= ikey >=200:
                    err_count += value
            except:
                pass
        return err_count
    else:
        return 0

def ok_detail(row):
    status = row['status']
    if status:
        arr = []
        for key,value in status.iteritems():
            ikey = int(key)
            if 400 >= ikey >=200:
                arr.append(u'%s: %d' % (key,value))
        return '\n'.join(arr)
    else:
        return ''
    
def err_detail(row):
    status = row['status']
    if status:
        arr = []
        for key,value in status.iteritems():
            ikey = int(key)
            if not 400 >= ikey >=200:
                arr.append(u'%s: %d' % (key,value))
        return '\n'.join(arr)
    else:
        return ''

############### tcp status #############
def tcp_ok_times(row):
    status = row['status']
    count = row['count']
    if status:
        ok_count = 0
        for key,value in status.iteritems():
            try:
                ikey = int(key)
                if ikey == 0:
                    ok_count += value
            except:
                pass
        if count > 0 and ok_count > 0:
            percent = ok_count*100.0/count
            if percent >= 99.0:
                return "%d (%.2f%%)" % (ok_count, percent)
            else:
                return "<font color='red'>%d (%.2f%%)</font>" % (ok_count, percent)
        else:
            return "<font color='red'>%d</font>" % ok_count
    else:
        return 0

def tcp_err_times(row):
    status = row['status']
    if status:
        err_count = 0
        for key,value in status.iteritems():
            try:
                ikey = int(key)
                if not ikey == 0:
                    err_count += value
            except:
                pass
        return err_count
    else:
        return 0

def tcp_ok_detail(row):
    status = row['status']
    if status:
        arr = []
        for key,value in status.iteritems():
            ikey = int(key)
            if ikey == 0:
                arr.append(u'%s: %d' % (key,value))
        return '\n'.join(arr)
    else:
        return ''
    
def tcp_err_detail(row):
    status = row['status']
    if status:
        arr = []
        for key,value in status.iteritems():
            ikey = int(key)
            if not ikey == 0:
                arr.append(u'%s: %d' % (key,value))
        return '\n'.join(arr)
    else:
        return ''
    
################tfs status####################3
def tfs_ok_times(row):
    status = row['status']
    count = row['count']
    if status:
        ok_count = 0
        for key,value in status.iteritems():
            try:
                ikey = int(key)
                if 400 >= ikey >=200 or ikey == 404:
                    ok_count += value
            except:
                pass
        if count > 0 and ok_count > 0:
            percent = ok_count*100.0/count
            if percent >= 99.0:
                return "%d (%.2f%%)" % (ok_count, percent)
            else:
                return "<font color='red'>%d (%.2f%%)</font>" % (ok_count, percent)
        else:
            return "<font color='red'>%d</font>" % ok_count
    else:
        return 0

def tfs_err_times(row):
    status = row['status']
    if status:
        err_count = 0
        for key,value in status.iteritems():
            try:
                ikey = int(key)
                if not 400 >= ikey >=200 and ikey != 404:
                    err_count += value
            except:
                pass
        return err_count
    else:
        return 0

def tfs_ok_detail(row):
    status = row['status']
    if status:
        arr = []
        for key,value in status.iteritems():
            ikey = int(key)
            if 400 >= ikey >=200 or ikey ==404:
                arr.append(u'%s: %d' % (key,value))
        return '\n'.join(arr)
    else:
        return ''
    
def tfs_err_detail(row):
    status = row['status']
    if status:
        arr = []
        for key,value in status.iteritems():
            ikey = int(key)
            if not 400 >= ikey >=200 and ikey!=404:
                arr.append(u'%s: %d' % (key,value))
        return '\n'.join(arr)
    else:
        return ''
    
def times_detail(row):
    hour_cnt = row['hour_cnt']
    #print "hour_cnt:", hour_cnt
    cnts = []

    for i in xrange(0,24, 2):
        key1 = '%02d' % i
        key2 = '%02d' % (i+1)
        cnt1 = hour_cnt.get(key1, '')
        cnt2 = hour_cnt.get(key2, '')
        if cnt1 or cnt2:
            cnts.append(u'%02d时：%s  %02d时：%s' % (i, cnt1,i+1,cnt2))
    s =  u"\n".join(cnts)
    return s

def req_time_all(row):
    req_time = row['req_time']
    if req_time:
        #count = row['status'].get('200',0)
        #if count:
        #    return "%.6f" % (req_time['200']/count)
        #else:
        #    return '0'
        all = req_time.get('all', 0)
        count = row.get('count',0) if row.get('count',0) > 0 else 1
        return "%.6f" % (all/count)
    else:
        return '0'
    
def req_time_detail(row):
    req_time = row['req_time']
    if req_time:
        hour_cnt = row['hour_cnt']
        if hour_cnt:
            times = []
            for i in xrange(0,24, 2):
                key1 = '%02d' % i
                key2 = '%02d' % (i+1)
                cnt1 = hour_cnt.get(key1, 0)
                if cnt1 < 1:
                    cnt1 = 1
                cnt2 = hour_cnt.get(key2, 0)
                if cnt2 < 1:
                    cnt2 = 1
                t1 = req_time.get(key1,0)/cnt1;
                t2 = req_time.get(key2,0)/cnt2;
                
                if t1 or t1:
                    times.append(u'%02d时：%.6f  %02d时：%.6f' % (i, t1,i+1,t2))
            return '\n'.join(times)
        else:
            return ''
    else:
        return ''


def urlencode(msg):
    s =  base64.urlsafe_b64encode(msg.encode('utf8'))
    return s
