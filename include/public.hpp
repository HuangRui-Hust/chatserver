#ifndef PUBLIC_H
#define PUBLIC_H
/*
server和client的公共文件
*/
enum EnMsgType{
    LOGIN_MSG=1,  //1登录消息
    LOGIN_MSG_ACK,//2登录响应消息
    LOGINOUT_MSG,//3注销

    REG_MSG,       //4注册消息 客户端发给服务器的
    REG_MSG_ACK,    //5服务器给客服端的确认响应消息 
    ONE_CHAT_MSG,   //6聊天消息
    ADD_FRIEND_MSG,  //7添加好友消息

    CREATE_GROUP_MSG,//8加入群组
    ADD_GROUP_MSG,//9加入群组
    GROUP_CHAT_MSG,//10群聊天
};

#endif