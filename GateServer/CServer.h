#pragma once
#include "const.h"

// CRTP
class CServer:public std::enable_shared_from_this<CServer>
{
public:
	CServer(boost::asio::io_context& ioc, unsigned short& port);  // 构造函数
	void Start(); // 启动服务器（开始监听连接）
private:
    tcp::acceptor _acceptor;       // TCP 接收器，用于监听和接收客户端连接
    net::io_context& _ioc;         // 引用 Boost.Asio 的 I/O 上下文（事件循环） 不存在拷贝构造和拷贝赋值
}; 

