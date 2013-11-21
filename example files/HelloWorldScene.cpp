#include "HelloWorldScene.h"

USING_NS_CC;
#include "CCPomelo.h"

int global_count = 0;


CCScene* HelloWorld::scene()
{
    // 'scene' is an autorelease object
    CCScene *scene = CCScene::create();
    
    // 'layer' is an autorelease object
    HelloWorld *layer = HelloWorld::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !CCLayer::init() )
    {
        return false;
    }
    
    CCSize visibleSize = CCDirector::sharedDirector()->getVisibleSize();
    CCPoint origin = CCDirector::sharedDirector()->getVisibleOrigin();

    /////////////////////////////
    // 2. add a menu item with "X" image, which is clicked to quit the program
    //    you may modify it.

    // add a "close" icon to exit the progress. it's an autorelease object
    CCMenuItemImage *pCloseItem = CCMenuItemImage::create(
                                        "CloseNormal.png",
                                        "CloseSelected.png",
                                        this,
                                        menu_selector(HelloWorld::menuCloseCallback));
    
	pCloseItem->setPosition(ccp(origin.x + visibleSize.width - pCloseItem->getContentSize().width/2 ,
                                origin.y + pCloseItem->getContentSize().height/2));

    // create menu, it's an autorelease object
    CCMenu* pMenu = CCMenu::create(pCloseItem, NULL);
    pMenu->setPosition(CCPointZero);
    this->addChild(pMenu, 1);

    /////////////////////////////
    // 3. add your codes below...

    // add a label shows "Hello World"
    // create and initialize a label
    
    CCLabelTTF* pLabel = CCLabelTTF::create("Hello World", "Arial", 24);
    
    // position the label on the center of the screen
    pLabel->setPosition(ccp(origin.x + visibleSize.width/2,
                            origin.y + visibleSize.height - pLabel->getContentSize().height));

    // add the label as a child to this layer
    this->addChild(pLabel, 1);

    // add "HelloWorld" splash screen"
    CCSprite* pSprite = CCSprite::create("HelloWorld.png");

    // position the sprite on the center of the screen
    pSprite->setPosition(ccp(visibleSize.width/2 + origin.x, visibleSize.height/2 + origin.y));

    // add the sprite as a child to this layer
    this->addChild(pSprite, 0);
    std::string ip = "127.0.0.1";
    int port = 3014;
    
    if (global_count%2==0) {
        global_count++;
        CCPomelo::getInstance()->stop();
        CCPomelo::getInstance()->asyncConnect(ip.c_str(), port,this,  callfuncND_selector(HelloWorld::connectCB));
    }else{
        global_count++;
        CCPomelo::getInstance()->addListener("onChat",this,  callfuncND_selector(HelloWorld::onChat));
        
        const char *route = "chat.chatHandler.send";
        json_t *msg = json_object();
        json_object_set(msg, "content", json_string("hello CCPomelo"));
        json_object_set(msg, "target", json_string("*"));
        CCPomelo::getInstance()->request(route, msg, this,  callfuncND_selector(HelloWorld::sendCB));
    }

    return true;
    
    if(CCPomelo::getInstance()->connect(ip.c_str(), port)) {
        // 这个略非主流。但是是延习libpomelo的 逻辑。by xdx
        CCLOG("connect fiald ");
    }else{
        const char *route = "gate.gateHandler.queryEntry";
        json_t *msg = json_object();
        json_object_set(msg, "uid", json_integer(1));
        CCPomelo::getInstance()->request(route, msg, this,  callfuncND_selector(HelloWorld::queryEntryCB));
    }
    
    return true;
}
void HelloWorld::connectCB(cocos2d::CCNode *node, void *resp){
    CCPomeloReponse*ccpomeloresp = (CCPomeloReponse*)resp;
    CCLOG("connectCB %s",json_dumps(ccpomeloresp->docs,JSON_COMPACT));

    const char *route = "gate.gateHandler.queryEntry";
    json_t *msg = json_object();

    json_object_set(msg, "uid", json_integer(global_count));
    CCPomelo::getInstance()->request(route, msg, this,  callfuncND_selector(HelloWorld::queryEntryCB));
}
void HelloWorld::queryEntryCB(cocos2d::CCNode *node, void *resp){
    CCPomeloReponse*ccpomeloresp = (CCPomeloReponse*)resp;
    CCLOG("queryEntry %s",json_dumps(ccpomeloresp->docs,JSON_COMPACT));
    
    const char *json_string_value(const json_t *string);
    json_int_t json_integer_value(const json_t *integer);
    int code = json_integer_value(json_object_get(ccpomeloresp->docs,"code"));
    if (code == 200) {
        const char* host = json_string_value(json_object_get(ccpomeloresp->docs,"host"));
        int port = json_integer_value(json_object_get(ccpomeloresp->docs,"port"));
        
        CCPomelo::getInstance()->stop();
        if (CCPomelo::getInstance()->connect(host, port)) {
           
        }else{
            std::string route = "connector.entryHandler.enter";
            json_t *msg = json_object();
            json_object_set(msg, "rid", json_string("1"));
            char temp[64]={0};
            sprintf(temp, "username_%d",global_count);
            json_object_set(msg, "username", json_string(temp));
            CCPomelo::getInstance()->request(route.c_str(), msg, this,  callfuncND_selector(HelloWorld::entryCB));
        }
    }
}

void HelloWorld::entryCB(cocos2d::CCNode *node, void *resp){
    CCPomeloReponse*ccpomeloresp = (CCPomeloReponse*)resp;
    CCLOG("entryCB %s",json_dumps(ccpomeloresp->docs,JSON_COMPACT));
    
    CCPomelo::getInstance()->addListener("onChat",this,  callfuncND_selector(HelloWorld::onChat));
    
    const char *route = "chat.chatHandler.send";
    json_t *msg = json_object();
    json_object_set(msg, "content", json_string("hello CCPomelo"));
    json_object_set(msg, "target", json_string("*"));
    CCPomelo::getInstance()->request(route, msg, this,  callfuncND_selector(HelloWorld::sendCB));
}
void HelloWorld::sendCB(cocos2d::CCNode *node, void *resp){
    CCPomeloReponse*ccpomeloresp = (CCPomeloReponse*)resp;
    CCLOG("sendCB %s",json_dumps(ccpomeloresp->docs,JSON_COMPACT));
}

void HelloWorld::onChat(cocos2d::CCNode *node, void *resp){
    CCPomeloReponse*ccpomeloresp = (CCPomeloReponse*)resp;
    CCLOG("onChat %s",json_dumps(ccpomeloresp->docs,JSON_COMPACT));
}

void HelloWorld::menuCloseCallback(CCObject* pSender)
{
    CCPomelo::getInstance()->cleanup();
    CCDirector::sharedDirector()->replaceScene(HelloWorld::scene());
}
