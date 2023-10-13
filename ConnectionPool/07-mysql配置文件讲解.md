````.ini
[client]    #客户端设置，即客户端默认的连接参数

# socket = /data/mysqldata/3306/mysql.sock    #用于本地连接的socket套接字
# 默认连接端口
port=3307    
#默认编码
default-character-set = utf8mb4 

[mysql]    #这个也是客户端设置
# 发生错误时不要发出蜂鸣声
no-beep    
#默认编码
default-character-set = utf8mb4    

[mysqld]    #服务端基本设置

# 下面的三个选项与SERVER_PORT互斥。
# skip-networking    仅允许本地（非TCP / IP）连接
# enable-named-pipe   将mysql通信方式改为命名管道
# shared-memory    （仅限Windows）服务器允许共享内存连接。

# shared-memory-base-name=MYSQL    （仅限Windows）用于共享内存连接的共享内存的名称。仅当启用服务器并启用shared_memory系统变量以支持共享内存连接时，此变量才适用

# socket=mysql=MYSQL    MySQL服务器将使用的管道
#MySQL服务器将侦听的TCP / IP端口
port=3307    
# MySQL安装根目录的路径。
basedir=D:\Software\mysql-8.0\mysql-8.0.20-winx64
# MySQL服务器数据目录的路径
datadir=D:\Software\mysql-8.0\mysql-8.0.20-winx64\data
#服务端默认编码（数据库级别）
character_set_server = utf8mb4 
# 连接到服务器时使用的默认身份验证插件
default_authentication_plugin=caching_sha2_password    
#在创建新表时将使用的默认存储引擎
default-storage-engine=INNODB    
#将SQL模式设置为strict
sql-mode="STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION"    

#一般查询日志和慢查询日志的设置
#日志选择文件类型
log-output=FILE    
#是否启用一般查询日志
general-log=0    
#一般查询日志文件的名称。
general_log_file="DESKTOP-PDK3N5G.log"    
#是否启用慢查询日志更多查看：https://dev.mysql.com/doc/refman/8.0/en/slow-query-log.html
slow-query-log=1    
#慢查询日志文件的名称。
slow_query_log_file="DESKTOP-PDK3N5G-slow.log"    
#慢查询日志包含执行时间超过long_query_time秒的SQL语句
long_query_time=10    

# log-bin = /data/mysqldata/3306/binlog/mysql-bin       #(windows一般不设)开启二进制日志功能，binlog数据位置 
#更多详情:https://dev.mysql.com/doc/refman/8.0/en/replication-options-binary-log.html
#错误记录。
log-error="DESKTOP-PDK3N5G.err"    
#更多详情:https://dev.mysql.com/doc/refman/8.0/en/error-log.html
#服务器ID。
server-id=121  
#安全文件权限.此变量用于限制数据导入和导出操作的影响.仅允许具有FILE权限的用户执行这些操作。
# secure-file-priv="C:/ProgramData/MySQL/MySQL Server 8.0/Uploads"    
#MySQL无论如何都会保留一个用于管理员（SUPER）登陆的连接，用于管理员连接数据库进行维护操作，即使当前连接数已经达到了max_connections.
#因此MySQL的实际最大可连接数为max_connections+1；这个参数实际起作用的最大值（实际最大可连接数）为16384，即该参数最大值不能超过16384，即使超过也以16384为准.
#增加max_connections参数的值，不会占用太多系统资源。系统资源（CPU、内存）的占用主要取决于查询的密度、效率等.该参数设置过小的最明显特征是出现”Too many connections”错误；
max_connections=16383
#所有线程的能打开表的总数量。增加此值会增加mysqld所需的文件描述符数。表描述符缓存大小，可减少文件打开/关闭次数；
table_open_cache=2000    
#它规定了内部内存临时表的最大值，每个线程都要分配。（实际起限制作用的是tmp_table_size和max_heap_table_size的最小值。）
tmp_table_size=32M    　
#thread_cahe_size线程池，线程缓存。用来缓存空闲的线程，以至于不被销毁，如果线程缓不存在的空闲线程，需要重新建立新连接，
thread_cache_size=10     
#允许MySQL在重新创建MyISAM索引时使用的临时文件的最大大小
myisam_max_sort_file_size=100G    
#当对MyISAM表执行repair table或创建索引时，用以缓存排序索引的大小；设置太小时可能会遇到 `myisam_sort_buffer_size is too small`
myisam_sort_buffer_size=16M    
#索引块的缓冲区大小，对MyISAM表性能影响最大的一个参数.决定索引处理的速度，尤其是索引读的速度。通过检查状态值Key_read_requests和Key_reads，可以知道key_buffer_size设置是否合理
key_buffer_size=16M    
#是MySQL读入缓冲区大小。对表进行顺序扫描的请求将分配一个读入缓冲区，MySQL会为它分配一段内存缓冲区。read_buffer_size变量控制这一缓冲区的大小。
#如果对表的顺序扫描请求非常频繁，并且你认为频繁扫描进行得太慢，可以通过增加该变量值以及内存缓冲区大小提高其性能。
read_buffer_size=16M

#是MySQL的随机读缓冲区大小。当按任意顺序读取行时(例如，按照排序顺序)，将分配一个随机读缓存区。进行排序查询时，MySQL会首先扫描一遍该缓冲，以避免磁盘搜索，
#提高查询速度，如果需要排序大量数据，可适当调高该值。但MySQL会为每个客户连接发放该缓冲空间，所以应尽量适当设置该值，以避免内存开销过大。
read_rnd_buffer_size=16M


#*** INNODB具体选项*** '更多详情参考:https://dev.mysql.com/doc/refman/8.0/en/innodb-parameters.html'

#InnoDB系统表空间数据文件的目录路径的公共部分。启用innodb_file_per_table时，此设置不会影响每表文件表空间的位置。默认值是MySQL数据目录。
#如果将值指定为空字符串，则可以为innodb_data_file_path指定绝对文件路径。
# innodb_data_home_dir=

#每次commit 日志缓存中的数据刷到磁盘中。通常设置为 1，意味着在事务提交前“日志已被写入磁盘”， 事务可以运行更长以及服务崩溃后的修复能力。
#如果你愿意减弱这个安全，或你运行的是比较小的事务处理，可以将它设置为 0 ，以减少写日志文件的磁盘 I/O。这个选项默认设置为 0。
innodb_flush_log_at_trx_commit=1


# #InnoDB 将日志写入日志磁盘文件前的缓冲大小。理想值为 1M 至 8M。大的日志缓冲允许事务运行时不需要将日志保存入磁盘而只到事务被提交(commit)。
#因此，如果有大的事务处理，设置大的日志缓冲可以减少磁盘I/O。
innodb_log_buffer_size=1M

#InnoDB 用来高速缓冲数据和索引内存缓冲大小。 更大的设置可以使访问数据时减少磁盘 I/O。
innodb_buffer_pool_size=8M    

#日志组中的每个日志文件的大小(单位 MB)。可以减少刷新缓冲池的次数，从而减少磁盘 I/O。但是大的日志文件意味着在崩溃时需要更长的时间来恢复数据。
innodb_log_file_size=32M    

#InnoDB尝试将InnoDB内并发的操作系统线程数小于或等于此变量给出的限制（InnoDB使用操作系统线程来处理用户事务）。
#一旦线程数达到此限制，就会在“先进先出”（FIFO）队列中将其他线程置于等待状态以供执行。等待锁的线程不计入并发执行线程的数量。
innodb_thread_concurrency=9    

# 增量大小（以MB为单位），用于在自动扩展的InnoDB系统表空间文件变满时扩展其大小。
innodb_autoextend_increment=64    

#InnoDB缓冲池分区的区域数。对于具有数千兆字节范围的缓冲池的系统，将缓冲池划分为单独的实例可以提高并发性，通过减少争用，因为不同的线程读写缓存页面。
innodb_buffer_pool_instances=8    

#确定可以同时进入InnoDB的线程数。
innodb_concurrency_tickets=5000   

#指定插入旧子列表的块在第一次访问后必须保留的长度（以毫秒为单位），然后才能将其移动到新的子列表。
innodb_old_blocks_time=1000    

# 它指定MySQL可以一次打开的最大.ibd文件数。最小值为10。
innodb_open_files=300    

# 启用此变量后，InnoDB会在元数据语句期间更新统计信息。
innodb_stats_on_metadata=0    

#当启用innodb_file_per_table（5.6.6及更高版本中的默认值）时，InnoDB会为每个新创建的表存储数据和索引.在单独的.ibd文件中，而不是在系统表空间中。
innodb_file_per_table=1    

# 指定如何生成和验证存储在InnoDB表空间的磁盘块中的校验和。
#使用以下值列表：0表示crc32,1表示strict_crc32，2表示innodb，3表示strict_innodb，4表示无，5表示strict_none。
innodb_checksum_algorithm=0


# /*****************************INNODB选项end**************************************/
#指定MySQL可能的连接数量。当MySQL主线程在很短的时间内得到非常多的连接请求，该参数就起作用，之后主线程花些时间（尽管很短）检查连接并且启动一个新线程。
#back_log参数的值指出在MySQL暂时停止响应新请求之前的短时间内多少个请求可以被存在堆栈中。
# back_log=80

#如果将此值设置为非零值，则每个flush_time秒都会关闭所有表以释放资源。将未刷新的数据同步到磁盘。此选项最适用于资源最少的系统。
flush_time=0    

# 用于普通索引扫描，范围索引扫描和不使用的连接的缓冲区的最小大小索引，从而执行全表扫描。
join_buffer_size=256K    

# 一个数据包或任何生成/中间字符串的最大大小
max_allowed_packet=4M    

 # 如果来自主机的多个连续连接请求在没有成功连接的情况下中断，则服务器会阻止该主机执行进一步的连接。
max_connect_errors=100   

#更改mysqld可用的文件描述符的数量。如果mysqld为您提供错误“打开太多文件”，则应尝试增加此选项的值。
open_files_limit=4161    

# 如果在SHOW GLOBAL STATUS输出中看到每秒多次sort_merge_passes，则可以考虑增加sort_buffer_size值以加快ORDER BY或GROUP BY操作，这些操作无法通过查询优化或改进的索引来改进。
sort_buffer_size=256K    

# 可以存储在定义高速缓存中的表定义（来自.frm文件）的数量。如果使用大量表，则可以创建大型表定义高速缓存以加快表的打开。表定义高速缓存占用较少与普通表缓存不同，它不使用文件描述符。最小值和默认值都是400。
table_definition_cache=1400    

# 指定基于行的二进制日志事件的最大大小（以字节为单位）。如果可能，将行分组为小于此大小的事件。该值应为256的倍数。
binlog_row_event_max_size=8K    

#如果此变量的值大于0，则复制从站将其master.info文件同步到磁盘。（在每个sync_master_info事件之后使用fdatasync（））。
sync_master_info=10000    

# 如果此变量的值大于0，则MySQL服务器将其中继日志同步到磁盘。（每次sync_relay_log写入中继日志后使用fdatasync（））。
sync_relay_log=10000    

# 如果此变量的值大于0，则复制从站将其relay-log.info文件同步到磁盘。（在每次sync_relay_log_info事务之后使用fdatasync（））。
sync_relay_log_info=10000    

#在开始时加载mysql插件。“plugin_x; plugin_y”。
plugin_load="mysqlx"    

# MySQL服务器的插件配置端口。
loose_mysqlx_port=33061    

````

