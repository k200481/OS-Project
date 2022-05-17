#ifndef FSLIB_H
#define FSLIB_H

void FS_Init();
void FS_Exit();

int FS_Create(int fd, char type, const char* name, int permissions);
int FS_Open(const char* path);
int FS_Close(int fd);

int FS_Read(int fd, char* buf, int size);
int FS_Write(int fd, char* buf, int size);
int FS_List(int fd, char* buf, int max_size);

#endif
