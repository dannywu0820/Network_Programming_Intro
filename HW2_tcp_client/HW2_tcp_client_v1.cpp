#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#define BUF_SIZE 128

void display(char *msg, int &win){ //wrong way to display
    char *num = msg;
    int count = 0;
    int status;
    int m[16], m_index = 0, num_index = 23;
    
    for(int i = 0; i < 16; i++) m[i] = 0;
    //sscanf(msg,"{\"status\":%d,\"message\":\"%s", &status, num);
    while(num[num_index] != NULL && count < 16){
        if(num[num_index] >= '0' && num[num_index] <= '9'){
            m[m_index] = m[m_index] * 10 + (num[num_index] - '0');
        }
        if(num[num_index] == ','){
            //printf("m[%d]:%d\n", m_index, m[m_index]);
            m_index++;
            count++;
        }
        num_index++;
    }
    //for(int i = 0; i < 16; i++) printf("%d ", m[i]);
    printf("---------------------\n");
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 4; j++){
            if(m[(4*i+j)] == 2048) win = 1;
            printf("|%4d", m[(4*i+j)]);
            if(j == 3) printf("|\n");
        }
        printf("---------------------\n");
    }
    
}

int main(){
   char *server_ip = "140.113.65.28";
   int server_port = 5566, sockfd;
   struct sockaddr_in server_addr;


   bzero(&server_addr, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(server_port);
   inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

   char action[10];
   int Connect = 0, New = 0, Last = 0, Win = 0;
   char *send_msg, recv_msg[BUF_SIZE], *prev_msg;

   printf("Welcome to Game 2048!\n");
   printf("enter 'help' to get more information\n\n");
   printf(">");
   scanf("%s", &action);


   while(1){
       if(strcmp("help", action) == 0){
           printf("Enter keyboard:\n");
           printf("'connect' - connect to game server\n");
           printf("'disconnect' - disconnect from game server\n");
           printf("'new' - new a game round\n");
           printf("'end' - close the game\n");
           printf("'w' - move bricks up\n");
           printf("'s' - move bricks down\n");
           printf("'a' - move bricks left\n");
           printf("'d' - move bricks down\n");
           printf("'u' - undo the last move\n");
       }
       if(strcmp("connect", action) == 0){
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
           else{
               printf("Have already connected to server\n");
           }
       }
       if(strcmp("disconnect", action) == 0){
           if(Connect == 0){
               printf("Please connect to server first\n");
           }
           else{
               Connect = 0;
               New = 0;
               Last = 0;
               int close_ret = close(sockfd);
               if(close_ret == 0) printf("disconnect from game server\n");
               else{
                   fprintf(stderr, "failed close()");
                   exit(-1);
               }
           }
       }
       if(strcmp("new", action) == 0){
           if(Connect == 0){printf("Please connect to server first\n");}
           else{
               if(New == 0){
                   New = 1;
                   //Last++;
                   send_msg = "{\"action\":\"New\"}";
                   write(sockfd, send_msg, strlen(send_msg));
                   read(sockfd, recv_msg, BUF_SIZE);
                   //if(strcmp(recv_msg, prev_msg) == 0) printf("not change\n");
                   //prev_msg = recv_msg;
                   //printf("%s\n", recv_msg);
                   display(recv_msg, Win);
               }
               else{printf("Have already in a game round\n");}
           }
       }
       if(strcmp("end", action) == 0){
          if(Connect == 0){printf("Please connect to server first\n");}
          else{
              if(New == 0){printf("Please new a game round first\n");}
              else{
                  New = 0;
                  Last = 0;
                  send_msg = "{\"action\":\"End\"}";
                  write(sockfd, send_msg, strlen(send_msg));
                  read(sockfd, recv_msg, BUF_SIZE);
                  //printf("%s\n", recv_msg);
                  printf("The game has closed\n");
              }
          }
       }
       if(strcmp("s", action) == 0){
              if(Connect == 0){printf("Please connect to server first\n");}
              else{
                  if(New == 0){printf("Please new a game round first\n");}
                  else{
                      //New = 0;
                      send_msg = "{\"action\":\"moveDown\"}";
                      write(sockfd, send_msg, strlen(send_msg));
                      read(sockfd, recv_msg, BUF_SIZE);
                      //printf("prev:%s\n", prev_msg);
                      //printf("%s\n", recv_msg);
                      if(strncmp(recv_msg, "{\"status\":0",11) == 0) printf("not change\n");
                      else{
                          display(recv_msg, Win);
                          Last++;
                          //prev_msg = recv_msg;
                          if(Win == 1){
                               printf("\nCongrats! You win the game\n");
                               printf("The game has closed\n");
                               send_msg = "{\"action\":\"End\"}";
                               write(sockfd, send_msg, strlen(send_msg));
                               read(sockfd, recv_msg, BUF_SIZE);
                               New = 0;
                               Last = 0;
                               Win = 0;
                          }
                      }
                  }
              }
       }
       if(strcmp("w", action) == 0){
               if(Connect == 0){printf("Please connect to server first\n");}
               else{
                   if(New == 0){printf("Please new a game round first\n");}
                   else{
                       //New = 0;
                       send_msg = "{\"action\":\"moveUp\"}";
                       write(sockfd, send_msg, strlen(send_msg));
                       read(sockfd, recv_msg, BUF_SIZE);
                       //printf("prev:%s\n", prev_msg);
                       //printf("%s\n", recv_msg);
                       if(strncmp(recv_msg, "{\"status\":0",11) == 0) printf("not change\n");
                       else{
                           display(recv_msg, Win);
                           Last++;
                           //prev_msg = recv_msg;
                           if(Win == 1){
                                printf("\nCongrats! You win the game\n");
                                printf("The game has closed\n");
                                send_msg = "{\"action\":\"End\"}";
                                write(sockfd, send_msg, strlen(send_msg));
                                read(sockfd, recv_msg, BUF_SIZE);
                                New = 0;
                                Last = 0;
                                Win = 0;
                           }

                       }
                   }
               }
       }
       if(strcmp("a", action) == 0){
                if(Connect == 0){printf("Please connect to server first\n");}
                else{
                    if(New == 0){printf("Please new a game round first\n");}
                    else{
                        //New = 0;
                        send_msg = "{\"action\":\"moveLeft\"}";
                        write(sockfd, send_msg, strlen(send_msg));
                        read(sockfd, recv_msg, BUF_SIZE);
                        //printf("prev:%s\n", prev_msg);
                        //printf("%s\n", recv_msg);
                        if(strncmp(recv_msg, "{\"status\":0",11) == 0) printf("not change\n");
                        else{
                            display(recv_msg, Win);
                            Last++;
                            //prev_msg = recv_msg;
                            if(Win == 1){
                                  printf("\nCongrats! You win the game\n");
                                  printf("The game has closed\n");
                                  send_msg = "{\"action\":\"End\"}";
                                  write(sockfd, send_msg, strlen(send_msg));
                                  read(sockfd, recv_msg, BUF_SIZE);
                                  New = 0;
                                  Last = 0;
                                  Win = 0;
                            }

                        }
                    }
                }
       }
       if(strcmp("d", action) == 0){
                 if(Connect == 0){printf("Please connect to server first\n");}
                 else{
                     if(New == 0){printf("Please new a game round first\n");}
                     else{
                         //New = 0;
                         send_msg = "{\"action\":\"moveRight\"}";
                         write(sockfd, send_msg, strlen(send_msg));
                         read(sockfd, recv_msg, BUF_SIZE);
                         //printf("prev:%s\n", prev_msg);
                         //printf("%s\n", recv_msg);
                         if(strncmp(recv_msg, "{\"status\":0",11) == 0) printf("not change\n");
                         else{
                             display(recv_msg, Win);
                             Last++;
                             //prev_msg = recv_msg;
                             if(Win == 1){
                                 printf("\nCongrats! You win the game\n");
                                 printf("The game has closed\n");
                                 send_msg = "{\"action\":\"End\"}";
                                 write(sockfd, send_msg, strlen(send_msg));
                                 read(sockfd, recv_msg, BUF_SIZE);
                                 New = 0;
                                 Last = 0;
                                 Win = 0;
                             }

                         }
                     }
                 }
       }
       if(strcmp("u", action) == 0){
           if(Connect == 0){printf("Plaease connect to server first\n");}
           else{
               if(New == 0){printf("Please new a game round first\n");}
               else{
                   if(Last == 0){printf("not change\n");}
                   else{
                       send_msg = "{\"action\":\"unDo\"}";
                       write(sockfd, send_msg, strlen(send_msg));
                       read(sockfd, recv_msg, BUF_SIZE);
                       //printf("%s\n", recv_msg);
                       display(recv_msg, Win);
                       Last--;
                       if(Win == 1){
                           printf("\nCongrats! You win the game\n");
                           printf("The game has closed\n");
                           send_msg = "{\"action\":\"End\"}";
                           write(sockfd, send_msg, strlen(send_msg));
                           read(sockfd, recv_msg, BUF_SIZE);
                           New = 0;
                           Last = 0;
                           Win = 0;
                       }
                   }
               }
           }
       }
       if(strcmp("whosyourdaddy", action) == 0){
            if(Connect == 0){printf("Plaease connect to server first\n");}
            else{
                if(New == 0){printf("Please new a game round first\n");}
                else{
                    //if(Last == 0){printf("not change\n");}
                    //else{
                        send_msg = "{\"action\":\"whosyourdaddy\"}";
                        write(sockfd, send_msg, strlen(send_msg));
                        read(sockfd, recv_msg, BUF_SIZE);
                        //printf("%s\n", recv_msg);
                        display(recv_msg, Win);
                        Last--;
                        if(Win == 1){
                            printf("\nCongrats! You win the game\n");
                            printf("The game has closed\n");
                            send_msg = "{\"action\":\"End\"}";
                            write(sockfd, send_msg, strlen(send_msg));
                            read(sockfd, recv_msg, BUF_SIZE);
                            New = 0;
                            Last = 0;
                            Win = 0;
                        }
                    //}
                }
            }
        }

       if(New == 0) printf(">");
       else printf("move>");
       scanf("%s", &action);
   }
}
