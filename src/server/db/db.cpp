//#include<server/db/db.h>
#include"db.h"
#include<muduo/base/Logging.h>


// 数据库配置信息 
static string server = "127.0.0.1"; 
static string user = "root"; 
static string password = "123456"; 
static string dbname = "test";
// 初始化数据库连接 
MySQL::MySQL() 
{    
    _conn = mysql_init(nullptr); //相当于开辟一块内存，用于后续与mysql建立连接
}
// 释放数据库连接资源
MySQL::~MySQL() 
{ 
    if (_conn != nullptr)   //
        mysql_close(_conn); 
}
// 连接数据库 
bool MySQL::connect() 
{ 
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(), //输入相应的参数，与数据库建立连接
        password.c_str(), dbname.c_str(), 3306, nullptr, 0); 
        if (p != nullptr) 
        { 
            //C和C++代码默认的编码字符是ASCII,如果不设置，从MYSQL上拉下来的中文显示会乱码
            mysql_query(_conn, "set names gbk");
            LOG_INFO<<"connect mysql success!";
        }
        else{
            LOG_INFO<<"connect mysql fail!";
        }
        return p; 
}
         // 更新操作 
bool  MySQL::update(string sql)  //改变数据库的数据都通过这个接口
{ 
    if (mysql_query(_conn, sql.c_str())) 
        { 
            LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "更新失败!"; 
            return false; 
        }
    return true; 
}
// 查询操作 
MYSQL_RES*  MySQL::query(string sql)  //select就可以用这个接口函数
{ 
    if (mysql_query(_conn, sql.c_str())) 
    { 
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" 
            << sql << "查询失败!"; return nullptr; 
    }
    return mysql_use_result(_conn); 
} 

//获取连接
MYSQL* MySQL::getConnection()
{
    return _conn;
}