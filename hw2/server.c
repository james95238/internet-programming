#include "server_handler_v2.h"

int main(void){
    
    struct sockaddr_in serveraddr, clientaddr;
    int ret, listenfd, sockfd, connfd, on;
    int maxfd, max_client_index, client_index, nready, read_count;
    char readbuf[MAXLEN], req[MAXLEN];

    cli_info client[FD_SETSIZE];
    socklen_t client_len;
    fd_set rset, allset;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0){
        printf("create listen socket error!\n");
	exit(0);
    }
    
    on = 1;
    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(3000);

    ret = bind(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
    if(ret < 0){
        printf("socket bind error!\n");
	exit(0);
    }

    ret = listen(listenfd, LISTEN_NQ);

    maxfd = listenfd;
    max_client_index = -1;
    
    for(client_index = 0; client_index < FD_SETSIZE; client_index++){
	client[client_index].sockfd = -1;
        strcpy(client[client_index].status, "unconnected");
        memset(client[client_index].rival, 0, 100);
        client[client_index].Game = NULL;
    }

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    for( ; ; ){

        rset = allset;
	nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        printf("nready: %d\n", nready);
	if(FD_ISSET(listenfd, &rset)){
	    //client connect
	    client_len = sizeof(clientaddr);
	    connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &client_len);

	    for(client_index = 0; client_index < FD_SETSIZE; client_index++){
	        if(client[client_index].sockfd < 0){
		    client[client_index].sockfd = connfd;
		    break;
		}
	    }

	    if(client_index == FD_SETSIZE){
	        printf("too many clients!\n");
		exit(0);
	    }

	    FD_SET(connfd, &allset);
	    if(connfd > maxfd)
                maxfd = connfd;
	    if(client_index > max_client_index)
		max_client_index = client_index;
	    if(--nready <= 0)
		continue;
	}

	for(client_index = 0; client_index <= max_client_index; client_index++){
	    if((sockfd = client[client_index].sockfd) < 0)
		continue;

	    if(FD_ISSET(sockfd, &rset)){
                
		//client disconnect
		read_count = read(sockfd, readbuf, MAXLEN);
                if(read_count <= 0){
                    printf("client %s exited!\n", client[client_index].username);
                    FD_CLR(sockfd, &allset);
                    client_exit_handler(client_index, max_client_index, client);
                    
                }else{
                    readbuf[read_count] = '\0';
                    printf("================================\n\n");
                    printf("%s\n", readbuf);
                    //printf("================================\n\n");
		    getcontent(readbuf, req, "Request-Type");
                    
		    if(strcmp(req, "log in") == 0){
		        login_handler(readbuf, client_index, max_client_index, client);
		    }else if(strcmp(req, "list") == 0){
                        list_handler(client_index, max_client_index, client);
		    }else if(strcmp(req, "match") == 0){
                        match_handler(readbuf, client_index, max_client_index, client);
		    }else if(strcmp(req, "chat") == 0){
                        chat_handler(readbuf, client_index, max_client_index, client);
                    }else if(strcmp(req, "gaming") == 0){
                        gaming_handler(readbuf, client_index, max_client_index, client);
                    }else if(strcmp(req, "log out") == 0){
                        printf("client %s log out\n", client[client_index].username);
		        logout_handler(&client[client_index]);                 
		    }
                }

		if(--nready <= 0)
                    break;
	    }
	}
    }
    return 0;
}
