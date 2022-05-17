#include "FSLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <FS_IPC.h>
#include <string.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

int qid;
sem_t mtx;
key_t shm_key;

static int GetNewSHM(int size, int permissions);
static int GetReturnValue(int shmid);

void FS_Init()
{
	int reg_qid = msgget(regq_key, 0666);
	if(reg_qid == -1)
	{
		perror("[Init 1] msgget");
		exit(EXIT_FAILURE);
	}

	qid = msgget(12346, IPC_CREAT | 0666);
	if(qid == -1)
	{
		perror("[Init 2] msgget");
		exit(EXIT_FAILURE);
	}
	
	struct RequestBuf req_buf = {
		.mtype = 1,
		.pid = getpid(),
		.uid = getuid(),
		.qid = qid,
	};
	
	if(msgsnd(reg_qid, &req_buf, sizeof(int) * 3, 0) == -1)
	{
		perror("[Init 3] msgsnd");
		exit(EXIT_FAILURE);
	}

    sem_init(&mtx, 0, 1);

    srand(time(NULL));
    shm_key = rand();
}

void FS_Exit()
{
    sem_destroy(&mtx);

	struct CommandBuf buf;
	buf.mtype = CommandType_Exit;
	if(msgsnd(qid, &buf, sizeof(buf.mtext), 0) == -1)
	{
		perror("[Exit 1] msgsnd");
		exit(EXIT_FAILURE);
	}
    sleep(1);
    msgctl(qid, IPC_RMID, NULL);
}

int FS_Create(int fd, char type, const char* name, int permissions)
{
	struct CommandBuf cbuf = { .mtype = CommandType_Create };

    const int shmid = GetNewSHM(strlen(name) + 1, 0666);
    if(shmid == -1)
    {
        perror("[Create 1] shmget");
        exit(EXIT_FAILURE);
    }

    char* shm = shmat(shmid, NULL, 0);
    strncpy(shm, name, 255);
    shmdt(shm);

    struct CreateParameters params = { 
        .etype = type, 
        .f_idx = fd, 
        .name_shmid = shmid, 
        .permissions = permissions 
    };
    
    memcpy(cbuf.mtext, &params, sizeof(params));
    
    // send the command
    if(msgsnd(qid, &cbuf, sizeof(cbuf.mtext), 0) == -1)
    {
        perror("[Create 2] msgsnd");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    // wait for response
    struct ReturnBuf rbuf;
    if(msgrcv(qid, &rbuf, sizeof(rbuf.retval), 10, 0) == -1)
    {
        perror("[Create 3] msgrcv");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    shmctl(shmid, IPC_RMID, NULL);
    return rbuf.retval;
}

int FS_Open(const char* path)
{
	struct CommandBuf cbuf = { .mtype = CommandType_Open };

    int shmid = GetNewSHM(strlen(path) + 1, 0666);
    if(shmid == -1)
    {
        perror("[Open 1] shmget");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    char* shm = shmat(shmid, NULL, 0);
    strcpy(shm, path);
    shmdt(shm);

    struct OpenParameters params = {
        .path_shmid = shmid
    };

    memcpy(cbuf.mtext, &params, sizeof(params));

    if(msgsnd(qid, &cbuf, sizeof(cbuf.mtext), 0) == -1)
    {
        perror("[Open 2] msgsnd");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    struct ReturnBuf rbuf;
    if(msgrcv(qid, &rbuf, sizeof(rbuf.retval), 10, 0) == -1)
    {
        perror("[Create 3] msgrcv");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    return rbuf.retval;
}

int FS_Close(int fd)
{
	
}

int FS_Read(int fd, char* buf, int size)
{
	
}

int FS_Write(int fd, char* buf, int size)
{
	
}

int FS_List(int fd, char* buf, int max_size)
{
	
}

static int GetNewSHM(int size, int permissions)
{
    sem_wait(&mtx);
    shm_key++;
    int shmid = shmget(shm_key, size, IPC_CREAT | IPC_EXCL | 0666);
    sem_post(&mtx);
    return shmid;
}

static int GetReturnValue(int shmid)
{
    struct ReturnBuf rbuf;
    if(msgrcv(qid, &rbuf, sizeof(rbuf.retval), 10, 0) == -1)
    {
        perror("[Create 3] msgrcv");
        if(shmid != -1)
            shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    return rbuf.retval;
}