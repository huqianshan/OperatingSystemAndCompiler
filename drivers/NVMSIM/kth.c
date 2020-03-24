#include<stdio.h>
#include<stdlib.h>
#include "kth.h"

int minium(int *arr, int n){
    if(arr==NULL||n<=0){
        return -2;
    }
    int i;
    int min = arr[0],index=0;
    for (i = 0; i < n; i++)
    {
        if(min>arr[i]){
            min = arr[i];
            index = i;
        }
    }
    return index;
}


int maxium(int *arr, int n){
    if(arr==NULL||n<=0){
        return -2;
    }
    int i;
    int max = arr[0],index=0;
    for (i = 0; i < n; i++)
    {
        if(max<arr[i]){
            max = arr[i];
            index = i;
        }
    }
    return index;
}

int bigK(int *arr, int n, int k){
    if(arr==NULL||n<=0||k>n||k<=0){
        return NULL;
    }

    int *tem = malloc(sizeof(int) * k);
    int i;
    for (i = 0; i < k;i++){
        tem[i] = arr[i];
    }

    int min_index = minium(tem, k);
    int min = tem[min_index];
    for (i = k; i < n; i++)
    {
        if (arr[i] > min)
        {
            tem[min_index] = arr[i];
            min_index = minium(tem,k);
            min = tem[min_index];
        }
    }
    return tem;
}

int bigK_index(int *arr, int n, int k){
    if(arr==NULL||n<=0||k>n||k<=0){
        return NULL;
    }

    int *tem = malloc(sizeof(int) * k);
    int *addr = malloc(sizeof(int) * k);
    int i;
    for (i = 0; i < k;i++){
        tem[i] = arr[i];
        addr[i] = i;
    }

    int min_index = minium(tem, k);
    int min = tem[min_index];
    for (i = k; i < n; i++)
    {
        if (arr[i] > min)
        {
            tem[min_index] = arr[i];
            addr[min_index] = i;
            min_index = minium(tem, k);
            min = tem[min_index];
        }
    }
    if(tem){
        free(tem);
    }

    return addr;
}

int smallK_index(int *arr, int n, int k){
    if(arr==NULL||n<=0||k>n||k<=0){
        return NULL;
    }

    int *tem = malloc(sizeof(int) * k);
    int *addr = malloc(sizeof(int) * k);
    int i;
    for (i = 0; i < k;i++){
        tem[i] = arr[i];
        addr[i] = i;
    }

    int max_index = maxium(tem, k);
    int max = tem[max_index];
    for (i = k; i < n; i++)
    {
        if (arr[i] < max)
        {
            tem[max_index] = arr[i];
            addr[max_index] = i;
            max_index = maxium(tem, k);
            max = tem[max_index];
        }
    }
    if(tem){
        free(tem);
    }

    return addr;
}

int main(){
    /*check for minum function*/
    //int arr[] = {3,3,3};
    //int arr[] = {};
    //int arr[] = {-1, -3, -2, -1};
    int arr[] = {1,3333, 1, 1,566866, -2222};
    int size = sizeof(arr) / sizeof(int);
    int min_index = maxium(arr, size);
    printf("Arr %x bytes: %d size %d minum index %d min: %d\n", (unsigned)arr, sizeof(arr),size, min_index, arr[min_index]);

    /**
     * check for bigK function
     */
    int brr[] = {-12,23,44, 1, 5, -2222};
    //int brr[] = {};
    //int brr[] = {3,6,15,894,13,64,1,8161,81,3,631,-115,6135,31135}; // ok
    //int brr[] = {1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};
    //int brr[] = {11, 11, 11, 11, 11};
    int bsize = sizeof(brr) / sizeof(int);
    int k = 2;
    //int k = 0;
    //int k = 1;
    //int k = bsize - 1;
    int *bigKarr = bigK(brr, bsize, k);

    if(bigKarr){
        printf("Sucess,Brr %x size %d k %d\n", brr, bsize, k);
        for (int i = 0; i < k;i++){
            printf("%d ", bigKarr[i]);
        }
        printf("\n");
        free(bigKarr);
    }
    else
    {
        printf("Failed,Brr %x size %d k %d\n", brr, bsize, k);
    }

    /**
     * check for bigK_index function
     */
    int *index = smallK_index(brr, bsize, k);
    if(index){
        printf("index:\n");
        for (int i = 0; i < k;i++){
            printf("brr[%d]=%d ", index[i],brr[index[i]]);
        }
        printf("\n");
        free(index);
    }else{
        printf("index failed\n");
    }
}
