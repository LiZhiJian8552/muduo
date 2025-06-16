#pragma once

#include<vector>
#include<string>
#include<algorithm>


/*
    网络库底层的缓冲区类型定义
    buffer:
        kCheapPrepend   |  reader    |   writer |
*/
class Buffer{
public:
    //记录数据包的长度 
    static const size_t kCheapPrepend=8;
    // 缓冲区的初始大小
    static const size_t kInitialSize=1024;

    explicit Buffer(size_t initialSize=kInitialSize):
            buffer_(kCheapPrepend+initialSize),
            readerIndex_(kCheapPrepend),
            writerIndex_(kCheapPrepend){}
    
    // 返回可读区域的大小
    size_t readableBytes()const{
        return writerIndex_-readerIndex_;
    }

    // 返回可写区域的大小
    size_t writableBytes()const{
        return buffer_.size()-writerIndex_;
    }
    // 返回记录缓冲区的内容的大小
    size_t prependableBytes()const{
        return readerIndex_;
    }

    // 判断缓冲区中可写部分的大小是否大于len,如果小于考虑扩容
    void ensureWriteableBytes(size_t len){
        
        if(writableBytes()<len){
            // 扩容 
            makeSpace(len);
        }
    }

    // 缓冲区复位
    void retrieve(size_t len){
        if(len<readableBytes()){
            readerIndex_+=len;
        }else{  //len==readableBytes()
            retrieveAll();
        }
    }

    void retrieveAll(){
        readerIndex_=kCheapPrepend;
        writerIndex_=kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据转成string类型的数据返回
    std::string retrieveAsAllString(){
        return retrieveAsString(readableBytes());
    }

     std::string retrieveAsString(size_t len){
        // 构造一个从peek()位置开始长度为len的string
        std::string result(peek(),len);
        retrieve(len);
        return result;
    }
    // 返回可写区域的起始地址
    char* beginWirte(){
        return begin()+writerIndex_;
    }
    const char* beginWirte()const{
        return begin()+writerIndex_;
    }

    // 把[data,data+len]内存上的数据，添加到writeable缓冲区当中
    void append(const char* data,size_t len){
        ensureWriteableBytes(len);

        std::copy(data,data+len,beginWirte());
        writerIndex_+=len;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd,int* saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd,int* saveErrno);

private:
    //vector首元素的地址，即数组的起始地址
    char* begin(){
        // 先调用it.operator*() --->第0号元素本身---->获取该元素地址
        return &*buffer_.begin();
    }
    //vector首元素的地址，即数组的起始地址
    const char* begin()const{
        return &*buffer_.begin();
    }

    // 返回缓冲区中可读数据的起始地址
    const char* peek()const{
        return begin()+readerIndex_;
    }
   

    // 扩容函数
    void makeSpace(size_t len){
        /* 
            buffer:
                kCheapPrepend   |  reader    |   writer |
                kCheapPrepend     |         len             |
        */
        //prependableBytes()  不一定每次都会返回8 可能reader读了一部分，readerIndex_ 后移了一部分
        if(writableBytes()+prependableBytes()<len+kCheapPrepend){
            buffer_.resize(writerIndex_+len);
        }else{  //如果可写的和kCheapPrepend+[可读的部分](可能会有)可以写下数据的话
            // 当前可读部分的大小
            size_t readable=readableBytes();
            // begin()+readerIndex_,begin()+writableBytes()之间的数据，拷贝到kCheapPrepend下标处
            std::copy(begin()+readerIndex_,begin()+writableBytes(),begin()+kCheapPrepend);
            readerIndex_=kCheapPrepend;
            writerIndex_=readerIndex_+readable;
        }
    }

    //缓冲区 
    std::vector<char> buffer_;
    // 数据可读的下标
    ssize_t readerIndex_;
    // 数据可写的下标
    ssize_t writerIndex_;
};