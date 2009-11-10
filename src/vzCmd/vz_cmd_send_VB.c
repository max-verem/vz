#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <oleauto.h>
#include "stdint.h"
#include "vz_cmd_send.h"

#include <stdio.h>

struct vzCmdSendPart_desc
{
	BSTR cmd;
};

static int init_done = 0;

VZCMDSEND_API long __stdcall vzCmdSendVB(char* host, int32_t count, struct vzCmdSendPart_desc *data)
{
	int i, l, r = 0;
	char** argv;
#ifdef _DEBUG
	FILE* f;
	char error_log[1024] = "";
#else
	char* error_log = NULL;
#endif /* _DEBUG */

	/* init sockets */
	if(0 == init_done)
	{
		vz_cmd_send_init();
		init_done = 1;
	};

#ifdef _DEBUG
	f = fopen("vz_cmd_send_VB.log", "wt");
	if(NULL != f)
	{
		fprintf(f, "host: [%s]\n", host);
		fprintf(f, "count: [%d]\n", count);
	};
#endif /* _DEBUG */

	/* allocate data */
	argv = (char**)malloc(count * sizeof(char*));
	for(i = 0; i < count; i++)
	{
		/* get width */
		l = WideCharToMultiByte(CP_UTF8, 0, data[i].cmd, -1, NULL, 0,  NULL, NULL);

#ifdef _DEBUG
		if(NULL != f)
		{
			fprintf(f, "cmd[%d] lenght: [%d]\n", i, l);
		};
#endif /* _DEBUG */

		/* allocate */
		argv[i] = (char*)malloc(l + 1);

		/* convert */
		WideCharToMultiByte(CP_UTF8, 0, data[i].cmd, -1, argv[i], l,  NULL, NULL);

#ifdef _DEBUG
		if(NULL != f)
		{
			fprintf(f, "cmd[%d] data: [%s]\n", i, argv[i]);
		};
#endif /* _DEBUG */
	};

	/* call sender */
	if(count)
		r = vz_cmd_send_strlist_udp(host, argv, count, error_log);

#ifdef _DEBUG
	if(NULL != f)
	{
		fprintf(f, "r = %d, error_log=[%s]\n", r, error_log);
		fclose(f);
	};
#endif /* _DEBUG */

	/* free data */
	for(i = 0; i < count; i++)
		free(argv[i]);
	free(argv);

	return r;
};

VZCMDSEND_API int32_t __stdcall vzCmdSendVB2008(char* host, int32_t count, LPSAFEARRAY FAR *ArrayData)
{
	int i, l, r = 0;
	char** argv;
	BSTR* data;
#ifdef _DEBUG
	FILE* f;
	char error_log[1024] = "";
#else
	char* error_log = NULL;
#endif /* _DEBUG */

	/* init sockets */
	if(0 == init_done)
	{
		vz_cmd_send_init();
		init_done = 1;
	};

#ifdef _DEBUG
	f = fopen("vz_cmd_send_VB2008.log", "wt");
	if(NULL != f)
	{
		fprintf(f, "host: [%s]\n", host);
		fprintf(f, "count: [%d]\n", count);
	};
#endif /* _DEBUG */

	/* allocate data */
	argv = (char**)malloc(count * sizeof(char*));
	data = (BSTR*)(*ArrayData)->pvData;
	for(i = 0; i < count; i++)
	{
		/* get width */
		l = WideCharToMultiByte(CP_UTF8, 0, data[i], -1, NULL, 0,  NULL, NULL);

#ifdef _DEBUG
		if(NULL != f)
		{
			fprintf(f, "cmd[%d] lenght: [%d]\n", i, l);
		};
#endif /* _DEBUG */

		/* allocate */
		argv[i] = (char*)malloc(l + 1);

		/* convert */
		WideCharToMultiByte(CP_UTF8, 0, data[i], -1, argv[i], l,  NULL, NULL);

#ifdef _DEBUG
		if(NULL != f)
		{
			fprintf(f, "cmd[%d] data: [%s]\n", i, argv[i]);
		};
#endif /* _DEBUG */
	};

	/* call sender */
	if(count)
		r = vz_cmd_send_strlist_udp(host, argv, count, error_log);

#ifdef _DEBUG
	if(NULL != f)
	{
		fprintf(f, "r = %d, error_log=[%s]\n", r, error_log);
		fclose(f);
	};
#endif /* _DEBUG */

	/* free data */
	for(i = 0; i < count; i++)
		free(argv[i]);
	free(argv);

	return r;
};
