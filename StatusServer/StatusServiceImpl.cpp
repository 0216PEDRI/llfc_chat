#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include <climits>
#include <vector>

// 生成一个唯一 token
std::string generate_unique_string() {
	// 创建UUID对象
	boost::uuids::uuid uuid = boost::uuids::random_generator()();

	// 将UUID转换为字符串
	std::string unique_string = to_string(uuid);

	return unique_string;
}

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
	std::string prefix("llfc status server has received :  ");
	const auto& server = getChatServer();
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::Success);
	reply->set_token(generate_unique_string());
	insertToken(request->uid(), reply->token());
	return Status::OK;
}

// 启动时加载所有 ChatServer 配置
StatusServiceImpl::StatusServiceImpl()
{
	auto& cfg = ConfigMgr::Inst();
	auto server_list = cfg["chatservers"]["Name"];

	std::vector<std::string> words;

	std::stringstream ss(server_list);
	std::string word;

	while (std::getline(ss, word, ',')) { // std::getline(ss, word, ',')：从字符串流ss中读取字符，直到遇到分隔符,为止(config.ini文件中就有，)，将读取的内容存入word；
		words.push_back(word);
	}

	for (auto& word : words) {  // 遍历每个服务器名称（chatserver1、chatserver2）
		if (cfg[word]["Name"].empty()) {
			continue;
		}

		ChatServer server;
		server.port = cfg[word]["Port"];
		server.host = cfg[word]["Host"];
		server.name = cfg[word]["Name"];
		_servers[server.name] = server;
	}

}

ChatServer StatusServiceImpl::getChatServer() {
	std::lock_guard<std::mutex> guard(_server_mtx);
	
	// 首先收集所有已启动的服务器（在Redis中有LOGIN_COUNT记录的服务器）
	std::vector<ChatServer> available_servers;
	
	for (auto& server_pair : _servers) {
		auto& server = server_pair.second;
		auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.name);
		
		// 如果Redis中有该服务器的记录，说明服务器已启动
		if (!count_str.empty()) {
			server.con_count = std::stoi(count_str);
			available_servers.push_back(server);
		}
	}
	
	// 如果没有已启动的服务器，返回配置中的第一个服务器（虽然可能连接失败，但至少不会崩溃）
	if (available_servers.empty()) {
		if (!_servers.empty()) {
			return _servers.begin()->second;
		}
		// 如果配置中也没有服务器，返回一个空服务器
		return ChatServer();
	}
	
	// 如果只有一个已启动的服务器，直接返回
	if (available_servers.size() == 1) {
		return available_servers[0];
	}
	
	// 如果有多个已启动的服务器，选择连接数最少的
	ChatServer minServer = available_servers[0];
	for (size_t i = 1; i < available_servers.size(); ++i) {
		if (available_servers[i].con_count < minServer.con_count) {
			minServer = available_servers[i];
		}
	}
	
	return minServer;  // 返回负载最低（连接数最少）的聊天服务器
}

Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{
	auto uid = request->uid();
	auto token = request->token();

	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
	if (!success) {
		reply->set_error(ErrorCodes::UidInvalid);
		return Status::OK;
	}
	
	if (token_value != token) {
		reply->set_error(ErrorCodes::TokenInvalid);
		return Status::OK;
	}
	reply->set_error(ErrorCodes::Success);
	reply->set_uid(uid);
	reply->set_token(token);
	return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	RedisMgr::GetInstance()->Set(token_key, token);
}
