//
//
//  Created by xudexin on 13-4-17.
//
//

#ifndef __CCPomelo__
#define __CCPomelo__

#include "cocos2d.h"

#include "jansson.h"
#include "pomelo.h"
#include<queue>

using namespace cocos2d;

class CCPomeloContent_;
class CCPomeloReponse_;
class CCPomeloEvent_ ;
class CCPomeloNotify_;

class CCPomeloReponse:public cocos2d::CCObject{
public:
    CCPomeloReponse(){}
    ~CCPomeloReponse(){}
    const char* rout;
    int status;
    json_t *docs;
};

class CCPomelo :public cocos2d::CCObject{
public:
    static CCPomelo *getInstance();
    static void destroyInstance();
    
    int connect(const char* addr,int port);
    void stop();

    int request(const char*route,json_t *msg,CCObject* pTarget, SEL_CallFuncND pSelector);
    int notify(const char*route,json_t *msg,CCObject* pTarget, SEL_CallFuncND pSelector);
    int addListener(const char* event,CCObject* pTarget, SEL_CallFuncND pSelector);
    
    void setLogLevel(int logLevel);

public:
    CCPomelo();
    virtual ~CCPomelo();
    
    void dispatchCallbacks(float delta);
    
    
    void lockReponsQeueue();
    void unlockReponsQeueue();
    void lockEventQeueue();
    void unlockEventQeueue();
    void lockNotifyQeueue();
    void unlockNotifyQeueue();
    
    void pushReponse(CCPomeloReponse_*resp);
    void pushEvent(CCPomeloEvent_*ev);
    void pushNotiyf(CCPomeloNotify_*ntf);
    
private:
    void incTaskCount();
    void desTaskCount();
    
    CCPomeloReponse_*popReponse();
    CCPomeloEvent_*popEvent();
    CCPomeloNotify_*popNotify();
    
    std::map<pc_notify_t*,CCPomeloContent_*> notify_content;
    pthread_mutex_t  notify_queue_mutex;
    std::queue<CCPomeloNotify_*> notify_queue;
    
    std::map<std::string,CCPomeloContent_*> event_content;
    pthread_mutex_t  event_queue_mutex;
    std::queue<CCPomeloEvent_*> event_queue;
    
    std::map<pc_request_t *,CCPomeloContent_*> request_content;
    pthread_mutex_t  reponse_queue_mutex;
    std::queue<CCPomeloReponse_*> reponse_queue;
    
    pthread_mutex_t  task_count_mutex;
    void dispatchRequest();
    void dispatchEvent();
    void dispatchNotify();
    pc_client_t *client;
    int task_count;
    int log_level;
};

#endif /* defined(__CCPomelo__) */
