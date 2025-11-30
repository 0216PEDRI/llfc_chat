#include "VerifyGrpcClient.h"  
#include "ConfigMgr.h" 

VerifyGrpcClient::VerifyGrpcClient() {
    auto& gCfgMgr = ConfigMgr::Inst(); // Global Config Manager
    std::string host = gCfgMgr["VarifyServer"]["Host"];
    std::string port = gCfgMgr["VarifyServer"]["Port"];
    pool_.reset(new RPConPool(5, host, port)); // 连接池的初始 / 最大连接数（即最多维护 5 个 gRPC Stub 连接）。
}