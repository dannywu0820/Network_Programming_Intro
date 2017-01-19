#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<time.h>
#include<sys/errno.h>
#include<sys/resource.h>
#include<stdarg.h>

#define BUF_SIZE 128
#define QUEUE_LEN 1000
#define NUM_OF_ACCOUNTS 3500

int Round = 0;

struct Account{
   int name;
   int id;
   int port;
   long long int deposit;   
};

struct Account A[NUM_OF_ACCOUNTS];
int num_of_accounts = 0;

int errno;
int error_exit(const char *format, ...){
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(-1);
}

int passive_sock(int service, const char *transport, int q_len){
    //service is port, transport is protocol
    struct sockaddr_in addr;
    int sock_fd, sock_type;

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(service);

    if(strcmp(transport, "udp") == 0) sock_type = SOCK_DGRAM;
    else sock_type = SOCK_STREAM;

    sock_fd = socket(AF_INET, sock_type, 0);
    if(sock_fd < 0) error_exit("failed socket(): %s\n", strerror(sock_fd));

    errno = bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
    if(errno < 0) error_exit("failed bind(): %s\n", strerror(errno));

    if(sock_type == SOCK_STREAM && listen(sock_fd, q_len) < 0){
        error_exit("failed listen(): %s\n", service, strerror(sock_type));
    }

    return sock_fd;
}

long long int Action(char *msg, int client_port){
    char s1[BUF_SIZE], s2[BUF_SIZE], s3[BUF_SIZE];
    int invalid = 0;
   
    //printf("%s\n", msg);
    sscanf(msg, "{%[^:]:%[^}]", s1, s2);
    //printf("s1: %s, s2: %s\n", s1, s2);
    if(strcmp(s1,"\"account_name\"") == 0){ //initialize a new account
        int new_name,new_id;
        
        sscanf(s2, "\"s%d\",%*[^,],%*[^:]:\"%d", &new_name, &new_id);
        
        for(int i = 0; i < num_of_accounts; i++){
            if(A[i].id == new_id){ //replicated account id
               invalid = 1;
               break;
            }
        }
        if(invalid){
            return 2;
        }
        else{
            A[num_of_accounts].name = new_name;
            A[num_of_accounts].id = new_id;
            A[num_of_accounts].port = client_port;
            A[num_of_accounts].deposit = 0;
            num_of_accounts++;
            printf("Initialize S%d ID:%d Deposit: %lld\n", new_name, new_id, A[num_of_accounts].deposit);
            return 1;
        }
    }
    else{ //"action"
        int destination;
        long long int money = 0;

        sscanf(s2,"\"%[^\"]\",%s", s1, s3);
        if(strcmp(s1, "save") == 0){ //save
            sscanf(s3, "\"money\":%lld", &money);
            //printf("%lld\n", money);
            invalid = 1;
            for(int i = 0; i < num_of_accounts; i++){
                if(A[i].port == client_port){
                	invalid = 0;
                    A[i].deposit+=money;
                    printf("%lld saves to S%d Deposit: %lld\n", money, A[i].name, A[i].deposit);
                    break;
                }
            }
            if(invalid){
            	return 3;
			}
            else{
                return 1;	
			}
        }
        else if(strcmp(s1, "withdraw") == 0){ //withdraw
            sscanf(s3, "\"money\":%lld", &money);
            //printf("%lld\n", money);
            invalid = 1;
            for(int i = 0; i < num_of_accounts; i++){
                if(A[i].port == client_port){
                    if(A[i].deposit < money) invalid = 1;
                    else{
                    	invalid = 0;
                    	A[i].deposit-=money;
					}
                    printf("%lld withdraws from S%d Deposit: %lld\n", money, A[i].name, A[i].deposit);
                    break;
                }
            }
            if(invalid){ //invalid transaction 
                return 3;
            }
            else{
                return 1;
            }
        }
        else if(strcmp(s1, "remit") == 0){ //remit
            int dest = 0;           
 
            //printf("s1: %s, s3: %s\n", s1, s3);
            sscanf(s3, "%*[^:]:%lld%*[^:]:\"s%d", &money, &destination);
            //printf("%lld %d\n", money, destination);
            for(int i = 0; i < num_of_accounts; i++){
                if(A[i].port == client_port){
                    if(A[i].deposit < money || A[i].name == destination){
                        invalid = 1;
                    }
                    else{
                        invalid = 1;
                        for(int j = 0; j < num_of_accounts; j++){
                            if(A[j].name == destination){
                                A[i].deposit-=money;
                                A[j].deposit+=money;
                                printf("S%d remits %lld to S%d\n", A[i].name, money, A[j].name);
                                printf("S%d Deposit: %lld\n", A[i].name, A[i].deposit);
                                printf("S%d Deposit: %lld\n", A[j].name, A[j].deposit);

                                invalid = 0;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            if(invalid){
                return 3;
            }
            else{
                return 1;
            }
        }
        else if(strcmp(s1, "show") == 0){ //show
            invalid = 1;
            for(int i = 0; i < num_of_accounts; i++){
                if(A[i].port == client_port){
                    money = A[i].deposit;
                    invalid = 0;
                    printf("S%d Deposit: %lld\n", A[i].name, A[i].deposit);
                    break;
                }
            }
            if(invalid){ //account not found
                return 4;
            }
            else{
                return money;
            }
        }
        else if(strcmp(s1, "bomb") == 0){ //bomb
            for(int i = 0; i < num_of_accounts; i++){
                A[i].deposit = 0;
            }
            return 1;
        }
        else if(strcmp(s1, "end") == 0){ //end
            for(int i = 0; i < num_of_accounts; i++){
                A[i].name = 0;
                A[i].id = 0;
                A[i].port = 0;
                A[i].deposit = 0;
            }
            num_of_accounts = 0;
            Round = 0;
            printf("ended\n");
            return -1;
        }
        else{ return 1; }
    }
}

void Response(long long int response, char *msg){
     
     switch(response){
         case -1: sprintf(msg, "%s", "{\"message\":\"end\"}");
                 break;
         case 1: sprintf(msg, "%s", "{\"message\":\"ok\"}");
                 break;
         case 2: sprintf(msg, "%s", "{\"message\":\"account_id has been registered\"}");       
                 break;
         case 3: sprintf(msg, "%s", "{\"message\":\"invalid transaction\"}");
                 break;
         case 4: sprintf(msg, "%s", "{\"message\":\"account not find\"}");
                 break;
         default:sprintf(msg, "{\"message\":%lld}", response); 
                 break;
     }
}


int main(int argc, char *argv[]){
    int server_port, client_port;
    struct sockaddr_in client_addr;
    unsigned int client_addrlen;
    int m_sockfd, action;
    long long int response = 0;
    ssize_t bytes_get;
    char recv_msg[BUF_SIZE], send_msg[BUF_SIZE];

    if(argc != 2) error_exit("usage: $s port\n", argv[0]);

    server_port = atoi(argv[1]);
    m_sockfd = passive_sock(server_port, "udp", QUEUE_LEN);

    while(1){
        Round++; printf("%d.", Round);
        //1.receive messages from clients
        client_addrlen = sizeof(client_addr);
        bytes_get = recvfrom(m_sockfd, recv_msg, BUF_SIZE, 0, (struct sockaddr *)&client_addr, &client_addrlen);
        client_port = ntohs(client_addr.sin_port);
        recv_msg[bytes_get] = '\0';
        //2.process received messages
        response = Action(recv_msg, client_port);
        Response(response, send_msg);
        //3.send response messages back to clients
        sendto(m_sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&client_addr, client_addrlen); 
    }
}
