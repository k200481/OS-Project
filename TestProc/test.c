#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <FSLib.h>
#include <pthread.h>
#include <string.h>


void Create();
void Remove();
void Ls();
void Read();
void Write();

int main(void)
{
	FS_Init();

    while(1)
    {
        char com[255] = {};
        printf("Enter a command: ");
        scanf(" %s", com);
        if(strcmp(com, "ls") == 0)
        {
            Ls();
        }
        else if(strcmp(com, "create") == 0)
        {
            Create();
        }
        else if(strcmp(com, "write") == 0)
        {
            Write();
        }
        else if(strcmp(com, "read") == 0)
        {
            Read();
        }
        else if(strcmp(com, "rm") == 0)
        {
            Remove();
        }
        else if(strcmp(com, "exit") == 0)
        {
            break;
        }
    }

	FS_Exit();
	return 0;
}

void Create()
{
    char path[255], ch;
    scanf(" %s %c", path, &ch);
    FS_Create(path, ch, 06);
}

void Remove()
{
    char path[255];
    scanf(" %s", path);
    FS_Remove(path);
}

void Ls()
{
    char path[255], buf[256];
    scanf(" %s", path);
    int fd = FS_Open(path);
    if(fd == -1)
    {
        printf("Error in FS_Open\n");
        return;
    }
    FS_List(fd, buf, 255);
    FS_Close(fd);
    printf("%s\n", buf);
}

void Read()
{
    char path[255], buf[256];
    scanf(" %s", path);
    int fd = FS_Open(path);
    if(fd == -1)
    {
        printf("Error in FS_Open\n");
        return;
    }
    int bytes;
    scanf(" %d", &bytes);
    int num = FS_Read(fd, buf, bytes);
    FS_Close(fd);
    buf[num] = '\0';
    printf("%s\n", buf);
}

void Write()
{
    char path[255], buf[255];
    scanf(" %s %s", path, buf);
    int fd = FS_Open(path);
    if(fd == -1)
    {
        printf("Error in FS_Open\n");
        return;
    }
    FS_Write(fd, buf, strlen(buf));
    FS_Close(fd);
}

