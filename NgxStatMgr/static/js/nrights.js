/**
 * @author nrights.ngx
 */

function dialogAddDone(json){
      DWZ.ajaxDone(json);
      if (json.statusCode == DWZ.statusCode.ok){
            if (json.navTabId){
            	alertMsg.error(json.navTabId);
                navTab.reloadFlag(json.navTabId);
            }
            alertMsg.correct(json.message);            
            setTimeout(function(){$.pdialog.closeCurrent();}, 100);
      }else{
      	alertMsg.error(json.message);
      }
}

