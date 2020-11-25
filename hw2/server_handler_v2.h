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

typedef struct user_info{
    char username[100];
    char password[100];
}user_info;

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
    char invited_from_username[100];
    char inviting_username[100];
    struct game_info *Game;
}cli_info;

user_info Users[5] = {
    {"char", "char"},
    {"int", "int"},
    {"double", "double"},
    {"float", "float"},
    {"long", "long"}
};

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

/*-----------------------------------------------------------Login Handler-----------------------------------------------------------*/
void login_handler(char *buf, int request_client_index, int max_client_index, cli_info* client){
    char response[MAXLEN];
    char username[100], password[100];
    char *tok;
    int login_success;
    int userscount;
    int client_index;

    getcontent(buf, username, "Username");
    getcontent(buf, password, "Password");

    login_success = 0;
    for(userscount = 0; userscount < 5; userscount++){
        if((strcmp(Users[userscount].username, username) == 0) && (strcmp(Users[userscount].password, password) == 0)){
            login_success = 1;
            break;
        }
    }

    if(login_success){
        for(client_index = 0; client_index <= max_client_index; client_index++){
            if(strcmp(client[client_index].username, Users[userscount].username) == 0){
                sprintf(response,
                    "Response-Type: log in\n\r"
                    "Result: fail\n\r"
                    "Content: Login Fail! Duplicated Log in!!\n\r"
                );
                write(client[request_client_index].sockfd, response, strlen(response));
                return;
            }
        }
        sprintf(response, 
            "Response-Type: log in\n\r"
            "Result: success\n\r"
            "Username: %s\n\r"
            "Content: Login Success! Welcome, User %s !\n\r"
            ,username
            ,username
        );
        strcpy(client[request_client_index].username, username);
        strcpy(client[request_client_index].status, "available");
    }else{
        sprintf(response,
            "Response-Type: log in\n\r"
            "Result: fail\n\r"
            "Content: Login Fail! The Username Is Not Exist Or Wrong Password!\n\r"
        );
    }
    
    write(client[request_client_index].sockfd, response, strlen(response));
}

/*-----------------------------------------------------------List Handler-----------------------------------------------------------*/
void list_handler(int request_client_index, int max_client_index, cli_info *client){
    int client_index;
    char response[MAXLEN];
    char userinfo[MAXLEN];
    char *tok;

    sprintf(response, 
        "Response-Type: list\n\r"
        "Content: "
    );
    

    for(client_index = 0; client_index <= max_client_index; client_index++){
        tok = (response + strlen(response));
        if((client[client_index].sockfd < 0) || strcmp(client[client_index].status, "unconnected") == 0)
            continue;
        memset(userinfo, '\0', strlen(userinfo));
        sprintf(userinfo,
            "username: %s, "
            "status: %s\n"
            ,client[client_index].username
            ,client[client_index].status
        );
        printf("userinfo: %s\n", userinfo);
        strcpy(tok, userinfo);
    }
    response[strlen(response)] = '\r';
    response[strlen(response)] = '\0';
    write(client[request_client_index].sockfd, response, strlen(response));
}

/*-----------------------------------------------------------Chat Handler-----------------------------------------------------------*/
void chat_handler(char *buf, int request_client_index, int max_client_index, cli_info *client){
    char to_username[100];
    char content[MAXLEN];
    char response[MAXLEN];
    int client_index;

    getcontent(buf, to_username, "To");
    getcontent(buf, content, "Content");

    if(strcmp(to_username, "all") == 0){
        sprintf(response,
            "Response-Type: chat\n\r"
            "Result: success\n\r"
            "Chat-Type: All\n\r"
            "From: %s\n\r"
            "Content: %s\n\r"
            ,client[request_client_index].username
            ,content
        );
        for(client_index = 0; client_index <= max_client_index; client_index++){
            if(client[client_index].sockfd < 0 || strcmp(client[client_index].status, "unconnected") == 0)
                continue;
            write(client[client_index].sockfd, response, strlen(response));
        }
    }else{
        int to_user_index = -1;
        for(client_index = 0; client_index <= max_client_index; client_index++){
            if(client[client_index].sockfd < 0 || strcmp(client[client_index].status, "unconnected") == 0)
                continue;
            if(strcmp(client[client_index].username, to_username) == 0){
                to_user_index = client_index;
                break;
            }
        }
        if(to_user_index == -1){
            sprintf(response,
                "Response-Type: chat\n\r"
                "Result: fail\n\r"
                "Content: %s Is Not Online, Please Choose Other Users!\n\r"
                ,to_username
            );
            write(client[request_client_index].sockfd, response, strlen(response));
        }else{
            if(strcmp(client[to_user_index].username, client[request_client_index].username) == 0){
                sprintf(response,
                    "Response-Type: chat\n\r"
                    "Result: fail\n\r"
                    "Content: Can Not Send Massage To Yourself!\n\r"
                );
            }else{
                sprintf(response,
                    "Response-Type: chat\n\r"
                    "Result: success\n\r"
                    "Chat-Type: Private\n\r"
                    "From: %s\n\r"
                    "Content: %s\n\r"
                    ,client[request_client_index].username
                    ,content
                );
            }
            write(client[to_user_index].sockfd, response, strlen(response));
        }
        
    }
}

/*-----------------------------------------------------------Start Game Handler-----------------------------------------------------------*/
void start_game_handler(cli_info *client1, cli_info *client2){

    game *Game = malloc(sizeof(game));
    char response[MAXLEN];

    Game->turn = 0;
    Game->round = 0;
    strcpy(Game->rival[0], client1->username);
    strcpy(Game->rival[1], client2->username);
    strcpy(client1->rival, client2->username);
    strcpy(client2->rival, client1->username);
    strcpy(client1->status, "gaming");
    strcpy(client2->status, "gaming");
    client1->Game = Game;
    client2->Game = Game;
    for(int i = 0; i < 9; i++){
        Game->gamebuf[i] = ' ';
    }
    sprintf(response,
        "Response-Type: start-game\n\r"
        "Result: success\n\r"
        "Content: "
        "-------------\n"
        "| %c | %c | %c |\n"
        "-------------\n"
        "| %c | %c | %c |\n"
        "-------------\n"
        "| %c | %c | %c |\n"
        "-------------\n"
        "It's %s's turn\n\r",
        Game->gamebuf[0], Game->gamebuf[1], Game->gamebuf[2], Game->gamebuf[3], Game->gamebuf[4], Game->gamebuf[5], Game->gamebuf[6], Game->gamebuf[7], Game->gamebuf[8], Game->rival[Game->turn]
    );
    write(client1->sockfd, response, strlen(response));
    write(client2->sockfd, response, strlen(response));
    printf("start_game_finish\n");
    printf("rival0: %s, rival1: %s\n", Game->rival[0], Game->rival[1]);
}

/*-----------------------------------------------------------Match Handler-----------------------------------------------------------*/
void match_handler(char *buf, int request_client_index, int max_client_index, cli_info *client){
    char response[MAXLEN], rival[100], type[10], result[20];
    char match_type[20];
    int client_index, find_success;
    
    getcontent(buf, match_type, "Match-Type");
    if(strcmp(match_type, "invite") == 0){
        if(strcmp(client[request_client_index].status, "available") != 0){
            sprintf(response, 
                "Response-Type: match\n\r"
                "Match-Type: invite\n\r"
                "Result: fail\n\r"
                "Content: Can Only Send One Match Request At One Time, Please Wait For Response!\n\r"
            );
            write(client[request_client_index].sockfd, response, strlen(response));
            
        }else{
            
            int find_rival_success = 0;
            int rival_index;

            getcontent(buf, rival, "Rival");
            strcpy(client[request_client_index].inviting_username, rival);
            for(client_index = 0; client_index <= max_client_index; client_index++){
                if(client[client_index].sockfd < 0)
                    continue;
                if(strcmp(client[client_index].username, rival) == 0){
                    rival_index = client_index;
                    find_rival_success = 1;
                    break;
                }
            }
            
            if(find_rival_success){
                if(strcmp(client[rival_index].status, "available") == 0){
                    strcpy(client[request_client_index].status, "sending-match-request");
                    strcpy(client[rival_index].status, "receiving-match-request");
                    sprintf(response, 
                        "Response-Type: match\n\r"
                        "Match-Type: invited\n\r"
                        "Rival: %s\n\r"
                        "Content: Client %s Want To Play With You, Please Response Yes Or No (Yes/No):\n\r"
                        ,client[request_client_index].username
                        ,client[request_client_index].username
                    );
                    printf("res1-----------------------\n%s", response);
                    write(client[rival_index].sockfd, response, strlen(response));
                    sprintf(response,
                        "Response-Type: match\n\r"
                        "Match-Type: invite\n\r"
                        "Result: success\n\r"
                        "Content: Send Match Invite To %s Success, Please Wait For Response\n\r"
                        ,client[rival_index].username
                    );
                    printf("res2-----------------------\n%s", response);
                    write(client[request_client_index].sockfd, response, strlen(response));
                    strcpy(client[rival_index].invited_from_username, client[request_client_index].username);
                }else{
                    sprintf(response, 
                        "Response-Type: match\n\r"
                        "Match-Type: invite\n\r"
                        "Result: fail\n\r"
                        "Content: Client %s Is Busy, Please Wait A Minute Or Try Again:\n\r"
                        ,client[rival_index].username
                    );
                    write(client[request_client_index].sockfd, response, strlen(response));
                }
            }else{
                sprintf(response,
                    "Response-Type: match\n\r"
                    "Match-Type: invite\n\r"
                    "Result: fail\n\r"
                    "Content: Client %s Is Not Online, Please Find Other Opponent!\n\r"
                    ,client[request_client_index].inviting_username
                );
                write(client[request_client_index].sockfd, response, strlen(response));
            }
        }
    }else if(strcmp(match_type, "invited") == 0){
        char invite_result[100];
        int rival_index;

        getcontent(buf, invite_result, "Agree-Or-Reject");
        for(client_index = 0; client_index <= max_client_index; client_index++){
            if(client[client_index].sockfd < 0)
                continue;
            if(strcmp(client[client_index].username, client[request_client_index].invited_from_username) == 0){
                rival_index = client_index;
                break;
            }
        }
        
        if(strcmp(invite_result, "yes") == 0){
            //agree the match request
            sprintf(response, 
                "Response-Type: match\n\r"
                "Match-Type: invite\n\r"
                "Result: success\n\r"
                "Content: Client %s Receive Your Challenge, Ready For Start The Game.......\n\r"
                ,client[request_client_index].username  
            );
            write(client[rival_index].sockfd, response, strlen(response));
            sprintf(response,
                "Response-Type: match\n\r"
                "Match-Type: invited\n\r"
                "Rival: %s\n\r"
                "Result: success\n\r"
                "Content: Ready For Start The Game.......\n\r"
                ,rival
            );
            write(client[request_client_index].sockfd, response, strlen(response));
            sleep(2);
            start_game_handler(&client[request_client_index], &client[rival_index]);
        }else if(strcmp(invite_result, "no") == 0){
            //reject the match request
            sprintf(response, 
                "Response-Type: match\n\r"
                "Match-Type: invite\n\r"
                "Result: fail\n\r"
                "Content: %s Reject To Play With You, Please Find Other Opponent Or Try Again\n\r"
                ,client[request_client_index].username
            );
            write(client[rival_index].sockfd, response, strlen(response));
            strcpy(client[request_client_index].status, "available");
            strcpy(client[rival_index].status, "available");
        }    
    }
}

/*-----------------------------------------------------------Gaming Handler-----------------------------------------------------------*/
void gaming_handler(char *buf, int request_client_index, int max_client_index, cli_info *client){
    int row, col, client_index, wins;
    char pos[100], response[MAXLEN];
    char result[20], res_result[100];
    char surrender[10];
    game *Game;

    getcontent(buf, surrender, "Surrender");
    Game = client[request_client_index].Game;
    if(Game == NULL) return;
    int ret;
    if((ret = strcmp(surrender, "true")) == 0){
        
        int rival_index = -1;
        for(client_index = 0; client_index <= max_client_index; client_index++){
            if(strcmp(client[client_index].username, client[request_client_index].rival) == 0){
                rival_index = client_index;
                break;
            }
        }
        //printf("rival_index: %d, name: %s\n", rival_index, client[rival_index].username);
        if(rival_index != -1){
            sprintf(response, 
                "Reponse-Type: gaming\n\r"
                "Result: finish\n\r"
                "Content: %s Surrendered, You Win This Game!\n\r"
                ,client[request_client_index].username
            );
            strcpy(client[request_client_index].status, "available");
            strcpy(client[rival_index].status, "available");
            write(client[rival_index].sockfd, response, strlen(response));
            free(client[request_client_index].Game);
            client[request_client_index].Game = NULL;
            client[rival_index].Game = NULL;
        }
    }else if(strcmp(Game->rival[Game->turn], client[request_client_index].username) != 0){
        sprintf(response, 
            "Response-Type: gaming\n\r"
            "Result: fail\n\r"
            "Content: it's not your turn!\n\r"
        );
        write(client[request_client_index].sockfd, response, strlen(response));
    }else{
        getcontent(buf, pos, "Position");
        //printf("pos: %s\n", pos);
        row = atoi(pos);
        col = atoi(pos + 2);
        //printf("row: %d, col: %d\n", row, col);
    
        if((row >= 1 && row <= 3) && (col >= 1 && col <= 3)){
            if(Game->gamebuf[(row-1) * 3 + (col-1)] != ' '){
                sprintf(response,
                    "Response-Type: gaming\n\r"
                    "Result: fail\n\r"
                    "Content: invalid position!\n\r"
                );
                write(client[request_client_index].sockfd, response, strlen(response));
            }else{
                //更新棋盤
                (Game->round)++;
                if(Game->turn == 0){
                    Game->gamebuf[(row-1) * 3 + (col-1)] = 'O';
                    Game->turn = 1;
                }else{
                    Game->gamebuf[(row-1) * 3 + (col-1)] = 'X';
                    Game->turn = 0;
                }
                //檢查棋盤目前結果
                wins = -1;
                for(int j = 0; j < 3; j++){
                    if((Game->gamebuf[j*3] == Game->gamebuf[j*3+1]) && (Game->gamebuf[j*3+1] == Game->gamebuf[j*3+2])){
                        if(Game->gamebuf[j*3] == 'O'){
                            wins = 0;
                        }else if(Game->gamebuf[j*3] == 'X'){
                            wins = 1;
                        }
                        break;
                    }

                    if((Game->gamebuf[j] == Game->gamebuf[j+3]) && (Game->gamebuf[j+3] == Game->gamebuf[j+6])){
                        if(Game->gamebuf[j] == 'O'){
                            wins = 0;
                        }else if(Game->gamebuf[j] == 'X'){
                            wins = 1;
                        }
                        break;
                    }
                }
                //比對斜線
                if(((Game->gamebuf[0] == Game->gamebuf[4]) && (Game->gamebuf[4] == Game->gamebuf[8])) || ((Game->gamebuf[2] == Game->gamebuf[4]) && (Game->gamebuf[4] == Game->gamebuf[6]))){
                    if(Game->gamebuf[4] == 'O'){
                        wins = 0;
                    }else if(Game->gamebuf[4] == 'X'){
                        wins = 1;
                    }
                    //printf("row --------   %c      %c      %c\n", Game->gamebuf[4], Game->gamebuf[4], Game->gamebuf[4]);
                }
                
                if(wins != -1){
                    strcpy(result, "end");
                    sprintf(res_result, "---%s Win This Game---", Game->rival[wins]);
                }else{
                    if(Game->round == 9){
                        strcpy(result, "finish");
                        sprintf(res_result, "The Game Is Ended In a Tie");
                    }else{
                        strcpy(result, "success");
                        sprintf(res_result, "it's %s's turn", Game->rival[Game->turn]);
                    }
                }

                sprintf(response, "---%s wins---", Game->rival[0]);
                sprintf(response, 
                    "Response-Type: gaming\n\r"
                    "Result: %s\n\r"
                    "Content: "
                    "-------------\n"
                    "| %c | %c | %c |\n"
                    "-------------\n"
                    "| %c | %c | %c |\n"
                    "-------------\n"
                    "| %c | %c | %c |\n"
                    "-------------\n"
                    "%s\n\r",
                    result,
                    Game->gamebuf[0], Game->gamebuf[1], Game->gamebuf[2], Game->gamebuf[3], Game->gamebuf[4], Game->gamebuf[5], Game->gamebuf[6], Game->gamebuf[7], Game->gamebuf[8], 
                    res_result
                );
                //printf("cur turn: %d\n", Game->turn);
                for(client_index = 0; client_index <= max_client_index; client_index++){
                    if(client[client_index].sockfd < 0)
                        continue;
                    if(strcmp(client[client_index].username, Game->rival[Game->turn]) == 0){
                        //printf("cmp ok\n");
                        //printf("client reqi: %s, G->turn: %s\n", client[request_client_index].username, Game->rival[Game->turn]);
                        break;
                    }
                }
                if(strcmp(result, "end") == 0){
                    strcpy(client[request_client_index].status, "available");
                    strcpy(client[client_index].status, "available");
                    free(client[request_client_index].Game);
                    client[request_client_index].Game = NULL;
                    client[client_index].Game = NULL;
                }
               
                write(client[client_index].sockfd, response, strlen(response));
                write(client[request_client_index].sockfd, response, strlen(response));
            } 
        }else{
            sprintf(response,
                "Response-Type: gaming\n\r"
                "Result: fail\n\r"
                "Content: invalid instruction!\n\r"
            );
            write(client[request_client_index].sockfd, response, strlen(response));
        }
    }    
}

/*-----------------------------------------------------------Client Exit Handler-----------------------------------------------------------*/
void client_exit_handler(int exit_client_index, int max_client_index, cli_info *client){
    char response[MAXLEN];

    if(strcmp(client->status, "gaming") == 0){
        int rival_index = -1;
        for(rival_index = 0; rival_index <= max_client_index; rival_index++){
            if(strcmp(client[exit_client_index].rival, client[rival_index].username) == 0)
                break;
        }
        if(rival_index != -1){
            game *Game = client[exit_client_index].Game;
            sprintf(response,
                "Response-Type: gaming\n\r"
                "Result: finish\n\r"
                "Content: "
                "%s Has Been Disconnected, You Win This Game!\n\r",
                client[exit_client_index].username
            );
            write(client[rival_index].sockfd, response, strlen(response));
        }
    }else if(strcmp(client->status, "receiving-match-request") == 0){
        int invited_from_index = -1;
        for(invited_from_index = 0; invited_from_index <= max_client_index; invited_from_index++){
            if(strcmp(client[exit_client_index].invited_from_username, client[invited_from_index].username) == 0){
                break;
            }
        }
        if(invited_from_index != -1){

            sprintf(response,
                "Response-Type: match\n\r"
                "Match-Type: invite\n\r"
                "Result: fail\n\r"
                "Content: %s Has Been Disconnected, Please Find Other Opponent!\n\r"
                ,client[exit_client_index].username
            );
            write(client[invited_from_index].sockfd, response, strlen(response));
        }
    }

    close(client[exit_client_index].sockfd);
    client[exit_client_index].sockfd = -1;
    strcpy(client[exit_client_index].status, "unconnected");
    memset(client[exit_client_index].username, 0, strlen(client[exit_client_index].username));
    memset(client[exit_client_index].rival, 0, strlen(client[exit_client_index].rival));   
}

/*-----------------------------------------------------------Log out Handler-----------------------------------------------------------*/
void logout_handler(cli_info *client){
    strcpy(client->status, "unconnected");
    memset(client->username, 0, strlen(client->username));
    memset(client->rival, 0, strlen(client->rival));
}

