#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<unistd.h> //read(),write()
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#include<sys/errno.h>
#include<stdarg.h>

#include<iostream>
#include<vector>
#include<ctime>
using namespace std;

#define BUF_SIZE 1024
#define QUEUE_LEN 32 //acceptable number of connections in the incoming queue
#define NUM_OF_ACCOUNTS 1000

int error_exit(const char *format, ...){
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

//This function builds passive socket for udp and tcp protocols
int passive_sock(const char *service, const char *transport, int queue_len){
     //service->port number, transport layer protocol-> "tcp" or "udp"
     struct sockaddr_in addr; //an Internet endpoint address
     int sock_fd, sock_type; //file descriptor and type of a socket
     unsigned short portbase = 0; //?

     bzero(&addr, sizeof(addr));
     addr.sin_family = AF_INET;
     addr.sin_addr.s_addr = INADDR_ANY;
     addr.sin_port = htons(atoi(service));

     if(strcmp(transport, "udp") == 0) sock_type = SOCK_DGRAM;
     else sock_type = SOCK_STREAM;

     //create a socket file descriptor
     sock_fd = socket(AF_INET, sock_type, 0);
     if(sock_fd < 0) error_exit("failed socket(): %s\n", strerror(sock_fd));

     //bind the socket file descriptor to a port
     errno = bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
     if(errno < 0) error_exit("failed bind(): %s\n", service, strerror(errno));

     //tcp needs to listen for connection requests
     if(sock_type == SOCK_STREAM && listen(sock_fd, queue_len) < 0){
         error_exit("failed listen(): %s\n", service, strerror(sock_type));
     }

     return sock_fd;
 }

struct Mail{
    bool read; //has been read
    char type[6]; //normal, reply or forward type
    char source[50];
    char destination[50];
    char title[50];
    char content[100];
    tm *t; //date time
    struct Mail *next; //try use queue instead
};

struct Account{
    int my_fd;
    char my_name[25];
    char my_address[50];
    vector<struct Mail> my_box;
};

vector<struct Account> A;

void Parse(char opt_arg[BUF_SIZE], char opt[BUF_SIZE], char arg[BUF_SIZE], int *double_quote){
    char str[BUF_SIZE];
    for(int i = 0; i < BUF_SIZE; i++) { str[i] = 0;}

    sscanf(opt_arg, "%*[ ]%s%*[ ]%[^\0]", opt, str);
    for(int i = 0 ; i < strlen(opt_arg); i++) opt_arg[i] = 0;
    sprintf(opt_arg, "%s", str);
    for(int i = 0 ; i < strlen(str); i++) str[i] = 0;
       
    //if(strcmp(opt, "-n") == 0){ \"number\" is args error}
    if(opt_arg[0] == '\"' && opt_arg[1] != '\"'){ //need to eliminate \"xxxxx\"\"
        *double_quote = 1;
        /*for(int i = 2; i < strlen(opt_arg); i++){
            if(opt_arg[i-1] == '\"' && opt_arg[i] == '\"') *double_quote = 2;
        }*/
        sscanf(opt_arg, "%*[\"]%[^\"]%*[\"]%[^\0]", arg, str); 
    }
    else{
        *double_quote = 0;
        sscanf(opt_arg, "%[^ ]%[^\0]", arg, str);
    }
    if(*double_quote == 1 && (str[0] != ' ' && str[0] != '\0')) *double_quote = 2;//wrong double quote \"xxxxx\"xxx
    for(int i = 0 ; i < strlen(opt_arg); i++) opt_arg[i] = 0;
    sprintf(opt_arg, "%s", str);
    for(int i = 0 ; i < strlen(str); i++) str[i] = 0;
        
    printf("%d[%s],%d[%s],%d[%s]\n", strlen(opt), opt, strlen(arg), arg, strlen(opt_arg), opt_arg);
}

void Initial(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE];
    //bool double_quote = false;
    int double_quote = 0;
    for(int i = 0; i < BUF_SIZE; i++){
        opt[i] = 0;
        arg[i] = 0;
    }
    
    //parse option and arguments   
    Parse(opt_arg, opt, arg, &double_quote);

    //check option and arguments
    if(strcmp(opt, "-u") == 0){
        for(int i = 0; i < strlen(arg); i++){
            if(!(arg[i] == '-' || arg[i] == '_'
              ||(arg[i]>47 && arg[i]<58)
              ||(arg[i]>64 && arg[i]<91)
              ||(arg[i]>96 && arg[i]<123))){
              if(arg[i] == ' '){
                  if(double_quote != 1){
                      arg[i] = '\0';
                      for(int j = i+1; j < strlen(arg); j++){
                          if(arg[j] != ' '){ 
                              *invalid = 4; 
                              break;
                          }
                      }
                  }
              }
              else{
                  *invalid = 4;
                  break;
              }
            }
        }
    }
    else{ *invalid = 3;}
    
    //last opt_arg is valid only if it's empty or all ' '
    if(*invalid == 0){
        for(int i = 0; i < strlen(opt_arg); i++){
            if(opt_arg[i] != ' '){ *invalid = 4; break;}
        }
    }
  
    //do the corresponding operation 
    if(*invalid == 0){
        vector<struct Account>::iterator it;
        for(it = A.begin(); it != A.end(); it++){
            if(strcmp(it->my_name, arg) == 0)
            break;
        }
        if(it == A.end()){ //no duplicated account
            struct Account temp;
            temp.my_fd = fd;
            sprintf(temp.my_name, "%s", arg);
            sprintf(temp.my_address, "%s@nctu.edu.tw", arg);
            A.push_back(temp);
            sprintf(send_msg, "%s@nctu.edu.tw\n", arg);
        }
        else{
            sprintf(send_msg, "This account has been registerd\n");
        }
    }
}

void List(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE];
    //bool double_quote = false;
    int double_quote = 0;
    int type; //1: -u 2: -l 3: -a
    for(int i = 0; i < BUF_SIZE; i++){
        opt[i] = 0;
        arg[i] = 0;
    }
    
    //parse option and arguments   
    Parse(opt_arg, opt, arg, &double_quote);

    //check if the fd is a valid account
    vector<struct Account>::iterator it;
    for(it = A.begin(); it != A.end(); it++){
        if(it->my_fd == fd){
            break;     
        }
    }
    if(it == A.end()) *invalid = 1;

    //check option and arguments
    if(*invalid == 0 || strcmp(opt, "-u") == 0){
        if(strcmp(opt, "-u") == 0){ *invalid = 0; type = 1;}
        else if(strcmp(opt, "-l") == 0){ type = 2;}
        else if(strcmp(opt, "-a") == 0){ type = 3;}
        else{ *invalid = 3;}
    }
    
    //last opt_arg is valid only if it's empty or all ' '
    if(*invalid == 0){
        for(int i = 0; i < strlen(opt_arg); i++){
            if(opt_arg[i] != ' '){ *invalid = 4; break;}
        }
        for(int i = 0; i < strlen(arg); i++){
            if(arg[i] != ' '){ *invalid = 4; break;}
        }
    }
    
    //do the corresponding operation 
    if(*invalid == 0){
        //vector<struct Account>::iterator it;
        switch(type){
            case 1: //-u
                    if(A.size() == 0){
                        sprintf(send_msg, "%s", "no accounts\n");
                    }
                    else{
                        for(it = A.begin(); it != A.end(); it++){
                             int start = strlen(send_msg);
                             sprintf(send_msg+start, "%s\n", it->my_address);
                        }
                    }
                    break;
            case 2: //-l
                    if(it->my_box.size() == 0){
                        sprintf(send_msg, "%s", "no mail\n");
                    }
                    else{
                        vector<struct Mail>::iterator mit; int i = 1;
                        for(mit = it->my_box.begin(); mit != it->my_box.end(); mit++){
                            int start = strlen(send_msg);
                            if(mit->read){
                                sprintf(send_msg+start, "%d.%s%s\n", i, mit->type, mit->title);
                            }
                            else{
                                sprintf(send_msg+start, "%d.%s%s(new)\n", i, mit->type, mit->title);
                            }
                            i++;
                        }
                    }
                    break;
            case 3: //-a
                    sprintf(send_msg, "Account: %s\nAddress: %s\nNumber of mails: %d\n", it->my_name, it->my_address, it->my_box.size());
                    break;
            default:
                    break;
        }
    }
}
void Remove(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE];
    //bool double_quote = false;
    int double_quote = 0;
    int type; //1: -D 2: -d
    for(int i = 0; i < BUF_SIZE; i++){
        opt[i] = 0;
        arg[i] = 0;
    }

    //parse option and arguments
    sscanf(opt_arg, "%*[ ]%s%*[ ]%[^\0]", opt, arg);   
    
    //check if the fd is a valid account
    vector<struct Account>::iterator it;
    for(it = A.begin(); it != A.end(); it++){
        if(it->my_fd == fd){
            break;     
        }
    }
    if(it == A.end()) *invalid = 1;
 
    //check option and arguments
    if(*invalid == 0){
        if(strcmp(opt, "-D") == 0){ 
            type = 1;
            for(int i = 0; i < strlen(arg); i++){
                if(arg[i] != ' '){
                    *invalid = 4;
                    break;
                }
            }
        }
        else if(strcmp(opt, "-d") == 0){ 
            type = 2;
            for(int i = 0; i < strlen(arg); i++){
                if(!((arg[i]>47 && arg[i]<58)
                   ||(arg[i]>64 && arg[i]<91)
                   ||(arg[i]>96 && arg[i]<123))){
                   if(arg[i] == ' '){
                       arg[i] = '\0';
                       for(int j = i+1; j < strlen(arg); j++){
                           if(arg[j] != ' '){
                               *invalid = 4;
                               break;
                           }
                       }
                   }
                   else{
                       *invalid = 4;
                       break;
                   }
                }
            }
        }
        else{ *invalid = 3;}
    }
    
    //do the corresponding operation 
    if(*invalid == 0){
        switch(type){
            case 1: //-D
                    it->my_box.clear();
                    sprintf(send_msg, "%s", "done\n");
                    break;
            case 2: //-d
                    int Nth;
                    sscanf(arg, "%d", &Nth);
                    if(Nth <= it->my_box.size()){
                        vector<struct Mail>::iterator mit = it->my_box.begin();
                        it->my_box.erase(mit+(Nth-1));
                        sprintf(send_msg, "%s", "done\n");
                    }
                    else{ *invalid = 4;}
                    break;
            default:
                    break;
        }
    }
}
void Read(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE];
    //bool double_quote = false;
    int double_quote = 0;
    for(int i = 0; i < BUF_SIZE; i++){
        opt[i] = 0;
        arg[i] = 0;
    }
    
    //parse option and arguments
    sscanf(opt_arg, "%*[ ]%s%*[ ]%[^\0]", opt, arg);   
    
    //check if the fd is a valid account
    vector<struct Account>::iterator it;
    for(it = A.begin(); it != A.end(); it++){
        if(it->my_fd == fd){
            break;     
        }
    }
    if(it == A.end()) *invalid = 1;

    //check option and arguments
    if(*invalid == 0){
        if(strcmp(opt, "-r") == 0){ 
            for(int i = 0; i < strlen(arg); i++){
                if(!((arg[i]>47 && arg[i]<58)
                   ||(arg[i]>64 && arg[i]<91)
                   ||(arg[i]>96 && arg[i]<123))){
                   if(arg[i] == ' '){
                       arg[i] = '\0';
                       for(int j = i+1; j < strlen(arg); j++){
                           if(arg[j] != ' '){
                               *invalid = 4;
                               break;
                           }
                       }
                   }
                   else{
                       *invalid = 4;
                       break;
                   }
                }
            }
        }
        else{ *invalid = 3;}
    }

    //do the corresponding operation
    if(*invalid == 0){
        int Nth;
        sscanf(arg, "%d", &Nth);
        if(Nth <= it->my_box.size()){
            vector<struct Mail>::iterator mit = it->my_box.begin()+(Nth-1);
            mit->read = true;
            sprintf(send_msg, "From: %s\nTo: %s\nDate: %d-%d-%d %d:%d:%d\nTitle: %s%s\nContent: %s\n", mit->source, mit->destination, (mit->t->tm_year)+1900, (mit->t->tm_mon)+1, mit->t->tm_mday, mit->t->tm_hour, mit->t->tm_min, mit->t->tm_sec ,mit->type ,mit->title, mit->content);

            //deal with reply and forward mails
            struct Mail *m = mit->next;
            while(m != NULL){
                int start = strlen(send_msg);
                sprintf(send_msg+start, "----\nFrom: %s\nTo: %s\nDate: %d-%d-%d %d:%d:%d\nTitle: %s%s\nContent: %s\n", m->source, m->destination, (m->t->tm_year)+1900, (m->t->tm_mon)+1, m->t->tm_mday, m->t->tm_hour, m->t->tm_min, m->t->tm_sec ,m->type ,m->title, m->content);
                m = m->next;
            }
        }
        else{ *invalid = 4;}
    }
}
void Write(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE];
    //bool double_quote = false;
    int double_quote = 0;
    int type; //1: -d 2: -t 3: -c
    struct Mail temp;
    
    //check if the fd is a valid account
    vector<struct Account>::iterator it;
    for(it = A.begin(); it != A.end(); it++){
        if(it->my_fd == fd){
            break;     
        }
    }
    if(it == A.end()) *invalid = 1;
    
    while(strlen(opt_arg) != 0 && *invalid == 0){
        for(int i = 0; i < BUF_SIZE; i++){
            opt[i] = 0;
            arg[i] = 0;
        }
        
        //parse option and arguments   
        Parse(opt_arg, opt, arg, &double_quote);

        //check option
        if(*invalid == 0){
            if(strcmp(opt, "-d") == 0){ type = 1;}            
            else if(strcmp(opt, "-t") == 0){ type = 2;}            
            else if(strcmp(opt, "-c") == 0){ type = 3;}            
            else{ *invalid = 3;}
        }

        //check arguments
        if(*invalid == 0){
            for(int i = 0; i < strlen(arg); i++){
                if(!(arg[i] == '-' || arg[i] == '_'
                   ||arg[i] == ':' || arg[i] == '.' || arg[i] == '@'
                   ||(arg[i]>47 && arg[i]<58)
                   ||(arg[i]>64 && arg[i]<91)
                   ||(arg[i]>96 && arg[i]<123))){
                    if(arg[i] == ' '){
                        if(double_quote != 1){
                            printf("\n\n%d\n\n\n",double_quote);
                            arg[i] = '\0';
                            for(int j = i+1; j < strlen(arg); j++){
                                if(arg[j] != ' '){ 
                                    *invalid = 4; 
                                    break;
                                }
                            }
                        }
                    }
                    else{
                        *invalid = 4;
                        break;
                    }
                }
            }
            if(type == 1){ //check if the destination existes->args error
                vector<struct Account>::iterator it2;
                for(it2 = A.begin(); it2 != A.end(); it2++){
                    if(strcmp(it2->my_address, arg) == 0) break;
                }
                if(it2 == A.end()) *invalid = 4;
            }
        }
    

        //do the corresponding operation
        if(*invalid == 0){
            
            switch(type){
                case 1: //-d
                        sprintf(temp.destination, "%s", arg);
                        break;
                case 2: //-t
                        sprintf(temp.title, "%s", arg);
                        break;
                case 3: //-c
                        sprintf(temp.content, "%s", arg);
                        break;
                default:
                        break;
            }
        }
    }

    //last opt_arg is valid only if it's empty or all ' '
    if(*invalid == 0){
        for(int i = 0; i < strlen(opt_arg); i++){
            if(opt_arg[i] != ' '){ *invalid = 4; break;}
        }        
    }

    if(*invalid == 0){
        sprintf(temp.source, "%s", it->my_address);
        temp.read = false;
        sprintf(temp.type, "%s", "");
        temp.next = NULL;
        time_t now = time(0);
        temp.t = localtime(&now);
        for(it = A.begin(); it != A.end(); it++){ //give receiver the mail
            if(strcmp(temp.destination, it->my_address) == 0){
                it->my_box.push_back(temp);
                break;
            }
        }
        sprintf(send_msg, "%s", "done\n");
    }
}
void Reply(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE];
    //bool double_quote = false;
    int double_quote = 0;
    int type; //1: -c 2: -n
    struct Mail temp;
    
    int Nth;
    vector<struct Mail>::iterator mit;
    
    //check if the fd is a valid account
    vector<struct Account>::iterator it;
    for(it = A.begin(); it != A.end(); it++){
        if(it->my_fd == fd){
            break;     
        }
    }
    if(it == A.end()) *invalid = 1;
    
    while(strlen(opt_arg) != 0 && *invalid == 0){
        for(int i = 0; i < BUF_SIZE; i++){
            opt[i] = 0;
            arg[i] = 0;
        }
        
        //parse option and arguments   
        Parse(opt_arg, opt, arg, &double_quote);

        //check option and arguments
        if(*invalid == 0){
            if(strcmp(opt, "-c") == 0){ 
                type = 1;
                for(int i = 0; i < strlen(arg); i++){
                    if(!(arg[i] == '-' || arg[i] == '_'
                       ||arg[i] == ':' || arg[i] == '.' || arg[i] == '@'
                       ||(arg[i]>47 && arg[i]<58)
                       ||(arg[i]>64 && arg[i]<91)
                       ||(arg[i]>96 && arg[i]<123))){
                        if(arg[i] == ' '){
                            if(double_quote != 1){
                                arg[i] = '\0';
                                for(int j = i+1; j < strlen(arg); j++){
                                    if(arg[j] != ' '){ 
                                        *invalid = 4; 
                                        break;
                                    }
                                }
                            }
                        }
                        else{
                            *invalid = 4;
                            break;
                        }
                    }
                }
            }            
            else if(strcmp(opt, "-n") == 0){ 
                type = 2;
                for(int i = 0; i < strlen(arg); i++){
                    if(!((arg[i]>47 && arg[i]<58)
                       ||(arg[i]>64 && arg[i]<91)
                       ||(arg[i]>96 && arg[i]<123))){
                        if(double_quote == 1 || double_quote == 2){ *invalid = 4; break;}
                        if(arg[i] == ' '){
                            arg[i] = '\0';
                            for(int j = i+1; j < strlen(arg); j++){
                                if(arg[j] != ' '){
                                    *invalid = 4;
                                    break;
                                }
                            }
                        }
                        else{
                            *invalid = 4;
                            break;
                        }
                    }
                }
            }            
            else{ *invalid = 3;}
        }

    
        //do the corresponding operation
        if(*invalid == 0){
            
            switch(type){
                case 1: //-c
                        sprintf(temp.content, "%s", arg);
                        break;
                case 2: //-n
                        sscanf(arg, "%d", &Nth);
                        if(Nth <= it->my_box.size()){
                            mit = it->my_box.begin()+(Nth-1);
                        }
                        else{ *invalid = 4;}
                        break;
                default:
                        break;
            }
        }
    }
    
    //last opt_arg is valid only if it's empty or all ' '
    if(*invalid == 0){
        for(int i = 0; i < strlen(opt_arg); i++){
            if(opt_arg[i] != ' '){ *invalid = 4; break;}
        }        
    }
  
    if(*invalid == 0){
        time_t now = time(0);
        temp.t = localtime(&now);
        temp.read = false;
        sprintf(temp.source, "%s", mit->destination);
        sprintf(temp.destination, "%s", mit->source);
        sprintf(temp.title, "%s", mit->title);
        sprintf(temp.type, "%s", "re:");

        //append content of the previous mail being replied
        struct Mail *m = (struct Mail *)malloc(sizeof(struct Mail));
        m->t = mit->t;
        m->read = mit->read;
        m->next = mit->next;
        sprintf(m->source, "%s", mit->source);
        sprintf(m->destination, "%s", mit->destination);
        sprintf(m->title, "%s", mit->title);
        sprintf(m->type, "%s", mit->type);
        sprintf(m->content, "%s", mit->content);
        temp.next = m;
        for(it = A.begin(); it != A.end(); it++){
            if(strcmp(it->my_address, temp.destination) == 0){
                it->my_box.push_back(temp);
                break;
            }
        }

        sprintf(send_msg, "%s", "done\n");
    }
}
void Forward(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
    char opt[BUF_SIZE], arg[BUF_SIZE];
    //bool double_quote = false;
    int double_quote = 0;
    int type; //1: -c 2: -n 3: -d
    struct Mail temp;
    
    int Nth;
    vector<struct Mail>::iterator mit;
    
    //check if the fd is a valid account
    vector<struct Account>::iterator it;
    for(it = A.begin(); it != A.end(); it++){
        if(it->my_fd == fd){
            break;     
        }
    }
    if(it == A.end()) *invalid = 1;
    
    while(strlen(opt_arg) != 0 && *invalid == 0){
        for(int i = 0; i < BUF_SIZE; i++){
            opt[i] = 0;
            arg[i] = 0;
        }
        
        //parse option and arguments   
        Parse(opt_arg, opt, arg, &double_quote);

        //check option and arguments
        if(*invalid == 0){
            if(strcmp(opt, "-c") == 0 || strcmp(opt, "-d") == 0){ 
                if(strcmp(opt, "-c") == 0) type = 1;
                if(strcmp(opt, "-d") == 0) type = 3;
                for(int i = 0; i < strlen(arg); i++){
                    if(!(arg[i] == '-' || arg[i] == '_'
                       ||arg[i] == ':' || arg[i] == '.' || arg[i] == '@'
                       ||(arg[i]>47 && arg[i]<58)
                       ||(arg[i]>64 && arg[i]<91)
                       ||(arg[i]>96 && arg[i]<123))){
                        if(arg[i] == ' '){
                            if(double_quote != 1){
                                arg[i] = '\0';
                                for(int j = i+1; j < strlen(arg); j++){
                                    if(arg[j] != ' '){ 
                                        *invalid = 4; 
                                        break;
                                    }
                                }
                            }
                        }
                        else{
                            *invalid = 4;
                            break;
                        }
                    }
                }
                if(type == 3){ //check if the destination existes->args error
                    vector<struct Account>::iterator it2;
                    for(it2 = A.begin(); it2 != A.end(); it2++){
                        if(strcmp(it2->my_address, arg) == 0) break;
                    }
                    if(it2 == A.end()) *invalid = 4;
                }
            }            
            else if(strcmp(opt, "-n") == 0){ 
                type = 2;
                for(int i = 0; i < strlen(arg); i++){
                    if(!((arg[i]>47 && arg[i]<58)
                       ||(arg[i]>64 && arg[i]<91)
                       ||(arg[i]>96 && arg[i]<123))){
                        if(double_quote == 1 || double_quote == 2){ *invalid = 4; break;} //\"0-9\"
                        if(arg[i] == ' '){
                            arg[i] = '\0';
                            for(int j = i+1; j < strlen(arg); j++){
                                if(arg[j] != ' '){
                                    *invalid = 4;
                                    break;
                                }
                            }
                        }
                        else{
                            *invalid = 4;
                            break;
                        }
                    }
                }
            }            
            else{ *invalid = 3;}
        }

    
        //do the corresponding operation
        if(*invalid == 0){
            
            switch(type){
                case 1: //-c
                        sprintf(temp.content, "%s", arg);
                        break;
                case 2: //-n
                        sscanf(arg, "%d", &Nth);
                        if(Nth <= it->my_box.size()){
                            mit = it->my_box.begin()+(Nth-1);
                        }
                        else{ *invalid = 4;}
                        break;
                case 3: //-d
                        sprintf(temp.destination, "%s", arg);
                        break;
                default:
                        break;
            }
        }
    }
    
    //last opt_arg is valid only if it's empty or all ' '
    if(*invalid == 0){
        for(int i = 0; i < strlen(opt_arg); i++){
            if(opt_arg[i] != ' '){ *invalid = 4; break;}
        }        
    }
  
    if(*invalid == 0){
        time_t now = time(0);
        temp.t = localtime(&now);
        temp.read = false;
        sprintf(temp.source, "%s", it->my_address);
        //sprintf(temp.destination, "%s", mit->source);
        sprintf(temp.title, "%s", mit->title);
        sprintf(temp.type, "%s", "fwd:");

        //append content of the previous mail being replied
        struct Mail *m = (struct Mail *)malloc(sizeof(struct Mail));
        m->t = mit->t;
        m->read = mit->read;
        m->next = mit->next;
        sprintf(m->source, "%s", mit->source);
        sprintf(m->destination, "%s", mit->destination);
        sprintf(m->title, "%s", mit->title);
        sprintf(m->type, "%s", mit->type);
        sprintf(m->content, "%s", mit->content);
        temp.next = m;
        for(it = A.begin(); it != A.end(); it++){
            if(strcmp(it->my_address, temp.destination) == 0){
                it->my_box.push_back(temp);
                break;
            }
        }

        sprintf(send_msg, "%s", "done\n");
    }
}

void Exit(char opt_arg[BUF_SIZE], int fd, char send_msg[BUF_SIZE], int *invalid){
     if(strlen(opt_arg) != 0){
         for(int i = 0; i < strlen(opt_arg); i++){
             if(opt_arg[i] != ' '){
                 *invalid = 4;
                 break;
             }
         }
     }
     vector<struct Account>::iterator it;
     for(it = A.begin(); it != A.end(); it++){
         if(it->my_fd == fd){
             A.erase(it);
             break;
         }
     }
     if(*invalid == 0) sprintf(send_msg, "%s", "exit\n");
}

int main(int argc, char *argv[]){
    struct sockaddr_in client_addr;
    socklen_t client_addrlen;
    char *service;
    int master_sock_fd, slave_sock_fd, max_sock_fd;

    if(argc != 2) error_exit("usage: %s [port number]\n", argv[0]);
    service = argv[1];
    master_sock_fd = passive_sock(service, "tcp", QUEUE_LEN);

    //Use select() to supervise on all connections
    fd_set active_fd_set, read_fd_set; //active shows which fd is set to 1
    FD_ZERO(&active_fd_set); //clear all fds in the set
    FD_SET(master_sock_fd, &active_fd_set); //add an fd into the set
    max_sock_fd = master_sock_fd;

    while(1){
        read_fd_set = active_fd_set; //read is used for processing
        //set the fd being requested to 1
        errno = select(max_sock_fd+1, &read_fd_set, NULL, NULL, NULL);
        if(errno < 0){ //-1
            error_exit("failed select(): %s\n", strerror(errno));
        }
        else{
            for(int fd = 0; fd <= max_sock_fd+1; fd++){
                if(FD_ISSET(fd, &read_fd_set)){ //chech if fd is set to 1
                    if(fd == master_sock_fd){
                        client_addrlen = sizeof(client_addr);
                        slave_sock_fd = accept(fd, (struct sockaddr *)&client_addr, &client_addrlen);
                        if(slave_sock_fd < 0){
                            error_exit("failed accept(): %s\n", strerror(slave_sock_fd));
                        }
                        else{
                            char from[INET_ADDRSTRLEN];
                            printf("new connection from ");
                            printf("%s ", inet_ntop(AF_INET, &client_addr.sin_addr, from, INET_ADDRSTRLEN));
                            printf("on sock_fd: %d\n", slave_sock_fd);
                            FD_SET(slave_sock_fd, &active_fd_set);
                            max_sock_fd = (slave_sock_fd > max_sock_fd?slave_sock_fd:max_sock_fd);
                        }
                    }
                    else{ //slave_sock_fds
                        char recv_msg[BUF_SIZE], send_msg[BUF_SIZE];
                        int bytes_get;
                        char cmd[BUF_SIZE], opt_arg[BUF_SIZE];
                        int invalid = 0; //0: OK, 1: unregistered, 2: cmd, 3: opt, 4: args error
                        //clear buffer string to '\0', or it will read wrong
                        for(int i = 0; i < BUF_SIZE; i++){
                            recv_msg[i] = 0;
                            send_msg[i] = 0;
                            cmd[i] = 0;
                            opt_arg[i] = 0;
                        }
                        //read a message from a fd
                        bytes_get = read(fd, recv_msg, BUF_SIZE);
                        if(bytes_get < 0){
                            error_exit("failed read(): %s\n", strerror(bytes_get));
                        }
                        else{ 
                            //printf("Read %d[%s]\n", strlen(recv_msg), recv_msg);
                        }
                        //get command and do the operation
                        if(recv_msg[0] != ' '){
                            sscanf(recv_msg, "%[^ ]%[^\0]", cmd, opt_arg);
                        }
                        else{
                            sscanf(recv_msg, "%*[ ]%[^ ]%[^\0]", cmd, opt_arg);
                        }
                        //printf("%d[%s],%d[%s]\n", strlen(cmd), cmd, strlen(opt_arg), opt_arg);
                        if(strcmp(cmd,"exit") == 0){
                            Exit(opt_arg, fd, send_msg, &invalid);
                        }
                        else if(strcmp(cmd,"init") == 0){ 
                            Initial(opt_arg, fd, send_msg, &invalid);
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
                        else{ invalid = 2;}
                       
                        if(invalid != 0 && strlen(send_msg) != 0){
                            for(int i = 0; i < BUF_SIZE; i++) send_msg[i] = 0;
                        }
 
                        //write a message to a fd
                        switch(invalid){
                            case 1:
                                    sprintf(send_msg, "%s", "init first\n");
                                    break;
                            case 2:
                                    sprintf(send_msg, "%s", "command error\n");
                                    break;
                            case 3:
                                    sprintf(send_msg, "%s", "option error\n");
                                    break;
                            case 4:
                                    sprintf(send_msg, "%s", "args error\n");
                                    break;
                            default:
                                    printf("vaild message\n");
                                    break;
                        }
                        bytes_get = write(fd, send_msg, strlen(send_msg));
                        if(bytes_get < 0){
                            error_exit("failed write(): %s\n", strerror(bytes_get));
                        }
                        if(strcmp(cmd, "exit") == 0){
                            printf("fd %d closed\n", fd);
                            FD_CLR(fd, &active_fd_set); //remove fd from the set
                            close(fd);
                        }
                    }
                }
            }
        }
    } 
}
