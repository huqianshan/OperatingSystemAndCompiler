#include <stdio.h>
#include <stdlib.h>

int inset_sort(int arr[], int len)
{
    //
    for (size_t i = 1; i < len; i++)
    {
        /* code */
        int key = arr[i];

        int j = i - 1;
        while (j >= 0&&arr[j] > key)
        {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

int arr[8] = {2,8,7,1,3,5,6,4};

void swap(int *a,int *b){
    int tem = *a;
    *a = *b;
    *b = tem;
}

/*Quick Sort*/
int partition(int *arr, int begin, int end)
{
    int x = arr[end];
    int i = begin-1;

    for (int j = begin; j < end;j++){
        if(arr[j]<=x){
            i++;
            swap(&arr[j], &arr[i]);
        }
    }
    swap(&arr[i+1], &arr[end]);
    return i + 1;
}

int quick_sort(int *arr,int begin,int end){
    if(begin<end){
        int q = partition(arr, begin, end);
        quick_sort(arr, begin, q - 1);
        quick_sort(arr, q+1, end);
    }
    return;
}

void Print(int* arr,int len){
    
    for (size_t i = 0; i <len; i++)
    {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(){
    int len = sizeof(arr) / sizeof(int);
    printf("len : %d,pointer: %p\n",len, arr);
    Print(arr, len);
    quick_sort(arr, 0,len-1);
    Print(arr,len);
   // printf("%d\n",re);
}