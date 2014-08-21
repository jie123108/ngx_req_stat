from NgxStatMgrSimple.py.ngx_stat_dao import (mongo_get_lt_10_totals,mongo_clr_lt_10_totals)
from NgxStatMgrSimple.py import settings
import sys


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print "Usage: python %s func" % sys.argv[0]
        print "python %s show   --- show all 'count' < 10 records!" % sys.argv[0]
        print "python %s clean  --- clean all 'count' < 10 records!" % sys.argv[0]
        
        sys.exit(1)
    
    func = sys.argv[1]
    if func == 'show':
        mongo_get_lt_10_totals(settings.servers)
    elif func == 'clean':
        mongo_clr_lt_10_totals(settings.servers)
    else:
        print "func [%s] invalid!" % func