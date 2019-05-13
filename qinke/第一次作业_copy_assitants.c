
// 复制制定目录下的指定日期之后的文件至新的目录，递归的保持文件目录结构
// Copy the file after the specified date in the specified directory 
//  to the new directory, recursively maintain the file directory structure,
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define __USE_XOPEN
#include<time.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<dirent.h>
#include<fcntl.h>
/*
: hujinlei
: 2016060203025
: 2019-3-11 22:24
*/

// anyone can write and read
#define FILE_MODE S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IWOTH|S_IROTH
#define DIR_MODE 777
#define FILE_MASK S_IXGRP|S_IXOTH|S_IWOTH
#define DIR_MAX_DEPTH 256
#define BUFF_MAX_SIZE 256


time_t time_to_int(char* time){
    if(strlen(time)!=14){
        printf("the time  %s is not correct \n",time);
        return 0;
    }

    struct tm* tmp_time=(struct tm*)malloc(sizeof(struct tm));
    strptime(time,"%Y%m%d%H%M%S",tmp_time);
    time_t t=mktime(tmp_time);

    free(tmp_time);
    return t;
}


int folder_make(char* path){
    
    if(!access(path,F_OK)){
        printf("maybe have no rights to make new dir or dir is existed %s \n",path);
        return 1;
    }

    int n;
    umask(000);
    n=mkdir(path,0777);
    if(n!=0){
        printf("when make dir %s ,meet bug\n",path);
        return -1;
    }
    return 0;
}


int copy_file(char* dest,char* src){
    int src_fd,dest_fd;
    char buf[BUFF_MAX_SIZE];

    if((src_fd=open(src,O_RDONLY,0))==-1){
        printf("some error in open file %s\n",src);
        return -1;
    }
    else{
        umask(FILE_MASK);
        dest_fd=open(dest,O_RDWR|O_CREAT,FILE_MODE);
    }

    if(dest_fd<0){
        printf("dest %s open failed maybe you have not rights to acess\n",dest);
        return -1;
    }

    int n;
    while((n=read(src_fd,buf,BUFF_MAX_SIZE))!=0){
        write(dest_fd,buf,n);
    }
    printf("copy ok %s \n",dest);
    close(src_fd);
    close(dest_fd);
    return 0;
}

// path must be directory
int recursive(char* des,char* path,time_t Time){

    DIR* dir_ptr;
    struct dirent* dep;

    if((dir_ptr=opendir(path))==NULL){
        printf("the path %s is not dir ; go away \n",path);
        return -1;
    }
    
    // if success return  pointer to next file in dir
    while((dep=readdir(dir_ptr))!=NULL){
        char file_name[DIR_MAX_DEPTH];
        strcpy(file_name,path);
        strcat(file_name,"/");
        strcat(file_name,dep->d_name);     

        // the metadat return by stat() funciton
        struct stat Stat;
        stat(file_name,&Stat);

        // ignore . file
        if(dep->d_name[0]=='.'){
            continue;
        }

        //printf("the file absolute name is %s \n",file_name);
        char dest_file[DIR_MAX_DEPTH];
        strcpy(dest_file,des);
        strcat(dest_file,"/");
        strcat(dest_file,dep->d_name);

        if(S_ISDIR(Stat.st_mode)){
            int t = folder_make(dest_file);
            if(t==0){
                printf("make dir %s\n",file_name);
            }
            recursive(dest_file,file_name,Time);
        }
        else if(S_ISREG(Stat.st_mode) && Stat.st_mtime>Time){
            int tem=copy_file(dest_file,file_name);
            if(tem!=0){
                printf("some error in %s file\n",dest_file);
            }
        }
    }
    return 0;
}



int main(int argc,char** argv){
    if(argc<3){
        printf("arguments missed \n");
        exit(-1);
    }

    time_t time_file = time_to_int(argv[1]);

    char *src,*des;
    des=argv[2];
    src=argv[3];

    //  determine if  directory existed
    if(access(src,F_OK)!=0){
        printf("the source file or dir is not existed \n");
        exit(-1);
    }
    if(access(des,F_OK)!=0){
        folder_make(des);
        printf("make a direcotry %s for you no thanks! \n",des);
    }

    // recursively scan src  
    recursive(des,src,time_file);

}
