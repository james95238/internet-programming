#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/types.h>
#define MAXLEN 1024

typedef struct chat_history{
    char chat_record[20][MAXLEN];
    int newest_chat_index;
    int chat_record_count;
}CH_HIS;
CH_HIS chat_history = {.newest_chat_index = 0, .chat_record_count = 0};
char my_username[100];

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


int send_login_request_handler(int sockfd, char *username, char *password){
    char req[MAXLEN], res[MAXLEN];
    sprintf(req,
        "Request-Type: log in\n\r"
        "Username: %s\n\r"
        "Password: %s\n\r",
        username,
        password
    );
    write(sockfd, req, strlen(req));
}

void send_list_request_handler(int sockfd){
    char request[MAXLEN];
    sprintf(request, 
        "Request-Type: list\n\r"
    );
    write(sockfd, request, strlen(request));
}

void send_match_request_handler(int sockfd, char *match_type, char *rival_name, char *result){
    char request[MAXLEN];
    if(strcmp(match_type, "inviting") == 0){
        sprintf(request,
            "Request-Type: match\n\r"
            "Match-Type: invite\n\r"
            "Rival: %s\n\r"
            ,rival_name
        );
    }else if(strcmp(match_type, "invited") == 0){
        sprintf(request,
            "Request-Type: match\n\r"
            "Match-Type: invited\n\r"
            "Agree-Or-Reject: %s\n\r"
            ,result
        );
    }else{
        return;
    }
    
    write(sockfd, request, strlen(request));
}

void send_game_request_handler(int sockfd, int surrender, char *my_status, char *position){
    char request[MAXLEN], surrender_s[10];
    if(surrender){
        strcpy(surrender_s, "true");
        strcpy(my_status, "available");
    }else{
        strcpy(surrender_s, "false");
    }
    sprintf(
        request,
        "Request-Type: gaming\n\r"
        "Surrender: %s\n\r"
        "Position: %s\n\r"
        ,surrender_s
        ,position
    );
    write(sockfd, request, strlen(request));
}

void send_logout_request_handler(char *my_status, int sockfd){
    char request[MAXLEN];

    sprintf(request,
        "Request-Type: log out\n\r"
    );
    write(sockfd, request, strlen(request));
    strcpy(my_status, "unconnected");
}

void send_chat_request_handler(int sockfd, char *chat_type, char *src){

    char response[MAXLEN];
    char record[MAXLEN];
    
    if(strcmp(chat_type, "all") != 0){
        sprintf(record,
            "(Private)(To %s): %s", chat_type, src
        );
        memset(chat_history.chat_record[chat_history.newest_chat_index], 0, MAXLEN);
        strcpy(chat_history.chat_record[chat_history.newest_chat_index], record);
        if(chat_history.chat_record_count < 20)
            chat_history.chat_record_count++;
        if(chat_history.newest_chat_index == 19){
            chat_history.newest_chat_index = 0;
        }else{
            chat_history.newest_chat_index++;
        }
        printf("%s\n", record);
    }

    sprintf(response, 
        "Request-Type: chat\n\r"
        "To: %s\n\r"
        "Content: %s\n\r"
        ,chat_type
        ,src
    );
    write(sockfd, response, strlen(response));
}

void send_server_request(char *my_status, char *chat_type, int sockfd, char *src){
    //printf("mystatus: %s, src: %s\n", my_status, src);
    char req[MAXLEN], *tok;

    if(strcmp(src, "chatall") == 0){
        printf("Current Chat Channel Is ALL\n");
        strcpy(chat_type, "all");
    }else if(strncmp(src, "to ", 3) == 0){
        printf("Current Chat To %s\n", src + 3);
        strcpy(chat_type, src + 3);
    }else if(strncmp(src, "send ", 5) == 0){
        if(strcmp(chat_type, "unknown") == 0){
            printf("Please Select The Chat Channel!\n");
        }else{
            send_chat_request_handler(sockfd, chat_type, src + 5);
        }
    }else if(strcmp(src, "history") == 0){
        if(chat_history.chat_record_count > 0){
            for(int i = chat_history.newest_chat_index; i < chat_history.chat_record_count; i++){
                printf("%s\n", chat_history.chat_record[i]);        
            }
            for(int i = 0; i < chat_history.newest_chat_index; i++){
                printf("%s\n", chat_history.chat_record[i]);
            }
        }else{
            printf("There Is No Chatting History!\n");
        }
    }else if(strcmp(my_status, "unconnected") == 0){
        if(strcmp(src, "login") == 0){
            char username[100], password[100];
            printf("Please Enter The Username\n");
            fgets(username, 100, stdin);
            printf("Please Enter The Password\n");
            fgets(password, 100, stdin);
            username[strlen(username) - 1] = '\0';
            password[strlen(password) - 1] = '\0';
            send_login_request_handler(sockfd, username, password);
        }else{
            printf("You Have not Login Yet, Please Login First!\n");
        }
    }else if(strcmp(my_status, "available") == 0){
        if(strcmp(src, "list") == 0){
            send_list_request_handler(sockfd);
        }else if((tok = strstr(src, "match")) != NULL){
            char rival_name[100];
            tok += (strlen("match") + 1);
            strcpy(rival_name, tok);
            send_match_request_handler(sockfd, "inviting", rival_name, NULL);
            strcpy(my_status, "inviting");
        }else if(strcmp(src, "logout") == 0){
            send_logout_request_handler(my_status, sockfd);
        }else{
            printf("No Such Instruction! Please Try Again!\n");
        }
    }else if(strcmp(my_status, "invited") == 0){
        char agree_or_reject[10];
        if((strcmp(src, "yes") == 0) || (strcmp(src, "no") == 0)){
            strcpy(agree_or_reject, src);
            send_match_request_handler(sockfd, "invited", NULL, agree_or_reject);
            if(strcmp(src, "no") == 0){
                strcpy(my_status, "available");
            }
        }else{
            printf("Please Answer Yes or No!\n");
        }
    }else if(strcmp(my_status, "inviting") == 0){
        printf("You have Send A Match Request, Please Wait For Resposne!\n");
    }else if(strcmp(my_status, "gaming") == 0){

        if(strcmp(src, "list") == 0){
            send_list_request_handler(sockfd);
        }else if(strcmp(src, "surrender") == 0){
            send_game_request_handler(sockfd, 1, my_status, src);
        }else{
            send_game_request_handler(sockfd, 0, my_status, src);
        }
    }
}

void recv_login_response_handler(char *my_status, char *buf){
    char result[20];
    char content[MAXLEN];
    char username[100];

    getcontent(buf, result, "Result");
    getcontent(buf, content, "Content");
    getcontent(buf, username, "Username");

    if(strcmp(result, "success") == 0){
        strcpy(my_status, "available");
        strcpy(my_username, username);        
    }
    printf("%s\n", content);
}

void recv_list_response_handler(char *buf){
    char content[MAXLEN];

    getcontent(buf, content, "Content");
    printf("%s\n", content);
}

void recv_match_response_handler(char *my_status, char *buf){
    char match_type[100];
    char result[20];
    char content[MAXLEN];

    getcontent(buf, match_type, "Match-Type");
    getcontent(buf, result, "Result");
    getcontent(buf, content, "Content");
    if(strcmp(match_type, "invite") == 0){
        printf("%s\n", content);
        if(strcmp(result, "fail") == 0){
            strcpy(my_status, "available");
        }
    }else if(strcmp(match_type, "invited") == 0){
        printf("%s\n", content);
        strcpy(my_status, "invited");       
    }
}

void recv_chat_response_handler(char *buf){
    char content[MAXLEN];
    char from_username[100];
    char chat_type[100];
    char record[MAXLEN];
    char result[20];

    getcontent(buf, chat_type, "Chat-Type");
    getcontent(buf, content, "Content");
    getcontent(buf, from_username, "From");
    getcontent(buf, result, "Result");
    if(strcmp(result, "fail") == 0){
        strcpy(record, content);
    }else{
        sprintf(record,
            "(%s)(From %s): %s", chat_type, from_username, content
        );
    }
    printf("%s\n", record);
    memset(chat_history.chat_record[chat_history.newest_chat_index], 0, MAXLEN);
    strcpy(chat_history.chat_record[chat_history.newest_chat_index], record);
    if(chat_history.chat_record_count < 20)
        chat_history.chat_record_count++;
    if(chat_history.newest_chat_index == 19){
        chat_history.newest_chat_index = 0;
    }else{
        chat_history.newest_chat_index++;
    }
    
}

void recv_start_game_response_handler(char *my_status, char *buf){
    char content[MAXLEN];
    getcontent(buf, content, "Content");
    strcpy(my_status, "gaming");
    system("clear");
    printf("%s\n", content);
}

void recv_gaming_response_handler(char *my_status, char *buf){
    char result[20];
    char content[MAXLEN];
    
    getcontent(buf, result, "Result");
    getcontent(buf, content, "Content");

    if(strcmp(result, "success") == 0){
        system("clear");
    }else if(strcmp(result, "finish") == 0){
        strcpy(my_status, "available");
    }else if(strcmp(result, "end") == 0){
        system("clear");
        strcpy(my_status, "available");
    }
    printf("status: %s\n", my_status);
    printf("%s\n", content);
}

void recv_server_response(char *my_status, char *buf){
    char response_type[100];

    getcontent(buf, response_type, "Response-Type");
    if(strcmp(response_type, "log in") == 0){
        recv_login_response_handler(my_status, buf);
    }else if(strcmp(response_type, "list") == 0){
        recv_list_response_handler(buf);
    }else if(strcmp(response_type, "match") == 0){
        recv_match_response_handler(my_status, buf);
    }else if(strcmp(response_type, "chat") == 0){
        recv_chat_response_handler(buf);
    }else if(strcmp(response_type, "start-game") == 0){
        recv_start_game_response_handler(my_status, buf);
    }else if(strcmp(response_type, "gaming") == 0){
        recv_gaming_response_handler(my_status, buf);
    }
}






