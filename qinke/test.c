#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>
int main()
{
    int i = 0;
    //printf("%d \n", (i - 1) % 5);
    printf("%-20dwww\n",i);
    srand((unsigned int)time(NULL));
    while(1){
        
        int tem = rand();
        printf("%-20d\n", tem);
        sleep(tem/RAND_MAX);
    }
}