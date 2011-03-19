#include "mysql.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <pthread.h>

#define MAX_QUERY_SIZE PIPE_BUF
#define RUNNER_RESULT_EAGAIN -1
#define RUNNER_RESULT_SQL_ERROR 0
#define RUNNER_RESULT_ROWS 1
#define RUNNER_RESULT_NONE 2
#define DEBUG 1

typedef struct _query_runner {
	pthread_t thread;
	MYSQL *conn;
	char server[56];
	char user[56];
	char password[56];
	char database[56];
	int request_pipe_fds[2];	
	int response_pipe_fds[2];	
} query_runner;

typedef void (*query_runner_callback)(int, MYSQL_RES *, void *);

query_runner *query_runner_init(char *hostname, char *user, 
                                char *password, char *database);
void query_runner_execute(query_runner *runner, char *query, 
                          query_runner_callback callback, void *callback_data );
int query_runner_get_response_fd(query_runner *runner);
int query_runner_get_request_fd(query_runner *runner);
void query_runner_shutdown(query_runner * runner);
int query_runner_handle_next_result(query_runner *runner);
void query_runner_handle_all_results(query_runner *runner);
