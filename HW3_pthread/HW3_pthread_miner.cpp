#include<iostream>
#include<fstream>
#include<sstream>
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>

using namespace std;

char Map[2001][2001];

struct parameter{
    int num;
    int start_x;
    int start_y;
    int bound_x;
    int bound_y;
    int *score;  
};

void* Mine(void *data){
    struct parameter p = *(struct parameter*)data;
    int x = p.start_x, y = p.start_y;
    //printf("I am thread%d, x:%d y:%d\n", p.num, x, y);
    while(x < p.bound_x || y < p.bound_y){
        if(Map[y][x] == '-') break;
        else if(Map[y][x] == '|'){
            x = p.start_x;
            y++;
        }
        else if(Map[y][x] == '*'){
            x++;
            (*p.score)++;
        }
        else{ x++;}
   }
    //if encounter '|' new line
    //if encounter '-' break
}

int main(int argc, char *argv[]){
    //read in the map
    //char map_name[10];
    //scanf("%s", &map_name);
    char *map_name = (char *)malloc(sizeof(argv[1]));
    map_name = argv[1];
    FILE *fp = fopen(map_name, "r");
    int width = 0, length = 0, w_mid = 0, l_mid = 0;

    char s[2001];
    while(fgets(s, 2001, fp) != NULL){
        //printf("%d %s", strlen(s), s);
        if(s[0] == '#' && s[1] == '#'){
            l_mid = length;
        }
        sprintf(Map[length], "%s", s);
        length++;
    }
    //printf("\n");
    //width = strlen(s) - 1;
    while(s[width] == '#' || s[width] == '.' || s[width] == '*') width++;
    if(l_mid == 0) while(Map[l_mid + 1][w_mid] != '#') w_mid++;
    else while(Map[l_mid - 1][w_mid] != '#') w_mid++;
    fclose(fp);
    

    //display the map as required
    for(int i = 1; i <= width + 2; i++){
        if(i == 1 || i == width + 2) printf(" ");
        else printf("-");
    }
    printf("\n");
    for(int i = 0; i < length; i++){
        printf("|");
        for(int j = 0; j < width; j++){
            if(Map[i][j] == '#'){
                if(i == l_mid) Map[i][j] = '-';
                else Map[i][j] = '|';
            }
            printf("%c", Map[i][j]);
            //printf("%s", Map[i]);
        }
        printf("|\n"); //Map[i][width] is also '\n'
    }
    for(int i = 1; i <= width + 2; i++){
        if(i == 1 || i == width + 2) printf(" ");
        else printf("-");
    }
    for(int i = 0; i < width; i++) Map[length][i] = '-';
    for(int i = 0; i < length; i++) Map[i][width] = '|';
    printf("\n");
    printf("map size: %d*%d\n", width, length);
    /*printf("%d %d\n", l_mid, w_mid);
    for(int i = 0; i <= length; i++){
        for(int j = 0; j <= width; j++){
            printf("%c", Map[i][j]);
        }
        printf("\n");
    }*/
   
    //create 4 threads as miners
    pthread_t Miner[4];
    struct parameter Para[4];
    int Score[4] = {0, 0, 0, 0};
    
    Para[0].start_x = 0; Para[0].start_y = 0;
    Para[1].start_x = w_mid + 1; Para[1].start_y = 0;
    Para[2].start_x = 0; Para[2].start_y = l_mid + 1;
    Para[3].start_x = w_mid + 1; Para[3].start_y = l_mid +1;
    for(int i = 0; i < 4; i++){
        Para[i].bound_x = width;
        Para[i].bound_y = length;
        Para[i].num = i+1;
        Para[i].score = &Score[i];
    }

    for(int i = 0; i < 4; i++){
        pthread_create(&Miner[i], NULL, Mine, (void *)&Para[i]);
    }

    for(int i = 0; i < 4; i++){
        pthread_join(Miner[i], NULL);
    }
    int highest = 0; 
    bool draw = false;
    for(int i = 0; i < 4; i++){
        if(Score[i] == highest) draw = true;
        if(Score[i] > highest){
           highest = Score[i];
           if(draw) draw = false;
        }
        //printf("%d %d\n", highest, draw);
    } 
    for(int i = 0; i < 4; i++){
        printf("Miner#%d: %d", i+1, Score[i]);
        if(Score[i] == highest){
            if(draw) printf(" (draw)");
            else printf(" (win)");
        }
        printf("\n");
    }
    //free(map_name); why this is wrong in linux1?
}
