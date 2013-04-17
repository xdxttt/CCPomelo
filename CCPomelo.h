//
//  ProtocolHandler.h
//  Ragnarok Battle Online
//
//  Created by xudexin on 13-4-17.
//
//

#ifndef __Ragnarok_Battle_Online__CCPomelo__
#define __Ragnarok_Battle_Online__CCPomelo__

#include "cocos2d.h"
#include "jansson.h"
#include "pomelo.h"
using namespace cocos2d;

class CCPomeloReponse{
public:
    CCPomeloReponse(){}
    ~CCPomeloReponse(){}
    
    int status;
    json_t *docs;
};


class CCPomelo :public cocos2d::CCObject{
public:
    CCPomelo();
    ~CCPomelo();
    static CCPomelo *getInstance();
    static void destroyInstance();
    
    int connect(const char* addr,int port);
    int request(const char*route,json_t *msg,CCObject* pTarget, SEL_CallFuncND pSelector);
    int notify(const char*route,json_t *msg,CCObject* pTarget, SEL_CallFuncND pSelector);
    int addListener(const char* event,CCObject* pTarget, SEL_CallFuncND pSelector);
    
    void dispatchCallbacks(float delta);

private:
    pc_client_t *client;

};

#endif /* defined(__Ragnarok_Battle_Online__CCPomelo__) */
