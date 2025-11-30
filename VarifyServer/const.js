let code_prefix = "code_"; // 验证码前缀

const Errors = {
    Success : 0,
    RedisErr : 1,
    Exception : 2,
}; // 错误码枚举

// 导出模块
module.exports = {code_prefix,Errors}