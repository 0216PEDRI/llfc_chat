#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "ChatServiceImpl.h"
#include "const.h"
#include <Windows.h>

using namespace std;
bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main()
{
	SetConsoleOutputCP(CP_UTF8); // 设置控制台输出为 UTF - 8 编码
	auto& cfg = ConfigMgr::Inst();
	auto server_name = cfg["SelfServer"]["Name"];
	try {
		auto pool = AsioIOServicePool::GetInstance();
		// 将当前服务器的登录数初始化为0（写入Redis）
		RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");
		Defer derfer([server_name]() {
			RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name); //  删除redis中的登录计数
			RedisMgr::GetInstance()->Close(); // 关闭redis链接
		});

		boost::asio::io_context  io_context;
		auto port_str = cfg["SelfServer"]["Port"];
		//创建Cserver智能指针
		auto pointer_server = std::make_shared<CServer>(io_context, atoi(port_str.c_str()));
		//启动定时器
		pointer_server->StartTimer();

		//定义一个GrpcServer
		std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
		ChatServiceImpl service;
		grpc::ServerBuilder builder;
		
		// 监听端口和添加服务
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
		
		service.RegisterServer(pointer_server);
		
		// 构建并启动gRPC服务器
		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
		std::cout << "RPC Server listening on " << server_address << std::endl;

		//单独启动一个线程处理grpc服务
		std::thread  grpc_server_thread([&server]() {
			server->Wait();
		});


		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&io_context, pool, &server](auto, auto) {
			io_context.stop();
			pool->Stop();
			server->Shutdown();
		});


		//将Cserver注册给逻辑类方便以后清除连接
		LogicSystem::GetInstance()->SetServer(pointer_server);
		io_context.run(); // 启动 Boost.Asio 的事件循环，处理异步事件。

		grpc_server_thread.join();  // 等待gRPC线程退出
		pointer_server->StopTimer(); // 停止TCP服务器的定时器
		return 0;
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << endl;
	}

}

