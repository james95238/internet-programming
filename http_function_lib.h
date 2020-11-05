#include<sys/socket.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<stddef.h>
#include<fcntl.h>
#include<dirent.h>

int readline(int fd, char *buf){
    char read_c, *bufptr;
    int n_read;
    int line_len = 0;

    bufptr = buf;
    while(n_read = read(fd, &read_c, 1)){
        if(n_read == -1){
	    printf("read error, exit!\n");
	    exit(0);
	}else if(n_read == 1){
	    *bufptr = read_c;
	    bufptr++;
	    line_len++;
	}
	if(read_c == '\n'){
	    *bufptr = '\0';
	    break;
	}
    }

    return line_len;
}

int parse_http_field(char *src, char *dst, char token1, char token2){
    char *src_ptr = src;
    char *dst_ptr = dst;
    int ret = 0;
    
    for( ; *src_ptr != token1; src_ptr++);
    src_ptr++;
    for( ; *src_ptr != token2; src_ptr++, dst_ptr++){
        *dst_ptr = *src_ptr;
        ret++;
    }
    *dst_ptr = '\0';
    return ret;
}

void homepage_handler(int sockfd){
    int fd, filecount = 0;
    char response[1024], readbuf[1024], *find_s;
    int readcount;
    struct dirent *dir_p;
    sprintf(response, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
    );
    write(sockfd, response, strlen(response));
    DIR* curdir = opendir("./upload/upload_file");
    fd = open("./upload/index.html", O_RDWR, S_IRWXU);
    
    while((readcount = read(fd, readbuf, 1024)) > 0){
        readbuf[readcount] = '\0';
        printf("%s", readbuf);
        printf("readcount: %d\n", readcount);

        if((find_s = strstr(readbuf, "No FILE")) != NULL){
            ptrdiff_t front_len = find_s - readbuf;
            write(sockfd, readbuf, front_len);
            while((dir_p = readdir(curdir)) != NULL){
                if(dir_p->d_name[0] == '.')
                    continue;
                sprintf(response, "<a href=\"/%s\" download=\"%s\"><font size = \"20\"> %s </font></a><br>",dir_p->d_name, dir_p->d_name, dir_p->d_name);
                printf("response: %s\n", response);
                write(sockfd, response, strlen(response));
                filecount++;
            }
            closedir(curdir);
            if(filecount == 0){
                write(sockfd, find_s, strlen(readbuf) - front_len);
            }else{
                find_s += strlen("No FILE");
                write(sockfd, find_s, strlen(readbuf)-strlen("No FILE")-front_len);
            }
        }else{
            write(sockfd, readbuf, readcount);
        }
    }
      
}

void http_GET_handler(int sockfd, char *request_line){
    char http_header[1024];
    char request_url[100], filepath[100];
    char *blank1, *blank2, *srcptr, *dstptr;
    char response_header[1024];
    char readbuf[1024];
    int nread;
    int open_fd;
    int readcount;

    //get request URL
    srcptr = request_line;
    dstptr = request_url;
    while(*srcptr != ' ') srcptr++;
    srcptr++;
    for( ; *srcptr != ' '; srcptr++, dstptr++) 
	    *dstptr = *srcptr;
    *dstptr = '\0';
    printf("request_URL: %s\n", request_url);
    
    while((readcount = readline(sockfd, readbuf)) > 2)
        printf("%s", readbuf);

    //open request url file
    if(strcmp(request_url, "/") == 0 || strcmp(request_url, "/index.html") == 0){
        homepage_handler(sockfd);
        return;
    }else if(strcmp(request_url, "/index.jpg") == 0){
        open_fd = open("./upload/index.jpg", O_RDONLY);
    }else if(strcmp(request_url, "/favicon.ico") == 0){
        open_fd = open("./upload/favicon.ico", O_RDONLY);
    }else{
        sprintf(filepath, "./upload/upload_file%s", request_url);
        open_fd = open(filepath, O_RDONLY);
    }

    if(open_fd == -1){
        printf("file open error!\n");
	sprintf(response_header, 
	    "HTTP/1.1 404 Not Found\r\n"	
	    "Connection: close\r\n"
	    "\r\n"
	);
	write(sockfd, response_header, strlen(response_header));
    }else if(open_fd > 0){
        printf("file open succeed!\n");
        printf("-----------start sending data------------\n");
        if(strstr(request_url, "jpg")!= NULL || strstr(request_url, "ico")!= NULL){
            sprintf(response_header,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: image/jpg\r\n"
                "Connection: close\r\n"
                "\r\n"
            );     
        }else{
	    sprintf(response_header, 
	        "HTTP/1.1 200 OK\r\n"
	        "Content-Type: text/html\r\n"
                "Connection: close\r\n"
                "\r\n"	    
	    );
        }
	write(sockfd, response_header, strlen(response_header));
	while((nread = read(open_fd, readbuf, 1024)) > 0){
	    write(sockfd, readbuf, nread);
	}
	printf("-----------sending data finish------------\n");
        close(open_fd);
    }
    return;
}

void http_POST_handler(int sockfd){

    char http_request_header[1024];
    char http_response[1024];
    char content_length[20];
    char request_readline[1024];
    char post_filename[100] = "./upload/upload_file/";
    char readbuf[8192];
    char *find_s;
    int readcount, ret;
    int content_size;
    int writefile_fd, open_fd;
    printf("sockfd: %d\n", sockfd);
    //Http Response
    sprintf(http_response, 
	"HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"	
        "\r\n"
    );
    write(sockfd, http_response, strlen(http_response));
    
    //read http header
    while((readcount = readline(sockfd, request_readline)) > 2){
	//printf("readcount: %d\n", readcount);
	printf("%s", request_readline);
        if(strstr(request_readline, "Content-Length") != NULL){
	    ret = parse_http_field(request_readline, content_length, ' ', '\0');
            printf("content-length: %s", content_length);
	    content_size = atoi(content_length);
            printf("int_content_length: %d\n", content_size);	    
	}
    }
    
    //remove http body boundary length
    readcount = readline(sockfd, request_readline);
    printf("%s", request_readline);
    content_size -= ((readcount * 2) + 2);
    //get upload filename, remove the length
    readcount = readline(sockfd, request_readline);
    printf("%s", request_readline);
    find_s = strstr(request_readline, "filename");
    if(find_s != NULL){
        ret = parse_http_field(find_s, post_filename + strlen(post_filename), '"', '"');
        //若檔名為空，回傳上傳失敗頁面
        if(ret == 0){
            readcount = read(sockfd, readbuf, 8192);
            open_fd = open("./upload/upload_fail.html", O_RDONLY);
            if(open_fd < 0){
                printf("open_upload_fail_html failed\n");
            }else{
                while((readcount = read(open_fd, readbuf, 8192)) > 0){
                    printf("readbuf: %s\n", readbuf);
                    write(sockfd, readbuf, readcount);
                }
            }
            return;
        }
    }
    
    //remove the length
    content_size -= readcount;
    printf("post_filename: %s\n", post_filename);
    //remove the length
    readcount = readline(sockfd, request_readline);
    printf("%s", request_readline);
    content_size -= readcount;
    //remove the length
    readcount = readline(sockfd, request_readline);
    content_size -= (readcount * 2);

    //reading data
    printf("-----------start reading data------------\n");
    printf("===========initial content size: %d===========\n", content_size);

    writefile_fd = open(post_filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    if(writefile_fd < 0){
        printf("open write file failed!\n");
	exit(0);
    }

    while(content_size > 0){
        if(content_size < 8192)
            readcount = read(sockfd, readbuf, content_size);
        else
            readcount = read(sockfd, readbuf, 8192);
        content_size -= readcount;
        write(writefile_fd, readbuf, readcount);
    }
    
    printf("-----------writing data finish------------\n");
    //讀完所有body的內容
    readcount = read(sockfd, readbuf, 8192);

    //回傳上傳成功頁面
    open_fd = open("./upload/upload_success.html", O_RDONLY);
    while((readcount = read(open_fd, readbuf, 8192)) > 0){
        write(sockfd, readbuf, readcount);
    }
    close(writefile_fd);
    return;
}
