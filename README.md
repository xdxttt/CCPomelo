CCPomelo
========

Libpomelo 的  cocos2d-x 封装类

CCPomelo  v0.0.3


<br>
1 结构上按照 cocos2d-x ext 中的 HttpClient 对象来做的。对于逻辑层来说是线程安全的。


CCPomelo  v0.0.3 log

添加了完整的基于 cocos2dx 的 chat expmale。
例子基于 cocos2d-x 2.2 并且需要开发者自行配置 libpomelo环境 :) 
例子对影 pomelo官方服务器例子程序：chatofpomelo-websocket（https://github.com/NetEase/chatofpomelo-websocket）







运行 example 文件步骤：
    1 建立一个 cocos2d-x 2.2版本的空项目。
    2 用例子的的2个文件，替换掉空项目内的同名文件。
    3 配置libpomelo 环境使项目通过编译（这块会稍微麻烦点，各位各显神通吧 ：） 我唯一遇到的问题就是libwebsocket.a好像有点问题，删除掉原来的重新引用就好了）
    4 启动 chatofpomelo-websocket 
    5 启动 例子 可在控制台看到 运行结果。




