#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOservicePool.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port)
    : _ioc(ioc),
    /*
     tcp::acceptor 的构造函数需要两个参数：
    第一个参数是 boost::asio::io_context&（I/O 上下文对象，用于驱动异步操作）；
    第二个参数是 tcp::endpoint（TCP 端点，包含 IP 协议版本和端口号）。
    */
      _acceptor(ioc, tcp::endpoint(tcp::v4(), port)) 
{

}

void CServer::Start() {
    auto self = shared_from_this(); // 用于获取当前对象的 std::shared_ptr
    auto& io_context = AsioIOServicePool::GetInstance()->GetIOService(); 
    std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context); 
    // 异步接受连接
    _acceptor.async_accept(new_con->GetSocket(), [self, new_con](beast::error_code ec) {
        try {
            // 出错则放弃这个连接，继续监听新链接
            if (ec) {
                self->Start(); //  监听新链接
                return; 
            }
            // 创建新链接，创建HttpConnection类管理新链接
            new_con->Start();  
            // 继续监听
            self->Start(); 
        }
        catch (std::exception &exp) {
            std::cout << "exception is" << exp.what() << std::endl; 
            self->Start(); 
        }
    });  
}