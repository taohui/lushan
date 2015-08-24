# Welcome to lushan!

## What is lushan?
<br />
lushan is a light weight key-value database, compatible with memached protocol and can mount multiple databases. 

It allows you to set up a cluster with serval hosts just like memcached.

## lushan features

1. **Memcached protocol.** lushan is compatible with memcached protocol, your clients can comminicate with lushan through various of memcached clients in different programming languages.
2. **Multiple databases.** lushan can mount mutiple databases, you don't need to deploy another lushan cluster or restart it when you add a database.
3. **Statistics chart.** lushan has detailed statistics, it's very useful for operating.
4. **Fast.** lushan is carefully designed, with I/O multiplex communication model, less memory copy and detailed timeout handle.
5. **Hadoop.** lushan allows you output it's database file format through LushanFileOutputFormat.java.

## Why I developed lushan?

About two years ago, when I was developing "Missed Weibo Recommendation". I had to deploy different storage system for different recommendation algorithms when I wanted to compare their effects. It was too low to do that, so I developed lushan, which can free you from tedious things. It's really cool!

Now, lushan is a basic infrastructure in advertisement and recommendation of Weibo.com. We have two clusters with 12 servers each, and it serves terabytes of data and billions of requests per day.

## Getting start

### Requirement

libevent 1.4 or above.

### Compile and install

1. Open Makefile, change LIBEVENT_HOME to you libevent install directory.
2. make
3. cp lushan to bin directory
4. Open conf/lushan.conf, change HDB_PATH and UPLOAD_PATH accord your install directory.
5. Set a lushan_upload module in rsyncd.conf, path is $UPLOAD_PATH you set above.

### Run the example

There is already an example database in examples directory. Fillow these steps:

1. bin/lushan.sh >dev/null 2>&1 &
2. rsync examples/hdict_20150820131415 127.0.0.1::lushan_upload/1/
3. touch done.flg; rsync done.flg 127.0.0.1::lushan_upload/1/hdict_20150820131415/
3. echo -n -e "get 1-123456\r\n" | nc 127.0.0.1 9999

The output is your value associated with key 123456.

Let's explain each step:

1. The first step is start your lushan. lushan.sh will start your lushan progress, scan $UPLOAD_PATH every 3 seconds. If there is a hdcit_xxxx directory in $UPLOAD_PATH/$no, and contains done.flg. Then it will move the hdict_xxxx directory to $HDB_PATH/$no and send a command to lushan progress to mount this hdict_xxxx to database $no.
2. rsync your hdict_xxxx to $UPLOAD_PATH/$no, where $no is your database no of this dataset. Always use rsync instead of cp, not only because rsync is a remote tranfer tool, but also it don't has middle state of a file. The next step is the same.
3. After the above step, it won't be mounted automatically. Because, lushan.sh dosen't know if the hdict_xxxx is completly transfered. So, you must rsync a done.flg to the hdict_xxxx directory.
4. You can query your database with memcached client. But the most simple way is send a message to lushan progress according memcached protocol. The query 1-123456 means query key 12345 in database no 1.

## hdict format

hdict format is very simple. There are two files in hdict directory. The first one is dat, which contains your dat. Another is idx, which contains key to value offset mapping. It defines:

	typedef struct {
        uint64_t key;
        uint64_t pos;
    } idx_t;

key is 64 bit number without database no. pos is composed by value offset in dat file and it's length.

	pos = (length << 40) | offset;
	
The idx file must be sorted by idx_t.key in ascending order. The dat file don't need. You can generate the idx file with an existing dat file, or generate it in the same time when you output your dat file.

Sorted file is very common in map-reduce computing model. So you can output hdict format simply by:

	job.setOutputFormat(LushanFileOutputFormat.class);
	
## Statistics chart

There are two commands to get statistics: stats and info. The first one output global statistics and another one output statistics of each database.

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
	
You can create your charts in html with the help of lushan.php.
	