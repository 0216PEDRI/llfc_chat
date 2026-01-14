const fs = require('fs'); // fs 是 Node.js 内置的核心模块（File System 的缩写，即文件系统模块 ）

// let 声明的变量是 “可变的”（可以被重新赋值）
let config = JSON.parse(fs.readFileSync('config.json', 'utf8'));
let email_user = config.email.user;
let email_pass = config.email.pass;
let mysql_host = config.mysql.host;
let mysql_port = config.mysql.port;
let redis_host = config.redis.host;
let redis_port = config.redis.port;
let redis_passwd = config.redis.passwd;
let code_prefix = "code_";

module.exports = {email_pass, email_user, mysql_host, mysql_port,redis_host, redis_port, redis_passwd, code_prefix}