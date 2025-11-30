#pragma once
#include "const.h"
#include <thread>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>

class SqlConnection {
public:
	// 构造函数：初始化数据库连接和最后操作时间
	// 参数：con - MySQL连接指针；lasttime - 最后操作时间戳（秒）
	SqlConnection(sql::Connection* con, int64_t lasttime) :_con(con), _last_oper_time(lasttime) {}
	std::unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;
};

// 封装mysql连接池
class MySqlPool {
public:
	// 构造函数：初始化连接池
	MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
		: url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false), _fail_count(0) {
		try {
			for (int i = 0; i < poolSize_; ++i) {
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance(); // 获取MySQL驱动实例
				auto* con = driver->connect(url_, user_, pass_); // 创建数据库连接
				con->setSchema(schema_); // 设置要使用的数据库schema
				// 获取当前时间戳
				auto currentTime = std::chrono::system_clock::now().time_since_epoch();
				// 将时间戳转换为秒
				long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
				pool_.push(std::make_unique<SqlConnection>(con, timestamp));  // 将新创建的连接封装为SqlConnection对象并加入连接池队列
			}
			// 启动定时检查线程：每隔60秒检查连接有效性
			_check_thread = std::thread([this]() {
				while (!b_stop_) { // 没有停止就会检测链接
					checkConnectionPro();
					std::this_thread::sleep_for(std::chrono::seconds(60));
				}
				});

			_check_thread.detach(); // 线程分离（独立运行，不阻塞主线程）
		}
		catch (sql::SQLException& e) {
			// 处理异常
			std::cout << "mysql pool init failed, error is " << e.what() << std::endl;
		}
	}
	// 连接检查核心逻辑（定时线程调用）：验证连接有效性，重建失效连接
	void checkConnectionPro() {
		// 1 先读取“目标处理数”
		size_t targetCount;
		{
			std::lock_guard<std::mutex> guard(mutex_);
			targetCount = pool_.size();
		}

		// 2 当前已经处理的数量
		size_t processed = 0;

		// 3 时间戳
		auto now = std::chrono::system_clock::now().time_since_epoch(); 
		// 将时间戳转换为秒
		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

		while (processed < targetCount) { // 遍历所有连接进行检查
			std::unique_ptr<SqlConnection> con;
			{
				std::lock_guard<std::mutex> guard(mutex_);
				if (pool_.empty()) {
					break;
				}
				con = std::move(pool_.front());
				pool_.pop();
			}

			bool healthy = true;
			// 解锁后做检查/重连逻辑：如果连接超过5秒未使用，进行有效性检查
			if (timestamp - con->_last_oper_time >= 5) {
				try {
					std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
					stmt->executeQuery("SELECT 1");
					con->_last_oper_time = timestamp;
				}
				catch (sql::SQLException& e) {
					std::cout << "Error keeping connection alive: " << e.what() << std::endl;
					healthy = false;
					_fail_count++;
				}
			}

			if (healthy) // 如果连接健康，放回连接池
			{
				std::lock_guard<std::mutex> guard(mutex_);
				pool_.push(std::move(con));
				cond_.notify_one();
			}

			++processed;
		}
		
		// 重建失效的连接
		while (_fail_count > 0) {
			auto b_res = reconnect(timestamp);
			if (b_res) {
				_fail_count--;
			}
			else {
				break;
			}
		}
	}

	// 重新建立数据库连接
	bool reconnect(long long timestamp) {
		try {

			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* con = driver->connect(url_, user_, pass_);
			con->setSchema(schema_);

			auto newCon = std::make_unique<SqlConnection>(con, timestamp);
			{
				std::lock_guard<std::mutex> guard(mutex_);
				pool_.push(std::move(newCon));
			}

			std::cout << "mysql connection reconnect success" << std::endl;
			return true;
		}
		catch (sql::SQLException& e) {
			std::cout << "Reconnect failed, error is " << e.what() << std::endl;
			return false;
		}
	}

	void checkConnection() {
		std::lock_guard<std::mutex> guard(mutex_);
		int poolsize = pool_.size();
		// 获取当前时间戳
		auto currentTime = std::chrono::system_clock::now().time_since_epoch();
		// 将时间戳转换为秒
		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
		for (int i = 0; i < poolsize; i++) {
			auto con = std::move(pool_.front());
			pool_.pop();
			Defer defer([this, &con]() {
				pool_.push(std::move(con));
				});

			// 如果连接5秒内使用过，跳过检查
			if (timestamp - con->_last_oper_time < 5) {
				continue;
			}

			try {
				// 执行心跳检测
				std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
				stmt->executeQuery("SELECT 1");
				con->_last_oper_time = timestamp; // 更新最后操作时间
				//std::cout << "execute timer alive query , cur is " << timestamp << std::endl;
			}
			catch (sql::SQLException& e) {
				std::cout << "Error keeping connection alive: " << e.what() << std::endl;
				// 重新创建连接并替换旧的连接
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				auto* newcon = driver->connect(url_, user_, pass_);
				newcon->setSchema(schema_);
				con->_con.reset(newcon); // 替换失效连接
				con->_last_oper_time = timestamp;
			}
		}
	}

	std::unique_ptr<SqlConnection> getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] {
			if (b_stop_) {
				return true;
			}
			return !pool_.empty(); 
		});
		if (b_stop_) {
			return nullptr;
		}
		std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
		pool_.pop();
		return con;
	}

	void returnConnection(std::unique_ptr<SqlConnection> con) {
		std::unique_lock<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;
		}
		pool_.push(std::move(con));
		cond_.notify_one(); // 通知等待的线程有可用连接
	}

	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}

	~MySqlPool() {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!pool_.empty()) {
			pool_.pop();
		}
	}

private:
	std::string url_;           // 数据库连接地址
	std::string user_;          // 数据库用户名
	std::string pass_;          // 数据库密码
	std::string schema_;        // 数据库名
	int poolSize_;              // 连接池最大容量
	std::queue<std::unique_ptr<SqlConnection>> pool_; // 存储连接的队列
	std::mutex mutex_;          // 保护连接池的互斥锁
	std::condition_variable cond_; // 条件变量，用于等待连接可用
	std::atomic<bool> b_stop_;  // 连接池停止标志（原子变量，线程安全）
	std::thread _check_thread;  // 定时检查连接的线程
	std::atomic<int> _fail_count; // 连接失效计数（原子变量，线程安全）
};

// 基本用户信息
struct UserInfo {
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
};

// 提供用户相关的数据库操作接口，基于连接池实现。
class MysqlDao
{
public:
	MysqlDao();
	~MysqlDao();
	// 用户注册接口
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	// 带事务的用户注册接口（支持更多用户信息）
	int RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon);
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& newpwd);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
	// bool TestProcedure(const std::string& email, int& uid, std::string& name);
private:
	std::unique_ptr<MySqlPool> pool_;
};
