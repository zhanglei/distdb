/*
 * nodes_op - ops for nodes
 *
 *
 * Written by microcai in 2009-2010
 *
 * This software is Public Domain
 *
 * For more infomation see COPYING file shipped with this software.
 *
 * If you have any question with this software, please contract microcai, the
 * original writer of this software.
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define __DISTDB_SERVER_SIDE_H

#include "../include/global_var.h"
#include "../include/distdb.h"
#include "../include/rpc.h"
#include "../include/db_def.h"



void * connect_nodes(void *unused)
{

}


/*
 * connet to ip and return the fd
 */
static int connectto(struct sockaddr_in * peer)
{
	int ret;
	int sk = socket(peer->sin_family,SOCK_STREAM,0);

	fcntl(sk,F_SETFL,fcntl(sk,F_GETFL)|O_NONBLOCK);

	if(sk > 0)
		ret =  connect(sk,(struct sockaddr*)peer,INET_ADDRSTRLEN);
	else
		return -1;
	if(ret && errno != EINPROGRESS)
		close(sk);
	return ret;
}

/*
 * connect to peers
 */
static void* connect_peer(struct nodes * node)
{
	pthread_t pt;
	//connect
	pthread_mutex_lock(&nodelist_lock);
	if (node->sock_peer == 0)
	{
		//有可能执行到这里的时候已经连接上东东了呢！
		node->sock_peer = connectto(&node->peer);
	}
	else
	{
		//已经被连接了，那就不用再费事连接到对方了
		pthread_mutex_unlock(&nodelist_lock);
		return 0;
	}
	//link to connected list

	node->refcount ++;
	LIST_ADDTOTAIL(&node_connectedlist,& node->connectedlist);
	LIST_DELETE_AT(&node->unconnectedlist);
	node->refcount --;
	pthread_mutex_unlock(&nodelist_lock);
	return service_loop(node);
}

int start_connect_nodes()
{
	do
	{
		// 随机 休息 0 - 20 秒
		sleep(rand() % 20);
		//struct nodes * n;
		pthread_t pt;

		struct list_node * n;

		pthread_mutex_lock(&nodelist_lock);

		for (n = node_unconnectedlist.head; n
				!= node_unconnectedlist.tail->next; n = n->next)
		{
			pthread_t pt;
			if (!LIST_HEAD(n,nodes,unconnectedlist)->sock_peer) // only connect unconnected.
				pthread_create(&pt, 0, (void *(*)(void *)) connect_peer,
						LIST_HEAD(n,nodes,unconnectedlist));
			//else n has been off link. Thank good ness, the n->next still works
		}
		pthread_mutex_unlock(&nodelist_lock);
	} while (1);
	return 0;
}

int open_nodes_socket()
{
	int opt = 1;
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = RPC_DEFAULT_PORT;
	g_socket = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(g_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (bind(g_socket, (struct sockaddr*) &addr, INET_ADDRSTRLEN) < 0)
	{
		close(g_socket);
		g_socket = -1;
		return -1;
	}
	return listen(g_socket, 20);
}