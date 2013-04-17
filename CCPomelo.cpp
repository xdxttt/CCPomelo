//
//  ProtocolHandler.cpp
//  Ragnarok Battle Online
//
//  Created by xudexin on 13-4-17.
//
//

#include "CCPomelo.h"
#include<queue>

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
        event = NULL;
        docs = NULL;
    }
    ~CCEvent_(){
        
    }
    int status;
    const char* event;
    json_t *docs;
};


static CCPomelo *s_CCPomelo = NULL; // pointer to singleton


std::map<pc_request_t *,CCRequestContent*> s_request_map;
std::queue<CCReponse_*> s_reponse_queue;
pthread_mutex_t  s_reponse_queue_mutex;


std::map<std::string,CCRequestContent*> s_event_map;
pthread_mutex_t  s_event_queue_mutex;
std::queue<CCEvent_*> s_event_queue;


void cc_pomelo_on_event_cb(pc_client_t *client, const char *event, void *data) {
    json_t *docs = (json_t *)data;
    
    pthread_mutex_lock(&s_event_queue_mutex);
    
    CCEvent_ *ev = new CCEvent_;
    ev->event = event;
    ev->docs = docs;
    json_incref(docs);
    s_event_queue.push(ev);
    
    pthread_mutex_unlock(&s_event_queue_mutex);
    CCDirector::sharedDirector()->getScheduler()->resumeTarget(s_CCPomelo);
}

void cc_pomelo_on_request_cb(pc_request_t *req, int status, json_t *docs) {
    
    pthread_mutex_lock(&s_reponse_queue_mutex);
    
    CCReponse_ *response = new CCReponse_;
    response->req = req;
    response->status = status;
    response->docs = docs;
    json_incref(docs);
    s_reponse_queue.push(response);
    
    pthread_mutex_unlock(&s_reponse_queue_mutex);
    CCDirector::sharedDirector()->getScheduler()->resumeTarget(s_CCPomelo);
}
void CCPomelo::dispatchCallbacks(float delta){
    
    pthread_mutex_lock(&s_reponse_queue_mutex);
    if (s_reponse_queue.size()>0) {
        CCReponse_ *response = s_reponse_queue.front();
        s_reponse_queue.pop();
        
        CCRequestContent * content = s_request_map[response->req];
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
        s_request_map.erase(response->req);
        pc_request_destroy(response->req);
        delete response;
    }
    pthread_mutex_unlock(&s_reponse_queue_mutex);
    
    
    // 
    pthread_mutex_lock(&s_event_queue_mutex);
    if (s_reponse_queue.size()>0) {
        CCEvent_ *event = s_event_queue.front();
        s_event_queue.pop();
        
        CCRequestContent * content = s_event_map[event->event];
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
            CCLog("Lost event content");
        }
        json_decref(event->docs);
        delete event;
    }
    pthread_mutex_unlock(&s_event_queue_mutex);
    
}

CCPomelo::CCPomelo(){
    CCDirector::sharedDirector()->getScheduler()->scheduleSelector(schedule_selector(CCPomelo::dispatchCallbacks), this, 0, false);
    CCDirector::sharedDirector()->getScheduler()->pauseTarget(this);

    client = pc_client_new();
    pthread_mutex_init(&s_reponse_queue_mutex, NULL);
    pthread_mutex_init(&s_event_queue_mutex, NULL);

    
}
CCPomelo::~CCPomelo(){
    
}
void CCPomelo::destroyInstance()
{
    CCDirector::sharedDirector()->getScheduler()->unscheduleSelector(schedule_selector(CCPomelo::dispatchCallbacks), s_CCPomelo);
    s_CCPomelo->release();
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
    s_request_map[req] = content;
    pc_request(client,req, route, msg, cc_pomelo_on_request_cb);
    return 0;
}

int CCPomelo::notify(const char*route,json_t *msg,CCObject* pTarget, SEL_CallFuncND pSelector){
    
    
    //pc_add_listener(client, "onHey", on_hey);
    
    return 0;
}

int CCPomelo::addListener(const char* event,CCObject* pTarget, SEL_CallFuncND pSelector){
    
    CCRequestContent *content = new CCRequestContent;
    content->pTarget = pTarget;
    content->pSelector = pSelector;
    
    s_event_map[event] = content;
    
    return pc_add_listener(client, event, cc_pomelo_on_event_cb);
}
