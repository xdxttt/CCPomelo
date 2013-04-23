CCPomelo
========

Libpomelo 的  cocos2d-x 封装类

CCPomelo  v0.0.2


<br>
1 结构上按照 cocos2d-x ext 中的 HttpClient 对象来做的。对于逻辑层来说是线程安全的。


expmale:

<<<<<<< HEAD
*** 省略上文
    
    
    CCPomelo* ccpml = CCPomelo::getInstance();
      if(ccpml->connect(ip, port)) {
           CCLog("fail to connect server.\n");
      }else{
=======

   *** 省略上文
   CCPomelo* ccpml = CCPomelo::getInstance();
    if(ccpml->connect(ip, port)) {
        CCLog("fail to connect server.\n");
    }else{
        // compose request
>>>>>>> 日常提交
        const char *route = "connector.connectorHandler.enter";
        json_t *msg = json_object();
        json_t *str = json_string("hellopemole");
        json_object_set(msg, "account", str);
        json_decref(str);
       
        ccpml->request(route, msg, this,  callfuncND_selector(HelloWorld::requestCB));
       
        ccpml->addListener("onHey",this,  callfuncND_selector(HelloWorld::on_hey));
        
        route = "connector.connectorHandler.notify";
        msg = json_object();
        json_t *json_str = json_string("hello");
        json_object_set(msg, "msg", json_str);
        json_decref(json_str);
        ccpml->notify(route,msg, this, callfuncND_selector(HelloWorld::notifyCB));
    }
<<<<<<< HEAD
    
    
    
*** 省略下文

*** 回调函数
=======
     *** 省略下文
>>>>>>> 日常提交


    void HelloWorld::requestCB(cocos2d::CCNode *node, void *resp){
        CCPomeloReponse*ccpomeloresp = (CCPomeloReponse*)resp;
    }
<<<<<<< HEAD
    void HelloWorld::notifyCB(cocos2d::CCNode *node, void *notify){
        CCPomeloReponse*ccpomeloresp = (CCPomeloReponse*)notify;
    }
    void HelloWorld::on_hey(cocos2d::CCNode *node, void *event){
        CCPomeloReponse*ccpomeloresp = (CCPomeloReponse*)event;
    }

=======
>>>>>>> 日常提交




注：

是在使用cocos2d-x 加 pomelo 开发游戏的过程中顺带开发的这个类。
只做了简单的测试（所有的功能都可用）。
后续会随着游戏项目的进展持续维护这个类。

第一个有价值的github项目。欢迎拍砖。


代码风格豪放不羁。见谅~~

