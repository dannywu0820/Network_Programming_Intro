#include<stdio.h>
#include<unistd.h> //bzero
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#define BUF_SIZE 128

bool Display(char *msg){
    //char *s1, *s2, *s3;
    bool win = false;
    int matrix[16], index = 23, order = 0;
    for(int i = 0; i < 16; i++) matrix[i] = 0;
    //sscanf(msg, "%23s%31s", s1, s2); //why segmentation fault?
    //printf("s1:%s s2:%s\n", s1, s2);
    while(msg[index] != NULL){
      if(msg[index] == '}') break; //since recv_msg
      if(msg[index] >= '0' && msg[index] <= '9'){
          matrix[order] = matrix[order]*10 + (msg[index]- '0');
      }
      if(msg[index] == ',') order++;
      index++;
    }
    printf("---------------------\n");
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 4; j++){
            if(matrix[4*i + j] == 2048) win = true;
            if(matrix[4*i + j] == 0){ printf("|    ");}
            else { printf("|%4d", matrix[4*i + j]);}
            if(j == 3) printf("|\n");    
        }
        printf("---------------------\n");
    }
    if(win) printf("\nCongrats! You win the game!\n");
    return win;
}

void Act(int &Action, int &Cstatus, int &Nstatus, int &Sockfd){
    char *A[8];
    A[0] = "New";
    A[1] = "End";
    A[2] = "moveUp";
    A[3] = "moveDown";
    A[4] = "moveLeft";
    A[5] = "moveRight";
    A[6] = "unDo";
    A[7] = "whosyourdaddy";
    char send_msg[BUF_SIZE], recv_msg[BUF_SIZE];
    sprintf(send_msg, "{\"action\":\"%s\"}", A[Action]);
    //printf("%s\n", send_msg);

    if(Cstatus == 0){ printf("Please connect to server first\n");}
    else{ 
        if(Action == 0){//new
            if(Nstatus == 0){
                Nstatus = 1;
                write(Sockfd, send_msg, strlen(send_msg));
                read(Sockfd, recv_msg, BUF_SIZE);
                //printf("%s\n", recv_msg);
                Display(recv_msg);
            }
            else{ printf("Have already in a game round\n");}
        }
        else if(Action == 1){//end
            if(Nstatus == 0){ printf("Please new a game round first\n");}
            else{
                Nstatus = 0;
                //Lstatus = 0;
                write(Sockfd, send_msg, strlen(send_msg));
                read(Sockfd, recv_msg, BUF_SIZE);
                //printf("%s\n", recv_msg);
                printf("The game has closed\n");
            }
        }
        else{
            if(Nstatus == 0){ printf("Please new a game round first\n");}
            else{
                write(Sockfd, send_msg, strlen(send_msg));
                read(Sockfd, recv_msg, BUF_SIZE);
                if(strncmp(recv_msg, "{\"status\":0", 11) == 0){ printf("not change\n");}
                else{
                    //printf("%s\n", recv_msg);
                    bool Win = Display(recv_msg);
                    if(Win){
                        Nstatus = 0;
                        sprintf(send_msg,"{\"action\":\"End\"}");
                        write(Sockfd, send_msg, strlen(send_msg));
                        read(Sockfd, recv_msg, BUF_SIZE);
                        //printf("%s\n", recv_msg);
                        printf("The game has closed\n");
                    } 
                }
            }
        }
    }
    
}

int main(){
    char *server_ip = "140.113.65.28", cmd[10];
    int server_port = 5566, sockfd, action; //file descriptor
    struct sockaddr_in server_addr;

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    printf("Welcome to Game 2048!\n");
    printf("enter 'help' to get more information\n\n>");
    scanf("%s", &cmd);
    
    int Connect = 0, New = 0;   
  
    while(1){
        //printf("%s\n", cmd);
        if(strcmp("help", cmd) == 0){ action = 10;}
        else if(strcmp("connect", cmd) == 0){ action = 8;}
        else if(strcmp("disconnect", cmd) == 0){ action = 9;}
        else if(strcmp("new", cmd) == 0){ action = 0;}
        else if(strcmp("end", cmd) == 0){ action = 1;}
        else if(strcmp("w", cmd) == 0){ action = 2;}
        else if(strcmp("s", cmd) == 0){ action = 3;}
        else if(strcmp("a", cmd) == 0){ action = 4;}
        else if(strcmp("d", cmd) == 0){ action = 5;}
        else if(strcmp("u", cmd) == 0){ action = 6;}
        else if(strcmp("whosyourdaddy", cmd) == 0){ action = 7;}
        else{ action = 11;}
        //printf("%d\n", action);
        if(action == 10){ 
            printf("Enter keyboard:\n");
            printf("'connect' - connect to game server\n");
            printf("'disconnect' - disconnect from game server\n");
            printf("'new' - new a game round\n");
            printf("'end' - close the game\n");
            printf("'w' - move bricks up\n");
            printf("'s' - move bricks down\n");
            printf("'a' - move bricks left\n");
            printf("'d' - move bricks right\n");
            printf("'u' - undo the last move\n");
        }
        else if(action == 8){//connect
            if(Connect == 0){
                Connect = 1;
                sockfd = socket(AF_INET, SOCK_STREAM, 0);
                int connect_ret = connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)); 
                if(connect_ret == 0) printf("connect to game server\n");
                else{
                    fprintf(stderr, "failed connect()");
                    exit(-1);
                }
            }
            else{ printf("Have already connected to server\n");}
        }
        else if(action == 9){//disconnect
            if(Connect == 0) printf("Please connect to server first\n");
            else{
                Connect = 0;
                New = 0;
                //Last = 0;
                int close_ret = close(sockfd);
                if(close_ret == 0) printf("disconnect from game server\n");
                else{
                    fprintf(stderr, "failed close()");
                    exit(-1);
                }
            }
        }
        else if(action >= 0 && action <= 7){
            Act(action, Connect, New, sockfd);
        }
        else{ printf("No such command\n");}

        if(New == 1) printf("move");
        printf(">");
        scanf("%s", &cmd);
    }
}
