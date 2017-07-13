/* 
 * zabbix_TCP-state-count-module
 *
 * zabbix loadable module for aggregating TCP sessions.
 *
 *
 * IWATA mutsumi
 *
 */

#include "tcp_count_netlink.h"


int state_to_flag(int num)
{

	if( num == 0 ){
		return 0xfff;
	}else if( num > TCP_CLOSING ){
		return -1;
	}

	return 1<<(num);
}


int open_sock(void)
{

	zabbix_log(LOG_LEVEL_DEBUG, "[%s] In %s() %s:%d",
	           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__);

	return socket(AF_NETLINK, SOCK_RAW, NETLINK_INET_DIAG);
}


ssize_t send_request(int fd, int src_port, int dst_port, int port_state)
{

	struct sockaddr_nl nladdr;
	struct msghdr msg;
	struct iovec iov;

	struct send_msg {
		struct nlmsghdr nlh;
		struct inet_diag_req req_diag;
	} send_msg;


	zabbix_log(LOG_LEVEL_DEBUG, "[%s] In %s() %s:%d",
	           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__);

	memset(&nladdr, 0, sizeof(nladdr) );
	memset(&send_msg.req_diag, 0, sizeof(send_msg.req_diag));

	send_msg.nlh.nlmsg_len = sizeof(send_msg);
	send_msg.nlh.nlmsg_type = TCPDIAG_GETSOCK;
	send_msg.nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST;
	send_msg.nlh.nlmsg_seq = 123123;
	send_msg.nlh.nlmsg_pid = 0;

	send_msg.req_diag.idiag_states = state_to_flag(port_state);
	send_msg.req_diag.id.idiag_sport = htons(src_port);
	send_msg.req_diag.id.idiag_dport = htons(dst_port);

	nladdr.nl_family = AF_NETLINK;

	iov.iov_base = &send_msg;
	iov.iov_len = sizeof(send_msg);
	
	msg = (struct msghdr) {	
	        .msg_name = (void*)&nladdr,
	        .msg_namelen = sizeof(nladdr),
	        .msg_iov = &iov,
	        .msg_iovlen = 1,
	};

	return sendmsg(fd, &msg, 0);
}


int recv_and_count(int fd)
{

	int count_state = 0;

	int recv_status;
	struct sockaddr_nl nladdr;
	struct iovec iov;
	struct msghdr rcv_msg;
	char buf[32768];
	int msglen;
	
	struct nlmsghdr *h ;
	struct inet_diag_msg *r;


	zabbix_log(LOG_LEVEL_DEBUG, "[%s] In %s() %s:%d",
	           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__);

	memset(&nladdr, 0, sizeof(nladdr) );
	nladdr.nl_family = AF_NETLINK;

	iov.iov_base = &buf;
	iov.iov_len = sizeof(buf);

	rcv_msg = (struct msghdr) {	
	        .msg_name = (void*)&nladdr,
	        .msg_namelen = sizeof(nladdr),
	        .msg_iov = &iov,
	        .msg_iovlen = 1,
	};

	int count = 0;
	int found_done = 0;


	while(found_done != 1){

		recv_status = recvmsg(fd, &rcv_msg, 0);

		if( recv_status < 0 ){
			zabbix_log(LOG_LEVEL_DEBUG, "[%s] In %s() %s:%d recv_status<0",
			           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__);

			return -1;
		}

		msglen = recv_status;
		h = (struct nlmsghdr*)buf;
		while( NLMSG_OK(h, msglen) ){
			count++;

			if(h->nlmsg_type == NLMSG_DONE){
				found_done=1;
				break;
			}

			r = NLMSG_DATA(h);

			count_state ++;

			h = NLMSG_NEXT(h, msglen);
		}

	}

	return count_state;
}


int get_port_count(int *ret_count, int src_port, int dst_port, int port_state)
{

	int sock_fd;


	zabbix_log(LOG_LEVEL_DEBUG, "[%s] In %s() %s:%d",
	           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__);

	sock_fd = open_sock();
	if( sock_fd < 0 ){

		zabbix_log(LOG_LEVEL_DEBUG, "[%s] In %s() %s:%d open_sock() fail",
	                   MODULE_NAME, __FUNCTION__, __FILE__, __LINE__);

		return SYSINFO_RET_FAIL;
	}

	if( 0 > send_request( sock_fd, src_port, dst_port, port_state ) ){

		zabbix_log(LOG_LEVEL_DEBUG, "[%s] In %s() %s:%d send_request() fail",
	                   MODULE_NAME, __FUNCTION__, __FILE__, __LINE__);

		close(sock_fd);
		return SYSINFO_RET_FAIL;
	}

	*ret_count = recv_and_count( sock_fd );

	close(sock_fd);

	return SYSINFO_RET_OK;
}

