#!/bin/bash

set -e

# 如果没有build目录，创建该目录
# 当前目录$(pwd)或`pwd`
if [ ! -d  $(pwd)/build ]; then
    mkdir  $(pwd)/build
fi

rm -rf  $(pwd)/build/*

cd  $(pwd)/build &&
    cmake .. && 
    make

cd ..

# 把头文件拷贝到 /usr/include/mymuduo so库拷贝到 /usr/lib  PATH
if [ ! -d /usr/include/mymuduo ];then
    mkdir /usr/include/mymuduo
fi

for header in `ls ./include/*.h`
do 
    cp $header /usr/include/mymuduo
done

cp  $(pwd)/lib/libmymuduo.so /usr/lib

# 刷新动态库缓存
ldconfig

cd example
make clean
make
./testserver