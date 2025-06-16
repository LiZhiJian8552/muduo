#include"Buffer.h"

#include<errno.h>
#include<sys/uio.h>
#include <unistd.h>
/**
 *  从fd上读取数据，Poller工作在LT模式下 
 *  buffer缓冲区是有大小的，但是从fd上读数据的时候，却不知道tcp数据最终的大小
 */
ssize_t Buffer::readFd(int fd, int *saveErrno){
    // 当buffer缓冲区无法一次性接受数据时，将剩下的数据写入extrabuf中
    char extrabuf[65536]={0};           //栈上的内存空间，64K

    /**
        struct iovec{
            void *iov_base; 
            size_t iov_len;
        }
     */
    struct iovec vec[2];

    // 获取当前缓冲区中可写数据的大小
    const ssize_t writable=writableBytes();

    vec[0].iov_base=begin()+writerIndex_;
    vec[0].iov_len=writable;

    vec[1].iov_base=extrabuf;
    vec[1].iov_len=sizeof extrabuf;

    // 一次性可以写64K
    const int iovcnt=(writable<sizeof extrabuf)?2:1;

    /**
     * readv可以从fd中读取数据，放到多个（iovcnt）不连续的空间（struct iovec）中写入同一个fd中读取的数据
     */
    const ssize_t n=::readv(fd,vec,iovcnt);

    if(n<0){    //读取数据出错
        *saveErrno=errno;
    }else if(n<=writable){  //如果当前缓冲区可以存入fd中的数据
        // 数据已经在readv调用时写入到了buffer缓冲区中，此时只需要更新writeIndex_指针位置就可
        writerIndex_+=n;
    }else{  //当前fd中的数据无法完全写入buffer，调用append,进行扩容
        // 将extrabuf中的数据append即可
        append(extrabuf,n-writable);
    }

    return n;
}



ssize_t Buffer::writeFd(int fd,int* saveErrno){
    ssize_t n=::write(fd,peek(),readableBytes());
    if(n<0){
        *saveErrno=errno;
    }
    return n;
}