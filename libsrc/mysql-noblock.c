#include "mysql-noblock.h"

/* these should be considered private */
typedef struct _query_runner_request {
	char *query;
	query_runner_callback callback;
	void *callback_data;
} query_runner_request;

typedef struct _query_runner_result {
	MYSQL_RES *res;
	int *result_code; /* Don't try to dereference me! */
	query_runner_callback callback;
	void *callback_data;
} query_runner_result;

int setnonblock(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0)
		return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0)
		return -1;

	return 0;
}

void query_runner_thread_cleanup(void *v) {	
#if DEBUG
	printf("(thread) this is cleanup\n");
#endif	
	query_runner *runner = v;
	mysql_close(runner->conn);
	mysql_thread_end();	
}

void *query_runner_thread(void *thread_info) {
	int bytes_read;	
	query_runner *runner = (query_runner *) thread_info;	
	query_runner_result result;
	int success=1;
	query_runner_request request;

	pthread_cleanup_push(query_runner_thread_cleanup, runner); 

	runner->conn = mysql_init(NULL);
	if (!mysql_real_connect(runner->conn, runner->server,
							runner->user, runner->password, 
	                        runner->database, 0, NULL, 0)) {
#if DEBUG
		fprintf(stderr, "Unable to start thread! %s\n", mysql_error(runner->conn));
#endif
		exit(1);
	}

#if DEBUG
	printf("(runner) Connected to mySQL: Doing blocking read() on pipe waiting for queries.\n");	
#endif

	while(1) {		
		bytes_read = read(runner->request_pipe_fds[0], 
						  &request, sizeof(query_runner_request));	

		if (bytes_read == 0) 
			printf("(runner) UHO OHLKJHSDLFKJSDLKFJ\n");	


#if DEBUG
		printf("(runner) We got a query: %s\n", request.query);	
#endif

		if (mysql_query(runner->conn, request.query)) {
#if DEBUG
			fprintf(stderr, "%s\n", mysql_error(runner->conn));
#endif
			result.res = NULL;
			result.result_code = NULL;
		} else {
			result.res = mysql_store_result(runner->conn);
			/* This will get freed by the time we read it on the other
			end, but all we care is that it is not a null *valid* pointer */
			result.result_code = &success; 
		}
		result.callback = request.callback;
		result.callback_data = request.callback_data;
		
		free(request.query);
#if DEBUG
		printf("(runner) Done query. Writing address of result to pipe.\n");	
#endif
		write(runner->response_pipe_fds[1], &result, sizeof(result));
	}	

	pthread_cleanup_pop(NULL); 
}

int query_runner_get_request_fd(query_runner *runner) {
	/* Clients will WRITE to the request fd */
	return runner->request_pipe_fds[1];
}

int query_runner_get_response_fd(query_runner *runner) {
	/* Clients will READ from the response fd */
	return runner->response_pipe_fds[0];
}

query_runner *query_runner_init(char *hostname, char *user, char *password, char *database) {
	query_runner *runner;
	int pthread_failure_code;

	runner = malloc(sizeof(query_runner));	

	strcpy(runner->server, hostname);	
	strcpy(runner->user, user);
	strcpy(runner->password, password);
	strcpy(runner->database, database);

	pipe(runner->request_pipe_fds);
	pipe(runner->response_pipe_fds);	

	setnonblock(query_runner_get_response_fd(runner));

	pthread_failure_code = pthread_create(&runner->thread, NULL, query_runner_thread, (void*) runner);
	if (pthread_failure_code) {
#if DEBUG
		printf("ERROR; return code from pthread_create() is %d\n", pthread_failure_code);
#endif
		exit(-1);
	}

	return runner;
}

void query_runner_shutdown(query_runner * runner) {
	pthread_cancel(runner->thread);	
	pthread_join(runner->thread, NULL);	
	close(runner->request_pipe_fds[0]);
	close(runner->request_pipe_fds[1]);
	close(runner->response_pipe_fds[0]);
	close(runner->response_pipe_fds[1]);
}

int query_runner_handle_next_result(query_runner *runner) {
	int read_result;
	int *success;
	query_runner_result result;
	int rv;
	
	read_result = read(query_runner_get_response_fd(runner), 
					   &result, sizeof(query_runner_result));

	if (read_result < 0) 
		return RUNNER_RESULT_EAGAIN;

	/* Will be NULL if there were errors */
	success = result.result_code; 

	if ( success == NULL ) 
		rv = RUNNER_RESULT_SQL_ERROR;
	else if (result.res == NULL)
		rv = RUNNER_RESULT_NONE;
	else
		rv = RUNNER_RESULT_ROWS;

	if (result.callback != NULL) {
#if DEBUG
		printf("(main) executing a callback\n");
#endif	
		result.callback(rv,result.res,result.callback_data);
		mysql_free_result(result.res );	
	}	

	return rv;	
}

void query_runner_handle_all_results(query_runner *runner) {
   	while ( query_runner_handle_next_result(runner) != RUNNER_RESULT_EAGAIN) {
		/* Handle more results until the pipe has no more */
	}	
}

void query_runner_execute(query_runner *runner, char *query, 
						  query_runner_callback callback, void *callback_data ) {	
	query_runner_request request;
	request.query = strdup(query);
	request.callback = callback;
	request.callback_data = callback_data;
	
	write( query_runner_get_request_fd(runner), &request, sizeof(query_runner_request));	
}
