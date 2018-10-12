# 欢迎来到llearner

## 一、说明
这是一个简化版本，用以演示lushan 在用于机器学习方面的功能。

## 二、快速开始

### 依赖

1. libmemcached-0.53 注意：更高版本不支持扩展memcached协议功能，所以目前必须用这个版本，后期会自己开发协议
2. tbb 用以支持lock free hash表，自己的硬件和软件是否支持请参见tbb README，下载地址：https://www.threadingbuildingblocks.org

### 编译和安装

1. 打开 Makefile, 把 LIBEVENT_HOME 改称你自己 libevent 安装路径，把TBB_HOME改成你安装tbb的路径，如果不使用tbb，可以设置一个空的目录，或者/usr/local，然后把CFLAGS里的-DUSE_TBB去掉
2. make
3. 把hmodule.so和conf/hmodule.conf安装到lushan相应的模块下，这里参考lushan的说明文档

## 三、训练数据格式

如下所示，第一个字段为label，必须为0或1，后面\t或空格分隔，64位无符号整数，表示特征的编号

1\t3924793274923\t4329943792492\n
0\t3924324320504\t9832327943284\n
