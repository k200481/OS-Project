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

	qid = msgget(getpid(), IPC_CREAT | 0666);
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

int FS_Create(const char* path, char type, int permissions)
{
	struct CommandBuf cbuf = { .mtype = CommandType_Create };

    const int shmid = GetNewSHM(strlen(path) + 1, 0666);
    if(shmid == -1)
    {
        perror("[Create 1] shmget");
        exit(EXIT_FAILURE);
    }

    char* shm = shmat(shmid, NULL, 0);
    strncpy(shm, path, 255);
    shmdt(shm);

    struct CreateParameters params = { 
        .etype = type, 
        .path_shmid = shmid, 
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

int FS_Remove(const char* path)
{
    struct CommandBuf cbuf = { .mtype = CommandType_Remove };
    int shmid = GetNewSHM(strlen(path) + 1, 0666);
    if(shmid == -1)
    {
        perror("[Remove 1] smget");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    char* shm = (char*)shmat(shmid, NULL, 0);
    strcpy(shm, path);
    shmdt(shm);

    struct RemoveParameters params = {
        .path_shmid = shmid
    };

    memcpy(&cbuf.mtext, &params, sizeof(params));

    if(msgsnd(qid, &cbuf, sizeof(cbuf.mtext), 0) == -1)
    {
        perror("[Remove 2] msgsnd");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    struct ReturnBuf rbuf;
    if(msgrcv(qid, &rbuf, sizeof(rbuf.retval), 10, 0) == -1)
    {
        perror("[Remove 3] msgrcv");
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
    shmctl(shmid, IPC_RMID, NULL);
    return rbuf.retval;
}

int FS_Close(int fd)
{
	struct CommandBuf cbuf = { .mtype = CommandType_Close };
    struct CloseParameters params = { .f_idx = fd };
    memcpy(cbuf.mtext, &params, sizeof(params));
    if(msgsnd(qid, &cbuf, sizeof(cbuf.mtext), 0) == -1)
    {
        perror("[Close 1] msgsnd");
        exit(EXIT_FAILURE);
    }

    struct ReturnBuf rbuf;
    if(msgrcv(qid, &rbuf, sizeof(rbuf.retval), 10, 0) == -1)
    {
        perror("[Close 2] msgrcv");
        exit(EXIT_FAILURE);
    }
    return rbuf.retval;
}

int FS_Read(int fd, char* buf, int size)
{
	struct CommandBuf cbuf = { .mtype = CommandType_Read };
    
    const int shmid = GetNewSHM(size, 0666);
    if(shmid == -1)
    {
        perror("[Read 1] shmget");
        exit(EXIT_FAILURE);
    }

    struct ReadParameters params = { 
        .f_idx = fd,
        .buf_shmid = shmid,
        .size = size
    };

    memcpy(cbuf.mtext, &params, sizeof(params));

    if(msgsnd(qid, &cbuf, sizeof(cbuf.mtext), 0) == -1)
    {
        perror("[Read 2] msgsnd");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    struct ReturnBuf rbuf;
    if(msgrcv(qid, &rbuf, sizeof(rbuf.retval), 10, 0) == -1)
    {
        perror("[Read 3] msgrcv");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    if(rbuf.retval > 0)
    {
        char* shm = shmat(shmid, NULL, 0);
        memcpy(buf, shm, rbuf.retval);
        shmdt(shm);
    }

    shmctl(shmid, IPC_RMID, NULL);
    return rbuf.retval;
}

int FS_Write(int fd, char* buf, int size)
{
	struct CommandBuf cbuf = { .mtype = CommandType_Write };
    
    const int shmid = GetNewSHM(size, 0666);
    if(shmid == -1)
    {
        perror("[Read 1] shmget");
        exit(EXIT_FAILURE);
    }

    char* shm = shmat(shmid, NULL, 0);
    memcpy(shm, buf, size);
    shmdt(shm);

    struct WriteParameters params = { 
        .f_idx = fd,
        .buf_shmid = shmid,
        .size = size
    };

    memcpy(cbuf.mtext, &params, sizeof(params));

    if(msgsnd(qid, &cbuf, sizeof(cbuf.mtext), 0) == -1)
    {
        perror("[Read 2] msgsnd");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    struct ReturnBuf rbuf;
    if(msgrcv(qid, &rbuf, sizeof(rbuf.retval), 10, 0) == -1)
    {
        perror("[Read 3] msgrcv");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    shmctl(shmid, IPC_RMID, NULL);
    return rbuf.retval;
}

int FS_List(int fd, char* buf, int max_size)
{
	struct CommandBuf cbuf = { .mtype = CommandType_List };
    
    const int shmid = GetNewSHM(max_size, 0666);
    if(shmid == -1)
    {
        perror("[Read 1] shmget");
        exit(EXIT_FAILURE);
    }

    struct ListParameters params = { 
        .f_idx = fd,
        .listing_shmid = shmid,
        .size = max_size
    };

    memcpy(cbuf.mtext, &params, sizeof(params));

    if(msgsnd(qid, &cbuf, sizeof(cbuf.mtext), 0) == -1)
    {
        perror("[Read 2] msgsnd");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    struct ReturnBuf rbuf;
    if(msgrcv(qid, &rbuf, sizeof(rbuf.retval), 10, 0) == -1)
    {
        perror("[Read 3] msgrcv");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    if(rbuf.retval > 0)
    {
        char* shm = shmat(shmid, NULL, 0);
        memcpy(buf, shm, rbuf.retval);
        shmdt(shm);
    }

    shmctl(shmid, IPC_RMID, NULL);
    return rbuf.retval;
}

int FS_GetErrorMsg(char* buf, int max_size)
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