//
//  ProtocolHandler.cpp
//  Ragnarok Battle Online
//
//  Created by xudexin on 13-4-17.
//
//

#include "CCPomelo.h"
#include <errno.h>

class CCPomeloContent_ {
public:
    CCPomeloContent_(){
        pTarget = NULL;
        pSelector = NULL;
    }
    ~CCPomeloContent_(){
        
    }
    CCObject* pTarget;
    SEL_CallFuncND pSelector;
};
class CCPomeloReponse_ {
public:
    CCPomeloReponse_(){
        request = NULL;
        docs = NULL;
    }
    ~CCPomeloReponse_(){
        
    }
    int status;
    pc_request_t *request;
    json_t *docs;
};
class CCPomeloEvent_ {
public:
    CCPomeloEvent_(){
        docs = NULL;
    }
    ~CCPomeloEvent_(){
        
    }
    int status;
    std::string event;
    json_t *docs;
};
class CCPomeloNotify_ {
public:
    CCPomeloNotify_(){
        notify = NULL;
    }
    ~CCPomeloNotify_(){
        
    }
    int status;
    pc_notify_t *notify;
};

class CCPomeloConnect_ {
public:
    CCPomeloConnect_(){
        req = NULL;
    }
    ~CCPomeloConnect_(){
        
    }
    int status;
    pc_connect_t *req;
    CCPomeloContent_ *content;
};

static CCPomelo *s_CCPomelo = NULL; // pointer to singleton

void   cc_pomelo_on_ansync_connect_cb  (pc_connect_t* conn_req, int status){
    //CCLOG("data = %p\n", conn_req->data);
    if (conn_req) {
        pc_connect_req_destroy(conn_req);
    }
    s_CCPomelo->connectCallBack(status);

}

void cc_pomelo_on_notify_cb(pc_notify_t *ntf, int status){
    
    s_CCPomelo->lockNotifyQeueue();
    
    CCPomeloNotify_ *notify = new CCPomeloNotify_;
    notify->notify = ntf;
    notify->status = status;
    
    s_CCPomelo->pushNotiyf(notify);
    
    s_CCPomelo->unlockNotifyQeueue();
}


void cc_pomelo_on_event_cb(pc_client_t *client, const char *event, void *data) {
    
    s_CCPomelo->lockEventQeueue();
    
    CCPomeloEvent_ *ev = new CCPomeloEvent_;
    ev->event = event;
    ev->docs = (json_t *)data;
    json_incref(ev->docs);
    
    s_CCPomelo->pushEvent(ev);
    
    s_CCPomelo->unlockEventQeueue();
    
}

void cc_pomelo_on_request_cb(pc_request_t *request, int status, json_t *docs) {
    
    s_CCPomelo->lockReponsQeueue();
    
    CCPomeloReponse_ *response = new CCPomeloReponse_;
    response->request = request;
    response->status = status;
    response->docs = docs;
    json_incref(docs);
    
    s_CCPomelo->pushReponse(response);
    
    s_CCPomelo->unlockReponsQeueue();
    
}


CCPomelo::CCPomelo(){
    CCDirector::sharedDirector()->getScheduler()->scheduleSelector(schedule_selector(CCPomelo::dispatchCallbacks), this, 0, false);
    CCDirector::sharedDirector()->getScheduler()->pauseTarget(this);
    client = pc_client_new();
    pthread_mutex_init(&reponse_queue_mutex, NULL);
    pthread_mutex_init(&event_queue_mutex, NULL);
    pthread_mutex_init(&notify_queue_mutex, NULL);
    pthread_mutex_init(&task_count_mutex, NULL);
    pthread_mutex_init(&connect_mutex, NULL);
    task_count = 0;
    connect_status = 0;
    connect_content = NULL;
}
CCPomelo::~CCPomelo(){
    CCDirector::sharedDirector()->getScheduler()->unscheduleSelector(schedule_selector(CCPomelo::dispatchCallbacks), s_CCPomelo);
}

void CCPomelo::dispatchRequest(){
    lockReponsQeueue();
    CCPomeloReponse_ *response = popReponse();
    if (response) {
        CCPomeloContent_ * content = NULL;
        if (request_content.find(response->request)!=request_content.end()) {
            content = request_content[response->request];
            request_content.erase(response->request);
        }
        if (content) {
            
            CCObject *pTarget = content->pTarget;
            SEL_CallFuncND pSelector = content->pSelector;
            if (pTarget && pSelector)
            {
                CCPomeloReponse resp;
                resp.status = response->status;
                resp.docs = response->docs;
                (pTarget->*pSelector)((CCNode *)this,&resp);
            }
        }else{
            CCLOG("dispatch response:\r\nlost content");
        }
        json_decref(response->docs);
        json_decref(response->request->msg);
        pc_request_destroy(response->request);
        delete response;
    }
    unlockReponsQeueue();
}
void CCPomelo::dispatchEvent(){
    lockEventQeueue();
    CCPomeloEvent_ *event = popEvent();
    if (event) {
        CCPomeloContent_ * content = NULL;
        if (event_content.find(event->event)!=event_content.end()) {
            content = event_content[event->event];
        }
        if (content) {
            CCObject *pTarget = content->pTarget;
            SEL_CallFuncND pSelector = content->pSelector;
            if (pTarget && pSelector)
            {
                CCPomeloReponse resp;
                resp.status = event->status;
                resp.docs = event->docs;
                (pTarget->*pSelector)((CCNode *)this,&resp);
            }
        }else{
            CCLOG("dispatch event::\r\n lost %s content",event->event.c_str());
        }
        json_decref(event->docs);
        delete event;
    }
    unlockEventQeueue();
}
void CCPomelo::dispatchNotify(){
    lockNotifyQeueue();
    CCPomeloNotify_ *ntf = popNotify();
    if (ntf) {
        CCPomeloContent_ * content = NULL;
        if (notify_content.find(ntf->notify)!=notify_content.end()) {
            content = notify_content[ntf->notify];
            notify_content.erase(ntf->notify);
        }
        if (content) {
            CCObject *pTarget = content->pTarget;
            SEL_CallFuncND pSelector = content->pSelector;
            if (pTarget && pSelector)
            {
                CCPomeloReponse resp;
                resp.status = ntf->status;
                resp.docs = NULL;
                (pTarget->*pSelector)((CCNode *)this,&resp);
            }
        }else{
            CCLOG("dispatch notify:\r\nlost content");
        }
        json_decref(ntf->notify->msg);
        pc_notify_destroy(ntf->notify);
        delete ntf;
    }
    unlockNotifyQeueue();
}

void CCPomelo::connectCallBack(){
    if (connect_content) {
        CCObject *pTarget = connect_content->content->pTarget;
        SEL_CallFuncND pSelector = connect_content->content->pSelector;
        if (pTarget && pSelector)
        {
            CCPomeloReponse resp;
            resp.status = connect_content->status;
            resp.docs = NULL;
            (pTarget->*pSelector)((CCNode *)this,&resp);
        }
        connect_status  = 0;
        desTaskCount();
        delete connect_content;
        connect_content = NULL;
    }
   
}
void CCPomelo::dispatchCallbacks(float delta){
    dispatchNotify();
    dispatchEvent();
    dispatchRequest();
    if (connect_status==1) {
        connectCallBack();
    }
    
    pthread_mutex_lock(&task_count_mutex);
    
    if (task_count==0) {
        CCDirector::sharedDirector()->getScheduler()->pauseTarget(this);
    }
    pthread_mutex_unlock(&task_count_mutex);
    
}

void CCPomelo::destroyInstance()
{
    if (s_CCPomelo) {
      
        s_CCPomelo->release();
    }
}

CCPomelo* CCPomelo::getInstance()
{
    if (s_CCPomelo == NULL) {
        s_CCPomelo = new CCPomelo();
    }
    return s_CCPomelo;
}

int CCPomelo::connect(const char* addr,int port){
    struct sockaddr_in address;
    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(addr);
    
    client = pc_client_new();
   
    int ret = pc_client_connect(client, &address);
    if(ret) {
        CCLOG("pc_client_connect error:%d",errno);
        pc_client_destroy(client);
    }
    return  ret;
}
void CCPomelo::asyncConnect(const char* addr,int port,CCObject* pTarget, SEL_CallFuncND pSelector){
   
    if (!connect_content) {
        connect_status = 0;
        connect_content = new CCPomeloConnect_;
        connect_content->content = new CCPomeloContent_;
        connect_content->content->pTarget = pTarget;
        connect_content->content->pSelector = pSelector;
    }else{
        CCLOG("can not call again before the first connect callback");
        return ;
    }
    
    struct sockaddr_in address;
    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(addr);
    
    client = pc_client_new();
    pc_connect_t* async = pc_connect_req_new(&address);
    int ret = pc_client_connect2(client,async,cc_pomelo_on_ansync_connect_cb);
    if(ret) {
        CCLOG("pc_client_connect2 error:%d",errno);
        pc_client_destroy(client);
        cc_pomelo_on_ansync_connect_cb(NULL,ret);
    }

}


void CCPomelo::stop(){
    if(client){
//      pc_client_stop(client);
        pc_client_destroy(client);
    }
}
void CCPomelo::cleanup(){
    cleanupEventContent();
    cleanupNotifyContent();
    cleanupRequestContent();
    pthread_mutex_lock(&task_count_mutex);
    pthread_mutex_unlock(&task_count_mutex);
}

void CCPomelo::cleanupEventContent(){
    std::map<std::string,CCPomeloContent_*>::iterator iter;
    for (iter = event_content.begin();iter != event_content.end();iter++) {
        CCPomeloContent_ *content = iter->second;
        delete  content;
        pc_remove_listener(client, iter->first.c_str(), cc_pomelo_on_event_cb);
    }
    event_content.clear();
}
void CCPomelo::cleanupNotifyContent(){
    std::map<pc_notify_t*,CCPomeloContent_*>::iterator iter;
    for (iter = notify_content.begin();iter != notify_content.end();iter++) {
        CCPomeloContent_ *content = iter->second;
        delete  content;
    }
    notify_content.clear();
}
void CCPomelo::cleanupRequestContent(){
    std::map<pc_request_t *,CCPomeloContent_*>::iterator iter;
    for (iter = request_content.begin();iter != request_content.end();iter++) {
        CCPomeloContent_ *content = iter->second;
        delete  content;
    }
    request_content.clear();
}

int CCPomelo::request(const char*route,json_t *msg,CCObject* pTarget, SEL_CallFuncND pSelector){
    
    pc_request_t *req   = pc_request_new();
    CCPomeloContent_ *content = new CCPomeloContent_;
    content->pTarget = pTarget;
    content->pSelector = pSelector;
    request_content[req] = content;
    return pc_request(client,req, route, json_deep_copy(msg), cc_pomelo_on_request_cb);
}

int CCPomelo::notify(const char*route,json_t *msg,CCObject* pTarget, SEL_CallFuncND pSelector){
    
    pc_notify_t *notify = pc_notify_new();
    CCPomeloContent_ *content = new CCPomeloContent_;
    content->pTarget = pTarget;
    content->pSelector = pSelector;
    notify_content[notify] = content;
    return pc_notify(client,notify, route, json_deep_copy(msg), cc_pomelo_on_notify_cb);
}
int CCPomelo::addListener(const char* event,CCObject* pTarget, SEL_CallFuncND pSelector){
    CCPomeloContent_ *content = new CCPomeloContent_;
    content->pTarget = pTarget;
    content->pSelector = pSelector;
    if (event_content.find(event)!=event_content.end()) {
        delete  event_content[event];
    }
    event_content[event] = content;
    return pc_add_listener(client, event, cc_pomelo_on_event_cb);
}
void CCPomelo::removeListener(const char *event){
    if (event_content.find(event)!=event_content.end()) {
        delete  event_content[event];
    }
    event_content.erase(event);
    pc_remove_listener(client, event, cc_pomelo_on_event_cb);
}
void CCPomelo::incTaskCount(){
    pthread_mutex_lock(&task_count_mutex);
    task_count++;
    pthread_mutex_unlock(&task_count_mutex);
    CCDirector::sharedDirector()->getScheduler()->resumeTarget(s_CCPomelo);
}
void CCPomelo::desTaskCount(){
    pthread_mutex_lock(&task_count_mutex);
    task_count--;
    pthread_mutex_unlock(&task_count_mutex);
}

void CCPomelo::lockReponsQeueue(){
    pthread_mutex_lock(&reponse_queue_mutex);
}

void CCPomelo::unlockReponsQeueue(){
    pthread_mutex_unlock(&reponse_queue_mutex);
}

void CCPomelo::lockEventQeueue(){
    pthread_mutex_lock(&event_queue_mutex);
}

void CCPomelo::unlockEventQeueue(){
    pthread_mutex_unlock(&event_queue_mutex);
}

void CCPomelo::lockNotifyQeueue(){
    pthread_mutex_lock(&notify_queue_mutex);
}

void CCPomelo::unlockNotifyQeueue(){
    pthread_mutex_unlock(&notify_queue_mutex);
}
void CCPomelo::lockConnectContent(){
    pthread_mutex_unlock(&connect_mutex);
}
void CCPomelo::unlockConnectContent(){
    pthread_mutex_unlock(&connect_mutex);
}

void CCPomelo::pushReponse(CCPomeloReponse_*response){
    reponse_queue.push(response);
    incTaskCount();
}
void CCPomelo::pushEvent(CCPomeloEvent_* event){
    event_queue.push(event);
    incTaskCount();
}
void CCPomelo::pushNotiyf(CCPomeloNotify_*notify){
    notify_queue.push(notify);
    incTaskCount();
}
void CCPomelo::connectCallBack(int status){
    connect_status = 1;
    connect_content->status = status;
    incTaskCount();
}
CCPomeloReponse_*CCPomelo::popReponse(){
    if (reponse_queue.size()>0) {
        CCPomeloReponse_ *response = reponse_queue.front();
        reponse_queue.pop();
        desTaskCount();
        return  response;
    }else{
        return  NULL;
    }
}
CCPomeloEvent_*CCPomelo::popEvent(){
    if (event_queue.size()>0) {
        CCPomeloEvent_ *event = event_queue.front();
        event_queue.pop();
        desTaskCount();
        return  event;
    }else{
        return  NULL;
    }
}
CCPomeloNotify_*CCPomelo::popNotify(){
    if (notify_queue.size()>0) {
        CCPomeloNotify_ *ntf = notify_queue.front();
        notify_queue.pop();
        desTaskCount();
        return  ntf;
    }else{
        return  NULL;
    }
}

