/*
 * version.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vcconfig.h"


#define __DISTDB_SERVER_SIDE_H

#include "../include/distdb.h"

const char* distdb_version()
{
	return PACKAGE_VERSION;
}

#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		WSADATA wsadata;
		WSAStartup(MAKEWORD(2,1),&wsadata);

	}else if (fdwReason == DLL_PROCESS_DETACH)
	{
		WSACleanup();
	}
}
#endif

