#pragma once
#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include "const.h"
#include "MsgNode.h"
using namespace std;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class CServer;
class LogicSystem;

class CSession: public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& io_context, CServer* server);
	~CSession();
	tcp::socket& GetSocket();
	std::string& GetSessionId();
	void SetUserId(int uid);
	int GetUserId();
	void Start(); 
	// 发送C-style字符串消息，限制最大长度和消息ID。
	void Send(char* msg,  short max_length, short msgid); 
	// 发送std::string类型的消息，包含消息ID。
	void Send(std::string msg, short msgid);
	void Close();
	// 返回一个指向当前 CSession 对象的 shared_ptr
	std::shared_ptr<CSession> SharedSelf();
	void AsyncReadBody(int length);
	void AsyncReadHead(int total_len);
	void NotifyOffline(int uid);
	//判断心跳是否过期
	bool IsHeartbeatExpired(std::time_t& now);
	//更新心跳
	void UpdateHeartbeat();
	//处理异常连接
	void DealExceptionSession();
private: 
	// 异步读取固定长度数据，读取完毕后触发回调处理。
	void asyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code& , std::size_t)> handler); 
	// 异步分批读取数据，直到读取完指定总长度，回调处理每次读取的结果。
	void asyncReadLen(std::size_t  read_len, std::size_t total_len, std::function<void(const boost::system::error_code&, std::size_t)> handler);
	 
	
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);
	tcp::socket _socket;
	std::string _session_id;
	char _data[MAX_LENGTH];
	CServer* _server;
	bool _b_close;
	std::queue<shared_ptr<SendNode> > _send_que;
	std::mutex _send_lock;
	//收到的消息结构
	std::shared_ptr<RecvNode> _recv_msg_node;
	// 消息头解析标志
	bool _b_head_parse;
	//收到的头部结构
	std::shared_ptr<MsgNode> _recv_head_node;
	int _user_uid;
	//记录上次接受数据的时间
	std::atomic<time_t> _last_heartbeat;
	//session 锁
	std::mutex _session_mtx;
};

class LogicNode {
	friend class LogicSystem;
public:
	LogicNode(shared_ptr<CSession>, shared_ptr<RecvNode>); 
private:
	shared_ptr<CSession> _session; // 会话对象
	shared_ptr<RecvNode> _recvnode; // 接收到的数据节点
};
