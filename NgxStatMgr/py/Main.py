# coding=utf8
'''
Created on 2014-05-28

@author: lxj
'''
from flask import Flask
import os
import logging
from logging import FileHandler
from logging.handlers import TimedRotatingFileHandler
from py import settings
from py.ngx_stat_query import views as views_ngx_stat
from py.ngx_stat_dao import (mongo_get_all_tables,mongo_query_stat)
from py import filter

JINJA2_FILTERS = {
        #'boolean_value': filter.boolean_value,
        #'timestamp2date': filter.timestamp2date,
        'ok_times': filter.ok_times,
        'err_times': filter.err_times,
        'err_detail': filter.err_detail,
        'ok_detail': filter.ok_detail,
        'tcp_ok_times': filter.tcp_ok_times,
        'tcp_err_times': filter.tcp_err_times,
        'tcp_err_detail': filter.tcp_err_detail,
        'tcp_ok_detail': filter.tcp_ok_detail,
        'tfs_ok_times': filter.tfs_ok_times,
        'tfs_err_times': filter.tfs_err_times,
        'tfs_err_detail': filter.tfs_err_detail,
        'tfs_ok_detail': filter.tfs_ok_detail,
        'times_detail': filter.times_detail,
        'req_time_all': filter.req_time_all,
        'req_time_detail':filter.req_time_detail,
        }


def create_app(debug):
    app = Flask(__name__,
        static_folder = "../static",
        template_folder = "../templates")
    app.debug = debug
    app.jinja_env.filters.update(JINJA2_FILTERS)
    app.register_blueprint(views_ngx_stat)
    return app

#logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)
filehandler = TimedRotatingFileHandler(os.path.join(settings.LOG_PATH, settings.LOG_FILE), 'D', 1)
filehandler.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
#filehandler.suffix = '%Y%m%d'
filehandler.setLevel(logging.DEBUG if settings.DEBUG else logging.INFO)
logger = logging.getLogger('')
logger.addHandler(filehandler)




if __name__ == '__main__':
    app = create_app(settings.DEBUG)
    print "listen at  %s:%d" % (settings.SERVER_HOST, settings.SERVER_PORT)
    mongo_get_all_tables(settings.servers)
    #mongo_query_stat('172.16.10.201:27017', 'stat', date='2013-11-07',url=None)
    
    app.run(settings.SERVER_HOST, settings.SERVER_PORT)
