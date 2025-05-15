#pragma once   //防止重复引用，类似于ifndef ,define,endif

/**
 * noncopyable 被继承后，派生类对象可以正常的构造和析构，但是无法进行拷贝和复制
 */
class noncopyable{
public:
    // 删除拷贝构造函数
    noncopyable(const noncopyable&)=delete;
    // 删除复制构造函数
    noncopyable& operator=(const noncopyable&)=delete;
protected:
    // 启动默认的构造函数和析构函数
    noncopyable()=default;
    ~noncopyable()=default;
};