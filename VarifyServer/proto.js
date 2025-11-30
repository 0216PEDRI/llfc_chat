const path = require('path')
const grpc = require('@grpc/grpc-js')
const protoLoader = require('@grpc/proto-loader')

const PROTO_PATH = path.join(__dirname, 'message.proto') // 拼接.proto文件的路径

// 同步加载.proto文件
const packageDefinition = protoLoader.loadSync(PROTO_PATH, { 
  keepCase: true, // 保持大小写的区分
  longs: String, // 将 long 类型转换为字符串
  enums: String, // 将 enum 类型转换为字符串
  defaults: true, // 设置默认值
  oneofs: true // 支持 oneof 语法
})
// 使用grpc加载包定义
const protoDescriptor = grpc.loadPackageDefinition(packageDefinition)
// 导出消息服务
const message_proto = protoDescriptor.message
// 导出模块
module.exports = message_proto