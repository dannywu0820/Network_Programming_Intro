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
#include<sys/signal.h>
#include<sys/wait.h>
#include<sys/errno.h>
#include<sys/resource.h>
#include<stdarg.h>

#define BUF_SIZE 128
#define QUEUE_LEN 1000

struct Account{
    int name;
    int id;
    int port;
    long long int deposit;
};

struct Account A[1005];
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
    struct sockaddr_in addr;
    int sock_fd, sock_type;
    
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(service);

    //transport protocol, either UDP or TCP
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

int main(int argc, char *argv[]){
    int service, client_port, money; //port for init
    struct sockaddr_in client_addr;
    unsigned int client_addrlen;
    int m_sockfd;
    char recv_msg[BUF_SIZE], send_msg[BUF_SIZE];
    ssize_t bytes_get;
    int account;
    int account_id;

    if(argc != 2) error_exit("usage: %s port\n", argv[0]);

    service = atoi(argv[1]);
    m_sockfd = passive_sock(service, "udp", QUEUE_LEN);

    while(1){
        client_addrlen = sizeof(client_addr);
        bytes_get = recvfrom(m_sockfd, recv_msg, BUF_SIZE, 0, (struct sockaddr *)&client_addr, &client_addrlen);
        recv_msg[bytes_get] = '\0';
        client_port = ntohs(client_addr.sin_port);
        if(strncmp(recv_msg, "{\"account_name\"}", 14) == 0){
            sscanf(recv_msg, "{\"account_name\":\"s%d\",\"action\":\"init\",\"account_id\":\"%d\"}", &account, &account_id);
            int invalid = 0;
            for(int i = 1; i <= num_of_accounts; i++){
                if(account_id == A[i].id){
                    invalid = 1;
                    break;
                }
            }
            if(invalid) sprintf(send_msg, "%s", "{\"message\":\"account_id has been registered\"}");
            else{
                sprintf(send_msg, "%s", "{\"message\":\"ok\"}");
                num_of_accounts++;
                A[num_of_accounts].name = account;
                A[num_of_accounts].id = account_id;
                printf("s%d id:%d port:%d\n", account, account_id, (int)ntohs(client_addr.sin_port));
                A[num_of_accounts].port = ntohs(client_addr.sin_port);
                A[num_of_accounts].deposit = 0;
            }
            sendto(m_sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&client_addr, client_addrlen);       
        }
        else if(strncmp(recv_msg, "{\"action\":\"save\"", 16) == 0){
            sscanf(recv_msg, "{\"action\":\"save\",\"money\":%d}", &money);
            printf("%d saves to %d", money, client_port);
            for(int i = 1; i <= num_of_accounts; i++){
                if(A[i].port == client_port){
                    A[i].deposit+=money;
                    printf(" A[%d] s%d has %lld\n", i, A[i].name, A[i].deposit);
                    break;
                } 
            }
            sprintf(send_msg, "%s", "{\"message\":\"ok\"}");
            sendto(m_sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&client_addr, client_addrlen);       
        }
        else if(strncmp(recv_msg, "{\"action\":\"withdraw\"", 20) == 0){
            sscanf(recv_msg, "{\"action\":\"withdraw\",\"money\":%d}", &money);
            printf("%d withdraws from %d", money, client_port);
            int invalid = 0;
            for(int i = 1; i <= num_of_accounts; i++){
                if(A[i].port == client_port){
                    if(money > A[i]. deposit) invalid = 1;
                    else{
                        A[i].deposit-=money;
                        printf(" A[%d] has %lld\n", i, A[i].deposit);
                    }
                    break;
                } 
            }
            if(invalid) sprintf(send_msg, "%s", "{\"message\":\"invalid transaction\"}");
            else sprintf(send_msg, "%s", "{\"message\":\"ok\"}");
            sendto(m_sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&client_addr, client_addrlen);       
        }
        else if(strncmp(recv_msg, "{\"action\":\"remit\"", 17) == 0){
            int destination;
            sscanf(recv_msg, "{\"action\":\"remit\",\"money\":%d,\"destination_name\":\"s%d\"}", &money, &destination);
            printf("remit %d to s%d", money, destination);
            int invalid = 0;
            for(int i = 1; i <= num_of_accounts; i++){
                if(A[i].port == client_port){
                    if(money > A[i].deposit || A[i].name == destination) invalid = 1;
                    else{
                        int j;
                        for(j = 1; j <= num_of_accounts; j++){
                            if(A[j].name == destination){
                            A[i].deposit-=money;
                            A[j].deposit+=money;
                            printf(" A[%d] has %lld and A[%d] has %lld\n", i, A[i].deposit, j, A[j].deposit);
                            break;
                            }
                        }
                        if(j > num_of_accounts) invalid = 1;
                    }
                    break;
                }
            }
            if(invalid) sprintf(send_msg, "%s", "{\"message\":\"invalid transaction\"}");
            else sprintf(send_msg, "%s", "{\"message\":\"ok\"}");
            sendto(m_sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&client_addr, client_addrlen);       
            
        }
        else if(strncmp(recv_msg, "{\"action\":\"show\"", 16) == 0){
            int find = 0;
            for(int i = 0; i <= num_of_accounts; i++){
                if(A[i].port == client_port){
                    find = i;
                    break;
                }
            }
            if(find) sprintf(send_msg, "%s%lld%s", "{\"message\": ",A[find].deposit,"}");
            else sprintf(send_msg, "%s", "{\"message\":\"account not find\"}");
            sendto(m_sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&client_addr, client_addrlen);       
             
        }
        else if(strncmp(recv_msg, "{\"action\":\"end\"", 15) == 0){
            for(int i = 1; i <= num_of_accounts; i++){
                A[i].id = 0;
                A[i].port = 0;
                A[i].deposit = 0;       
            }
            num_of_accounts = 0;
            sprintf(send_msg, "%s", "{\"message\":\"end\"}");
            sendto(m_sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&client_addr, client_addrlen);       
        }
        else if(strncmp(recv_msg, "{\"action\":\"bomb\"", 16) == 0){
            for(int i = 1; i <= num_of_accounts; i++) A[i].deposit = 0;
            sprintf(send_msg, "%s", "{\"message\":\"ok\"}");
            sendto(m_sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&client_addr, client_addrlen);       
        }
        else{
            printf("other actions\n");
 
        }
    }
}
