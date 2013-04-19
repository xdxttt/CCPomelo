//
//  ProtocolHandler.cpp
//  Ragnarok Battle Online
//
//  Created by xudexin on 13-4-17.
//
//

#include "CCPomelo.h"

class CCRequestContent {
public:
    CCRequestContent(){
        pTarget = NULL;
        pSelector = NULL;
    }
    CCObject* pTarget;
    SEL_CallFuncND pSelector;
};
class CCReponse_ {
public:
    CCReponse_(){
        req = NULL;
        docs = NULL;
    }
    ~CCReponse_(){
        
    }
    int status;
    pc_request_t *req;
    json_t *docs;
};
class CCEvent_ {
public:
    CCEvent_(){
        docs = NULL;
    }
    ~CCEvent_(){
        
    }
    int status;
    std::string event;
    json_t *docs;
};
class CCNotiyf_ {
public:
    CCNotiyf_(){
        notify = NULL;
    }
    ~CCNotiyf_(){
        
    }
    int status;
    pc_notify_t *notify;
};

static CCPomelo *s_CCPomelo = NULL; // pointer to singleton

void cc_pomelo_on_notify_cb(pc_notify_t *ntf, int status){
    
    s_CCPomelo->lockNotifyQeueue();
    
    CCNotiyf_ *notify = new CCNotiyf_;
    notify->notify = ntf;
    notify->status = status;
    
    s_CCPomelo->pushNotiyf(notify);
    
    s_CCPomelo->unlockNotifyQeueue();
}


void cc_pomelo_on_event_cb(pc_client_t *client, const char *event, void *data) {
    
    s_CCPomelo->lockEventQeueue();
    
    CCEvent_ *ev = new CCEvent_;
    ev->event = event;
    ev->docs = (json_t *)data;
    json_incref(ev->docs);
    
    s_CCPomelo->pushEvent(ev);
    
    s_CCPomelo->unlockEventQeueue();
    
}

void cc_pomelo_on_request_cb(pc_request_t *req, int status, json_t *docs) {
    
    s_CCPomelo->lockReponsQeueue();
    
    CCReponse_ *response = new CCReponse_;
    response->req = req;
    response->status = status;
    response->docs = docs;
    json_incref(docs);
    
    s_CCPomelo->pushReponse(response);
    
    s_CCPomelo->unlockReponsQeueue();
    
}
void CCPomelo::dispatchRequest(){
    lockReponsQeueue();
    CCReponse_ *response = popReponse();
    if (response) {
        CCRequestContent * content = request_content[response->req];
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
            CCLog("Lost request content");
        }
        json_decref(response->docs);
        request_content.erase(response->req);
        pc_request_destroy(response->req);
        delete response;
    }
    unlockReponsQeueue();
}
void CCPomelo::dispatchEvent(){
    lockEventQeueue();
    CCEvent_ *event = popEvent();
    if (event) {
        CCRequestContent * content = event_content[event->event];
        if (content) {
            CCObject *pTarget = content->pTarget;
            SEL_CallFuncND pSelector = content->pSelector;
            if (pTarget && pSelector)
            {
                CCPomeloReponse resp;
                resp.status = 0;
                resp.docs = event->docs;
                (pTarget->*pSelector)((CCNode *)this,&resp);
            }
        }else{
            CCLog("Lost event %s content",event->event.c_str());
        }
        json_decref(event->docs);
        delete event;
    }
    unlockEventQeueue();
}
void CCPomelo::dispatchNotify(){
    lockNotifyQeueue();
    CCNotiyf_ *ntf = popNotify();
    if (ntf) {
        CCRequestContent * content = NULL;
        if (notify_content.find(ntf->notify)!=notify_content.end()) {
            content = notify_content[ntf->notify];
        }
        if (content) {
            CCObject *pTarget = content->pTarget;
            SEL_CallFuncND pSelector = content->pSelector;
            if (pTarget && pSelector)
            {
                CCPomeloReponse resp;
                resp.status = 0;
                resp.docs = NULL;
                (pTarget->*pSelector)((CCNode *)this,&resp);
            }
        }else{
            CCLog("Lost event content");
        }
        notify_content.erase(ntf->notify);
        pc_notify_destroy(ntf->notify);
        delete ntf;
    }
    unlockNotifyQeueue();
}
void CCPomelo::dispatchCallbacks(float delta){
    dispatchNotify();
    dispatchEvent();
    dispatchRequest();
    
    pthread_mutex_lock(&task_count_mutex);
    
    
    if (task_count==0) {
        CCDirector::sharedDirector()->getScheduler()->pauseTarget(this);
    }
    pthread_mutex_unlock(&task_count_mutex);
    
}

CCPomelo::CCPomelo(){
    CCDirector::sharedDirector()->getScheduler()->scheduleSelector(schedule_selector(CCPomelo::dispatchCallbacks), this, 0, false);
    CCDirector::sharedDirector()->getScheduler()->pauseTarget(this);
    client = pc_client_new();
    pthread_mutex_init(&reponse_queue_mutex, NULL);
    pthread_mutex_init(&event_queue_mutex, NULL);
    pthread_mutex_init(&notify_queue_mutex, NULL);
    pthread_mutex_init(&task_count_mutex, NULL);

    
    task_count = 0;
}
CCPomelo::~CCPomelo(){
    CCDirector::sharedDirector()->getScheduler()->unscheduleSelector(schedule_selector(CCPomelo::dispatchCallbacks), s_CCPomelo);
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
    
    int ret = pc_client_connect(client, &address);
    if (ret) {
        pc_client_destroy(client);
    }
    return ret;
}
int CCPomelo::request(const char*route,json_t *msg,CCObject* pTarget, SEL_CallFuncND pSelector){
    pc_request_t *req   = pc_request_new();
    CCRequestContent *content = new CCRequestContent;
    content->pTarget = pTarget;
    content->pSelector = pSelector;
    request_content[req] = content;
    pc_request(client,req, route, msg, cc_pomelo_on_request_cb);
    return 0;
}

int CCPomelo::notify(const char*route,json_t *msg,CCObject* pTarget, SEL_CallFuncND pSelector){
    
    pc_notify_t *notify = pc_notify_new();
    CCRequestContent *content = new CCRequestContent;
    content->pTarget = pTarget;
    content->pSelector = pSelector;
    notify_content[notify] = content;
    pc_notify(client,notify, route, msg, cc_pomelo_on_notify_cb);
    
    return 0;
}

int CCPomelo::addListener(const char* event,CCObject* pTarget, SEL_CallFuncND pSelector){
    
    CCRequestContent *content = new CCRequestContent;
    content->pTarget = pTarget;
    content->pSelector = pSelector;
    
    event_content[event] = content;
    
    return pc_add_listener(client, event, cc_pomelo_on_event_cb);
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
void CCPomelo::pushReponse(CCReponse_*response){
    reponse_queue.push(response);
    incTaskCount();
}
void CCPomelo::pushEvent(CCEvent_* event){
    event_queue.push(event);
    incTaskCount();
}
void CCPomelo::pushNotiyf(CCNotiyf_*notify){
    notify_queue.push(notify);
    incTaskCount();
}
CCReponse_*CCPomelo::popReponse(){
    if (reponse_queue.size()>0) {
        CCReponse_ *response = reponse_queue.front();
        reponse_queue.pop();
        desTaskCount();
        return  response;
    }else{
        return  NULL;
    }
}
CCEvent_*CCPomelo::popEvent(){
    if (event_queue.size()>0) {
        CCEvent_ *event = event_queue.front();
        event_queue.pop();
        desTaskCount();
        return  event;
    }else{
        return  NULL;
    }
}
CCNotiyf_*CCPomelo::popNotify(){
    if (notify_queue.size()>0) {
        CCNotiyf_ *ntf = notify_queue.front();
        notify_queue.pop();
        desTaskCount();
        return  ntf;
    }else{
        return  NULL;
    }
}

