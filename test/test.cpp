/***************************************************************************
 * 
 * Copyright (c) 2007 Baidu.com, Inc. All Rights Reserved
 * $Id: test.cpp,v 1.4 2008/09/22 06:57:46 baonh Exp $ 
 * 
 **************************************************************************/



/**
 * @file test.cpp
 * @author baonh(baonenghui@baidu.com)
 * @version $Revision: 1.4 $ 
 * @brief 演示了epending pool的使用
 * 
 * <pre>
 * sock的数量和队列的长度需要事先分配,不能动态改变
 *
 * 启动服务线程，bind 端口，获得连接加入epending
 * pool中，连接加入成功再放入就绪队列,由另外的线程处理实际的请求
 *
 * ependingpool 是线程安全的
 * 注意: 
 * ependingpool使用了epool模型，运行需要的2.6内核环境下.
 * 对于需要在2.4内核下编译的程序,需要使用epoll.c编译出来的epoll.o(要用gcc,不能用g++),但是只能运行在2.6的机器上
 * 在遇到大量socket的时候程序需要使用ulimit方式启动
 * </pre>
 *
 **/

#include <pthread.h>
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
 * @brief  处理实际的请求 
 *
 * @param [in] arg   : 线程参数(这里为NULL)
 * @see server_thread_func 
 * @author baonh
 * @date 2007/12/03 16:23:57
 **/

void *listen_thread_func(void *arg)
{
	int		offset = -1;
	int     client = -1;
//	int     handle;
	char    buff[BUFF_SIZE];
	int		ret;
	struct timeval timeout = {1, 0};
	ul_logstat_t log_state;
	log_state.spec = 0;
	log_state.to_syslog = 0;
	log_state.events = UL_LOG_TRACE;
	///log_state.events = UL_LOG_DEBUG;	
	if (ul_openlog_r("listen_thread_func", &log_state) < 0) {
		return NULL;
	}
	
	while (g_workpool.is_run()) {

			//从已就绪的队列中中取一个已建立好的连接
			//handle, client是引用类型
			if (g_workpool.fetch_item(&offset, &client) != 0)
				continue;
			setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeval)); 
			setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeval));
			buff[0] = 0;
			//通过连接获取数据
			if ((ret = recv(client, buff, BUFF_SIZE, MSG_WAITALL)) <= 0) {
				//连接失败，关闭连接
				ul_writelog(UL_LOG_WARNING, "read socket (%d) fail %m", client);
				g_workpool.reset_item(offset, false);
				continue;
			}
//			ul_writelog(UL_LOG_TRACE, "revice %.*s ", BUFF_SIZE, buff);

			
			snprintf(buff, sizeof(buff), "%s", OK);
			
			if ((ret=send(client, buff, BUFF_SIZE, MSG_WAITALL)) < 0) {
				ul_writelog(UL_LOG_WARNING, "write %s fail! %m\n", OK);
				g_workpool.reset_item(offset, false);
				continue;
			}
			
			ul_writelog(UL_LOG_TRACE, "send ok");
			//短连接关闭	
			g_workpool.reset_item(offset, false);
	}
	ul_closelog_r(0);
	
	return NULL;
}

int my_accept(int lis)
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
 * @date 2007/12/03 16:22:23
 **/
int server_thread_func()
{
	int listen_sd;
	//bind PORT端口
	if ((listen_sd = ul_tcplisten(PORT, 128)) == -1) {
		ul_writelog (UL_LOG_FATAL, "create listening soket error! port:%d %m", PORT);
		exit(-1);
	}
	g_workpool.set_listen_fd(listen_sd);
	while (g_workpool.is_run()) {
		
		//检查超时等情况
		g_workpool.check_item();
	}
	return 0;
}

int main(int argc, char *argv[])
{
	pthread_t tid[THREAD_NUM];
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
	//开辟THREAD_NUM个线程
	for (int i = 0; i < THREAD_NUM; ++i) {
		pthread_create(&tid[i], NULL, listen_thread_func, NULL);
	}

	//启动服务
	server_thread_func();
	for (int i = 0; i < THREAD_NUM; ++i) {
		pthread_join(tid[i], NULL);
	}
	ul_closelog(0);
}

/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
