#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <windows.h>
#include <stdarg.h>
#include "vzLogger.h"

#include <Mmsystem.h>
#pragma comment(lib, "winmm.lib") 

#define LOCK_TIMEOUT	10000	/**< lock ackqure timeout, miliseconds */
#define MAX_MSG_LEN		10000	/**< maximal message length */
#define MAX_MSG_CNT		10000	/**< maximum messages in queue */

static int f_exit = 0;			/**< queue thread finish flag */
static HANDLE lock = INVALID_HANDLE_VALUE;	/**< module lock */
static HANDLE th_h = INVALID_HANDLE_VALUE;	/**< thread handle */
static unsigned long th_id = 0;	/**< thread id */
static char** msg_q_list = NULL;/**< module messages queue */
static int msg_q_tail;			/**< message tail pointer */
static time_t rot_last = 0;		/**< last log file rotation process */
static time_t rot_period = 24*3600;
static int dup_to_stderr = 1;
static char log_file_name[MAX_PATH] = "log";

static char* fetch_msg()
{
	int r;
	char* buf = NULL;

	/* lock struct */
	r = WaitForSingleObject(lock, LOCK_TIMEOUT);

	/* check if lock is success */
	if(0 == r)
	{
		/* check if head is present */
		if(NULL != msg_q_list[0])
		{
			/* save string pointer to file */
			buf = msg_q_list[0];
			msg_q_tail--;

			/* move momory */
			memmove
			(
				&msg_q_list[0],
				&msg_q_list[1],
				sizeof(char*) * (MAX_MSG_CNT)
			);

			/* clear last element */
			msg_q_list[MAX_MSG_CNT] = NULL;
		};
	};

	/* realese mutex */
	ReleaseMutex(lock);

	return buf;
};

static void output_msg(char* buf)
{
	static char *templ = "-%Y%m%d_%H%M%S";
	char* file_name = NULL;
	time_t ltime;
	struct tm *rtime;
	FILE* f;

	/* output to console */
	if(0 != dup_to_stderr)
		fprintf(stderr, buf);

	/* check if log file should be rotated */
	time( &ltime );
	if(rot_last + rot_period < ltime)
	{
		/* allocate file name */
		file_name = (char*)malloc(MAX_PATH + 1);

		/* compose file name */
		strncpy(file_name, log_file_name, MAX_PATH);

		/* append date to filename */
		rtime = localtime( &ltime );
		strftime
		(
			file_name + strlen(file_name), 
			MAX_PATH - strlen(file_name),
			templ , 
			rtime 
		);	/* append time */

		/* rename */
		MoveFile(log_file_name, file_name);

		/* free file name */
		free(file_name);

		/* assign new time */
		rot_last = ltime;
	};

	/* try to create or append file */
	if(NULL != (f = fopen(log_file_name, "a")))
	{
		fprintf(f, buf);
		fclose(f);
	};
};

static void process_queue()
{
	char* buf;

	while(NULL != (buf = fetch_msg()))
	{
		/* output msg */
		output_msg(buf);

		/* free mem of message */
		free(buf);
	};
};

static unsigned long WINAPI logger_queue_proc(void* desc_)
{
	while(0 == f_exit)
	{
		process_queue();
		Sleep(100);
	};

	/* ensure all messages sent */
	process_queue();

	return 0;
};

VZLOGGER_API int logger_setup(char* log_file, int rotate_interval, int dup_to_stderr_e)
{
	/* store arguments */
	dup_to_stderr = dup_to_stderr_e;
	rot_period = rotate_interval;
	strncpy(log_file_name, log_file, MAX_PATH);

	return 0;
};

static int logger_release()
{
	int i, r;

    /* check if initialized */
    if
    (
        (INVALID_HANDLE_VALUE == th_h)
        ||
        (INVALID_HANDLE_VALUE == lock)
        ||
        (NULL == msg_q_list)
    )
        return -1;

    /* notify thread to exit */
	f_exit = 1;

	/* wait for thread end */
	r = WaitForSingleObject(th_h, LOCK_TIMEOUT);
    if(0 != r)
        return -2;
    CloseHandle(th_h);

	/* destroy mutex */
	CloseHandle(lock);

	/* destroy queue and messages */
	for(i = 0; i<MAX_MSG_CNT; i++)
		if(NULL != msg_q_list[i])
			free(msg_q_list[i]);
	free(msg_q_list);

	return (0 == r)?0:-1;
};

static int logger_init()
{
    /* create mutex */
    lock = CreateMutex(NULL,FALSE,NULL);

    /* setup queue state */
    msg_q_tail = 0;
    msg_q_list = (char**)malloc(sizeof(char*) * (MAX_MSG_CNT + 1));
    memset(msg_q_list, 0, sizeof(char*) * (MAX_MSG_CNT + 1));

    /* create thread */
    th_h = CreateThread
    (
        NULL,				/* LPSECURITY_ATTRIBUTES lpThreadAttributes,*/
        8096,				/* SIZE_T dwStackSize,						*/
        logger_queue_proc,	/* LPTHREAD_START_ROUTINE lpStartAddress,	*/
        NULL,				/* LPVOID lpParameter,						*/
        0,					/* DWORD dwCreationFlags,					*/
        &th_id				/* LPDWORD lpThreadId						*/
    );

	return 0;
};

VZLOGGER_API int logger_printf(int type, char* message, ...)
{
	int r;
	char* buf;
    long mili_seconds;
	va_list args;
	static char *templ = "[%Y-%m-%d %H:%M:%S";
    static char *mili_seconds_template = ".%.03d] ";
	struct tm *rtime;
	time_t ltime;

    /* save time */
    mili_seconds = timeGetTime();
	time( &ltime );

    /* allocate space for buffer */
    if(NULL == (buf = (char*)malloc(MAX_MSG_LEN)))
    {
        fprintf(stderr, "ERROR: unable to allocate %d bytes\n", MAX_MSG_LEN);
        return -1;
    };

	/* put error code */
	buf[0] = ' ';
	buf[1] = ' ';
	buf[2] = 0;
	switch(type)
	{
		case 1: buf[0] = '!'; break;
		case 2: buf[0] = '?'; break;
	};

	/* put date into buf */
	rtime = localtime( &ltime );					/* local */
	strftime(buf + 2, MAX_MSG_LEN - 2, templ , rtime );	/* append time */
	
    /* append miliseconds */
    _snprintf
	(
		buf + strlen(buf),
		MAX_MSG_LEN - strlen(buf) - 10,
		mili_seconds_template,
		mili_seconds % 1000
	);

	/* init va arg */
	va_start( args, message);

	/* compose message */
	_vsnprintf
	(
		buf + strlen(buf),
		MAX_MSG_LEN - strlen(buf) - 10,
		message,
		args
	);

	/* add newline to message */
	strcat(buf, "\n");

    /* 'defragment' message */
    buf = (char*)realloc(buf, strlen(buf) + 1);

	/* lock struct */
	r = WaitForSingleObject(lock, LOCK_TIMEOUT);

	/* check if lock is success */
	if(0 == r)
	{
		if(msg_q_tail < MAX_MSG_LEN)
		{
			/* store current message in queue */
			msg_q_list[msg_q_tail] = buf;
			msg_q_tail++;

            /* mark buffer as used */
            buf = NULL;
		}
		else
			/* notify about queue overflow */
			fprintf(stderr, "ERROR! message queue OVERFLOW!!!\n");

		/* realese mutex */
		ReleaseMutex(lock);
	}
	else
		/* notify about queue overflow */
		fprintf(stderr, "ERROR! message queue TIMEOUT or FAIL!!!\n");

    /* free useless message */
	free(buf);

    /* init va arg */
    va_end(args);

	return 0;
};

BOOL APIENTRY DllMain
(
	HANDLE hModule, 
    DWORD  ul_reason_for_call, 
    LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			logger_init();
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			logger_release();
			break;
    }
    return TRUE;
}
