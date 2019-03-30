#include "csapp.h"
#define MAX 8296

int main(int argc,char **argv){
    if(argc!=2){
        printf("enter filename\n");
        exit(1);
    }

    int n,fd;
    rio_t rio;
    char buf[MAX+1];
    char out[MAX];
    char blank = ' ';

    if(fd=open(argv[1],O_RDONLY,0)){
        n = read(fd, buf, MAX);
        printf("file length is %d\n", n);
    }

    int j = 0;
    buf[MAX] = '\0';
    int i;
    for (i = 0; i < n; i++)
    {
        // find first null and last 
        if(buf[i]==blank){
            continue;
        }
        if(buf[i]=='/'&&buf[i+1]=='/'){
            int t=i+2;
            while (buf[t++] != '\n')
                ;
            i = t;
        }

        if(buf[i]=='/'&&buf[i+1]=='*'){
            int t = i + 2;
            while(buf[t]!='*'||buf[t+1]!='/'){
                t++;
            }
            i = t+2;
        }
        out[j++] = buf[i];
    }
    out[j] = '\0';
    printf("%s\n", out);

    int wt = open("test.s", O_CREAT | O_APPEND | O_RDWR, 0);
    int q=write(wt, out, 256);
    printf("\n%d\n", q);
    return 0;
}