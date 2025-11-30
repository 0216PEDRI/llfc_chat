#ifndef SINGLETON_H
#define SINGLETON_H
#include <global.h>

template <typename T>
class Singleton {
    // 我们希望子类在继承基类的时候可以使用构造函数
protected:
    Singleton() = default; // 保护的默认构造函数：允许派生类实例化，但禁止外部直接创建对象
    Singleton(const Singleton<T>&) = delete; // 禁用拷贝构造：防止通过拷贝创建新实例，破坏单例唯一性
    Singleton operator=(const Singleton&) = delete; // 禁用拷贝赋值操作：防止实例间赋值导致的多实例风险
    static std::shared_ptr<T> _instance; // 智能指针的实例
public:
    // 获取单例实例的静态接口，全局唯一入口
    static std::shared_ptr<T> GetInstance() {
        // 静态局部变量：确保flag只初始化一次，用于线程安全的初始化标记
        static std::once_flag s_flag;

        // 确保lambda中的代码仅执行一次（多线程环境下安全）
        // 即使多个线程同时调用GetInstance，也只会有一个线程执行实例初始化
        std::call_once(s_flag, [&]() {
            // 初始化单例实例：通过new创建T对象，并托管给shared_ptr
            // 此处是单例对象唯一的创建点
            _instance = std::shared_ptr<T>(new T);
        });

        // 返回全局唯一的实例指针
        return _instance;
    }
    void PrintAddress() {
        // 打印单例实例在内存中的地址，用于验证单例的 “唯一性”—— 无论多少次调用 GetInstance()，打印的地址都应该是同一个，以此证明整个程序中确实只有一个实例
        std::cout << _instance.get() << std::endl;
    }
    ~Singleton() {
        std::cout << "this is singleton destruct" << std::endl;
    }
};

// 类内声明，类外初始化
// 声明格式：数据类型 类名::静态成员变量名 = 初始值
template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

#endif // SINGLETON_H
