#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <mutex>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

class  ChatServer {
public:
	ChatServer():host(""),port(""),name(""),con_count(0){}
	ChatServer(const ChatServer& cs):host(cs.host), port(cs.port), name(cs.name), con_count(cs.con_count){}
	ChatServer& operator=(const ChatServer& cs) {
		if (&cs == this) {
			return *this;
		}

		host = cs.host;
		name = cs.name;
		port = cs.port;
		con_count = cs.con_count;
		return *this;
	}
	std::string host;
	std::string port;
	std::string name;
	int con_count;
};

class StatusServiceImpl final : public StatusService::Service
{
public:
	StatusServiceImpl(); 
	
	// 获取聊天服务器信息
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply) override; 
	// 用户登录验证
	Status Login(ServerContext* context, const LoginReq* request, LoginRsp* reply) override;
private:
	// 把生成的 token 写入 Redis，后面 Login 校验用。 
	void insertToken(int uid, std::string token); 
	// 获取当前连接数最少的聊天服务器
	ChatServer getChatServer(); 
	
	std::unordered_map<std::string, ChatServer> _servers;
	std::mutex _server_mtx;
};

