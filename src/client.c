/*
 * @file client.c - rpc 连接.
 *
 * Copyright (C) 2009-2010 microcai
 *
 * This software is Public Domain
 *
 * For more infomation see COPYING file shipped with this software.
 *
 * If you have any question with this software, please contract microcai, the
 * original writer of this software.
 *
 */

static char zeropage[4096];

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#include "./vcconfig.h"
#endif


#include "../include/distdb.h"
#include "../include/rpc.h"
#include "../include/list.h"

/**
 * Internal use only
 */
struct DISTDB_SQL_RESULT {
	/**
	 * 结果
	 */
	char **result;
	/**
	 * 列
	 */
	int	 col;
	/**
	 * 传给 RPC
	 */
	char  sql_result[8];
};

static int rpc_socket = -1 ;
static struct sockaddr_in server_addr = {0};
long	seq;

int distdb_rpc_connectto(const char * server)
{
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = RPC_DEFAULT_PORT;
	server_addr.sin_addr.s_addr = LOCALHOST;
	if(server)
	{
		int s1,s2,s3,s4,port;
		switch(sscanf(server,"%d.%d.%d.%d:%d",&s1,&s2,&s3,&s4,&port))
		{
		case 5:
			server_addr.sin_port = port;
		case 4:
			server_addr.sin_addr.s_addr = MAKEINET(s1,s2,s3,s4);
			break;
		default:
			return -1;
		}
	}
	rpc_socket = socket(server_addr.sin_family,SOCK_STREAM,0);
	if(rpc_socket <= 0)
		return -1;
	return connect(rpc_socket,(struct sockaddr*)&server_addr,INET_ADDRSTRLEN);
}

void distdb_rpc_disconnect()
{
	close(rpc_socket);
	rpc_socket = -1;
}

static int do_exchange(struct rpc_packet_call * call , struct rpc_packet_ret * ret)
{
	if (send(rpc_socket, call, call->len ,0) < 0)
		return -1;

	memset(call,0,call->len);

	if (recv(rpc_socket, ret, RPC_PACKET_HEADER_SIZE,MSG_PEEK|MSG_NOSIGNAL) < 0)
		return -1;

	if (recv(rpc_socket, ret , ret->len,0) < 0)
		return -1;

	return 0;
}

int distdb_execute_sql_bin(DISTDB_NODE * nodes,struct DISTDB_SQL_RESULT ** out,const char *sql,size_t length,int executeflag)
{
	socklen_t	addrlen = INET_ADDRSTRLEN;

	char	buff[8192];

	struct rpc_packet_call * sbuff = (typeof(sbuff))buff;
	struct rpc_packet_ret * rbuff = (typeof(rbuff))buff;
	struct DISTDB_SQL_RESULT	* forout = NULL;

	sbuff->rpc_call_id = DISTDB_RPC_EXECUTE_SQL_BIN;
	sbuff->call_seq = ++seq;
	struct execute_sql_bin * pdata = (typeof(pdata))(sbuff->data);
	pdata->length = length;
	pdata->flag = executeflag;
	memcpy(pdata->data, sql, length);
	sbuff->len = pdata->length + RPC_PACKET_HEADER_SIZE + SIZE_EXECUTE_SQL_BIN  ;

	if(do_exchange(sbuff,rbuff))
		return -1;

	if(rbuff->call_seq != seq)
		return -1;
	if(memcmp(rbuff->data,zeropage,8) !=0)
	{
		forout = (DISTDB_SQL_RESULT*)malloc(sizeof(struct DISTDB_SQL_RESULT));
		forout->result = NULL;
		memcpy(forout->sql_result,buff + 12,8);
	}
	*out = forout ;
	return rbuff->ret;
}


int distdb_rpc_free_result(struct DISTDB_SQL_RESULT *reslt)
{
	int i;
	socklen_t	addrlen = INET_ADDRSTRLEN;
	char	buff[8192];
	struct rpc_packet_call * sbuff = (typeof(sbuff))buff;
	struct rpc_packet_ret * rbuff = (typeof(rbuff))buff;
	sbuff->rpc_call_id = DISTDB_RPC_FREE_RESLUT;
	sbuff->call_seq = ++seq;
	memcpy(sbuff->data,reslt->sql_result,8);
	sbuff->len = 8 + RPC_PACKET_HEADER_SIZE;

	do_exchange(sbuff,rbuff);

	if(rbuff->call_seq != seq)
		return -1;
	for(i=0;i<reslt->col;++i)
		free(reslt->result[i]);
	free(reslt->result);
	free(reslt);
	return rbuff->ret;	 //:D
}

/**
 * @brief distdb_rpc_fetch_result 获得结果.
 *
 * @return 成功返回 0 ，失败 -1
 */
int distdb_rpc_fetch_result(struct DISTDB_SQL_RESULT * reslt,char ** result[])
{
	int i;
	char	buff[8192];
	struct rpc_packet_call * sbuff = (typeof(sbuff))buff;
	struct rpc_packet_ret * rbuff = (typeof(rbuff))buff;
	sbuff->rpc_call_id = DISTDB_RPC_FETCH_RESULT;
	sbuff->call_seq = ++seq;
	struct rpc_sql_result * res = (typeof(res))rbuff->data;

	memcpy(sbuff->data,reslt->sql_result,8);

	sbuff->len = 8 + RPC_PACKET_HEADER_SIZE;

	do_exchange(sbuff,rbuff);

	if(rbuff->call_seq != seq)
		return -1;

	for(i=0 ;i < reslt->col;++i)
		free(reslt->result[i]);

	free(reslt->result);

	reslt->result = calloc(res->number,sizeof(char*));
	reslt->col = res->number;

	for(i=0 ;i < res->number;++i)
	{
		reslt->result[i] = strdup(rbuff->data + res->offsets[i]);
	}
	*result = reslt->result;
	return rbuff->ret;	 //:D
}
