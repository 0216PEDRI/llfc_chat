#pragma once
#include "const.h" 

class HttpConnection :public std::enable_shared_from_this< HttpConnection>
{
public:
	friend class LogicSystem; 
	HttpConnection(boost::asio::io_context& ioc); 
	void Start(); 
	tcp::socket& GetSocket() {
		return _socket; 
	}
private: 
	void CheckDeadline();  // 检测连接超时
	void WriteResponse();  // 发送HTTP响应给客户端
	void HandleReq();      // 处理HTTP请求（业务逻辑入口）
	void PreParseGetParam();  // 预处理并解析GET请求的查询参数
	
	tcp::socket _socket;  // TCP socket，用于与客户端通信
	beast::flat_buffer _buffer{ 8192 };  // 读写缓冲区（8192字节），存储HTTP请求/响应数据
	http::request<http::dynamic_body> _request;  // 存储解析后的HTTP请求
	http::response<http::dynamic_body> _response; // 存储待发送的HTTP响应

	// 超时定时器：60秒超时，绑定到socket的执行器（IO线程）
	net::steady_timer deadline_{ _socket.get_executor(), std::chrono::seconds(60) };

	std::string _get_url;  // 存储GET请求的路径（不含查询参数）
	std::unordered_map<std::string, std::string> _get_params;  // 存储GET请求的查询参数（键值对）
}; 
 
