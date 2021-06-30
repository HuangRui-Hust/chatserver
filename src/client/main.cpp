#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

//记录当前系统登陆的用户消息
User g_currentUser;

//记录当前登陆用户的好友列表信息
vector<User> g_currentUserFriendList;

//记录当前登陆用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 控制主菜单页面程序
bool isMainMenuRunning = false;

//显示当前登陆成功用户的基本信息
void showCurrentUserData();

//接收线程 接收服务器发来的消息响应
void readTaskHandler(int clientfd);
//获取系统时间 聊天信息时间
string gerCurrenttime();
//主聊天页面程序
void mainMenu(int fd);

//聊天客户端实现 main线程用作发送线程(读取用户的输入并发送) 子线程用作接收线程（接受服务器的响应消息)
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << "command invalid example:./ChatClient 127.0.0.1 6000 " << endl;
        exit(-1);
    }

    //解析通过命令行参数传递的Ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    //创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    //填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(port);

    //clietn和server连接
    if (connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)) == -1) //同时自动绑定客户端ip和port
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    //main线程用于接受用户输入 负责发送json数据
    for (;;)
    {
        //显示页面菜单、登陆、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get(); //读掉缓冲区残留的回车
        switch (choice)
        {
        case 1: //1. 登陆
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get();//读掉缓冲区残留的回车
            cout << "password:";
            cin.getline(pwd, 50); //cin.getline()避免遇见空格就停止 例如Huang Rui

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;

            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send loging msg error:" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv login response error" << endl;
                }
                else
                { //success:responsejs{msgid errno id name}
                    //fail: {msgid errno errmsg}
                    json responsejs = json::parse(buffer);

                    if (0 != responsejs["errno"].get<int>()) // 登录失败
                    {
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else //登陆成功
                    {
                        //记录当前用户的Id和name
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        //记录当前用户的好友列表信息
                        if (responsejs.contains("friends"))
                        {
                            //初始化 避免用户异常退出 又登陆后 重复往里面加入friend列表信息
                            g_currentUserFriendList.clear();
                            vector<string> vec = responsejs["friends"];
                            // {id name state}
                            for (string &str : vec)
                            {
                                User user;
                                json js = json::parse(str);
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);

                                g_currentUserFriendList.push_back(user);
                            }
                        }
                            /*
                            测试使用                          
                            cout<<"--------------------3"<<endl;
                            Group group;
                            group.setId(100);
                            group.setName("test");
                            group.setDesc("build for test!");

                            g_currentUserGroupList.push_back(group);
                            cout<<"--------------------3"<<endl;
                            */
                        //记录当前用户的群组消息
                        if (responsejs.contains("groups"))
                        {
                            //cout<<"--------------------4"<<endl;
                            //初始化
                            g_currentUserGroupList.clear();
                            vector<string> vec1 = responsejs["groups"]; //{"id:**","groupname:**","groupdesc:***""users"
                            //cout<<"--------------------4"<<endl;
                            for (string &groupstr : vec1)
                            {
                                //cout<<"--------------------5"<<endl;
                                json grpjs = json::parse(groupstr);
                                //cout<<"--------------------5"<<endl;
                                Group group;
                                group.setId(grpjs["id"].get<int>());
                                group.setName(grpjs["groupname"]);
                                group.setDesc(grpjs["groupdesc"]);
                                // vector<string> vec2 = responsejs["users"];//这里有Bug 不是responsejs 是grpjs . {id groupname groupdesc users 是平级的}
                                //cout<<"--------------------6"<<endl;
                                vector<string> vec2 = grpjs["users"];
                                //cout<<"--------------------6"<<endl;
                                for (string &userstr : vec2)
                                {
                                    //cout<<"--------------------7"<<endl;
                                    json groupuserjs = json::parse(userstr);
                                    //cout<<"--------------------7"<<endl;
                                    GroupUser grpuser;
                                    grpuser.setId(groupuserjs["id"].get<int>());
                                    grpuser.setName(groupuserjs["name"]);
                                    grpuser.setState(groupuserjs["state"]);
                                    grpuser.setRole(groupuserjs["role"]);
                                    group.getUsers().push_back(grpuser);
                                }
                                g_currentUserGroupList.push_back(group);
                            }
                        }

                        //显示登陆用户的基本信息
                        showCurrentUserData(); //hr:上面存储了vector<User>g_currentUserFriendList; 和vector<Group>g_currentUserGroupList;

                        //显示当前用户的离线消息 个人聊天信息或者群组信息
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                //私聊消息
                                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                                {
                                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                                         << " said " << js["msg"].get<string>() << endl;
                                }
                                else //群聊消息
                                {
                                    cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                                         << " said: " << js["msg"].get<string>() << endl;
                                }
                            }
                        }

                        //登陆成功，启动接收线程 负责接受服务器发送的响应数据，该线程只启动一次
                        static int readthreadnumber = 0;
                        if (readthreadnumber == 0)
                        {
                            //1. 子线程 从服务器读取在线消息
                            std::thread readTask(readTaskHandler, clientfd); //pthread_create
                            readTask.detach();                               //pthread_detach
                            readthreadnumber++;
                        }

                        //进入聊天主菜单页面
                        isMainMenuRunning = true;
                        mainMenu(clientfd); //2.Main线程 给服务器发消息   当执行LogOut后 isMainMenuRunning =false ,此时客户端只能接受消息 不能执行chat groupchat addfriend等等操作
                    }                       //如国不加isMainMenuRunning进行控制 因为mainMenu是个死循环 循环一直在里面 出不来 不能返回本代码前面部分
                }
            }
        }
        break;
        case 2: //2.注册业务
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "password:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;

            string request = js.dump();
            int len = send(clientfd, request.c_str(), request.size() + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (responsejs["errno"].get<int>() != 0) //注册失败
                    {
                        cerr << name << " is already exist, register error!" << endl;
                    }
                    else //注册成功
                    {
                        cout << name << " register success, userid is " << responsejs["id"]
                             << ", do not forget it!" << endl;
                    }
                }
            }
        }

        break;
        case 3: //quit业务
        {
            close(clientfd);
            exit(0);
        }
        break;

        default:
            cerr << "invalid input please input again!" << endl;
            break;
        }
    }
    return 0;
}

//接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0); //没有数据的时候会阻塞
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收ChatServer转发的数据，反序列化生成json数据对象 
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgtype)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }

        if (GROUP_CHAT_MSG == msgtype)
        {
            cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
    }
}

//显示用户的基本信息
void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }

    cout << "======================================================" << endl;
}

void help(int fd = 0, string str = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);

//系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}};

// 注册系统支持的客户端命令处理   int代表fd,string是要发送的消息
unordered_map<string, function<void(int, string)>> commandHandlerMap = {   
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}  
    };

//主聊天页面程序
void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024); //读取命令程序  输入的时候都是：“groupchat:groupid:message"
        string commandbuf(buffer);
        string command; //存储命令

        int idx = commandbuf.find(":");
        if (-1 == idx) //说明只有命令 没有具体指令格式 如：“注销，格式”
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }
        else
            it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}

//“help”
void help(int, string)
{
    cout << "help command list>>>" << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << " " << p.second << endl;
    }
    cout << endl;
    //显示效果 ⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇⬇
    // {"help", "显示所有支持的命令，格式help"},
    // {"chat", "一对一聊天，格式chat:friendid:message"},
    // {"addfriend", "添加好友，格式addfriend:friendid"},
    // {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    // {"addgroup", "加入群组，格式addgroup:groupid"},
    // {"groupchat", "群聊，格式groupchat:groupid:message"},
    // {"loginout", "注销，格式loginout"}};
}
// 一对一聊天，格式chat:friendid:message
void chat(int clientfd, string str)
{
    int idx = str.find(":"); //friendid:message
    if (idx == -1)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = gerCurrenttime();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), buffer.size() + 1 /*strlen(buffer.c_str())*/, 0);
    if (len == -1)
    {
        cerr << "send chat msg error->" << buffer << endl;
    }
}
//addfriend:friendid
void addfriend(int clientfd, string str)
{

    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), buffer.size() + 1, 0);

    if (len == -1)
    {
        cerr << "send addfriend msg error->" << buffer << endl;
    }
}

//"creategroup" command handler     groupname:groupdesc
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), buffer.size() + 1, 0);
    if (len == -1)
    {
        cerr << "send creategroup msg error->" << buffer << endl;
    }
}
//输入的命令行为addgroup:groupid  由于前面对命令参数addgroup做了过滤处理 此时传入的字符串只有groupid
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), buffer.size() + 1, 0);
    if (len == -1)
    {
        cerr << "send addgroup msg error->" << buffer << endl;
    }
}

//groupchat:groupid:message  传入到该函数当中只剩groupid:message
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "groupchat commmand error" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());

    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = gerCurrenttime();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), buffer.size() + 1 /*strlen(buffer.c_str())*/, 0);
    if (len == -1)
    {
        cerr << "send groupchat msg error->" << buffer << endl;
    }
}
//loginout
void loginout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), buffer.size() + 1, 0);
    if (len == -1)
    {
        cerr << "send loginout msg error->" << buffer << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}

//获取系统时间（聊天信息 需添加时间信息）
string gerCurrenttime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}
