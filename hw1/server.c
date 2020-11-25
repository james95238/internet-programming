#include "http_function_lib.h"
#define LISTENNQ 10

void child_process_handler(int signo){
    pid_t pid;
    int stat;

    while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("child %d terminated\n\n", pid);
    return;
}

int main(void){

    int listenfd, connfd, ret, on;
    struct sockaddr_in servaddr, clientaddr;
    char http_request_line[100];
    char http_request_header[1024];
    pid_t pid;
    socklen_t clientsize;

    signal(SIGCHLD, child_process_handler);
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        printf("socket create error!\n");
	exit(0);
    }
    on = 1;
    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(3000);

    if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
        printf("socket bind error\n");
	exit(0);
    }

    if(listen(listenfd, LISTENNQ) == -1){
        printf("socket listen error\n");
	exit(0);
    }


    for( ; ; ){
	clientsize = sizeof(clientaddr);
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientsize);
	if((pid = fork()) == 0){
	    close(listenfd);
            if(readline(connfd, http_request_line) > 0){
	        printf("request_line: %s\n", http_request_line);
	    }else{
	        printf("read_request_line_error!\n");
		close(connfd);
		exit(0);
	    }

	    if(strstr(http_request_line, "GET") != NULL){
                printf("enter GET handler\n");
	        http_GET_handler(connfd, http_request_line);
	    }else if(strstr(http_request_line, "POST") != NULL){
		printf("enter POST handler\n");
	        http_POST_handler(connfd);
	    }else{
	        printf("unsupported_http_request_type!\n");
	    }

	    close(connfd);
	    printf("endding process\n");
	    exit(0);
	}
	close(connfd);
    }
    return 0;
}
