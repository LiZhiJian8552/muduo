#include"Logger.h"
#include"Timestamp.h"
#include<iostream>

//获取日志唯一的实例对象 
Logger& Logger::instance(){
    static Logger logger;
    return logger;
}

// 设置日志级别
void Logger::setLogLevel(int level){
    logLevel_=level;
}

// 写日志 [级别] time:msg
void Logger::log(std::string msg){
    using  std::cout;
    switch (logLevel_){
        case INFO:
            cout<<"[INFO] ";
            break;
        case ERROR:
            cout<<"[ERROR] ";
            break;
        case FATAL:
            cout<<"[FATAL] ";
            break;
        case DEBUG:
            cout<<"[DEBUG] ";
            break;
    }
    // TODO 添加时间
    cout<<Timestamp::now().toString()<<" : "<<msg<<std::endl;
}