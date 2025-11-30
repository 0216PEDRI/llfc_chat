#include "AsioIOServicePool.h"
#include <iostream>
using namespace std;

/**
 * @brief 构造函数，初始化 IO 服务池
 * @param size IO 服务（io_context）的数量，默认值可在声明处指定
 * @details
 * 1. 初始化 _ioServices 容器，创建指定数量的 io_context 对象
 * 2. 为每个 io_context 创建工作守卫（work guard），防止 io_context 因无待处理任务而退出事件循环
 * 3. 为每个 io_context 启动独立的工作线程，运行其事件循环（io_context::run()）
 */
AsioIOServicePool::AsioIOServicePool(std::size_t size)
	: _ioServices(size)          // 初始化 IO 服务容器，创建 `size` 个 io_context 对象
	, _workGuards(size)          // 初始化工作守卫容器，预留 `size` 个位置
	, _nextIOService(0)          // 初始化负载均衡索引，从第 0 个 IO 服务开始分配
{
	// 为每个 io_context 创建工作守卫，保持其事件循环持续运行
	for (std::size_t i = 0; i < size; ++i) {
		_workGuards[i] = std::make_unique<WorkGuard>(
			boost::asio::make_work_guard(_ioServices[i])
		);
	}

	// 遍历多个ioservice，创建多个线程，每个线程内部启动一个ioservice
	for (std::size_t i = 0; i < _ioServices.size(); ++i) {
		_threads.emplace_back([this, i]() { // emplace_back 在容器末尾原地构造元素
			_ioServices[i].run();
			});
	}
}

AsioIOServicePool::~AsioIOServicePool() {
	Stop(); // 利用 RAII 机制，确保对象销毁时资源正确释放
	std::cout << "AsioIOServicePool destruct" << std::endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
	auto& service = _ioServices[_nextIOService++]; // 选择下一个 IO 服务（轮询）
	// 重置索引，实现循环轮询
	if (_nextIOService == _ioServices.size()) {
		_nextIOService = 0;
	}
	return service;
}

void AsioIOServicePool::Stop() {
	// 步骤 1：释放 work_guard，允许 io_context 退出
	for (auto& guard : _workGuards) {
		guard->reset();
	}
	// 步骤 2：显式停止每个 io_context（防止有未处理的阻塞任务）
	for (auto& io : _ioServices) {
		io.stop();
	}
	// 步骤 3：等待所有工作线程退出（因为线程可能还在跑）
	for (auto& t : _threads) {
		if (t.joinable()) {
			t.join(); // 阻塞等待线程结束，确保资源正确释放
		}
	}
}