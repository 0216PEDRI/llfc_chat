#include "httpmgr.h"

HttpMgr::~HttpMgr()
{

}

HttpMgr::HttpMgr()
{
    connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finish);
}

// HTTP POST 请求发送函数
// 参数:
//   url - 请求的目标URL
//   json - 要发送的JSON数据
//   req_id - 请求ID，用于标识请求
//   mod - 模块标识，用于区分不同模块的请求
void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{
    // 将JSON对象转换为字节数组
    QByteArray data = QJsonDocument(json).toJson();
    
    // 创建网络请求并配置URL
    QNetworkRequest request(url);
    // 设置请求头为JSON类型
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // 设置内容长度头
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));
    
    // 获取当前对象的共享指针，确保生命周期
    auto self = shared_from_this();
    // 发送POST请求
    QNetworkReply * reply = _manager.post(request, data);
    
    // 连接请求完成信号，处理响应
    QObject::connect(reply, &QNetworkReply::finished,[self, reply, req_id, mod](){
        // 检查是否有网络错误
        if(reply->error() != QNetworkReply::NoError){
            qDebug() << reply->errorString();
            // 发送错误信号
            emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
            reply->deleteLater();
            return;
        }
        
        // 读取响应内容
        QString res = reply->readAll();
        // 发送成功信号，返回响应内容
        emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
        // 释放资源
        reply->deleteLater();
        return;
    });
}

void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    if(mod == Modules::REGISTERMOD){
        //发送信号通知指定模块http响应结束
        emit sig_reg_mod_finish(id, res, err);
    }

    if(mod == Modules::RESETMOD){
        //发送信号通知指定模块http响应结束
        emit sig_reset_mod_finish(id, res, err);
    }

    if(mod == Modules::LOGINMOD){
        //发送信号通知指定模块http响应结束
        emit sig_login_mod_finish(id, res, err);
    }
}

