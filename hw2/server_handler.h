#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<sys/types.h>
#include<netinet/in.h>
#define LISTEN_NQ 10
#define MAXLEN 1024

typedef struct game_info{
    int turn;
    int round;
    char rival[2][100];
    char gamebuf[10];
}game;

typedef struct client_info{
    int id;
    int sockfd;
    char username[100];
    char status[100];
    char rival[100];
    struct game_info *Game;
}cli_info;

void getcontent(char *src, char *des, char *field){
    char *tok, *qtr, *ptr;
    tok = strstr(src, field);
    if(tok == NULL) return;
    tok += (strlen(field) + 2);
    
    ptr = tok;
    qtr = des;
    for(ptr=tok, qtr=des; *ptr != '\r'; ptr++, qtr++){
        *qtr = *ptr;
    }
    *(qtr - 1) = '\0';
}

void login_handler(char *buf, int sockfd, cli_info* client){
    char res[MAXLEN];
    char *tok;
    int ret;

    getcontent(buf, client->username, "username");
    
    sprintf(res,
        "welcome, %s\n",
        client->username
    );
    strcpy(client->status, "available");
    write(sockfd, res, strlen(res));
}

void start_game_handler(cli_info *client1, cli_info *client2){

    game *G = malloc(sizeof(game));
    char res[MAXLEN];

    G->turn = 0;
    G->round = 0;
    strcpy(G->rival[0], client1->username);
    strcpy(G->rival[1], client2->username);
    strcpy(client1->status, "gaming");
    strcpy(client2->status, "gaming");
    client1->Game = G;
    client2->Game = G;
    for(int i = 0; i < 9; i++){
        G->gamebuf[i] = ' ';
    }
    sprintf(res,
        "request: match\n\r"
        "status: gaming\n\r"
        "type: send\n\r"
        "result: agree\n\r" 
        "response: "
        "-------------\n"
        "| %c | %c | %c |\n"
        "-------------\n"
        "| %c | %c | %c |\n"
        "-------------\n"
        "| %c | %c | %c |\n"
        "-------------\n"
        "It's %s's turn\n\r",
        G->gamebuf[0], G->gamebuf[1], G->gamebuf[2], G->gamebuf[3], G->gamebuf[4], G->gamebuf[5], G->gamebuf[6], G->gamebuf[7], G->gamebuf[8], G->rival[G->turn]
    );
    write(client1->sockfd, res, strlen(res));
    write(client2->sockfd, res, strlen(res));
    printf("start_game_finish\n");
    printf("rival0: %s, rival1: %s\n", G->rival[0], G->rival[1]);
}

void list_handler(int sockfd, int maxi, cli_info *client){
    char res[MAXLEN];
    char userinfo[MAXLEN];
    
    sprintf(res, 
        "request: list\n\r"
        "response: "
    );

    for(int i = 0; i <= maxi; i++){
        if(client[i].sockfd < 0)
            continue;

        sprintf(userinfo,
            "username: %s, "
            "status: %s\n",
            client[i].username,
            client[i].status
        );
        printf("userinfo: %s\n", userinfo);
        strcat(res, userinfo);
    }
    res[strlen(res)] = '\r';
    write(sockfd, res, strlen(res));
}

void match_handler(char *buf, int sockfd, int reqi, int maxi, cli_info *client){
    char res[MAXLEN], rival[100], type[10], result[20];
    int i, find_success;
    
    getcontent(buf, type, "type");
    getcontent(buf, rival, "rival");

    if(strcmp(type, "send") == 0){

        //判斷是否為非available狀態發送 request
        if(strcmp(client[reqi].status, "available") != 0){
            return;
        }

        find_success = 0;
        for(i = 0; i <= maxi; i++){
 
            if(client[i].sockfd < 0)
                continue;
       
            if(strcmp(client[i].username, rival) == 0){
                if(strcmp(client[i].status, "available") == 0){

                    find_success = 1;
                    strcpy(client[i].status, "busy");
                }
                break;
            }
        }
        printf("find success: %d\n", find_success);
        if(find_success){
            sprintf(res,
                "request: match\n\r"
                "status: %s\n\r"
                "type: recv\n\r"
                "rival: %s\n\r"
                "result: success\n\r"
                "response: %s want to play with you, please answer (yes/no)\n\r",
                client[i].status, 
                client[reqi].username, 
                client[reqi].username
            );
            printf("********** send  **********");
            printf("---------- client %s ----------\n", client[i].username);
            printf("res: %s", res);
            strcpy(client[i].status, "busy");
            write(client[i].sockfd, res, strlen(res));
        }else{
            sprintf(res,
                "request: match\n\r"
                "status: %s\n\r"
                "type: send\n\r"
                "rival: %s\n\r"
                "result: reject\n\r"
                "response: %s is offline or busy, please find other opponent\n\r",
                client[reqi].status, 
                rival, 
                rival
            );
            printf("********** send  **********");
            printf("---------- client %s ----------\n", client[i].username);
            printf("res: %s", res);
            write(sockfd, res, strlen(res));
        }
    }else if(strcmp(type, "recv") == 0){
        getcontent(buf, result, "result");
        if(strcmp(result, "agree") == 0){
            strcpy(client[reqi].rival, rival);
            for(i = 0; i < maxi; i++){
                if(strcmp(client[i].username, rival) == 0){
                    strcpy(client[i].rival, client[reqi].username);
                }
            }
            printf("OK\n");
            start_game_handler(client + i, client + reqi);
        }else if(strcmp(result, "reject") == 0){
            strcpy(client[reqi].status, "available");
            //找出邀請他的對手
            for(i = 0; i <= maxi; i++){
                if(client[i].sockfd < 0)
                    continue;
                if(strcmp(rival, client[i].username) == 0){
                    break;
                }
            }
            sprintf(res, 
                "request: match\n\r"
                "status: %s\n\r"
                "type: send\n\r"
                "rival: %s\n\r"
                "result: reject\n\r"
                "response: %s refuse to play with you\n\r",
                client[i].status, 
                client[i].username, 
                client[i].username
            );
            printf("********** send  **********");
            printf("---------- client %s ----------\n", client[i].username);
            printf("res: %s", res);
            write(client[i].sockfd, res, strlen(res));
            strcpy(client[i].status, "available");
        }
    }
}

void gaming_handler(char *buf, int sockfd, int reqi, int maxi, cli_info *client){
    int row, col, i, wins;
    char pos[100], res[MAXLEN];
    char result[20], res_result[100];
    game *G;

    G = client[reqi].Game;
    if(strcmp(G->rival[G->turn], client[reqi].username) != 0){
        sprintf(res, 
            "request: gaming\n\r"
            "result: fail\n\r"
            "response: it's not your turn!\n\r"
        );
        write(client[reqi].sockfd, res, strlen(res));
    }else{
        getcontent(buf, pos, "position");
        printf("pos: %s\n", pos);
        row = atoi(pos);
        col = atoi(pos + 2);
        printf("row: %d, col: %d\n", row, col);
    
        if((row >= 1 && row <= 3) && (col >= 1 && col <= 3)){
            if(G->gamebuf[(row-1) * 3 + (col-1)] != ' '){
                sprintf(res,
                    "request: gaming\n\r"
                    "result: fail\n\r"
                    "response: invalid position!\n\r"
                );
                write(client[reqi].sockfd, res, strlen(res));
            }else{
                //更新棋盤
                if(G->turn == 0){
                    G->gamebuf[(row-1) * 3 + (col-1)] = 'O';
                    G->turn = 1;
                }else{
                    G->gamebuf[(row-1) * 3 + (col-1)] = 'X';
                    G->turn = 0;
                }
                //檢查棋盤目前結果
                wins = -1;
                for(int j = 0; j < 3; j++){
                    if((G->gamebuf[j*3] == G->gamebuf[j*3+1]) && (G->gamebuf[j*3+1] == G->gamebuf[j*3+2])){
                        if(G->gamebuf[j*3] == 'O'){
                            wins = 0;
                        }else if(G->gamebuf[j*3] == 'X'){
                            wins = 1;
                        }
                        break;
                    }

                    if((G->gamebuf[j] == G->gamebuf[j+3]) && (G->gamebuf[j+3] == G->gamebuf[j+6])){
                        if(G->gamebuf[j] == 'O'){
                            wins = 0;
                        }else if(G->gamebuf[j] == 'X'){
                            wins = 1;
                        }
                        break;
                    }
                }
                //比對斜線
                if(((G->gamebuf[0] == G->gamebuf[4]) && (G->gamebuf[4] == G->gamebuf[8])) || ((G->gamebuf[2] == G->gamebuf[4]) && (G->gamebuf[4] == G->gamebuf[6]))){
                    if(G->gamebuf[4] == 'O'){
                        wins = 0;
                    }else if(G->gamebuf[4] == 'X'){
                        wins = 1;
                    }
                    printf("row --------   %c      %c      %c\n", G->gamebuf[4], G->gamebuf[4], G->gamebuf[4]);
                }
                
                if(wins != -1){
                    strcpy(result, "finish");
                    sprintf(res_result, "---%s wins---", G->rival[wins]);
                }else{
                    strcpy(result, "success");
                    sprintf(res_result, "it's %s's turn", G->rival[G->turn]);
                }

                sprintf(res, "---%s wins---", G->rival[0]);
                sprintf(res, 
                    "request: gaming\n\r"
                    "result: %s\n\r"
                    "response: "
                    "-------------\n"
                    "| %c | %c | %c |\n"
                    "-------------\n"
                    "| %c | %c | %c |\n"
                    "-------------\n"
                    "| %c | %c | %c |\n"
                    "-------------\n"
                    "%s\n\r",
                    result,
                    G->gamebuf[0], G->gamebuf[1], G->gamebuf[2], G->gamebuf[3], G->gamebuf[4], G->gamebuf[5], G->gamebuf[6], G->gamebuf[7], G->gamebuf[8], 
                    res_result
                );
                printf("cur turn: %d\n", G->turn);
                for(i = 0; i <= maxi; i++){
                    if(client[i].sockfd < 0)
                        continue;
                    if(strcmp(client[i].username, G->rival[G->turn]) == 0){
                        printf("cmp ok\n");
                        printf("client reqi: %s, G->turn: %s\n", client[reqi].username, G->rival[G->turn]);
                        break;
                    }
                }
                if(strcmp(result, "finish") == 0){
                    strcpy(client[reqi].status, "available");
                    strcpy(client[i].status, "available");
                }
               
                write(client[i].sockfd, res, strlen(res));
                write(client[reqi].sockfd, res, strlen(res));
            } 
        }else{
            sprintf(res,
                "request: gaming\n\r"
                "result: fail\n\r"
                "response: invalid instruction!\n\r"
            );
            write(client[reqi].sockfd, res, strlen(res));
        }
    }    
}

void logout_handler(int maxi, int reqi, cli_info *client){
    int i;
    char res[MAXLEN];
    game *G;

    if(strcmp(client[reqi].status, "gaming") == 0){
        for(i = 0; i < maxi; i++){
            if(strcmp(client[reqi].rival, client[i].username) == 0)
                break;
        }
        G = client[reqi].Game;
        sprintf(res, 
            "request: gaming\n\r"
            "result: finish\n\r"
            "response: "
            "-------------\n"
            "| %c | %c | %c |\n"
            "-------------\n"
            "| %c | %c | %c |\n"
            "-------------\n"
            "| %c | %c | %c |\n"
            "-------------\n"
            "%s has been disconnect, you win this game!\n\r",
            G->gamebuf[0], G->gamebuf[1], G->gamebuf[2], G->gamebuf[3], G->gamebuf[4], G->gamebuf[5], G->gamebuf[6], G->gamebuf[7], G->gamebuf[8],
            client[reqi].username
        );
        write(client[i].sockfd, res, strlen(res));
        strcpy(client[i].status, "available");
    }
    
    memset(client[reqi].username, 0, strlen(client[reqi].username));
    memset(client[reqi].rival, 0, strlen(client[reqi].rival));
    memset(client[reqi].status, 0, strlen(client[reqi].status));
    close(client[reqi].sockfd);
    client[reqi].sockfd = -1;
}

