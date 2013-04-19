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

class CCRequestContent;
class CCReponse_;
class CCEvent_ ;
class CCNotiyf_;

class CCPomeloReponse{
public:
    CCPomeloReponse(){}
    ~CCPomeloReponse(){}
    int status;
    json_t *docs;
};

class CCPomelo :public cocos2d::CCObject{
public:
    static CCPomelo *getInstance();
    static void destroyInstance();
    
    int connect(const char* addr,int port);
    int request(const char*route,json_t *msg,CCObject* pTarget, SEL_CallFuncND pSelector);
    int notify(const char*route,json_t *msg,CCObject* pTarget, SEL_CallFuncND pSelector);
    int addListener(const char* event,CCObject* pTarget, SEL_CallFuncND pSelector);
 
    
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
    
    void pushReponse(CCReponse_*resp);
    void pushEvent(CCEvent_*ev);
    void pushNotiyf(CCNotiyf_*ntf);

    
private:
    void incTaskCount();
    void desTaskCount();
    
    CCReponse_*popReponse();
    CCEvent_*popEvent();
    CCNotiyf_*popNotify();
    
    std::map<pc_notify_t*,CCRequestContent*> notify_content;
    pthread_mutex_t  notify_queue_mutex;
    std::queue<CCNotiyf_*> notify_queue;

    std::map<std::string,CCRequestContent*> event_content;
    pthread_mutex_t  event_queue_mutex;
    std::queue<CCEvent_*> event_queue;

    std::map<pc_request_t *,CCRequestContent*> request_content;
    pthread_mutex_t  reponse_queue_mutex;
    std::queue<CCReponse_*> reponse_queue;

    pthread_mutex_t  task_count_mutex;
    void dispatchRequest();
    void dispatchEvent();
    void dispatchNotify();
    pc_client_t *client;
    int task_count;
};

#endif /* defined(__CCPomelo__) */
