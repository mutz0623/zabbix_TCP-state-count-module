/* 
 * zabbix_TCP-state-count-module
 *
 * zabbix loadable module for aggregating TCP sessions.
 *
 *
 * mutsumi Iwata
 *
 */


/*
** Zabbix
** Copyright (C) 2001-2014 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#include "sysinc.h"
#include "module.h"
#include "common.h"
#include "log.h"
#include "zbxjson.h"

#include "tcp_count_netlink.h"


extern unsigned char program_type ;

#define MODULE_NAME "tcp_count.so"


/* the variable keeps timeout setting for item processing */
static int	item_timeout = 0;

int	zbx_module_NET_TCP_COUNT(AGENT_REQUEST *request, AGENT_RESULT *result);
int	zbx_module_NET_TCP_COUNT_BULK(AGENT_REQUEST *request, AGENT_RESULT *result);

static ZBX_METRIC keys[] =
/*      KEY                     FLAG		FUNCTION        	TEST PARAMETERS */
{
	{"net.tcp.count",	CF_HAVEPARAMS,	zbx_module_NET_TCP_COUNT, "80,0,LISTEN"},
	{"net.tcp.count.bulk",	CF_HAVEPARAMS,	zbx_module_NET_TCP_COUNT_BULK, "80,0"},
	{NULL}
};

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_api_version                                           *
 *                                                                            *
 * Purpose: returns version number of the module interface                    *
 *                                                                            *
 * Return value: ZBX_MODULE_API_VERSION_ONE - the only version supported by   *
 *               Zabbix currently                                             *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_api_version(void)
{
	return ZBX_MODULE_API_VERSION_ONE;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_item_timeout                                          *
 *                                                                            *
 * Purpose: set timeout value for processing of items                         *
 *                                                                            *
 * Parameters: timeout - timeout in seconds, 0 - no timeout set               *
 *                                                                            *
 ******************************************************************************/
void	zbx_module_item_timeout(int timeout)
{
	item_timeout = timeout;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_item_list                                             *
 *                                                                            *
 * Purpose: returns list of item keys supported by the module                 *
 *                                                                            *
 * Return value: list of item keys                                            *
 *                                                                            *
 ******************************************************************************/
ZBX_METRIC	*zbx_module_item_list(void)
{
	return keys;
}


/******************************************************************************
 *                                                                            *
 * Function: zbx_module_NET_TCP_COUNT                                         *
 *                                                                            *
 * Purpose: Aggregate TCP connections by port number                          *
 *                                                                            *
 * Parameters: source port                                                    *
 *             destination port                                               *
 *             state (LISTEN,ESTABLISHED,TIME-WAIT,etc)                       *
 *                                                                            *
 * Return value: SYSINFO_RET_FAIL - function failed, item will be marked      *
 *                                 as not supported by zabbix                 *
 *               SYSINFO_RET_OK - success                                     *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_NET_TCP_COUNT(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	int	src_port = 0;
	int	dst_port = 0;
	int	port_state = 0;
	char	*port_state_tmp = NULL;

	int	count = 0;
	int	ret = SYSINFO_RET_FAIL;

	zabbix_log(LOG_LEVEL_DEBUG, "[%s] param num [%d] (%s:%d)",
	           MODULE_NAME, request->nparam, __FILE__, __LINE__ );
	if (request->nparam == 0){
		src_port = 0;
		dst_port = 0;
		port_state = 0;

	}else if (request->nparam == 1){

		src_port = atoi( get_rparam(request, 0) );

		dst_port = 0;
		port_state = 0;

	}else if (request->nparam == 2){

		src_port = atoi( get_rparam(request, 0) );
		dst_port = atoi( get_rparam(request, 1) );

		port_state = 0;

	}else if (request->nparam == 3){

		src_port = atoi( get_rparam(request, 0) );
		dst_port = atoi( get_rparam(request, 1) );

		port_state_tmp = get_rparam(request, 2);
		zabbix_log(LOG_LEVEL_DEBUG, "[%s] specified 3rd param str [%s] (%s:%d)",
		           MODULE_NAME, port_state_tmp, __FILE__, __LINE__ );

		if (0 == strcmp(port_state_tmp, "ESTABLISHED")) {
			port_state = TCP_ESTABLISHED;

		}else if(0 == strcmp(port_state_tmp, "SYN-SENT")) {
			port_state = TCP_SYN_SENT;

		}else if(0 == strcmp(port_state_tmp, "SYN_RECV")) {
			port_state = TCP_SYN_RECV;

		}else if(0 == strcmp(port_state_tmp, "FIN_WAIT1")) {
			port_state = TCP_FIN_WAIT1;

		}else if(0 == strcmp(port_state_tmp, "FIN_WAIT2")) {
			port_state = TCP_FIN_WAIT2;

		}else if(0 == strcmp(port_state_tmp, "TIME_WAIT")) {
			port_state = TCP_TIME_WAIT;

		}else if(0 == strcmp(port_state_tmp, "CLOSE")) {
			port_state = TCP_CLOSE;

		}else if(0 == strcmp(port_state_tmp, "CLOSE_WAIT")) {
			port_state = TCP_CLOSE_WAIT;

		}else if(0 == strcmp(port_state_tmp, "LAST_ACK")) {
			port_state = TCP_LAST_ACK;

		}else if(0 == strcmp(port_state_tmp, "LISTEN")) {
			port_state = TCP_LISTEN;

		}else if(0 == strcmp(port_state_tmp, "CLOSING")) {
			port_state = TCP_CLOSING;

		}else if(0 == strcmp(port_state_tmp, "")) { // not specified state => count all state
			port_state = 0;

		}else{
			SET_MSG_RESULT(result, strdup("Invalid state parameter"));
			return SYSINFO_RET_FAIL;

		}

	}else{
		SET_MSG_RESULT(result, strdup("Invalid parameter"));
		return SYSINFO_RET_FAIL;
	}


	zabbix_log(LOG_LEVEL_DEBUG, 
	           "[%s] specified srcport,destport,state => %d,%d,%s(%d) (%s:%d)",
	           MODULE_NAME, src_port, dst_port, port_state_tmp, port_state, __FILE__, __LINE__);

	if( src_port < 0 || src_port >65535 ){
		SET_MSG_RESULT(result, strdup("Ivalid source port value"));
		return SYSINFO_RET_FAIL;
	}

	if( dst_port < 0 || dst_port >65535 ){
		SET_MSG_RESULT(result, strdup("Ivalid dest port value"));
		return SYSINFO_RET_FAIL;
	}

	ret = get_port_count(&count, src_port, dst_port, port_state, NULL);

	if(ret == SYSINFO_RET_FAIL ){
		SET_MSG_RESULT(result, strdup("Error in get_port_count()"));
		return SYSINFO_RET_FAIL;
	}

	SET_UI64_RESULT(result, count);
	return SYSINFO_RET_OK;
}


/******************************************************************************
 *                                                                            *
 * Function: zbx_module_NET_TCP_COUNT_BULK                                    *
 *                                                                            *
 * Purpose: Aggregate TCP connections by port number                          *
 *          and return json object aggregated with state                      *
 *                                                                            *
 * Parameters: source port                                                    *
 *             destination port                                               *
 *             int array contains atate                                       *
 *                                                                            *
 * Return value: SYSINFO_RET_FAIL - function failed, item will be marked      *
 *                                 as not supported by zabbix                 *
 *               SYSINFO_RET_OK - success                                     *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_NET_TCP_COUNT_BULK(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	int	src_port = 0;
	int	dst_port = 0;
	int	port_state = 0;
	char	*port_state_tmp = NULL;

	int	count = 0;
	int	ret = SYSINFO_RET_FAIL;

	int	counter[TCP_STATE_NUM] = {0};
	struct	zbx_json json;


	zabbix_log(LOG_LEVEL_DEBUG, "[%s] param num [%d] (%s:%d)",
	           MODULE_NAME, request->nparam, __FILE__, __LINE__ );

	if (request->nparam == 0){
		src_port = 0;
		dst_port = 0;
		port_state = 0;

	}else if (request->nparam == 1){

		src_port = atoi( get_rparam(request, 0) );

		dst_port = 0;
		port_state = 0;

	}else if (request->nparam == 2){

		src_port = atoi( get_rparam(request, 0) );
		dst_port = atoi( get_rparam(request, 1) );

		port_state = 0;

	}else{
		SET_MSG_RESULT(result, strdup("Invalid parameter"));
		return SYSINFO_RET_FAIL;
	}


	zabbix_log(LOG_LEVEL_DEBUG, 
	           "[%s] specified srcport,destport,state => %d,%d,%s(%d) (%s:%d)",
	           MODULE_NAME, src_port, dst_port, port_state_tmp, port_state, __FILE__, __LINE__);

	if( src_port < 0 || src_port >65535 ){
		SET_MSG_RESULT(result, strdup("Ivalid source port value"));
		return SYSINFO_RET_FAIL;
	}

	if( dst_port < 0 || dst_port >65535 ){
		SET_MSG_RESULT(result, strdup("Ivalid dest port value"));
		return SYSINFO_RET_FAIL;
	}

	ret = get_port_count(&count, src_port, dst_port, port_state, &counter[0] );

	if(ret == SYSINFO_RET_FAIL ){
		SET_MSG_RESULT(result, strdup("Error in get_port_count()"));
		return SYSINFO_RET_FAIL;
	}


	zbx_json_init(&json, ZBX_JSON_STAT_BUF_LEN);
	zbx_json_addarray(&json, ZBX_PROTO_TAG_DATA);
	
	zbx_json_addobject(&json, NULL);
	zbx_json_adduint64(&json, "{#TCP sock count ALL}", count);
	zbx_json_close(&json);
	
	int i;
	for(i=1;i<TCP_STATE_NUM;i++){
		zbx_json_addobject(&json, NULL);
		zbx_json_adduint64(&json, "{#TCP sock state}", i);
		zbx_json_adduint64(&json, "{#TCP sock count}", counter[i]);
		zbx_json_close(&json);

		zabbix_log(LOG_LEVEL_WARNING, "[%s] In %s() %s:%d %d -> %d (total %d)",
		            MODULE_NAME, __FUNCTION__,  __FILE__, __LINE__, i, counter[i], count );
	}
	
	zbx_json_close(&json);
	SET_STR_RESULT(result, zbx_strdup(result->str, json.buffer));
	
	zbx_json_free(&json);

	return SYSINFO_RET_OK;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_init                                                  *
 *                                                                            *
 * Purpose: the function is called on agent startup                           *
 *          It should be used to call any initialization routines             *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - module initialization failed               *
 *                                                                            *
 * Comment: the module won't be loaded in case of ZBX_MODULE_FAIL             *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_init(void)
{
	int ret = ZBX_MODULE_FAIL;

	switch (program_type){
		case ZBX_PROGRAM_TYPE_SERVER:
		case ZBX_PROGRAM_TYPE_PROXY_ACTIVE:
		case ZBX_PROGRAM_TYPE_PROXY_PASSIVE:
		case ZBX_PROGRAM_TYPE_PROXY:
		case ZBX_PROGRAM_TYPE_AGENTD:
			zabbix_log(LOG_LEVEL_DEBUG, "[%s] module loaded by %s process. [%d]",
			           MODULE_NAME, get_program_type_string(program_type), program_type);
			ret = ZBX_MODULE_OK;
			break;
		default:
			zabbix_log(LOG_LEVEL_WARNING, "[%s] unknown value program_type [%d]", MODULE_NAME, program_type );
	}


	return ret;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_uninit                                                *
 *                                                                            *
 * Purpose: the function is called on agent shutdown                          *
 *          It should be used to cleanup used resources if there are any      *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - function failed                            *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_uninit(void)
{
	return ZBX_MODULE_OK;
}

