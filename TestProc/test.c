#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <FSLib.h>
#include <pthread.h>

void* Fun(void* params)
{
    if(!FS_Create(0, 'D', "A", 06))
    {
        printf("Create [A] failed\n");
        //return NULL;
    }
    printf("1\n");

    int fd = FS_Open("/A/");
    if(fd == -1)
    {
        printf("Open [A] failed\n");
        //return NULL;
    }
    printf("2\n");
    
    if(!FS_Create(fd, 'F', "C", 06))
    {
        printf("Create [C] failed\n");
        //return NULL;
    }
    printf("3\n");
    return NULL;
}

int main(void)
{
	FS_Init();

    pthread_t t;
    pthread_create(&t, NULL, Fun, NULL);
    getchar();
    pthread_cancel(t);
	
	FS_Exit();
	return 0;
}


