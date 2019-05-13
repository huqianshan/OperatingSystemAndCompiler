#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKENSIZE 256
#define MAXSIZE 2048
#define MAXLINE 128
#define DYD_FILE_NAME "out.dyd"
#define ERR_FILE_NAME "out.err"

char code[TOKENSIZE][MAXLINE] = {'\0'};
char *get_word(char *dest,char *src){
    char *raw = dest;
    while ((*src++) == ' ')
    {
    };
    src--;
    while ((*src) != ' ' && (*src) != '\0')
    {
        *dest = (*src);
        src++;
        dest++;
    }
    *dest = '\0';
    return src;
}

int read(char *filename)
{
    FILE *fp = NULL;

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Open src file failedd \n");
        return -1;
    }

    char *buf = malloc(MAXLINE * sizeof(char));
    int n = 0;
    while (fgets(buf, MAXLINE, fp) != NULL)
    {
        //strncpy(code[n++], 16,1);
        printf("\n%s\n", buf);
    }

    fclose(fp);
    free(buf);
    return 0;
}


int main(){
    //read("out.dyd")
    char my[256]="    123 321";
    char you[256] = {'\0'};
    
    printf("%s\n", get_word(you, my));
    get_word(you, get_word(you, my));
    printf("%s\n", you);

}