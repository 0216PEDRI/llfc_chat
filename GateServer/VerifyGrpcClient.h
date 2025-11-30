#pragma once
#include <grpcpp/grpcpp.h> 
#include "message.grpc.pb.h"
#include "const.h" 
#include "Singleton.h" 

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

/**
 * @brief gRPC客户端连接池类
 * @details 管理多个gRPC Stub实例，实现Stub的复用与线程安全的获取/归还，
 *          避免频繁创建/销毁Stub带来的性能开销，支持优雅关闭
 *           (stub的本质是一个客户端代理对象，封装了与远程服务通信的细节（如网络传输、数据序列化 / 反序列化等），
 *           让客户端可以像调用本地子程序（函数）一样调用远程服务的接口。)
 */
class RPConPool {
public:
    /**
     * @brief 构造函数，初始化连接池
     * @param poolSize 连接池大小（即预创建的Stub实例数量）
     * @param host gRPC服务端IP地址
     * @param port gRPC服务端端口号
     * @details 1. 初始化连接池配置参数（大小、服务端地址）
     *          2. 预创建指定数量的gRPC Channel和Stub，存入连接队列
     */
    RPConPool(size_t poolSize, std::string host, std::string port)
        : poolSize_(poolSize)    // 初始化连接池容量
        , host_(std::move(host)) // 初始化服务端IP（移动语义避免拷贝）
        , port_(std::move(port)) // 初始化服务端端口（移动语义避免拷贝）
        , b_stop_(false)         // 初始化关闭标记为false（连接池正常运行）
    {
        // 循环创建指定数量的Stub实例，存入连接队列
        for (size_t i = 0; i < poolSize_; ++i) {
            // 1. 创建gRPC通道（Channel）：封装与服务端的连接，使用不安全凭证（非TLS）
            std::shared_ptr<Channel> channel = grpc::CreateChannel(
                host_ + ":" + port_,  // 拼接服务端地址（IP:端口）
                grpc::InsecureChannelCredentials()
            );

            // 2. 通过通道创建Stub（客户端代理），并加入连接队列
            // NewStub()返回unique_ptr<VarifyService::Stub>，直接入队
            connections_.push(VarifyService::NewStub(channel));
        }
    }

    /**
     * @brief 析构函数，释放连接池资源
     * @details 1. 加锁确保线程安全，避免析构时其他线程操作连接队列
     *          2. 调用Close()触发连接池关闭逻辑
     *          3. 清空连接队列，释放所有Stub实例
     */
    ~RPConPool() {
        // 加锁保护：防止析构时其他线程（如getConnection/returnConnection）操作connections_
        std::lock_guard<std::mutex> lock(mutex_);

        Close(); // 触发连接池关闭（唤醒所有等待线程，标记关闭状态）

        // 清空连接队列：弹出所有Stub，unique_ptr会自动释放资源
        while (!connections_.empty()) {
            connections_.pop();
        }
    }

    /**
     * @brief 从连接池获取一个Stub实例（线程安全）
     * @return 成功：指向Stub的unique_ptr；失败（连接池关闭）：nullptr
     * @details 1. 加锁并阻塞等待，直到满足“连接池未关闭”且“队列非空”
     *          2. 若连接池已关闭，返回空指针
     *          3. 若队列有可用Stub，弹出并返回（移动语义转移所有权）
     */
    std::unique_ptr<VarifyService::Stub> getConnection() {
        // 独占锁：支持条件变量的wait操作（可临时释放锁）
        std::unique_lock<std::mutex> lock(mutex_);

        // 条件变量等待：直到“连接池关闭”或“队列有可用连接”
        cond_.wait(lock, [this] {
            // 条件1：连接池已关闭（直接唤醒，返回空）
            if (b_stop_) {
                return true;
            }
            // 条件2：队列非空（有可用Stub，唤醒后获取）
            return !connections_.empty();
            });

        // 若连接池已关闭，返回空指针（避免操作已失效的Stub）
        if (b_stop_) {
            return nullptr;
        }

        // 从队列头部获取Stub：用move转移unique_ptr所有权（队列不再持有该Stub）
        auto context = std::move(connections_.front());
        connections_.pop(); // 从队列移除该Stub（避免重复获取）

        return context;
    }

    /**
     * @brief 将用完的Stub实例归还到连接池（线程安全）
     * @param context 待归还的Stub实例（unique_ptr所有权）
     * @details 1. 加锁确保线程安全，避免多线程同时修改连接队列
     *          2. 若连接池已关闭，直接丢弃Stub（不归还）
     *          3. 若连接池正常，将Stub归队，并唤醒一个等待获取的线程
     */
    void returnConnection(std::unique_ptr<VarifyService::Stub> context) {
        // 加锁保护：防止多线程同时修改connections_队列
        std::lock_guard<std::mutex> lock(mutex_);

        // 若连接池已关闭，直接返回（不归还Stub，unique_ptr会自动释放）
        if (b_stop_) {
            return;
        }

        // 将Stub归队：用move转移unique_ptr所有权（队列重新持有该Stub）
        connections_.push(std::move(context));

        // 唤醒一个等待获取Stub的线程（条件变量notify_one()）
        cond_.notify_one();
    }

    /**
     * @brief 关闭连接池
     * @details 1. 标记连接池为关闭状态（原子变量，线程可见）
     *          2. 唤醒所有等待获取Stub的线程（避免线程阻塞在wait()）
     */
    void Close() {
        // 原子操作设置关闭标记：确保所有线程能立即看到状态变化
        b_stop_ = true;
        // 唤醒所有等待的线程：让阻塞在getConnection()的线程退出等待
        cond_.notify_all();
    }

private:
    std::atomic<bool> b_stop_;                          // 连接池关闭标记（原子变量，线程安全）
    size_t poolSize_;                                   // 连接池容量（预创建的Stub数量）
    std::string host_;                                  // gRPC服务端IP地址
    std::string port_;                                  // gRPC服务端端口号
    std::queue<std::unique_ptr<VarifyService::Stub>> connections_; // Stub实例队列（连接池核心）
    std::mutex mutex_;                                  // 互斥锁：保护connections_队列的线程安全访问
    std::condition_variable cond_;                      // 条件变量：协调Stub的获取与等待（避免轮询）
};
class VerifyGrpcClient: public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>; 
public: 
	GetVarifyRsp GetVarifyCode(std::string email) {
		ClientContext context; 
		GetVarifyRsp reply;
		GetVarifyReq request; 
		request.set_email(email); 
        auto stub = pool_->getConnection(); 
		Status status = stub->GetVarifyCode(&context, request, &reply); 
		if (status.ok()) {
            pool_->returnConnection(std::move(stub)); 
			return reply; 
		}
		else {
            pool_->returnConnection(std::move(stub));
			reply.set_error(ErrorCodes::RPCFailed); 
			return reply; 
		}
	}
private: 
    VerifyGrpcClient(); 
    std::unique_ptr<RPConPool> pool_; 
};