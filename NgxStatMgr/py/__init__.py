# coding=utf8
import hashlib, urllib2, simplejson as json

from flask import request
from functools import wraps

import settings

# **args navTabId="",rel="",forwardUrl="",callbackType="closeCurrent"
def ConsDwzRsp(status, msg, **args):
    rsp = {
      "statusCode":status, 
      "message": msg, 
      "callbackType":args.get('callbackType', ''),
      "navTabId":args.get('navTabId',''), 
      "rel":args.get('rel',''),
      "forwardUrl":args.get('forwardUrl', '')}
    
    return json.dumps(rsp)