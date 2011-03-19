#include "mysql-noblock.h"

int easy_select(fd_set *read_set, fd_set *write_set,
	            int *read_socks, int read_socks_count,
	            int *write_socks, int write_socks_count, 
	            int timeout_seconds, int timeout_useconds) {
	struct timeval tv;
	int max_fd = -1;
	int i;

	tv.tv_sec = timeout_seconds;
	tv.tv_usec = timeout_useconds;

	if (read_set != NULL)
		FD_ZERO(read_set);
	if (write_set != NULL)
		FD_ZERO(write_set);
	
	for(i=0;i<write_socks_count;i++) {
		if ( write_socks[i] > max_fd) 
			max_fd = write_socks[i];
		FD_SET(write_socks[i], write_set);		
	}

	for(i=0;i<read_socks_count;i++) {
		if ( read_socks[i] > max_fd) 
			max_fd = read_socks[i];
		FD_SET(read_socks[i], read_set);		
	}

	if (max_fd == -1) {
		printf("Error. Tried to select() without providing file descriptors\n");
		exit(1);
	}

	return select(max_fd+1, read_set, write_set, NULL, &tv);
}

void print_results_callback(int result_code, MYSQL_RES *res, void *user_data) {
	MYSQL_ROW row;

	printf("(print_results_callback) now we're in the callback\n");

	if (result_code == RUNNER_RESULT_SQL_ERROR) {
		printf("(print_results_callback) Oops. The query failed.\n");			
	} else if (result_code == RUNNER_RESULT_NONE) {
		printf("(print_results_callback) Oops. The query provided has no results.\n");					
	} else {
		while ((row = mysql_fetch_row(res)) != NULL)
			printf("%s \n", row[0]);
	}
}

int main(int argc, char ** argv) {
	fd_set read_fds;
	int select_result;
	query_runner *runner;
	char *long_query = "SELECT SLEEP(3)";
	int response_fd;
	int read_socks[2];
	int read_socks_count;	
	int loop_count=0;

	if (argc > 1)
		long_query = argv[1];

	printf("(main) Starting the query runner in another thread.\n");
	runner = query_runner_init("localhost","root","","mysql");

	response_fd = query_runner_get_response_fd(runner);
	read_socks[0] = response_fd;	
	read_socks_count = 1;
		
	printf("(main) blasting a long query to the runner: %s\n", long_query);
	query_runner_execute(runner, long_query, print_results_callback, NULL);

	while (loop_count++ < 5) {
		select_result = easy_select(&read_fds, NULL, 
		                            read_socks, read_socks_count, 
 		                            NULL, 0, 
		                            1, 0);
		if (select_result == -1) {
			perror("select()");
		} else if ( select_result ) {
			if (FD_ISSET(response_fd, &read_fds)) {
				query_runner_handle_all_results(runner); 
			}
		} else {
			printf("(main) 1 second has passed. No data yet.\n");
		}
	}
	
	query_runner_shutdown(runner);
	free(runner);
	printf("(main) Goodbye.\n");
	return 0;	
}
