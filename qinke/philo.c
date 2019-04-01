#include<stdio.h>
#include<stdlib.h>
#include<semaphore.h>
#include<pthread.h>
#include<time.h>
#include<unistd.h>
#include<string.h>

#define NUM 5

sem_t mutex, chick[NUM];
pthread_t philo[NUM];

int  P(sem_t* i){
    return sem_wait(i);
}

int V(sem_t* i){
    return sem_post(i);
}

void *sophi(void *j){
    
    int i = *((int *)j);
    free(j);
    int t = 0;
    while ((t++)<NUM)
    {

    printf("philo %d is thinking\n", i);
    srand((unsigned int)time(NULL));
    P(&mutex);
    P(&chick[i]);
    P(&chick[(i + 1) % NUM]);
    V(&mutex);

    printf("           philo %d now  is eating\n", i);
    
    sleep(rand() % NUM);
    printf("                     philo %d finishned\n", i);

    V(&chick[i]);
    V(&chick[(i + 1) % NUM]);
    }
    return (void*)NULL;
}

int main()
{
    int err;
    sem_init(&mutex, 0, 1);
    for (int i = 0; i < NUM;i++){
        sem_init(&chick[i], 0, 1);
    }

    for (int k = 0; k < NUM;k++){
        int *tem = (int *)malloc(sizeof(int));
        *tem = k;
        err = pthread_create(&philo[k], NULL, sophi, tem);
        if (err != 0)
        {
            printf("create thread faile %s\n", strerror(err));
        }
    }

    for (int j = 0; j < NUM;j++){
        pthread_join(philo[j],NULL);
    }

}