#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h> //?
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

#include<iostream>
#include<vector>
#include<ctime>
using namespace std;

#define BUF_SIZE 1024
#define QUEUE_LEN 32 //?
#define NUM_OF_ACCOUNTS 1000

int error_exit(const char *format, ...){
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

int passive_sock(const char *service, const char *transport, int queue_len){
    //service->port number, transport layer protocol-> "tcp"/"udp"
    struct sockaddr_in addr; //an Internet endpoint address
    int sock_fd, sock_type; //file descriptor and type of a socket
    struct servent *pse; //pointer to a service informaion entry
    struct protoent *ppe; //pointer to a protocol information entry
    unsigned short portbase = 0; //?

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(service));

    /*if(pse = getservbyname(service, transport))
        addr.sin_port = htons(ntohs((unsigned short)pse->s_port) + portbase);
    else if((addr.sin_port=htons((unsigned short)atoi(service))) == 0)
        error_exit("can't get \"%s\" service entry\n", service);

    if((ppe = getprotobyname(transport)) == 0)
        error_exit("can't get \"%s\" protocol entry\n", transport);*/

    if(strcmp(transport, "udp") == 0) sock_type = SOCK_DGRAM;
    else sock_type = SOCK_STREAM;

    sock_fd = socket(AF_INET, sock_type, 0); //create socket
    if(sock_fd < 0) error_exit("failed socket(): %s\n", strerror(sock_fd));

    errno = bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
    if(errno < 0) error_exit("failed bind(): %s\n", service, strerror(errno))    ;

    if(sock_type == SOCK_STREAM && listen(sock_fd, queue_len) < 0){
        error_exit("failed listen(): %s\n", service, strerror(sock_type));
    }

    return sock_fd;
}

struct Mail{
    bool has_read;
    char sou[25];
    char des[25];
    char title[50];
    char content[100];
    tm *t;
    char type[10];
    struct Mail *next;
};

struct Account{
    int my_fd;
    char my_name[10];
    char my_address[25];
    //int num_of_mail;
    vector<struct Mail> box;
    //bool registered;
};


vector<struct Account> A;

void Init(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int* invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE];
    for(int i = 0; i < BUF_SIZE; i++){
        opt[i] = 0;
        arg[i] = 0;
    }
    
    sscanf(opt_arg, "%*[ ]%s%*[ ]%[^\0]", opt, arg);
    if(strcmp(opt, "-u") == 0){ //valid option
        int start = 0;
        for(int i = 0; i < strlen(arg); i++){ //arguments checking
            if(!( arg[i] == '-' 
               || arg[i] == '_'
               ||(arg[i]>47 && arg[i]<58)
               ||(arg[i]>64 && arg[i]<91)
               ||(arg[i]>96 && arg[i]<123))){
                if(arg[i] == ' '){
                    for(int j = i+1; j < strlen(arg); j++){
                        if(arg[j] != ' '){
                            *invalid = 3;
                            break;
                        }
                    }
                    arg[i] = '\0';
                    break;
                }
                else if(arg[i]=='\"'){
                    if(i != 0) arg[i] = '\0';
                    start = 1; 
                }
                else{
                    *invalid = 3;
                    break;
                }
               }
        }
        if(*invalid == 0){
            vector<struct Account>::iterator it;
            for(it = A.begin(); it != A.end(); it++){
                if(strcmp(it->my_name, arg) == 0){
                    break;
                }
            }
            if(it == A.end()){ //no replicated account
                struct Account temp;
                temp.my_fd = fd;
                //temp.num_of_mail = 0;
                sprintf(temp.my_name, "%s", arg+start);
                sprintf(temp.my_address, "%s@nctu.edu.tw", arg+start);
                sprintf(send_msg, "%s@nctu.edu.tw\n", arg+start);
                A.push_back(temp);
            }
            else{
                sprintf(send_msg, "This account has been registered\n");
            } 
        }
    }
    else{
        *invalid = 2;
    }
}

void Exit(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    vector<struct Account>::iterator it;
    for(it = A.begin(); it != A.end(); it++){
        if(it->my_fd == fd){
            A.erase(it);
            break;
        }
    }
    sprintf(send_msg, "%s", "exit\n");
}

void List(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE];
    for(int i = 0; i < BUF_SIZE; i++){
        opt[i] = 0;
        arg[i] = 0;
    }
    sscanf(opt_arg, "%*[ ]%s%*[ ]%[^\0]", opt, arg);
    //printf();
    if(strlen(arg) != 0){
        for(int i = 0; i < strlen(arg); i++){
            if(arg[i] != ' '){
                *invalid = 3;
                break;
            }
        }
    }

    if(*invalid == 0){
        if(strcmp(opt, "-u") == 0){
            if(A.size() == 0){
                sprintf(send_msg, "%s", "no accounts\n");
            }
            else{
                vector<struct Account>::iterator it;
                for(it = A.begin(); it != A.end(); it++){
                    int start = strlen(send_msg);
                    sprintf(send_msg+start, "%s@nctu.edu.tw\n", it->my_name);
                }
            }
        }
        else if(strcmp(opt, "-l") == 0){
            vector<struct Account>::iterator it;
            for(it = A.begin(); it != A.end(); it++){
                if(it->my_fd == fd){
                    break;
                }
            }
            if(it != A.end()){
                if(it->box.size() != 0){
                    vector<struct Mail>::iterator mit; int i = 1;
                    for(mit = it->box.begin(); mit!= it->box.end(); mit++){
                       int start = strlen(send_msg);
                       if(mit->has_read){
                           sprintf(send_msg+start, "%d.%s%s\n", i, mit->type, mit->title);
                       }
                       else{
                           sprintf(send_msg+start, "%d.%s%s(new)\n", i, mit->type, mit->title);
                       }
                       i++; 
                    }
                }
                else{
                    sprintf(send_msg, "%s", "no mail\n");
                }
            }
            else{ //hasn't registered
                sprintf(send_msg, "%s", "init first\n");
            }
             
        }
        else if(strcmp(opt, "-a") == 0){
            vector<struct Account>::iterator it;
            for(it = A.begin(); it != A.end(); it++){
                if(it->my_fd == fd){
                    break;
                }
            }
            if(it != A.end()){
                sprintf(send_msg, "Account:%s\nMail address:%s\nNumber of mails:%d\n", it->my_name, it->my_address, it->box.size()); 
            }
            else{ //hasn't registered
                sprintf(send_msg, "%s", "init first\n");
            }
        }
        else{
            *invalid = 2;
        }
    }
}

void Write(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE], S[BUF_SIZE];
    struct Mail temp;
    temp.next = NULL;
    sprintf(temp.type, "%s", "");

    vector<struct Account>::iterator it;
    for(it = A.begin(); it != A.end(); it++){
        if(it->my_fd == fd) break;
    }
    if(it == A.end()){
        sprintf(send_msg, "%s", "init first\n");
    }
 
    while(strlen(opt_arg) != 0){
        printf("loop\n");
        int start = 0, double_quote = 0;
        for(int i = 0; i < BUF_SIZE; i++){
            opt[i] = 0;
            arg[i] = 0;
            S[i] = 0;
        }
        for(int i = 0; i < strlen(opt_arg); i++){
            if(opt_arg[i] == '\"'){
                double_quote = 1;
                break;
            }
        }
        if(double_quote == 1){ 
            sscanf(opt_arg, "%*[ ]%s%*[ ]%*[\"]%[^\"]%*[\"]%[^\0]", opt, arg, S);
        }
        else sscanf(opt_arg, "%*[ ]%s%*[ ]%s%[^\0]", opt, arg, S);
        printf("||%s||%s||%s||\n",opt,arg,S);
        if(strcmp(opt, "-d") == 0){
            for(int i = 0; i < strlen(arg); i++){ //arguments checking
                if(!( arg[i] == '-' 
                   || arg[i] == '_'
                   || arg[i] == ':'
                   || arg[i] == '.'
                   || arg[i] == '@'
                   ||(arg[i]>47 && arg[i]<58)
                   ||(arg[i]>64 && arg[i]<91)
                   ||(arg[i]>96 && arg[i]<123))){
                    if(arg[i] == ' ' && double_quote != 1){
                        if(double_quote != 1){
                            for(int j = i+1; j < strlen(arg); j++){
                                if(arg[j] != ' '){
                                    *invalid = 3;
                                    break;
                                }
                            }
                            arg[i] = '\0';
                            break;
                        }
                    }
                    else if(arg[i] == '\"'){
                        if(i != 0) arg[i] = '\0';
                        //start = 1;
                    }
                    else{
                        *invalid = 3;
                        break;
                    }
                }          
            }
            //if the destination doesn't exist->args error
            vector<struct Account>::iterator it2;
            *invalid = 3;
            for(it2 = A.begin(); it2 != A.end(); it2++){
                if(strcmp(it2->my_address, arg) == 0){
                    *invalid = 0;
                    break;
                }
            }
            
            if(*invalid == 0) sprintf(temp.des, "%s", arg+start);
            else break;
        }
        else if(strcmp(opt, "-t") == 0){
            for(int i = 0; i < strlen(arg); i++){ //arguments checking
                if(!( arg[i] == '-' 
                   || arg[i] == '_'
                   || arg[i] == ':'
                   || arg[i] == '.'
                   || arg[i] == '@'
                   ||(arg[i]>47 && arg[i]<58)
                   ||(arg[i]>64 && arg[i]<91)
                   ||(arg[i]>96 && arg[i]<123))){
                    if(arg[i] == ' '){
                        if(double_quote != 1){
                            for(int j = i+1; j < strlen(arg); j++){
                                if(arg[j] != ' '){
                                    *invalid = 3;
                                    break;
                                }
                            }
                            arg[i] = '\0';
                            break;
                        }
                    }
                    else if(arg[i] == '\"'){
                        if(i != 0) arg[i] = '\0';
                        //start = 1;
                    }
                    else{
                        *invalid = 3;
                        break;
                    }
                }          
            }
            
            if(*invalid == 0) sprintf(temp.title, "%s", arg+start);
            else break;
        }
        else if(strcmp(opt, "-c") == 0){
            for(int i = 0; i < strlen(arg); i++){ //arguments checking
                if(!( arg[i] == '-' 
                   || arg[i] == '_'
                   || arg[i] == ':'
                   || arg[i] == '.'
                   || arg[i] == '@'
                   ||(arg[i]>47 && arg[i]<58)
                   ||(arg[i]>64 && arg[i]<91)
                   ||(arg[i]>96 && arg[i]<123))){
                    if(arg[i] == ' '){
                        if(double_quote != 1){
                            for(int j = i+1; j < strlen(arg); j++){
                                if(arg[j] != ' '){
                                    *invalid = 3;
                                    break;
                                }
                            }
                            arg[i] = '\0';
                            break;
                        }
                    }
                    else if(arg[i] == '\"'){
                        if(i != 0) arg[i] = '\0';
                        //start = 1;
                    }
                    else{
                        *invalid = 3;
                        break;
                    }
                }          
            }
            if(*invalid == 0) sprintf(temp.content, "%s", arg+start);
            else break;
        }
        else{ *invalid = 2; }
        for(int i = 0; i < BUF_SIZE; i++) opt_arg[i] = 0;
        sprintf(opt_arg, "%s", S);
    }
    if(*invalid == 0 && (it != A.end())){
        temp.has_read = false;
        sprintf(temp.sou, "%s", it->my_address);
        time_t now = time(0);
        temp.t = localtime(&now);
        for(it = A.begin(); it != A.end(); it++){
            if(strcmp(temp.des, it->my_address) == 0){
                it->box.push_back(temp);
                break;
            }   
        }
        //printf("%s,%s,%s,%s",temp.sou,temp.des,temp.title,temp.content);
        sprintf(send_msg, "%s", "done\n");
    }
}

void Read(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE];
    for(int i = 0; i < BUF_SIZE; i++){
        opt[i] = 0;
        arg[i] = 0;
    }
    sscanf(opt_arg, "%*[ ]%s%*[ ]%[^\0]", opt, arg);
   
    if(strcmp(opt, "-r") == 0){
        for(int i = 0; i < strlen(arg); i++){ //arguments checking
            if(!((arg[i]>47 && arg[i]<58)
               ||(arg[i]>64 && arg[i]<91)
               ||(arg[i]>96 && arg[i]<123))){
                if(arg[i] == ' '){
                    for(int j = i+1; j < strlen(arg); j++){
                        if(arg[j] != ' '){
                            *invalid = 3;
                            break;
                        }
                    }
                    arg[i] = '\0';
                    break;
                }
                else{
                    *invalid = 3;
                    break;
                }
               }
        }
        if(*invalid == 0){
            vector<struct Account>::iterator it;
            for(it = A.begin(); it != A.end(); it++){
                if(it->my_fd == fd){
                    break;
                }
            }
            if(it != A.end()){
                int Nth;
                sscanf(arg, "%d", &Nth);
                if(Nth <= it->box.size()){
                    vector<struct Mail>::iterator mit = it->box.begin();
                    for(int i = 1; i < Nth; i++) mit++;
                    //char type[4]="";
                    sprintf(send_msg, "From: %s\nTo: %s\nDate: %d-%d-%d %d:%d:%d\nTitle: %s%s\nContent: %s\n",mit->sou, mit->des, (mit->t->tm_year)+1900, (mit->t->tm_mon)+1, mit->t->tm_mday, mit->t->tm_hour, mit->t->tm_min, mit->t->tm_sec ,mit->type ,mit->title, mit->content);
                    mit->has_read = true;
                    struct Mail *m = mit->next;
                    while(m != NULL){
                        int begin = strlen(send_msg);
                        sprintf(send_msg+begin, "----\nFrom: %s\nTo: %s\nDate: %d-%d-%d %d:%d:%d\nTitle: %s%s\nContent: %s\n",m->sou, m->des, (m->t->tm_year)+1900, (m->t->tm_mon)+1, m->t->tm_mday, m->t->tm_hour, m->t->tm_min, m->t->tm_sec ,m->type ,m->title, m->content);
                        m = m->next;
                        //printf("fwd or re mail\n");
                    }
                }
                else{ *invalid = 3; }
            }
            else{ //hasn't registered
                sprintf(send_msg, "%s", "init first\n");
            }
        }
    }
    else{
        *invalid = 2;
    } 
}

void Remove(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE];
    for(int i = 0; i < BUF_SIZE; i++){
        opt[i] = 0;
        arg[i] = 0;
    }
    sscanf(opt_arg, "%*[ ]%s%*[ ]%[^\0]", opt, arg);
    
    vector<struct Account>::iterator it;
    for(it = A.begin(); it != A.end(); it++){
        if(it->my_fd == fd) break;
    }
    if(it == A.end()){
        sprintf(send_msg, "%s", "init first\n");
    }

    if(strcmp(opt, "-d") == 0){
        for(int i = 0; i < strlen(arg); i++){ //arguments checking
            if(!((arg[i]>47 && arg[i]<58)
               ||(arg[i]>64 && arg[i]<91)
               ||(arg[i]>96 && arg[i]<123))){
                if(arg[i] == ' '){
                    for(int j = i+1; j < strlen(arg); j++){
                        if(arg[j] != ' '){
                            *invalid = 3;
                            break;
                        }
                    }
                    arg[i] = '\0';
                    break;
                }
                /*else if(arg[i] == '\"'){
                    *invalid = 3;
                    break;
                }*/
                else{
                    *invalid = 3;
                    break;
                }
               }
        }
        if(*invalid == 0 && it != A.end()){
             int Nth;
             sscanf(arg, "%d", &Nth);
             if(Nth <= it->box.size()){
                vector<struct Mail>::iterator mit = it->box.begin();
                for(int i = 1; i < Nth; i++) mit++;
                it->box.erase(mit);
                sprintf(send_msg, "%s", "done\n");
             }
             else{ *invalid = 3; }

        }
    }
    else if(strcmp(opt, "-D") == 0){
        for(int i = 0; i < strlen(arg); i++){ //arguments checking
            if(arg[i] != ' '){
                *invalid = 3;
            }
        }
        if(*invalid == 0 && it != A.end()){
            it->box.clear();
            sprintf(send_msg, "%s", "done\n");
        }
    }
    else{
        *invalid = 2;
    }
}

void Reply(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE], S[BUF_SIZE];
    struct Mail temp;
    temp.next = NULL;
    temp.has_read = false;
    int Nth = 0;
    //check if the user is valid
    vector<struct Account>::iterator it;
    for(it = A.begin(); it != A.end(); it++){
        if(it->my_fd == fd) break;
    }
    if(it == A.end()){
        sprintf(send_msg, "%s", "init first\n");
    }

    while(strlen(opt_arg) != 0){
        printf("loop\n");
        int start = 0, double_quote = 0;
        for(int i = 0; i < BUF_SIZE; i++){
            opt[i] = 0;
            arg[i] = 0;
            S[i] = 0;
        }
        //check if args have '\"'
        for(int i = 0; i < strlen(opt_arg); i++){
            if(opt_arg[i] == '\"'){
                double_quote = 1;
                break;
            }
        }
        if(double_quote == 1) sscanf(opt_arg, "%*[ ]%s%*[ ]%*[\"]%[^\"]%*[\"]%[^\0]", opt, arg, S);
        else sscanf(opt_arg, "%*[ ]%s%*[ ]%s%[^\0]", opt, arg, S);
        printf("||%s||%s||%s||\n",opt,arg,S);
        
        if(strcmp(opt, "-c") == 0){
            for(int i = 0; i < strlen(arg); i++){ //arguments checking
                if(!( arg[i] == '-' 
                   || arg[i] == '_'
                   || arg[i] == ':'
                   || arg[i] == '.'
                   || arg[i] == '@'
                   ||(arg[i]>47 && arg[i]<58)
                   ||(arg[i]>64 && arg[i]<91)
                   ||(arg[i]>96 && arg[i]<123))){
                    if(arg[i] == ' '){
                        if(double_quote != 1){
                            for(int j = i+1; j < strlen(arg); j++){
                                if(arg[j] != ' '){
                                    *invalid = 3;
                                    break;
                                }
                            }
                            arg[i] = '\0';
                            break;
                        }
                    }
                    else if(arg[i] == '\"'){
                        if(i != 0) arg[i] = '\0';
                        //start = 1;
                    }
                    else{
                        *invalid = 3;
                        break;
                    }
                }          
            }
            if(*invalid == 0) sprintf(temp.content, "%s", arg+start);
            else break;
        }
        else if(strcmp(opt, "-n") == 0){
            for(int i = 0; i < strlen(arg); i++){ //arguments checking
                if(!((arg[i]>47 && arg[i]<58)
                   ||(arg[i]>64 && arg[i]<91)
                   ||(arg[i]>96 && arg[i]<123))){
                    if(arg[i] == ' '){
                        for(int j = i+1; j < strlen(arg); j++){
                            if(arg[j] != ' '){
                                *invalid = 3;
                                break;
                            }
                        }
                        arg[i] = '\0';
                        break;
                    }
                    /*else if(arg[i] == '\"'){
                        *invalid = 3;
                        break;
                    }*/
                    else{
                        *invalid = 3;
                        break;
                    }
                }
            }
            if(it != A.end() && *invalid == 0){
                //int Nth;
                sscanf(arg, "%d", &Nth);
                if(Nth <= it->box.size()){
                    vector<struct Mail>::iterator mit = it->box.begin();
                    for(int i = 1; i < Nth; i++) mit++;
                    sprintf(temp.sou, "%s", mit->des);
                    sprintf(temp.des, "%s", mit->sou);
                    sprintf(temp.title, "%s", mit->title);
                    sprintf(temp.type, "%s", "re:");
                    time_t now = time(0);
                    temp.t = localtime(&now);
                    struct Mail *test = (struct Mail *)malloc(sizeof(struct Mail));
                    test->has_read = mit->has_read;//has_read
                    sprintf(test->sou, "%s", mit->sou);//sou
                    sprintf(test->des, "%s", mit->des);//des
                    sprintf(test->title, "%s", mit->title);//title
                    sprintf(test->content, "%s", mit->content);//content
                    sprintf(test->type, "%s", mit->type);
                    test->t = mit->t;//time
                    test->next = mit->next;//next
                    temp.next = test;
                    sprintf(send_msg, "%s", "done\n");
                    /*sprintf(send_msg, "From:%s\nTo:%s\nDate:%d-%d-%d %d:%d:%d\nTitle:%s\nContent:%s\n",mit->sou, mit->des, (mit->t->tm_year)+1900, (mit->t->tm_mon)+1, mit->t->tm_mday, mit->t->tm_hour, mit->t->tm_min, mit->t->tm_sec ,mit->title, mit->content);
                    mit->has_read = true;*/
                }
                else{ *invalid = 3; }
            }
            /*else{ //hasn't registered
                sprintf(send_msg, "%s", "init first\n");
            }*/
        }
        else{ *invalid = 2; }
        for(int i = 0; i < BUF_SIZE; i++) opt_arg[i] = 0;
        sprintf(opt_arg, "%s", S);
    }
    if(*invalid == 0){
        for(it = A.begin(); it != A.end(); it++){
            if(strcmp(it->my_address, temp.des) == 0){
                it->box.push_back(temp);
                break;
            }
        }
    }
}

void Forward(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE], S[BUF_SIZE];
    int Nth = 0;
    struct Mail temp;
    temp.next = NULL;
    temp.has_read = false;
    //check if the user is valid
    vector<struct Account>::iterator it;
    for(it = A.begin(); it != A.end(); it++){
        if(it->my_fd == fd) break;
    }
    if(it == A.end()){
        sprintf(send_msg, "%s", "init first\n");
    }
    //check options and arguments
    int l = 0;
    while(strlen(opt_arg) != 0 && l<3){
        l++;
        printf("loop\n");
        int start = 0, double_quote = 0;
        for(int i = 0; i < BUF_SIZE; i++){
            opt[i] = 0;
            arg[i] = 0;
            S[i] = 0;
        }
        //check if args have '\"'
        for(int i = 0; i < strlen(opt_arg); i++){
            if(opt_arg[i] == '\"'){
                double_quote = 1;
                break;
            }
        }
        if(double_quote == 1) sscanf(opt_arg, "%*[ ]%s%*[ ]%*[\"]%[^\"]%*[\"]%[^\0]", opt, arg, S);
        else sscanf(opt_arg, "%*[ ]%s%*[ ]%s%[^\0]", opt, arg, S);
        printf("||%s||%s||%s||\n",opt,arg,S);
        
        if(strcmp(opt, "-d") == 0){
            for(int i = 0; i < strlen(arg); i++){ //arguments checking
                if(!( arg[i] == '-' 
                   || arg[i] == '_'
                   || arg[i] == ':'
                   || arg[i] == '.'
                   || arg[i] == '@'
                   ||(arg[i]>47 && arg[i]<58)
                   ||(arg[i]>64 && arg[i]<91)
                   ||(arg[i]>96 && arg[i]<123))){
                    if(arg[i] == ' ' && double_quote != 1){
                        if(double_quote != 1){
                            for(int j = i+1; j < strlen(arg); j++){
                                if(arg[j] != ' '){
                                    *invalid = 3;
                                    break;
                                }
                            }
                            arg[i] = '\0';
                            break;
                        }
                    }
                    else if(arg[i] == '\"'){
                        if(i != 0) arg[i] = '\0';
                        //start = 1;
                    }
                    else{
                        *invalid = 3;
                        break;
                    }
                }          
            }
            //if the destination doesn't exist->args error
            vector<struct Account>::iterator it2;
            *invalid = 3;
            for(it2 = A.begin(); it2 != A.end(); it2++){
                if(strcmp(it2->my_address, arg) == 0){
                    *invalid = 0;
                    break;
                }
            }
            
            if(*invalid == 0) sprintf(temp.des, "%s", arg+start);
            else break;
        }
        else if(strcmp(opt, "-c") == 0){
            for(int i = 0; i < strlen(arg); i++){ //arguments checking
                if(!( arg[i] == '-' 
                   || arg[i] == '_'
                   || arg[i] == ':'
                   || arg[i] == '.'
                   || arg[i] == '@'
                   ||(arg[i]>47 && arg[i]<58)
                   ||(arg[i]>64 && arg[i]<91)
                   ||(arg[i]>96 && arg[i]<123))){
                    if(arg[i] == ' '){
                        if(double_quote != 1){
                            for(int j = i+1; j < strlen(arg); j++){
                                if(arg[j] != ' '){
                                    *invalid = 3;
                                    break;
                                }
                            }
                            arg[i] = '\0';
                            break;
                        }
                    }
                    else if(arg[i] == '\"'){
                        if(i != 0) arg[i] = '\0';
                        //start = 1;
                    }
                    else{
                        *invalid = 3;
                        break;
                    }
                }          
            }
            if(*invalid == 0) sprintf(temp.content, "%s", arg+start);
            else break;
        }
        else if(strcmp(opt, "-n") == 0){
            for(int i = 0; i < strlen(arg); i++){ //arguments checking
                if(!((arg[i]>47 && arg[i]<58)
                   ||(arg[i]>64 && arg[i]<91)
                   ||(arg[i]>96 && arg[i]<123))){
                    if(arg[i] == ' '){
                        for(int j = i+1; j < strlen(arg); j++){
                            if(arg[j] != ' '){
                                *invalid = 3;
                                break;
                            }
                        }
                        arg[i] = '\0';
                        break;
                    }
                    else{
                        *invalid = 3;
                        break;
                    }
                }
            }
            if(it != A.end() && *invalid == 0){
                //int Nth;
                sscanf(arg, "%d", &Nth);
                if(Nth <= it->box.size()){
                    vector<struct Mail>::iterator mit = it->box.begin();
                    for(int i = 1; i < Nth; i++) mit++;
                    sprintf(temp.sou, "%s", it->my_address);
                    //sprintf(temp.des, "%s", mit->sou);
                    sprintf(temp.title, "%s", mit->title);
                    sprintf(temp.type, "%s", "fwd:");
                    time_t now = time(0);
                    temp.t = localtime(&now);
                    struct Mail *test = (struct Mail *)malloc(sizeof(struct Mail));
                    test->has_read = mit->has_read;//has_read
                    sprintf(test->sou, "%s", mit->sou);//sou
                    sprintf(test->des, "%s", mit->des);//des
                    sprintf(test->title, "%s", mit->title);//title
                    sprintf(test->content, "%s", mit->content);//content
                    sprintf(test->type, "%s", mit->type);
                    test->t = mit->t;//time
                    test->next = mit->next;//next
                    temp.next = test;
                    sprintf(send_msg, "%s", "done\n");
                }
                else{ *invalid = 3; }
            }
        }
        else{ *invalid = 2; }
        for(int i = 0; i < BUF_SIZE; i++) opt_arg[i] = 0;
        sprintf(opt_arg, "%s", S);
    }
    //forward mail to receiver's mailbox
    if(*invalid == 0){
        for(it = A.begin(); it != A.end(); it++){
            if(strcmp(it->my_address, temp.des) == 0){
                it->box.push_back(temp);
                break;
            }
        }
    }
}

int main(int argc, char *argv[]){
    struct sockaddr_in client_addr;
    socklen_t client_addrlen;
    char *service;
    int master_sock_fd, slave_sock_fd, max_fd;

    if(argc != 2) error_exit("usage: %s port\n", argv[0]);
    service = argv[1];
    master_sock_fd = passive_sock(service, "tcp", QUEUE_LEN);

    fd_set active_fd_set, read_fd_set; //figure out
    FD_ZERO(&active_fd_set);
    FD_SET(master_sock_fd, &active_fd_set);
    max_fd = master_sock_fd;

    while(1){
        int errno; //error number
        
        read_fd_set = active_fd_set;
        errno = select(max_fd + 1, &read_fd_set, NULL, NULL, NULL);
        if(errno < 0){ //-1
            error_exit("failed select(): %s\n", strerror(errno));
        }
        else{ //select() successful
            for(int fd = 0; fd <= max_fd + 1; fd++){
                if(FD_ISSET(fd, &read_fd_set)){ //fd is set to 1
                    if(fd == master_sock_fd){
                        client_addrlen = sizeof(client_addr);
                        slave_sock_fd = accept(fd, (struct sockaddr *)&client_addr, &client_addrlen);
                        if(slave_sock_fd < 0){
                            error_exit("failed accept(): %s\n");
                        }
                        else{
                            char from[INET_ADDRSTRLEN];
                            printf("new connection from ");
                            printf("%s\n", inet_ntop(AF_INET, &(client_addr.sin_addr), from, INET_ADDRSTRLEN));
                            printf("use socket %d to handle\n", slave_sock_fd);
                            FD_SET(slave_sock_fd, &active_fd_set);
                            max_fd = (slave_sock_fd > max_fd?slave_sock_fd:max_fd);
                        }
                    }
                    else{ //slave sockets
                        char recv_msg[BUF_SIZE], send_msg[BUF_SIZE], opt_arg[BUF_SIZE];
                        int bytes_get;
                        int invalid = 0; //0: OK, 1:cmd, 2:opt, 3:args error
                        char cmd[BUF_SIZE];//, opt[BUF_SIZE], arg[BUF_SIZE]; 
                        for(int i = 0; i < BUF_SIZE; i++){ 
                            //clear buffer string to '\0' $important
                            recv_msg[i] = 0;
                            send_msg[i] = 0;
                            opt_arg[i] = 0;
                            cmd[i] = 0;
                            //opt[i] = 0;
                            //arg[i] = 0;
                        }
                        /**/
                        bytes_get = read(fd, recv_msg, BUF_SIZE);
                        if(bytes_get < 0){ 
                            error_exit("failed read(): %s\n", strerror(bytes_get));
                        } 
                        else{
                            //printf("%d,%s\n", strlen(recv_msg), recv_msg);
                        }
                        /**/
                        int k = 0;
                        while(recv_msg[k] == ' '){
                            k++;
                        }
                        sscanf(recv_msg+k, "%[^ ]%[^\0]", cmd, opt_arg);
                        /*if(k != 0){
                            char cmd2[BUF_SIZE];
                            sprintf(cmd2, "%s", cmd+k);
                            for(int i = 0; i < BUF_SIZE; i++) cmd[i] = 0;
                            sprintf(cmd, "%s", cmd2);
                        }*/ 
                        printf("%d->%s,%d->%s\n", strlen(cmd), cmd, strlen(opt_arg), opt_arg);
                        if(strcmp(cmd,"init") == 0){
                            Init(opt_arg, fd, send_msg, &invalid);
                        }
                        else if(strcmp(cmd,"ls") == 0){
                            List(opt_arg, fd, send_msg, &invalid);
                        }
                        else if(strcmp(cmd,"rm") == 0){
                            Remove(opt_arg, fd, send_msg, &invalid);
                        }
                        else if(strcmp(cmd,"rd") == 0){
                            Read(opt_arg, fd, send_msg, &invalid);
                        }
                        else if(strcmp(cmd,"wt") == 0){ 
                            Write(opt_arg, fd, send_msg, &invalid); 
                        }
                        else if(strcmp(cmd,"re") == 0){ 
                            Reply(opt_arg, fd, send_msg, &invalid);   
                        }
                        else if(strcmp(cmd,"fwd") == 0){ 
                            Forward(opt_arg, fd, send_msg, &invalid);
                        }
                        else if(strcmp(cmd,"exit") == 0){
                            Exit(opt_arg, fd, send_msg, &invalid);
                        }
                        else{
                            invalid = 1;
                        }
                        /**/
                        if(invalid == 1){ sprintf(send_msg, "%s", "command error\n");}
                        else if(invalid == 2){ sprintf(send_msg, "%s", "option error\n");}
                        else if(invalid == 3){ sprintf(send_msg, "%s", "args error\n");}
                        else{ printf("valid msg\n"); }
                        bytes_get = write(fd, send_msg, strlen(send_msg));
                        if(bytes_get < 0){
                            error_exit("failed write(): %s\n", strerror(bytes_get));
                        }
                        if(strcmp(cmd, "exit") == 0){
                            FD_CLR(fd, &active_fd_set);
                            close(fd);  
                        }
                        
                    }
                }
            }
        }
    }
}
