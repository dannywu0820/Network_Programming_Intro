#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>

int main(){
    int sockfd, sockfd2, S1_port = 5566;
    struct sockaddr_in servaddr, serv2addr;
    char *server_ip = (char*)"140.113.65.28";
  
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(S1_port);
    inet_pton(AF_INET, server_ip, &servaddr.sin_addr);
    //192.168.0.101 is internal network ip owned by router
    //140.113.65.28 is external network ip that others see this computer
    if((sockfd=socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("socket() error:"); //print error message
        exit(EXIT_FAILURE); //terminate process,EXIT_FAILURE or EXIT_SUCCESS
    }

    int n, l = 3000, r = 60000, m = (l+r)/2, count=1;
    socklen_t len = sizeof(servaddr), len2 = sizeof(serv2addr);
    char sendline[128], recvline[128], *result;
    //FILE *fp=fopen("test.txt", "r");
    //fgets(sendline, 128, fp);
    //fclose(fp);
    while(1){
        //printf("l:%d r:%d m:%d\n",l,r,m);
        //printf("#%d\n",count); count++;
        sprintf(sendline,"{\"guess\":%d}",m);
        printf("send%s\n", sendline);
        sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr *)&servaddr,  len);
        n=recvfrom(sockfd, recvline, 128, 0, (struct sockaddr *)&servaddr, &len);
        sscanf(recvline, "{\"result\":\"%[a-z]", result);
        printf("receive%s\n",recvline);
        //printf("%s\n", result);
        
        //guess numbers by binary search
        if(strcmp("larger",result)==0){l=m+1;m=(l+r)/2;}
        if(strcmp("smaller",result)==0){r=m-1;m=(l+r)/2;}
        if(strcmp("bingo",result)==0){S1_port=m; break;}
    }
    //printf("%d",S1_port);
    bzero(&serv2addr, sizeof(serv2addr));
    serv2addr.sin_family = AF_INET;
    serv2addr.sin_port = htons(S1_port);
    server_ip = (char*)"140.113.65.28";
    inet_pton(AF_INET, server_ip, &serv2addr.sin_addr);
    sockfd2 = socket(AF_INET, SOCK_DGRAM, 0);
    //printf("#%d\n",count);
    sprintf(sendline, "{\"student_id\":\"%s\"}", "0116233");
    printf("send%s\n",sendline);
    sendto(sockfd2, sendline, strlen(sendline), 0, (struct sockaddr *)&serv2addr, len2);
    n=recvfrom(sockfd2, recvline, 128, 0, (struct sockaddr *)&serv2addr, &len2);
    printf("receive%s\n",recvline);
    exit(0);
}
