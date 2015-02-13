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
 * @brief ��ʾ��epending pool��ʹ��
 * 
 * <pre>
 * sock�������Ͷ��еĳ�����Ҫ���ȷ���,���ܶ�̬�ı�
 *
 * ���������̣߳�bind �˿ڣ�������Ӽ���epending
 *
 * ependingpool ���̰߳�ȫ��
 * ע��: 
 * ependingpoolʹ����epoolģ�ͣ�������Ҫ��2.6�ں˻�����. �����������2.4�ں˻�����
 * ����������socket��ʱ�������Ҫʹ��ulimit��ʽ����
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

#define THREAD_NUM 5     /**< �����߳���  */
#define PORT 9999       /**< �����˿�    */

#define BUSY "busy"
#define OK "OK"

#define BUFF_SIZE 1024
/**
 * @brief  �����߳� 
 *
 * @param [in] arg   : �̲߳���(����ΪNULL)
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

		//���Ѿ����Ķ�������ȡһ���ѽ����õ�����
		if (g_workpool.fetch_item(&offset, &client) != 0)
			continue;

		//����û�д������ݵĴ��룬�Ѿ�ʹ����SOCK_FETCH �Ļص���
	}
	ul_closelog_r(0);
	
	return NULL;
}

/**
 * @brief ʹ��SOCK_FETCH�¼�ʵ����test.cpp����ͬ�Ĺ���
 *
 * SOCK_FETCH �����ڹ����߳��У���дʱע���߳�ͬ�����⡣
 *
 * @param [in] sock   : ʹ�õľ��
 * @param [in] arg   : ��sock�����ָ��
 * @author baonh
**/
int my_fetch(int sock, void **arg)
{
	char buff[BUFF_SIZE];
	buff[0] = 0;
	int ret = -1;
	//ͨ�����ӻ�ȡ����
	if ((ret = read(sock, buff, BUFF_SIZE)) <= 0) {
		//����ʧ�ܣ��ر�����
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
	//2Ϊ�����ӣ�����1Ϊ������, ����3�����ependingpool�����ر�sock
	//ע�ⳤ���ӵ�ʱ���������TCP_NODELAYģʽ
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
 * @brief �������̣߳�bind �˿ڣ�������ܵ�������
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
	//bind PORT�˿�
	if ((listen_sd = ul_tcplisten(PORT, 128)) == -1) {
		ul_writelog (UL_LOG_FATAL, "create listening soket error! port:%d %m", PORT);
		exit(-1);
	}
	//ע�ⳤ���ӵ�ʱ���������TCP_NODELAYģʽ
	int on = 1;
	setsockopt(listen_sd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
	g_workpool.set_listen_fd(listen_sd);
	//��Բ�ͬ���¼�ʹ����Ӧ�Ļص�����
	//���ﶨ����SOCK_ACCEPT, �� FETCH�¼�
	g_workpool.set_event_callback(ependingpool::SOCK_ACCEPT, my_accept);
	g_workpool.set_event_callback(ependingpool::SOCK_FETCH, my_fetch);
	
	while (g_workpool.is_run()) {
		
		//��鳬ʱ�����
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
	//���ó�ʱʱ��(��), Ĭ��Ϊ1s
	g_workpool.set_conn_timeo(6);	
	//���ÿɴ洢socket������
	g_workpool.set_sock_num(2000);
	//�����Ѿ������еĳ���
	g_workpool.set_queue_len(1000);
	//socket���������Ѿ������еĳ��ȶ��������߳̿�ʼǰ���ã����Ҳ��ܶ�̬�޸�
	int thread_num = 5;
	pthread_t *tid_thread = (pthread_t*)malloc(sizeof(pthread_t)*thread_num);
	//����THREAD_NUM���߳�
	for (int i = 0; i < thread_num; i++) {
		pthread_create(&tid_thread[i], NULL, listen_thread_func, NULL);
	}

	//��������
	pthread_create(&tid, NULL, server_thread_func, NULL);

	for (int i = 0; i < thread_num; i++) {
		pthread_join(tid_thread[i], NULL);
	}
	pthread_join(tid, NULL);
	ul_closelog(0);
}

/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
