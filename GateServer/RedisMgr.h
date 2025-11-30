#pragma once
#include "const.h"

// 封装redis连接池
class RedisConPool {
public:
	RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
		: poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
		for (size_t i = 0; i < poolSize_; ++i) {
			auto* context = redisConnect(host, port);
			if (context == nullptr || context->err != 0) {
				if (context != nullptr) {
					redisFree(context);
				}
				continue;
			}

			auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
			if (reply->type == REDIS_REPLY_ERROR) {
				std::cout << "认证失败" << std::endl;
				//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
				freeReplyObject(reply);
				redisFree(context);
				continue;
			}

			//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
			freeReplyObject(reply);
			std::cout << "认证成功" << std::endl;
			connections_.push(context);
		}
	}

	~RedisConPool() {
		std::lock_guard<std::mutex> lock(mutex_);
		while (!connections_.empty()) {
			connections_.pop();
		}
	}

	redisContext* getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] {
			if (b_stop_) {
				return true;
			}
			return !connections_.empty();
			});
		//如果停止则直接返回空指针
		if (b_stop_) {
			return  nullptr;
		}
		auto* context = connections_.front();
		connections_.pop();
		return context;
	}

	void returnConnection(redisContext* context) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;
		}
		connections_.push(context);
		cond_.notify_one();
	}

	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}

private:
	std::atomic<bool> b_stop_;          // 连接池停止标志（原子变量，多线程安全读写）
	size_t poolSize_;                   // 连接池容量（最大连接数）
	const char* host_;                  // Redis服务器主机地址
	int port_;                          // Redis服务器端口号
	std::queue<redisContext*> connections_;  // 存储空闲Redis连接的队列
	std::mutex mutex_;                  // 保护connections_和b_stop_的互斥锁（线程安全）
	std::condition_variable cond_;      // 条件变量，用于线程等待/唤醒（配合mutex_使用）
}; 

class RedisMgr : public Singleton<RedisMgr>,
    public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>; 

public:
    ~RedisMgr();
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool Auth(const std::string& password);

    //有序的字符串双向链表(列表)：LPush（左插）、RPush（右插）、LPop（左弹）、RPop（右弹）
    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);

    // 键值对的嵌套哈希表: HSet（设字段值）、HGet（取字段值）
	// 区别1：参数类型：前者全为std::string（C++ 字符串对象），后者为const char*（C 风格字符串）+ 长度size_t；
	// 区别2：数据支持：前者仅支持以\0结尾的文本字符串（含\0会截断），后者支持任意二进制数据（通过长度保证完整存储）；
    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    
	std::string HGet(const std::string& key, const std::string& hkey);

    bool Del(const std::string& key);
    bool ExistsKey(const std::string& key);
    void Close();

private:
    RedisMgr();
    std::unique_ptr<RedisConPool> _con_pool;
};