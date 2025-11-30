#pragma once
#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include <grpcpp/grpcpp.h> 
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <queue>
#include "data.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::ChatService;

using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;

using message::KickUserReq;
using message::KickUserRsp;


class ChatConPool {
public:
	ChatConPool(size_t poolSize, std::string host, std::string port)
		: poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
		for (size_t i = 0; i < poolSize_; ++i) {
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
			connections_.push(ChatService::NewStub(channel)); // NewStub相当于 邮递员 信使
		}
	}

	~ChatConPool() {
		std::lock_guard<std::mutex> lock(mutex_);
		Close();
		while (!connections_.empty()) {
			connections_.pop();
		}
	}

	std::unique_ptr<ChatService::Stub> getConnection() {
		std::unique_lock<std::mutex> lock(mutex_); 
		// 只有当这个条件表达式返回 true 时，线程才会被唤醒并继续执行；否则，线程会一直阻塞等待。
		cond_.wait(lock, [this] { 
			if (b_stop_) {
				return true; // 如果 b_stop_（连接池停止标志）为 true，返回 true → 唤醒线程，退出等待（此时要返回空指针）。
			}
			return !connections_.empty(); // 如果连接池为空，connections_.empty()为true，会阻塞等待
		});
		
		// 如果被唤醒是因为 b_stop_ 为 true（连接池已停止），则返回空指针，告知调用者 “无法获取连接”。
		if (b_stop_) {
			return  nullptr;
		} 
		// 否则就是连接池不为空并且没有停止，取出连接并返回
		auto context = std::move(connections_.front());
		connections_.pop();
		return context;
	}


	void returnConnection(std::unique_ptr<ChatService::Stub> context) {
		std::lock_guard<std::mutex> lock(mutex_); 
		// 2. 如果连接池已停止（b_stop_为true），则不归还（避免向已关闭的池添加连接）
		if (b_stop_) {
			return;
		}
		// 3. 将连接（Stub）放回队列（用std::move转移unique_ptr的所有权，因为不能复制）
		connections_.push(std::move(context)); 
		// 4. 唤醒一个正在等待连接的线程（调用getConnection()时阻塞的线程）
		cond_.notify_one();
	}

	void Close() {
		// 1. 标记连接池为“停止”状态
		b_stop_ = true;

		// 2. 唤醒所有因等待连接而阻塞的线程
		cond_.notify_all();
	}

private:
	atomic<bool> b_stop_;
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<ChatService::Stub> > connections_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

// 利用单例模式实现grpc通信的客户端
class ChatGrpcClient :public Singleton<ChatGrpcClient>
{
	friend class Singleton<ChatGrpcClient>;
public:
	~ChatGrpcClient() { }

	AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req);
	AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req);
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
	TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue);
	KickUserRsp NotifyKickUser(std::string server_ip, const KickUserReq& req);
private:
	ChatGrpcClient();
	unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;	
};



