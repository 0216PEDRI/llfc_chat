/**
 * 引入grpc核心模块，用于创建grpc服务端
 */
const grpc = require('@grpc/grpc-js')
/**
 * 引入protobuf定义的服务和消息结构（用于grpc通信协议）
 */
const message_proto = require('./proto')
/**
 * 引入常量配置模块（包含错误码、前缀等常量）
 */
const const_module = require('./const')
/**
 * 引入邮件发送模块（用于发送验证码邮件）
 */
const emailModule = require('./email')
/**
 * 引入uuid工具，用于生成唯一验证码（截取前4位） Universally Unique Identifier
 */
const { v4: uuidv4 } = require('uuid');
/**
 * 引入redis操作模块（用于缓存验证码及过期时间）
 */
const redis_module = require('./redis')

/**
 * 处理获取验证码的grpc请求
 * @param {Object} call - grpc请求对象，包含客户端传入的参数（如email）
 * @param {Function} callback - grpc响应回调函数，用于返回结果给客户端
 */
async function GetVarifyCode(call, callback) {
    // 打印客户端请求的邮箱地址（调试用）
    console.log("email is ", call.request.email)
    try {
        // 从redis中查询该邮箱已存在的验证码（通过前缀+邮箱作为key）
        let query_res = await redis_module.GetRedis(const_module.code_prefix + call.request.email);
        console.log("query_res is ", query_res)
        let uniqueId = query_res; // 初始化验证码为redis查询结果

        // 如果redis中无该邮箱的验证码（首次请求或已过期）
        if (query_res == null) {
            // 生成uuid作为基础验证码
            uniqueId = uuidv4();
            // 截取uuid前4位作为最终验证码（简化验证码长度）
            if (uniqueId.length > 4) {
                uniqueId = uniqueId.substring(0, 4);
            }
            // 将验证码存入redis，并设置10分钟（600秒）过期时间
            let bres = await redis_module.SetRedisExpire(const_module.code_prefix + call.request.email, uniqueId, 600)
            // 如果redis存储失败，返回Redis错误码给客户端
            if (!bres) {
                callback(null, { 
                    email: call.request.email,
                    error: const_module.Errors.RedisErr // Redis操作失败错误码
                });
                return; // 终止函数执行
            }
        }

        // 打印最终要发送的验证码（调试用）
        console.log("uniqueId is ", uniqueId)
        // 拼接邮件正文内容
        let text_str =  '您的验证码为'+ uniqueId +'请三分钟内完成注册'
        
        // 配置邮件发送参数
        let mailOptions = {
            from: '15234316745@163.com', // 发件人邮箱（项目固定邮箱）
            to: call.request.email,      // 收件人邮箱（客户端请求的邮箱）
            subject: '验证码',           // 邮件主题
            text: text_str,              // 邮件正文（包含验证码）
        };
    
        // 调用邮件模块发送验证码邮件
        let send_res = await emailModule.SendMail(mailOptions);
        console.log("send res is ", send_res) // 打印邮件发送结果（调试用）

        // 邮件发送成功，返回成功响应给客户端
        callback(null, { 
            email:  call.request.email,
            error: const_module.Errors.Success // 成功错误码
        }); 

    } catch(error) {
        // 捕获所有异常（如redis操作失败、邮件发送失败等）
        console.log("catch error is ", error)

        // 返回异常错误码给客户端
        callback(null, { 
            email:  call.request.email,
            error: const_module.Errors.Exception // 系统异常错误码
        }); 
    }
}

/**
 * 启动grpc服务端的主函数
 */
function main() {
    // 创建grpc服务器实例
    var server = new grpc.Server()
    // 注册protobuf定义的VerifyService服务，并绑定GetVarifyCode方法
    server.addService(message_proto.VarifyService.service, { GetVarifyCode: GetVarifyCode })
    // 绑定服务器地址（0.0.0.0:50051），使用不安全的凭据（生产环境需用SSL）
    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        // 启动grpc服务器
        console.log('grpc server started') // 打印服务器启动成功日志
    })
}

// 执行主函数，启动服务
main()