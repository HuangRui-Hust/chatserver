#ifndef CHATSERVER_H    //防止头文件被重复引用
#define CHATSERVER_H

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include"json.hpp"  
using namespace muduo;
using namespace muduo::net;

    //聊天服务器的主类
class ChatServer{
public:
    //初始化聊天服务器对象,EventLoop相当于一个反应堆和事件分发器（反应堆相当于红黑树上的一个结点，mainReactor相当监听结点，subReactor相当于连接结点，事件分发器帮助我们检测各个反应堆有没有相应的事件发生）
    //事件分发器会创建一个线程池，多个线程工作去轮询查询反应堆有无相应的事件发生，如果有会利用eventfd()中的notify(内核提供的唤醒机制)唤醒相应的反应堆，反应堆回去map表(该表中维护着相应的事件和事件处理器的映射关系)
  ChatServer(EventLoop *loop,
            const InetAddress&listenAddr,
            const string &nameArg);
    //启动服务
    void start();
private:
    //上报连接相关信息的回调函数
    void onConnection(const TcpConnectionPtr&);

    //上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr &,
                    Buffer *,
                    Timestamp);
    TcpServer _server;   //组合的muduo库，实现服务器功能的对象
    EventLoop *_loop;    //指向事件循环对象的指针
};
#endif