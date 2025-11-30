#include "RedisMgr.h"
#include "ConfigMgr.h"

// 手动编写空构造 / 析构，本质是通过显式控制类的初始化逻辑，解决默认特殊成员函数无法适配 “单例模式、指针资源管理、继承体系” 的问题。不写的话会报错
RedisMgr::RedisMgr()
{
	auto& gCfgMgr = ConfigMgr::Inst();
	auto host = gCfgMgr["Redis"]["Host"];
	auto port = gCfgMgr["Redis"]["Port"];
	auto pwd = gCfgMgr["Redis"]["Passwd"];
	// 不能直接用 = 赋值来初始化动态对象，必须通过 reset 或构造函数来绑定对象。
	_con_pool.reset(new RedisConPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

RedisMgr::~RedisMgr()
{
	Close(); 
}

// 从Redis中获取指定key对应的字符串值
bool RedisMgr::Get(const std::string& key, std::string& value)
{
	auto connect = _con_pool->getConnection(); 
	if (connect == nullptr) {
		return false; 
	}
	// redisCommand：向Redis发送命令，参数格式为「连接句柄 + 命令字符串 + 可变参数」
	// 此处执行 "GET key"，key.c_str() 将C++字符串转为C风格字符串（hiredis要求）
	auto reply = (redisReply*)redisCommand(connect, "GET %s", key.c_str());
	if (reply == NULL) {
		std::cout << "[ GET  " << key << " ] failed" << std::endl;
		// freeReplyObject(reply); // 注：此处有问题！reply为NULL时调用会触发未定义行为
		return false;
	}

	if (reply->type != REDIS_REPLY_STRING) {
		std::cout << "[ GET  " << key << " ] failed" << std::endl;
		//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
		freeReplyObject(reply);
		return false;
	}

	value = reply->str;
	freeReplyObject(reply);

	std::cout << "Succeed to execute command [ GET " << key << "  ]" << std::endl;
	return true;
}

// 向Redis中设置键值对（字符串类型）
bool RedisMgr::Set(const std::string& key, const std::string& value) {
	
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}

	//执行redis命令行
	auto reply = (redisReply*)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());

	//如果返回NULL则说明执行失败
	if (NULL == reply)
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		// freeReplyObject(reply);
		return false;
	}

	//如果执行失败则释放连接
	if (!(reply->type == REDIS_REPLY_STATUS && (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0)))
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}

	//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
	freeReplyObject(reply);
	std::cout << "Execut command [ SET " << key << "  " << value << " ] success ! " << std::endl;
	return true;
}

// redis密码认证
bool RedisMgr::Auth(const std::string& password)
{
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "AUTH %s", password.c_str());
	if (reply->type == REDIS_REPLY_ERROR) {
		std::cout << "认证失败" << std::endl;
		freeReplyObject(reply);
		return false;
	}
	else {
		//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
		freeReplyObject(reply);
		std::cout << "认证成功" << std::endl;
		return true;
	}
}

// 向Redis列表（List）左侧插入元素（头插法）
bool RedisMgr::LPush(const std::string& key, const std::string& value)
{
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "LPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == reply)
	{
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}
	// 返回 REDIS_REPLY_INTEGER（整数类型）—— 这是 Redis 对 LPUSH 命令的固定响应类型，表示执行命令后列表的当前长度。
	// 返回执行 LPUSH 后列表的新长度（大于 0 的整数）：
	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}

	std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	return true;
}

// 从Redis列表（List）左侧弹出元素（头删法）
bool RedisMgr::LPop(const std::string& key, std::string& value) {
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "LPOP %s ", key.c_str());
	if (reply == nullptr || reply->type == REDIS_REPLY_NIL) { // 命令执行成功，但请求的内容不存在
		std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}
	value = reply->str; // 将 Redis 返回的弹出元素值赋0值给传出参数 带引用 会直接更改
	std::cout << "Execut command [ LPOP " << key << " ] success ! " << std::endl; 
	freeReplyObject(reply);
	return true;
}

// 向Redis列表（List）右侧插入元素（尾插法）
bool RedisMgr::RPush(const std::string& key, const std::string& value) {
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "RPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == reply)
	{
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}

	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}

	std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	return true;
}

// 从Redis列表（List）右侧弹出元素（尾删法）
bool RedisMgr::RPop(const std::string& key, std::string& value) {
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "RPOP %s ", key.c_str());
	if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}
	value = reply->str;
	std::cout << "Execut command [ RPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	return true;
}

/**
 * @brief 向Redis哈希表（Hash）中设置字段和值（字符串参数版）
 * @details 封装Redis的HSET命令，用于给指定哈希表设置字段-值对；
 *          若哈希表不存在则自动创建，若字段已存在则覆盖旧值
 * @param[in] key Redis哈希表的键（外层Key）
 * @param[in] hkey 哈希表中的字段名（内层Key）
 * @param[in] value 哈希表中字段对应的值
 * @return 操作成功返回true，失败返回false
 */
bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value) {
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	return true;
}

/**
 * @brief 向Redis哈希表（Hash）中设置字段和值（二进制安全版）
 * @details 封装Redis的HSET命令，支持存储包含\0等特殊字符的二进制数据；
 *          不依赖字符串\0结尾，通过显式指定长度保证数据完整存储，哈希表不存在则自动创建，字段已存在则覆盖旧值
 * @param[in] key Redis哈希表的键（外层Key，C风格字符串）
 * @param[in] hkey 哈希表中的字段名（内层Key，C风格字符串）
 * @param[in] hvalue 哈希表中字段对应的二进制值（C风格内存块，可含\0等特殊字符）
 * @param[in] hvaluelen 二进制值的长度（字节数），需与hvalue实际长度一致
 * @return 操作成功返回true，失败返回false
 */
bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
{
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	const char* argv[4];
	size_t argvlen[4];
	argv[0] = "HSET";
	argvlen[0] = 4;
	argv[1] = key;
	argvlen[1] = strlen(key);
	argv[2] = hkey;
	argvlen[2] = strlen(hkey);
	argv[3] = hvalue;
	argvlen[3] = hvaluelen;
	auto reply = (redisReply*)redisCommandArgv(connect, 4, argv, argvlen);
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] success ! " << std::endl;
	freeReplyObject(reply);
	return true;
}

/**
 * @brief 从Redis哈希表（Hash）中获取指定字段的值
 * @details 封装Redis的HGET命令，支持通过哈希表键和字段名获取对应值；
 *          若哈希表/字段不存在或操作失败，返回空字符串
 * @param[in] key Redis哈希表的键（外层Key）
 * @param[in] hkey 哈希表中的字段名（内层Key）
 * @return 成功返回字段对应的值，失败或字段不存在返回空字符串
 */
std::string RedisMgr::HGet(const std::string& key, const std::string& hkey)
{
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return "";
	}
	const char* argv[3];
	size_t argvlen[3];
	argv[0] = "HGET";
	argvlen[0] = 4;
	argv[1] = key.c_str();
	argvlen[1] = key.length();
	argv[2] = hkey.c_str();
	argvlen[2] = hkey.length();
	auto reply = (redisReply*)redisCommandArgv(connect, 3, argv, argvlen);
	if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
		freeReplyObject(reply);
		std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
		return "";
	}

	std::string value = reply->str;
	freeReplyObject(reply);
	std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
	return value;
}

// 删除Redis中指定的键（支持所有数据类型）
bool RedisMgr::Del(const std::string& key)
{
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "DEL %s", key.c_str());
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}
	std::cout << "Execut command [ Del " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	return true;
}

// 检查Redis中指定键是否存在
bool RedisMgr::ExistsKey(const std::string& key)
{
	auto connect = _con_pool->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "exists %s", key.c_str());
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
		std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}
	std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
	freeReplyObject(reply);
	return true;
}

//  关闭与Redis服务器的连接
void RedisMgr::Close()
{
	_con_pool->Close(); 
}