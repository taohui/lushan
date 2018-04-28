# 欢迎来到lushan

**Tao Hui** http://weibo.com/taohui3

[**English Version**](docs/README_en.md)

[**设计理念**](docs/lushan_concepts.md)

## 什么是lushan?
lushan 在1.0版本的时候是一个轻量级的key-value数据库，使用memcached协议，可以挂载多个库。使用lushan你可以很容易的像memcached一样在几台机器上搭建一个集群。lushan 2.0 同时是一个轻量级的应用框架，可以挂载多个共享库，这样你在一个进程内同时具有访问数据和计算的能力，这使得编写大数据量、高性能的服务变得前所未有的简单。特别适合互联网推荐、广告和搜索的业务场景。目前lushan 已经在新浪微博推荐和广告业务中应用多年。

## lushan的特点

1. **Memcached 协议.** lushan 使用memcached协议，这样可以利用已经广泛使用的memcached的各种语言客户端。
2. **多个库.** lushan 可以挂载多个库，这样你不必为每增加一份数据而重新部署一套服务，也不必重启他。
3. **多个计算模块.** lushan 可以挂载多个共享库，这样你可以把计算放到和数据同一个进程里，程序编写非常容易，而且对于依赖大数据量的服务，性能依然非常高。
4. **统计状态数据.** lushan 拥有详细的统计状态数据，这些数据对运维非常重要。
5. **非常快.** lushan 设计和开发中注重细节，使用IO多路复用的通信模型，细致的超时处理和尽量少的内存拷贝。
6. **Hadoop.** lushan 提供了LushanFileOutputFormat.java，你可以利用他在hadoop上直接数据可以挂载的库格式。并且提供了transfer框架，可以让你的数据简单而严谨的上线。
7. **Redis.** lushan 可以通过lproxy模块连接Redis，从而把批量更新的静态数据和实时更新的动态数据结合起来，形成一个功能极其强大的key-value存储集群。

## 为什么开发 lushan?

大概在2013年，我在开发“错过的微博“推荐的时候。我需要在线提供几份数据存储来实验不同的算法，以及他们的在线和测试版本。这样需要部署几套系统。这种做法太low了。所以，在某个周末时间我开发了lushan，他可以把你从这些事情中解放出来，非常cool。

事实也是如此，lushan后来已经成为微博推荐和广告业务的基础设施。现在运维着两个集群，分别有12台机器，服务着上T的在线查询数据，每天10亿以上的查询。

在开发完lushan 第一个版本的时候，我一直有让lushan 同时能够挂载共享库的冲动。但迟迟没有下定决心，一是因为我相信一个框架最好能有自己的定位，二是架构上应该把易变更部分和稳定部分分开。但在2015年，开发微博广告的时候，用户的兴趣数据、关系数据、CTR预估的特征数据都很容易通过Hadoop整理后存储到lushan，这个时候很简单的编写两个模块即可实现targeting和ctr预估功能，而且性能非常强劲。于是我放弃了原来的想法，实现了第二版，让lushan 功能更加强大。应用中你仍然可以只用作key-value database 使用，或者把只提供数据的lushan 集群和同时提供计算的集群分开单独部署，在微博广告内部也是这么使用的。

## 快速开始

### 依赖

libevent 1.4 或以上.

### 编译和安装

1. 打开 Makefile, 把 LIBEVENT_HOME 改称你自己 libevent 安装路径.
2. make
3. 拷贝 lushan 到 bin 目录
4. 打开 conf/lushan.conf, 把 HDB_PATH、UPLOAD_PATH、HMOD_PATH 和 LOG_PATH 依照你的安装目录修改.
5. 在rsyncd.conf中增加 lushan_upload 模块, 路径和你上面设置的 $UPLOAD_PATH 一致.

### 数据存取例子

在examples目录下已经提供了一个样例库，按照下面的步骤去挂载：

1. bin/lushan.sh >dev/null 2>&1 &
2. rsync examples/hdict_20150820131415 127.0.0.1::lushan_upload/1/
3. touch done.flg; rsync done.flg 127.0.0.1::lushan_upload/1/hdict_20150820131415/
3. echo -n -e "get 1-123456\r\n" | nc 127.0.0.1 9999

输出就是key 123456所对应的value.

解释一下每一步:

1. 第一步是启动lushan进程。lushan.sh 负责启动lushan进程, 每3秒检查一下 $UPLOAD_PATH 目录。 如果在$UPLOAD_PATH/$no 目录下有 hdcit_xxxx 个数的文件夹, 并且里面包含 done.flg文件。那么把它移到$HDB_PATH/$no 并且发送命令给lushan 进程把 hdict_xxxx 挂载到 $no编号下。
2. rsync 你的 hdict_xxxx 到 $UPLOAD_PATH/$no, 其中 $no 是这份数据集即将在lushan上挂载的编号。请使用rsync代替cp，不仅仅是因为rsync可以把数据传递到另外的机器上，而且rsync要么传输成功，要么失败，不会有一半文件的中间状态。 
3. 上面的步骤结束后，你的库仍然不会被自动挂载。因为lushan.sh不知道hdict_xxxx是否已经传输完成，还是只传输了其中的部分文件。所以，需要你rsync done.flg到hdict_xxxx文件夹下。 同时这样做也可以让你在保持一份数据在多个lushan server上同时上线，在下面的“如何搭建一个集群”中会说到。
4. 你可以使用memcached客户端查询你挂载的库。但更简单的方式是发送符合memcached协议的命令。上面的名利意味着你要查询编号为1的库中的key 123456。

### 计算模块例子

在modules 目录下提供了lproxy 的例子，对于一个请求的key, 先查询redis，redis里有的则直接返回，redis没有的查询本机挂载的库。在简单的情况下这个例子也可以用于生产环境，如果有更复杂的需求请修改此代码。

按下面步骤操作：

1. 创建一个文本文件x.txt，输入下面两行, 第一个tab之前的是key, 后面的是value，如下：

        168	hello lushan
		187	line 2

2. 用tools里的lushan_line_maker 转成lushan 的文件格式，假如为hdict_20180428192000，通过前面数据存取例子挂载到lushan 编号为1的目录下。

3. 在本机起一个redis, set dbno 为1, 然后加入一条记录, key为168, value为"hello redis"

4. 在hproxy目录下执行make, 把生成的hmodule.so和hmodule.conf放到hmod/15/1.0.0目录下, 修改hmodule.conf里的host 和port 为你部署的redis 的ip 和端口。

5. 执行：

		echo -ne "hmod_open /mnt/lushan/hmod/15/1.0.0/ 15\r\n" | nc 127.0.0.1 9999

	如果返回OPENED 则打开成功，否则检查libhiredis 是否在LD_LIBRARY_PATH 里。

6. 查询：

		echo -ne "get m15?k=1-168\r\n" | nc 127.0.0.1 9999
		VALUE m15?k=1-168 0 11
		hello redis
		END

		echo -ne "get m15?k=1-187\r\n" | nc 127.0.0.1 9999
		VALUE m15?k=1-187 0 6
		line 2
		END

	则如我们预期，redis 存在则返回redis 结果，否则查询lushan 数据。

7. 关闭一个模块：

		echo -ne "hmod_close 15\r\n" | nc 127.0.0.1 9999

	如果你的所有模块都没有全局变量，可以用hmod_open 把一个新的库直接替换旧的库，这样对线上的服务没有任何损失。

## hdict 格式

hdict 是lushan所挂载的库格式。他非常简单。在hdict_xxxx目录下有两个必须的文件，dat和idx。前者包含你的数据，后者是key到value在dat文件中位置偏移的映射。 定义:

	typedef struct {
        uint64_t key;
        uint64_t pos;
    } idx_t;

key 是64位无符号长整形，不包含库编号。 pos 是由value的长度和其在dat文件中的偏移组合而成的：

	pos = (length << 40) | offset;
	
idx文件必须是按照idx_t.key升序排列的。dat文件则不需要。你既可以对已经存在的一个dat文件创建索引，也可以在输出文件的时候同时生成索引。

有序文件在map-reduce计算模型中非常常见。你可以在hadoop中指定输出文件格式，从而生成hdict格式的库。例如如下命令：

	job.setOutputFormat(LushanFileOutputFormat.class);
	
## 统计状态数据

有三个命令可以获得统计状态数据: stats, info 和 hmod_info。前者输出全局数据，后两者输出每个库和每个模块的数据。

	echo -n -e "stats\r\n" | nc 127.0.0.1 9999
	
	STAT pid 13810
	STAT uptime 1435075686
	STAT curr_connections 1411
	STAT connection_structures 4061
	STAT cmd_get 2099151223
	STAT get_hits 3950240117
	STAT get_misses 2443878402
	STAT threads 16
	STAT timeouts 117
	STAT waiting_requests 0
	STAT ialloc_failed 0
	END

	echo -n -e "info\r\n" | nc 127.0.0.1 9999
	
	id                label state ref   num_qry  idx_num     open_time path
	----------------------------------------------------------------
	1   interest_CF_trends  OPEN  0   139922 18419392 150824-042654 /mnt/lushan/hdb/12/hdict_20150711204737
	2   interest_CF_trends  OPEN  0   190508 26175141 150824-050246 /mnt/lushan/hdb/12/hdict_20150711204737
	
	echo -ne "hmod_info\r\n" | nc 127.0.0.1 9999
	id                label state ref   num_qry     open_time path
	----------------------------------------------------------------
 	0                       OPEN  0  267130787 180419-174502 /mnt/lushan/hmod/0
 	5                       OPEN  0  336829974 180419-174503 /mnt/lushan/hmod/5

你可以利用lushan.php创建图形化的统计状态页面。

## 如何搭建一个集群?


如果你有mysql的经验，会很容易搭建一个简单的集群。首先，你要把你的数据分成若干组，通常是你机器数的某个倍数。然后考虑你要部署几套服务，通常是两套分布在不同的IDC。然后按照分组的规则去通过memcached客户端查询你的数据。

尽管非常简单，但lushan仍然提供了一个简单的框架去帮助你处理一些数据传输方面的细节，名字叫做transfer.py，可以帮助你：

1. 周期性检查hdict格式的库是否生成了。
2. 从hadoop上下载hdict格式的库到lushan所在机器，或者是从本地传输到lushan所在机器。在这里提供了插件可以让你在上线前检查你的数据是否合法，甚至你可以编写自己的插件来把原来“假的hdict格式文件”创建成“真的hdict格式文件”。
3. Rsync hdict 格式文件到相应编号的每个lushan服务副本上。都传输成功后，再rsync done.flg到每个lushan服务副本上，来保证同一份数据再不同服务上同步上线。

## lushan 非最佳实践

1. **域名.** 为每台lushan 的机器配置一个域名，这样替换机器的时候只需要切换域名指向即可，传数据环节配置域名可以做到不用动。
2. **数据是否按时上线的监控.** 通过lushan.php 你可以简单写一个脚本来检查一个库的打开时间是否符合这个库预期的更新时间，如果不符合可以报警。
3. **lushan 服务是否有重启.** lushan 作为key-value database使用的时候非常稳定，通常运行几年不出任何问题。但作为一个software framework来使用的时候则可能因为模块代码有bug而导致重启，通过php 获取 stats可以得到lushan 启动时间，从而判断是否有重启。如果重启则报警。
4. **跳闸.** lushan 重启加载hmodule的时候会判断该目录下是否存在hmodule.disable文件，如果存在则不会加载。你可以把新上线的hmodule目录下增加这个文件，如果该模块存在bug，则下次重启跳过，保障了其他模块的正常服务。

