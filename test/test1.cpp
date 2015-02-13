/***************************************************************************
 * 
 * Copyright (c) 2007 Baidu.com, Inc. All Rights Reserved
 * $Id: test1.cpp,v 1.4 2009/06/29 13:10:00 baonh Exp $ 
 * 
 **************************************************************************/



/**
 * @file test.cpp
 * @author baonh(baonenghui@baidu.com)
 * @date 2007/12/03 17:21:53
 * @version $Revision: 1.4 $ 
 * @brief 演示了epending pool的使用
 * 
 * <pre>
 * sock的数量和队列的长度需要事先分配,不能动态改变
 *
 * 启动服务线程，bind 端口，获得连接加入epending
 *
 * ependingpool 是线程安全的
 * 注意: 
 * ependingpool使用了epool模型，运行需要的2.6内核环境下. 但编译可以在2.4内核机器上
 * 在遇到大量socket的时候程序需要使用ulimit方式启动
 * </pre>
 *
 **/

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include "ul_log.h"
#include "ul_net.h"
#include "ependingpool.h"

ependingpool g_workpool;

#define THREAD_NUM 5     /**< 开启线程数  */
#define PORT 9999       /**< 监听端口    */

#define BUSY "busy"
#define OK "OK"

#define BUFF_SIZE 1024
/**
 * @brief  工作线程 
 *
 * @param [in] arg   : 线程参数(这里为NULL)
 * @see server_thread_func 
 * @author baonh
 **/

void *listen_thread_func(void *arg)
{
	int		offset = -1;
	int     client = -1;
	ul_logstat_t log_state;
	log_state.spec = 0;
	log_state.to_syslog = 0;
	log_state.events = UL_LOG_TRACE;
	//log_state.events = UL_LOG_DEBUG;	
	if (ul_openlog_r("listen_thread_func", &log_state) < 0) {
		return NULL;
	}
	while (g_workpool.is_run()) {

		//从已就绪的队列中中取一个已建立好的连接
		if (g_workpool.fetch_item(&offset, &client) != 0)
			continue;

		//这里没有处理数据的代码，已经使用了SOCK_FETCH 的回调。
	}
	ul_closelog_r(0);
	
	return NULL;
}

/**
 * @brief 使用SOCK_FETCH事件实现与test.cpp中相同的功能
 *
 * SOCK_FETCH 触发在工作线程中，编写时注意线程同步问题。
 *
 * @param [in] sock   : 使用的句柄
 * @param [in] arg   : 与sock捆绑的指针
 * @author baonh
**/
int my_fetch(int sock, void **arg)
{
	char buff[BUFF_SIZE];
	buff[0] = 0;
	int ret = -1;
	//通过连接获取数据
	if ((ret = read(sock, buff, BUFF_SIZE)) <= 0) {
		//连接失败，关闭连接
		ul_writelog(UL_LOG_WARNING, "read socket (%d) fail %m", sock);
		return -1;
	}
	ul_writelog(UL_LOG_TRACE, "revice %.*s ", BUFF_SIZE, buff);

	snprintf(buff, sizeof(buff), "%s", OK);

	if ((ret=write(sock, buff, BUFF_SIZE)) < 0) {
		ul_writelog(UL_LOG_WARNING, "write %s fail! %m\n", OK);
		return -1;
	}

	ul_writelog(UL_LOG_TRACE, "send ok");
	//2为长连接，返回1为短连接, 返回3清除出ependingpool但不关闭sock
	//注意长连接的时候最好设置TCP_NODELAY模式
	return 2;
}

int my_accept(int lis, void **arg)
{
	int work_sock;
	while ((work_sock = accept(lis, NULL, NULL)) < 0) {
		if (errno == ECONNABORTED)
			continue;
		return -1;
	}
	return work_sock;
	
}


/**
 * @brief 服务主线程，bind 端口，处理接受到的请求
 *
 * @see listen_thread_func 
 * @author baonh
 **/
void *server_thread_func(void *arg)
{
	ul_logstat_t log_state;
	log_state.spec = 0;
	log_state.to_syslog = 0;
	log_state.events = UL_LOG_TRACE;
	//log_state.events = UL_LOG_DEBUG;	
	if (ul_openlog_r("listen_thread_func", &log_state) < 0) {
		return NULL;
	}
	
	int listen_sd;
	//bind PORT端口
	if ((listen_sd = ul_tcplisten(PORT, 128)) == -1) {
		ul_writelog (UL_LOG_FATAL, "create listening soket error! port:%d %m", PORT);
		exit(-1);
	}
	//注意长连接的时候最好设置TCP_NODELAY模式
	int on = 1;
	setsockopt(listen_sd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
	g_workpool.set_listen_fd(listen_sd);
	//针对不同的事件使用相应的回调函数
	//这里定义了SOCK_ACCEPT, 和 FETCH事件
	g_workpool.set_event_callback(ependingpool::SOCK_ACCEPT, my_accept);
	g_workpool.set_event_callback(ependingpool::SOCK_FETCH, my_fetch);
	
	while (g_workpool.is_run()) {
		
		//检查超时等情况
		g_workpool.check_item();
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t tid;
	ul_logstat_t log_state;
	log_state.spec = 0;
	log_state.to_syslog = 0;
	signal(SIGPIPE,SIG_IGN);
	log_state.events = UL_LOG_TRACE;
	//log_state.events = UL_LOG_DEBUG;
	ul_openlog("./log", "test", &log_state, 1024*1024);

	g_workpool.set_epoll_timeo(50);	
	//设置超时时间(秒), 默认为1s
	g_workpool.set_conn_timeo(6);	
	//设置可存储socket的数量
	g_workpool.set_sock_num(2000);
	//设置已就绪队列的长度
	g_workpool.set_queue_len(1000);
	//socket的数量和已就绪队列的长度都必须在线程开始前设置，并且不能动态修改
	int thread_num = 5;
	pthread_t *tid_thread = (pthread_t*)malloc(sizeof(pthread_t)*thread_num);
	//开辟THREAD_NUM个线程
	for (int i = 0; i < thread_num; i++) {
		pthread_create(&tid_thread[i], NULL, listen_thread_func, NULL);
	}

	//启动服务
	pthread_create(&tid, NULL, server_thread_func, NULL);

	for (int i = 0; i < thread_num; i++) {
		pthread_join(tid_thread[i], NULL);
	}
	pthread_join(tid, NULL);
	ul_closelog(0);
}

/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
