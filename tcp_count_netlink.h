#ifndef __ZABBIX_TCP_COUNT_NETLINK_H
#define __ZABBIX_TCP_COUNT_NETLINK_H


#include "sysinc.h"
#include "module.h"
#include "common.h"
#include "log.h"


#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/inet_diag.h>
#include <netinet/in.h>

#define MODULE_NAME "tcp_count.so"

#define TCP_STATE_NUM 12

extern int get_port_count(int *, int , int , int , int *);

enum
{
  TCP_ESTABLISHED = 1,
  TCP_SYN_SENT,
  TCP_SYN_RECV,
  TCP_FIN_WAIT1,
  TCP_FIN_WAIT2,
  TCP_TIME_WAIT,
  TCP_CLOSE,
  TCP_CLOSE_WAIT,
  TCP_LAST_ACK,
  TCP_LISTEN,
  TCP_CLOSING   /* now a valid state */
};


#endif /* __ZABBIX_TCP_COUNT_NETLINK_H */


