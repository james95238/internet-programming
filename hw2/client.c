#include "client_handler.h"

int main(void){
    
    struct sockaddr_in my_addr;
    int connfd, maxfd, ret, nready, read_count;
    char user_input[MAXLEN], username[MAXLEN], password[MAXLEN];
    char response_type[100], readbuf[MAXLEN];
    char my_status[100], chat_type[100];

    fd_set allset;

    connfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(3000);
    inet_pton(AF_INET, "127.0.0.1", &my_addr.sin_addr);
    
    ret = connect(connfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if(ret < 0){
        printf("connect socket error!\n");
	exit(0);
    }
    
    printf("Please Login--------\n");

    strcpy(my_status, "unconnected");
    strcpy(chat_type, "unknown");
    maxfd = connfd;
    FD_ZERO(&allset);

    for( ; ; ){
        FD_SET(connfd, &allset);
        FD_SET(STDIN_FILENO, &allset);
        nready = select(maxfd + 1, &allset, NULL, NULL, NULL);
        //printf("nready: %d\n", nready);
        if(FD_ISSET(connfd, &allset)){

            read_count = read(connfd, readbuf, MAXLEN);
            if(read_count == 0){
                printf("Server Close The Socket, End The Program\n");
                close(connfd);
                exit(0);
            }

            readbuf[read_count] = '\0';
            //printf("================================\n");
            //printf("Response From Server: --------\n");
            //printf("%s\n", readbuf);
            //printf("================================\n\n");
            recv_server_response(my_status, readbuf);
        }

        if(FD_ISSET(STDIN_FILENO, &allset)){
            fgets(readbuf, MAXLEN, stdin);
            //printf("fgets: %s\n", readbuf);
            readbuf[strlen(readbuf) - 1] = '\0';
            send_server_request(my_status, chat_type, connfd, readbuf);
        }
    }
    return 0;
}
