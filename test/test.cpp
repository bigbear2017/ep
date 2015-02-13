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
 * @brief ��ʾ��epending pool��ʹ��
 * 
 * <pre>
 * sock�������Ͷ��еĳ�����Ҫ���ȷ���,���ܶ�̬�ı�
 *
 * ���������̣߳�bind �˿ڣ�������Ӽ���epending
 * pool�У����Ӽ���ɹ��ٷ����������,��������̴߳���ʵ�ʵ�����
 *
 * ependingpool ���̰߳�ȫ��
 * ע��: 
 * ependingpoolʹ����epoolģ�ͣ�������Ҫ��2.6�ں˻�����.
 * ������Ҫ��2.4�ں��±���ĳ���,��Ҫʹ��epoll.c���������epoll.o(Ҫ��gcc,������g++),����ֻ��������2.6�Ļ�����
 * ����������socket��ʱ�������Ҫʹ��ulimit��ʽ����
 * </pre>
 *
 **/

#include <pthread.h>
#include <stdio.h>
#include "ul_log.h"
#include "ul_net.h"
#include "ependingpool.h"

ependingpool g_workpool;

#define THREAD_NUM 5     /**< �����߳���  */
#define PORT 9999       /**< �����˿�    */

#define BUSY "busy"
#define OK "OK"

#define BUFF_SIZE 1024
/**
 * @brief  ����ʵ�ʵ����� 
 *
 * @param [in] arg   : �̲߳���(����ΪNULL)
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

			//���Ѿ����Ķ�������ȡһ���ѽ����õ�����
			//handle, client����������
			if (g_workpool.fetch_item(&offset, &client) != 0)
				continue;
			setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeval)); 
			setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeval));
			buff[0] = 0;
			//ͨ�����ӻ�ȡ����
			if ((ret = recv(client, buff, BUFF_SIZE, MSG_WAITALL)) <= 0) {
				//����ʧ�ܣ��ر�����
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
			//�����ӹر�	
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
 * @brief �������̣߳�bind �˿ڣ�������ܵ�������
 *
 * @see listen_thread_func 
 * @author baonh
 * @date 2007/12/03 16:22:23
 **/
int server_thread_func()
{
	int listen_sd;
	//bind PORT�˿�
	if ((listen_sd = ul_tcplisten(PORT, 128)) == -1) {
		ul_writelog (UL_LOG_FATAL, "create listening soket error! port:%d %m", PORT);
		exit(-1);
	}
	g_workpool.set_listen_fd(listen_sd);
	while (g_workpool.is_run()) {
		
		//��鳬ʱ�����
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
	//���ó�ʱʱ��(��), Ĭ��Ϊ1s
	g_workpool.set_conn_timeo(6);	
	//���ÿɴ洢socket������
	g_workpool.set_sock_num(2000);
	//�����Ѿ������еĳ���
	g_workpool.set_queue_len(1000);
	//socket���������Ѿ������еĳ��ȶ��������߳̿�ʼǰ���ã����Ҳ��ܶ�̬�޸�
	//����THREAD_NUM���߳�
	for (int i = 0; i < THREAD_NUM; ++i) {
		pthread_create(&tid[i], NULL, listen_thread_func, NULL);
	}

	//��������
	server_thread_func();
	for (int i = 0; i < THREAD_NUM; ++i) {
		pthread_join(tid[i], NULL);
	}
	ul_closelog(0);
}

/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
