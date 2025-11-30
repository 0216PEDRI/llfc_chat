#pragma once

#include <boost/asio.hpp>
#include "CSession.h"
#include <memory.h>
#include <map>
#include <mutex>
#include <boost/asio/steady_timer.hpp>

using boost::asio::ip::tcp;

class CServer:public std::enable_shared_from_this<CServer>
{
public:
	CServer(boost::asio::io_context& io_context, short port);
	~CServer();
	// 根据标识（如uid）清理会话（断开客户端）
	void ClearSession(std::string); 
	// 根据uid获取session
	shared_ptr<CSession> GetSession(std::string);
	// 检查标识对应的会话是否有效（是否在线）
	bool CheckValid(std::string);
	
	// 定时器回调函数（如心跳检测、超时清理）
	void on_timer(const boost::system::error_code& ec);
	// 启动定时器（周期性执行任务）
	void StartTimer(); 
	// 停止定时器
	void StopTimer();
private: 
	// 处理新连接的回调
	void HandleAccept(shared_ptr<CSession>, const boost::system::error_code & error);
	// 开始异步监听客户端连接
	void StartAccept();
	
	boost::asio::io_context &_io_context;
	short _port;
	tcp::acceptor _acceptor;
	std::map<std::string, shared_ptr<CSession>> _sessions;
	std::mutex _mutex;
	boost::asio::steady_timer _timer;
};

