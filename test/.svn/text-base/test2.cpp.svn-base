/***************************************************************************
 * 
 * Copyright (c) 2007 Baidu.com, Inc. All Rights Reserved
 * $Id: test2.cpp,v 1.4 2009/06/29 13:10:00 baonh Exp $ 
 * 
 **************************************************************************/



/**
 * @file test.cpp
 * @author baonh(baonenghui@baidu.com)
 * @version $Revision: 1.4 $ 
 * @brief ÑİÊ¾ÁËepending poolµÄÊ¹ÓÃ
 * 
 * <pre>
 * ÕâÀïÑİÊ¾ÁËÊ¹ÓÃependingpoolµÄ¶ÁĞ´»Øµ÷£¬epengdingpoolµÄ¶ÁĞ´»Øµ÷ÊÇÔÚ¼àÌıÏß³ÌÖĞÊµÏÖµÄ£¬
 * ËùÒÔ¶ÁĞ´Ê±ÇëÔÚSOCK_ACCEPTÊÂ¼şÖĞ½«sockÉèÖÃÎª·Ç¶ÂÈû×´Ì¬, ÕâÀïÑİÊ¾ÁË¶Ônshead°üµÄ·Ç
 * ¶ÂÈû¶ÁĞ´.·Ç¶ÂÈû¶ÁĞ´ĞèÒª¼ÇÂ¼¶ÁĞ´¹ı³ÌÖĞµÄ×´Ì¬,¸ù¾İ²»Í¬µÄ×´Ì¬¾ö¶¨ºóĞøµÄÊÂ¼ş,¿ÉÒÔÄ£ÄâÒì²½¶ÁĞ´
 * ÕâÀïÌá¹©ÁË¶Ônshead°ü½øĞĞ¶ÁĞ´µÄDEMO
 * </pre>
 *
 **/

#include <stdio.h>
#include <pthread.h>

#include "ul_log.h"
#include "ul_net.h"
#include "nshead.h"
#include "ependingpool.h"

ependingpool g_workpool;

#define THREAD_NUM 5     /**< ¿ªÆôÏß³ÌÊı  */
#define PORT 10001     /**< ¼àÌı¶Ë¿Ú    */

#define BUFF_SIZE 1024

//·Ç¶ÂÈû¶ÁĞ´nsheadÊı¾İ°üµÄ×´Ì¬
enum {
	NSHEAD_NONE = 0,

	NSHEAD_READ_HEAD,
	NSHEAD_READ_REQ,
	NSHEAD_READ_BUFF,

	NSHEAD_WRITE_HEAD,
	NSHEAD_WRITE_REQ,
	NSHEAD_WRITE_BUFF

};

/**
 * @brief ¼ÇÂ¼·Ç¶ÂÈû¶ÁĞ´nsheadÊı¾İ°üµÄ×´Ì¬
 **/
typedef struct _nshead_status_t {
	int status;			  /**< µ±Ç°¶ÁÈ¡nsheadµÄ×´Ì¬       */
	int cur_size;		  /**< µ±Ç°¶ÁÈ¡µÄÊı¾İ³¤¶È       */
} nshead_status_t;

/**
 * @brief ¶ÁĞ´Êı¾İÊ¹ÓÃµÄ½á¹¹£¬¼ÇÂ¼ÁËµ±Ç°sockµÄ×´Ì¬
 *
 **/
typedef struct _my_req_t {
	nshead_status_t sock_iostatus;
	nshead_t head;
	char *buff;
	int buff_size;
} my_req_t;

/**
 * @brief ·Ç¶ÂÈû¶ÁbuffÊı¾İ
 *
 * @param [in] sock   : socket
 * @param [in] buff   : Êı¾İbuff
 * @param [in/out] *cur_size   : ±¾´Î¶ÁÒÔÇ°ÒÑ¾­¶ÁÈ¡µÄÊı¾İ³¤¶È
 * @param [in] size   : ĞèÒª¶ÁÈ¡µÄ×Ü³¤¶È
 * @return  ¶ÁÈ¡µÄ½á¹û
 * @retval  -1 Ê§°Ü 0¶ÁÈ¡³É¹¦£¬¶ÁÈ¡½áÊø 1¶ÁÈ¡³É¹¦£¬µ«»¹ÓĞÊı¾İÎ´¶ÁÍê
 * @author baonh
**/
int noblock_read_buff(int sock, char *buff, int *cur_size, int size);

/**
 * @brief ·Ç¶ÂÈû¶ÁnsheadÊı¾İ°ü 
 *
 * @param [in/out] nshead_status   : ¼ÇÂ¼µ±Ç°¶ÁÈ¡µÄ×´Ì¬
 * @param [in] sock   : socket
 * @param [in/out] head   : nsheadÍ·
 * @param [in/out] req   : ×Ô¶¨ÒåÍ·
 * @param [in] req_size   : ×Ô¶¨ÒåÍ·³¤¶È
 * @param [in] buf   : ±ä³¤Êı¾İ»º³å(ÓÉÍâ²¿·ÖÅä), Êµ¼Ê¶ÁµÄ³¤¶ÈÎªbody_len - req_size
 * @param [in] buf_size   : ´æ·Å±ä³¤Êı¾İ»º³åµÄ×î´ó³¤¶È
 * @param [in] flags   : ÊÇ·ñcheck MAGICNUMÊı×Ö
 * @return  ¶ÁÊı¾İ½á¹û 
 * @retval  -1 ¶ÁÈ¡Ê§°Ü£¬ 0 ¶ÁÈ¡³É¹¦£¬È«²¿¶ÁÈ¡Íê±Ï 1 ¶ÁÈ¡³É¹¦µ«Ã»ÓĞ¶ÁÈ¡Íê±Ï 
 * @author baonh
**/
int noblock_nshead_read(nshead_status_t *nshead_status, int sock, nshead_t *head,void *req,
		size_t req_size, void *buf, size_t buf_size,unsigned flags = NSHEAD_CHECK_MAGICNUM);

/**
 * @brief ·Ç¶ÂÈûĞ´buffÊı¾İ
 *
 * @param [in] sock   : socket
 * @param [in] buff   : Êı¾İbuff
 * @param [in/out] *cur_size   : ±¾´ÎĞ´ÒÔÇ°ÒÑ¾­Ğ´µÄÊı¾İ³¤¶È
 * @param [in] size   : ĞèÒªĞ´µÄ×Ü³¤¶È
 * @return  Ğ´Êı¾İµÄ½á¹û
 * @retval  -1 Ê§°Ü 0Ğ´³É¹¦²¢Ğ´½áÊø 1Ğ´³É¹¦£¬µ«»¹ÓĞÊı¾İÎ´Ğ´Íê
 * @author baonh
**/
int noblock_write_buff(int sock, char *buff, int *cur_size, int size);

/**
 * @brief ·Ç¶ÂÈûĞ´nsheadÊı¾İ°ü 
 *
 * @param [in/out] nshead_status   : ¼ÇÂ¼µ±Ç°¶ÁÈ¡µÄ×´Ì¬
 * @param [in] sock   : socket
 * @param [in/out] head   : nsheadÍ·
 * @param [in/out] req   : ×Ô¶¨ÒåÍ·
 * @param [in] req_size   : ×Ô¶¨ÒåÍ·³¤¶È
 * @param [in] buf   : ±ä³¤Êı¾İ»º³å, Ğ´µÄ³¤¶ÈÎªbody_len - req_size
 * @param [in] flags   : ÊÇ·ñcheck MAGICNUMÊı×Ö
 * @return  Ğ´Êı¾İ½á¹û 
 * @retval  -1 Ğ´Ê§°Ü£¬ 0 Ğ´³É¹¦ÇÒÈ«²¿Ğ´Íê±Ï 1 Ğ´³É¹¦µ«Ã»ÓĞĞ´Íê±Ï 
 * @author baonh
**/
int noblock_nshead_write(nshead_status_t *nshead_status, int sock, nshead_t *head,
		void *req, size_t req_size, void *buf, unsigned flags = NSHEAD_CHECK_MAGICNUM);
	



/**
 * @brief ×Ô¶¨Òå¶Á»Øµ÷£
 *
 * @param [in] sock   : socket
 * @param [in] arg   : Ö¸ÏòÓësockÀ¦°óµÄÖ¸Õë
 * @return  int 
 * @author baonh
**/
int my_read(int sock, void **arg)
{
	int ret = -1;
	//»ñÈ¡¶ÁÈ¡buffµÄÖ¸Õë
	my_req_t *req = (my_req_t *)(*arg);
	ret = noblock_nshead_read(&(req->sock_iostatus), sock, &(req->head), NULL, 0,
			req->buff, req->buff_size);
	if (0 == ret) {
		//2´¥·¢SOCK_TODOÊÂ¼ş
		return 2;
	}
	return ret;
	
}
/**
 * @brief ×Ô¶¨ÒåĞ´»Øµ÷
 *
 * @param [in] sock   : socket
 * @param [in] arg   : Ö¸ÏòÓësockÀ¦°óµÄÖ¸Õë
 * @return  int 
 * @author baonh
**/

int my_write(int sock, void **arg)
{
	int ret = -1;	
	my_req_t *req = (my_req_t *)(*arg);
	
	ret = noblock_nshead_write(&(req->sock_iostatus), sock, &(req->head), NULL, 0,
			req->buff);

	if (0 == ret) {
		req->buff[0] = '\0';
		req->buff_size = BUFF_SIZE;
		//Ğ´Êı¾İÍê±Ï£¬Ê¹ÓÃ³¤Á¬½Ó£¬·µ»Ø0
		return 0;
	}
	return ret;
	

}

/**
 * @brief ×Ô¶¨ÒåACCEPT»Øµ÷£¬
 *
 * ÕâÀïÖ÷ÒªÄ¿µÄÊÇÎªÁËÉèÖÃ·Ç¶ÂÈûÄ£Ê½
 * 
 * @param [in] lis  : ¼àÌı¾ä±ú
 * @param [in] arg   : Ö¸ÏòÓësockÀ¦°óµÄÖ¸Õë,ÔÚÕâÀïÃ»ÓĞ×÷ÓÃ£¬ ÊÇ¸öNULL;
 * @return  int 
 * @author baonh
**/

int my_accept(int lis, void **arg)
{
	int work_sock;
	work_sock = ul_accept(lis, NULL, NULL);

	if (work_sock != -1) {
		//ÕâÀïĞèÒªÉèÖÃ·Ç¶ÂÈûÄ£Ê½£¬ ·ñÔò¶ÁĞ´µÄÊ±ºò»á±»hand×¡
		if (ul_setsocktonoblock(work_sock)) {
			close(work_sock);
			return -1;
		}
	}
	return work_sock;
	
}
/**
 * @brief ×Ô¶¨Òå´¦ÀíÊÂ¼ş
 *
 * ÓĞSOCK_TODO´¥·¢£¬ÔËĞĞÔÚ¼àÌıÏß³ÌÖĞ£¬Ö÷Òª½øĞĞÒ»Ğ©¼òµ¥µÄ´¦Àí
 * 
 * @param [in] sock  : socket
 * @param [in] arg   : Ö¸ÏòÓësockÀ¦°óµÄÖ¸Õë
 * @return  int 
 * @author baonh
**/


int my_todo(int sock, void **arg)
{
	my_req_t *req = (my_req_t *)(*arg);
	char *buff = req->buff;
	ul_writelog(UL_LOG_TRACE, "rev buff %s body_len %d", buff, req->head.body_len);

	//ÌîĞ´ĞèÒª·µ»Ø¸øclient¶ËµÄĞÅÏ¢
	snprintf(req->buff, req->buff_size, "this server , send buff is server");
	req->head.body_len = strlen(req->buff)+1;

		
	//·µ»Ø0½øĞĞĞ´²Ù×÷
	return 0;
}


int my_todo_ex(struct ependingpool::ependingpool_task_t *v)
{
	int sock = v->sock;
	int offset = v->offset;
	void *arg = v->arg;

//	ependingpool *pool = (ependingpool*)user_arg;
//	pool->fetch_handle_arg(offset, &arg);
	my_req_t *req = (my_req_t *)(arg);
	char *buff = req->buff;
	ul_writelog(UL_LOG_TRACE, "rev buff %s body_len %d", buff, req->head.body_len);
	//ÌîĞ´ĞèÒª·µ»Ø¸øclient¶ËµÄĞÅÏ¢
	snprintf(req->buff, req->buff_size, "this server , send buff is server");
	//req->head.body_len = strlen(req->buff)+1;
	req->head.body_len = 24;
	ul_writelog(UL_LOG_TRACE, "send body_len is %d offset %d", req->head.body_len, offset);

	//·µ»Ø0½øĞĞĞ´²Ù×÷,·µ»Ø3Ê²Ã´Ò²²»×ö
	return 0;
}

int nshead_status_init(nshead_status_t *nshead_status)
{
	nshead_status->cur_size = 0;
	nshead_status->status = NSHEAD_NONE;
	return 0;
}

/**
 * @brief ¶ÔsocketÀ¦°óµÄÖ¸Õë½øĞĞ³õÊ¼»¯ 
 *
 * @param [in] sock   : socket
 * @param [in] arg   : ÓësocketÀ¦°óµÄÖ¸Õë£¬ ³õÊ¼*arg ÎªNULL
 * @author baonh
**/
int my_init(int sock, void **arg)
{
	my_req_t *req = (my_req_t *)malloc(sizeof(my_req_t));
	if (NULL == req) {
		return -1;
	}
	//·ÖÅä¿Õ¼ä¸øbuffÊ¹ÓÃ
	req->buff = (char *)malloc(BUFF_SIZE);
	if (NULL == req->buff) {
		free (req);
		return -1;
	}

	nshead_status_init(&(req->sock_iostatus));
	req->buff_size = BUFF_SIZE;
	ul_writelog(UL_LOG_TRACE, "malloc buff for use");
	
	//½«Ö¸Õë¸³¸øsockÀ¦°óµÄÖ¸Õë
	*arg = req;
	return 0;
}
/**
 * @brief ¶ÔsocketÀ¦°óµÄÖ¸Õë½øĞĞÊÍ·Å
 *
 * @param [in] sock   : socket
 * @param [in] arg   : ÓësocketÀ¦°óµÄÖ¸Õë£¬ SOCK_CLEARÊÂ¼ş´¦ÀíÍêºó*arg»á±»ÉèÎªNULL
 * @author baonh
**/

int my_clear(int sock, void **arg)
{
	my_req_t *req = (my_req_t*)(*arg);
	if (req->buff != NULL) {
		free (req->buff);
	}
	if (req != NULL) {
		free (req);
	}
	return 0;
}

/**
 * @brief ·şÎñÖ÷Ïß³Ì£¬bind ¶Ë¿Ú£¬´¦Àí½ÓÊÜµ½µÄÇëÇó
 *
 * @see listen_thread_func 
 * @author baonh
 **/
int server_thread_func()
{
	int listen_sd;
	//bind PORT¶Ë¿Ú
	if ((listen_sd = ul_tcplisten(PORT, 128)) == -1) {
		ul_writelog (UL_LOG_FATAL, "create listening soket error! port:%d %m", PORT);
		exit(-1);
	}
	g_workpool.set_listen_fd(listen_sd);
	//Õë¶Ô²»Í¬µÄÊÂ¼şÊ¹ÓÃÏàÓ¦µÄ»Øµ÷º¯Êı
	g_workpool.set_event_callback(ependingpool::SOCK_ACCEPT, my_accept);
	//³õÊ¼»¯ÓësockÀ¦°óµÄÊı¾İ
	g_workpool.set_event_callback(ependingpool::SOCK_INIT, my_init);
    //ÊÍ·ÅÓësockÀ¦°óµÄÊı¾İ
	g_workpool.set_event_callback(ependingpool::SOCK_CLEAR, my_clear);
	//Ê¹ÓÃ·Ç¶ÂÈû¶ÁĞ´·½Ê½£¬Ä£ÄâÒì²½¶ÁĞ´
	g_workpool.set_event_callback(ependingpool::SOCK_READ, my_read);
	g_workpool.set_event_callback(ependingpool::SOCK_WRITE, my_write);
	//Êı¾İ´¦Àí×öµÄÊÂÇéºÜÉÙÃ»±ØÒªÊ¹ÓÃfetch½øĞĞ¶àÏß³Ì´¦Àí,Ê¹ÓÃtodoµ¥Ïß³Ì´¦Àí¾Í¿ÉÒÔÁË
//	g_workpool.set_event_callback(ependingpool::SOCK_TODO, my_todo);
	g_workpool.set_todo_event_ex(my_todo_ex, NULL);
	
	while (g_workpool.is_run()) {
		//¼ì²é³¬Ê±µÈÇé¿ö
		g_workpool.check_item();
	}
	return 0;
}

int main(int argc, char *argv[])
{
	ul_logstat_t log_state;
	log_state.spec = 0;
	log_state.to_syslog = 0;
	signal(SIGPIPE,SIG_IGN);
	//log_state.events = UL_LOG_TRACE;
	log_state.events = UL_LOG_DEBUG;
	//´ò¿ªÈÕÖ¾
	ul_openlog("./log", "test", &log_state, 1024);

	g_workpool.set_epoll_timeo(50);	
	//ÉèÖÃÁ¬½Ó³¬Ê±Ê±¼ä(Ãë), Ä¬ÈÏÎª1s
	g_workpool.set_conn_timeo(10);	
	//ÉèÖÃ¶Á³¬Ê±Ê±¼ä(Ãë), Ä¬ÈÏÎª1s
	g_workpool.set_read_timeo(10);	
	//ÉèÖÃĞ´³¬Ê±Ê±¼ä(Ãë), Ä¬ÈÏÎª1s
	g_workpool.set_write_timeo(10);	
	
	//ÉèÖÃ¿É´æ´¢socketµÄÊıÁ¿
	g_workpool.set_sock_num(2000);
	//ÉèÖÃÒÑ¾ÍĞ÷¶ÓÁĞµÄ³¤¶È
	g_workpool.set_queue_len(1000);
	//socketµÄÊıÁ¿ºÍÒÑ¾ÍĞ÷¶ÓÁĞµÄ³¤¶È¶¼±ØĞëÔÚÏß³Ì¿ªÊ¼Ç°ÉèÖÃ£¬²¢ÇÒÒª¶¯Ì¬ĞŞ¸Ä

	//Æô¶¯·şÎñ
	server_thread_func();

	ul_closelog(0);
}

int noblock_read_buff(int sock, char *buff, int *cur_size, int size)
{
	int ret;
	ul_writelog(UL_LOG_DEBUG, "before read cur_size:%d size:%d", *cur_size, size);
	if (*cur_size > size) {
		ul_writelog(UL_LOG_WARNING, "read buff error, cur_size[%d] > size[%d]",
				*cur_size, size);
		return -1;
	}
	while (1) {
		ret = read(sock, buff+(*cur_size), size-(*cur_size));
		//·Ç¶ÂÈû¶Á³öÏÖEINTRÊÇÕı³£ÏÖÏó, ÔÙ¶ÁÒ»´Î¼´¿É
		if (-1 == ret && (EINTR == errno))
			continue;
		break;
	}
	if (ret < 0) {
		if (EAGAIN == errno) {
			//³öÏÖEAGAIN±íÊ¾¶ÁµÄÊ±ºò£¬ÍøÂçµ×²ãÕ»ÀïÃ»Êı¾İ,·µ»Ø1µÈ´ıÏÂ´Î¶ÁÈ¡
			ul_writelog(UL_LOG_DEBUG, "read EAGAIN");
			return 1;
		}
		//¶ÁÊı¾İ³ö´íÁË
		ul_writelog(UL_LOG_WARNING, "read buff fail [%m]");
		return -1;
	}
	if (0 == ret) {
		ul_writelog(UL_LOG_DEBUG, "read 0, close it");
		//¶Áµ½0Ò»°ãÊÇ¶Ô¶ËcloseÖ÷¶¯¶Ï¿ª
		return -1;
	}

	*cur_size += ret;
	ul_writelog(UL_LOG_DEBUG, "after read cur_size:%d size:%d", *cur_size, size);
	if (*cur_size == size) {
		ul_writelog(UL_LOG_DEBUG, "read over total :%d", size);
		return 0;
	}
	//Êı¾İÎ´¶ÁÍê£¬ĞèÒª¼ÌĞø¶Á
	return 1;
}

int noblock_nshead_read(nshead_status_t *nshead_status, int sock, nshead_t *head,
		void *req, size_t  req_size, void *buf, size_t buf_size, unsigned int flags)
{
	int ret = 0;
	switch(nshead_status->status) {
		case NSHEAD_NONE:
			nshead_status->cur_size = 0;
			nshead_status->status = NSHEAD_READ_HEAD;

		case NSHEAD_READ_HEAD:
			ret = noblock_read_buff(sock, (char*)head, &(nshead_status->cur_size), sizeof(nshead_t));
			//¶ÁÈ¡Íê±Ï
			switch (ret) {
				case 0: //¶ÁÈ¡Íê±Ï
					/* ¼ì²é Magic Number */
					if (flags & NSHEAD_CHECK_MAGICNUM) {
						if (head->magic_num != NSHEAD_MAGICNUM) {
							ul_writelog(UL_LOG_WARNING,
									"<%u> nshead_read magic num mismatch: ret %x want %x",
									head->log_id, head->magic_num, NSHEAD_MAGICNUM);
							//magic_numÓĞ´í£¬ÈÏÎª¶ÁÈ¡Ê§°Ü
							goto fail;
						} 
					}
					/* ¶Ô body µÄ³¤¶È½øĞĞ¼ì²é, ¹ı³¤ºÍ¹ı¶Ì¶¼²»·ûºÏÒªÇó */
					if (head->body_len < req_size || head->body_len - req_size > buf_size) {
						ul_writelog(UL_LOG_WARNING,
								"<%u> nshead_read body_len error: req_size=%u buf_size=%u body_len=%u",
								head->log_id, (unsigned int)req_size, (unsigned int)buf_size,
								head->body_len);
						goto fail;
					}
					nshead_status->cur_size = 0;
					//ifÓĞreqÍ·Ôò¶ÁÈ¡reqÍ·
					if (req_size > 0) {
						nshead_status->status = NSHEAD_READ_REQ;
					} else {
						//Ã»ÓĞreqÍ·£¬Ö±½Ó¶ÁÈ¡ºóÃæµÄ¶¯Ì¬Êı¾İ
						nshead_status->status = NSHEAD_READ_BUFF;
					}
					break;	
					
				case 1: //»¹ÓĞÊı¾İÃ»ÓĞ¶ÁÈ¡Íê±Ï, ÏÈ·µ»Ø
					return 1;

				default: //¶ÁÈ¡Ê§°Ü
					goto fail;
			}

			
		case NSHEAD_READ_REQ:
			if (req_size > 0) {
			//¶ÁÈ¡×Ô¶¨ÒåÍ·
				ret = noblock_read_buff(sock, (char*)req, &(nshead_status->cur_size), req_size);
			} else {
				ret = 0;
			}
			
			//¶ÁÈ¡reqÍê±Ï
			if (0 == ret) {
				nshead_status->cur_size = 0;
				nshead_status->status = NSHEAD_READ_BUFF;
			}

		case NSHEAD_READ_BUFF:
			//nsheadÍ·ºóÃæÓĞ´ø±ä³¤Êı¾İ
			if (head->body_len > req_size) {
				//¶ÁÈ¡±ä³¤Êı¾İÊı¾İ
				ret = noblock_read_buff(sock, (char*)buf, &(nshead_status->cur_size), head->body_len - req_size);
			}

			if (0 == ret) {
				//È«²¿¶ÁÈ¡Íê±Ï£¬½«×´Ì¬ÉèÖÃÎªNONE
				nshead_status->status = NSHEAD_NONE;
			}

			break;

		default:
			break;
	}

	return ret;

fail:
	if (ret < 0) {
		ul_writelog(UL_LOG_WARNING, "read nshead fail");
		//¶ÁÈ¡Ê§°Ü£¬½«×´Ì¬ÉèÖÃÎªNONE
		nshead_status->status = NSHEAD_NONE;
	}
		
	return ret;
}

int noblock_write_buff(int sock, char *buff, int *cur_size, int size)
{
	int ret;

	while (1) {
		ret = write(sock, buff+(*cur_size), size-(*cur_size));
		if (-1 == ret && (EINTR == errno))
			continue;
		break;
	}
	if (ret < 0) {
		if (EAGAIN == errno) {
			return 1;
		}
		//Ğ´Êı¾İ³ö´íÁË
		ul_writelog(UL_LOG_WARNING, "write buff fail [%m]");
		return -1;
	}
	if (0 == ret) {
		return -1;
	}
	*cur_size += ret;
	if (*cur_size == size) {
		return 0;
	}
	return 1;
}


int noblock_nshead_write(nshead_status_t *nshead_status, int sock, nshead_t *head,
		void *req, size_t req_size, void *buf, unsigned int flags)
{
	int ret = 0;
	switch(nshead_status->status) {
		case NSHEAD_NONE:
			/* Ğ£Ñé size */
			if (head->body_len < req_size) {
				ul_writelog(UL_LOG_WARNING,
						"<%u> nshead_write body_len error: req_size=%zu body_len=%u",
						head->log_id, req_size, head->body_len);
				return NSHEAD_RET_EBODYLEN;
			}   
			if (flags & NSHEAD_CHECK_MAGICNUM) {
				head->magic_num = NSHEAD_MAGICNUM;
			}
			nshead_status->cur_size = 0;
			nshead_status->status = NSHEAD_WRITE_HEAD;

		case NSHEAD_WRITE_HEAD:
			ret = noblock_write_buff(sock, (char*)head, &(nshead_status->cur_size), sizeof(nshead_t));
			//Ğ´headÍ· Íê±Ï
			switch (ret) {
				case 0:
					nshead_status->cur_size = 0;
					//Èç¹ûÓĞreqÍ·ÔòĞ´reqÍ·
					if (req_size > 0) {
						nshead_status->status = NSHEAD_WRITE_REQ;
					} else {
						//Ã»ÓĞreqÍ·£¬Ö±½ÓĞ´ºóÃæµÄ¶¯Ì¬Êı¾İ
						nshead_status->status = NSHEAD_WRITE_BUFF;
					}
					break;	
					
				case 1: //»¹ÓĞÊı¾İÃ»ÓĞĞ´Íê±Ï, ÏÈ·µ»Ø
					return 1;

				default: //Ğ´Êı¾İÊ§°Ü
					goto fail;
			}

			
		case NSHEAD_WRITE_REQ:
			if (req_size > 0) {
			//Ğ´×Ô¶¨ÒåÍ·
				ret = noblock_write_buff(sock, (char*)req, &(nshead_status->cur_size), req_size);
			} else {
				ret = 0;
			}
			
			//Ğ´×Ô¶¨ÒåÍ·Íê±Ï
			if (0 == ret) {
				nshead_status->cur_size = 0;
				nshead_status->status = NSHEAD_WRITE_BUFF;
			}

		case NSHEAD_WRITE_BUFF:
			//nsheadÍ·ºóÃæÓĞ´ø±ä³¤Êı¾İ
			if (head->body_len > req_size) {
				//Ğ´±ä³¤Êı¾İÊı¾İ
				ret = noblock_write_buff(sock, (char*)buf, &(nshead_status->cur_size), head->body_len - req_size);
			}

			if (0 == ret) {
				//È«²¿¶ÁÈ¡Íê±Ï£¬½«×´Ì¬ÉèÖÃÎªNONE
				nshead_status->status = NSHEAD_NONE;
			}

			break;

		default:
			break;
	}

	return ret;

fail:
	if (ret < 0) {
		//¶ÁÈ¡Ê§°Ü£¬½«×´Ì¬ÉèÖÃÎªNONE
		nshead_status->status = NSHEAD_NONE;
	}
		
	return ret;
}



/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
