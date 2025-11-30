#pragma once

// 老版本中io_context叫做IOService
#include <vector> 
#include <memory> 
#include <thread>
#include <boost/asio.hpp> 
#include "Singleton.h" 

/**
 * @brief Boost.Asio的IO服务池实现（单例模式）
 * @details 管理多个io_context实例和对应的工作线程，实现异步IO操作的负载均衡
 */
class AsioIOServicePool : public Singleton<AsioIOServicePool>
{
	friend Singleton<AsioIOServicePool>; 
public:
	// 1. 如果io_context没有绑定异步的读写事件，run起来之后就会立即退出，所以我们需要WorkGuard 
	// 2. io_context没有拷贝构造
	using IOService = boost::asio::io_context; 
	using WorkGuard = boost::asio::executor_work_guard<IOService::executor_type>; // 工作守卫类型，用于防止io_context退出事件循环
	using WorkGuardPtr = std::unique_ptr<WorkGuard>;

	~AsioIOServicePool();

	// 基类没有拷贝构造和拷贝赋值，所以子类也没有，这两句可写可不写
	AsioIOServicePool(const AsioIOServicePool&) = delete;
	AsioIOServicePool& operator= (const AsioIOServicePool&) = delete;

	// 使用 round-robin 的方式返回一个 io_service
	boost::asio::io_context& GetIOService();
	void Stop();

private:
	// cpu有几个核就分配多少个线程
	explicit AsioIOServicePool(std::size_t size = 2/*std::thread::hardware_concurrency()*/); // 默认两个线程
	std::vector<IOService> _ioServices;
	std::vector<WorkGuardPtr> _workGuards;
	std::vector<std::thread> _threads; // 存储工作线程的容器（每个线程运行一个io_context的run()）
	std::size_t _nextIOService = 0;  // 轮询索引，记录下一个要分配的io_context在容器中的位置
};