#pragma once

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory> // 提供了智能指针和CRTP
#include <iostream> 
#include "Singleton.h" 
#include <functional>
#include <map> 
#include <unordered_map> 
#include <json/json.h> // 基本功能
#include <json/value.h> // 基本结构节点
#include <json/reader.h> // 解析
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp> 
#include <atomic> 
#include <queue>
#include <mutex>
#include <condition_variable>
#include <hiredis.h> 
#include <cassert>

// namespace 别名针对整个命名空间，影响该命名空间内的所有成员；
// using 类型别名针对单个类型，只简化该类型的名称。
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes {
    Success = 0,
    Error_Json = 1001,  //Json解析错误
    RPCFailed = 1002,  //RPC请求错误
    VarifyExpired = 1003, //验证码过期
    VarifyCodeErr = 1004, //验证码错误
    UserExist = 1005,      //用户已经存在
    PasswdErr = 1006,     //密码错误
    EmailNotMatch = 1007,  //邮箱不匹配
    PasswdUpFailed = 1008,  //更新密码失败
    PasswdInvalid = 1009,   //密码更新失败
}; 

// Defer类 (当前作用域结束时（比如函数返回、代码块结束）自动执行一些清理、收尾操作)
class Defer {
public:
    // 接受一个lambda表达式或者函数指针
    Defer(std::function<void()> func) : func_(func) {}

    // 析构函数中执行传入的函数
    ~Defer() {
        func_();
    }

private:
    std::function<void()> func_;
};

#define CODEPREFIX "code_"
