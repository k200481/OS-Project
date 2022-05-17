#include <FSP.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sstream>
#include <sys/shm.h>

#define FS_RETURN(val) ReturnValue(p_idx, val); return;

void* RegistrationRedirect(void* params);
void* ServiceRedirect(void* params);

FSP::FSP()
	:
	inf(filename)
{
    qid = msgget(FS_IPC::regq_key, IPC_CREAT | FS_IPC::regq_permissions);
    if(qid == -1)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    sem_init(&mtx, 0, 1);
    sem_init(&copy_sem, 0, 0);

    if(pthread_create(&registration_thread, NULL, RegistrationRedirect, this) == -1)
    {
        perror("pthread_create");
    }
}

FSP::~FSP()
{
    sem_wait(&mtx);
    running = false;
    sem_post(&mtx);

    sem_destroy(&copy_sem);
    sem_destroy(&mtx);

    if(pthread_join(registration_thread, NULL))
    {
        perror("pthread_join");
    }
    msgctl(qid, IPC_RMID, NULL);
}

void FSP::Run()
{
    getchar();
    int fd = inf.Open("/A/");

    if(fd != -1)
    {
        const auto& list = inf.List(fd);
        for(int i = 0; i < list.size(); i++)
        {
            std::cout << list[i].first << std::endl;
        }
    }
}

void* FSP::Registration(void* params)
{
    FS_IPC::RequestBuf rbuf = {};

    while(true)
    {
        sem_wait(&mtx);
        if(!running)
        {
            sem_post(&mtx);
            break;
        }
        sem_post(&mtx);

        if(msgrcv(qid, &rbuf, sizeof(rbuf) - sizeof(rbuf.mtype), 1, IPC_NOWAIT) == -1)
        {
            if(errno != ENOMSG)
            {
                std::ostringstream oss;
                oss << "[Listener] msgrcv";
                perror(oss.str().c_str());
                return NULL;
            }
        }
        else
        {
            ServiceThreadParams p = {
                ._this = this, .proc_idx = processes.size()
            };

            processes.push_back(Process{ 
                .pid = rbuf.pid,
                .uid = rbuf.uid,
                .qid = rbuf.qid,
                .opened = { 0 },
            });

            pthread_create(&processes.back().t, NULL, ServiceRedirect, &p);
            sem_wait(&copy_sem);
            //sleep(1);
        }
    }
    return NULL;
}

void* FSP::Service(void* params)
{
    const int p_idx = ((ServiceThreadParams*)params)->proc_idx;
    sem_post(&copy_sem);

    FS_IPC::CommandBuf cbuf;
    bool end = false;
    while(!end)
    {
        sem_wait(&mtx);
        end = !running;
        sem_post(&mtx);

        if(msgrcv(processes[p_idx].qid, &cbuf, sizeof(cbuf.params), -9, IPC_NOWAIT) == -1)
        {
            if(errno != ENOMSG)
            {
                std::ostringstream oss;
                oss << "[" << p_idx << "] msgrcv";
                perror(oss.str().c_str());
                return NULL;
            }
        }
        else
        {
            switch (cbuf.mtype)
            {
                case FS_IPC::Type::Open:
                {
                    FS_IPC::OpenParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    Open(p_idx, p);
                    break;
                }
                case FS_IPC::Type::Close:
                {
                    FS_IPC::CloseParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    Close(p_idx, p);
                    break;
                }
                case FS_IPC::Type::Read:
                {
                    FS_IPC::ReadParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    Read(p_idx, p);
                    break;
                }
                case FS_IPC::Type::Write:
                {
                    FS_IPC::WriteParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    Write(p_idx, p);
                    break;
                }
                case FS_IPC::Type::Create:
                {
                    FS_IPC::CreateParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    Create(p_idx, p);
                    break;
                }
                case FS_IPC::Type::Remove:
                {
                    FS_IPC::RemoveParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    Remove(p_idx, p);
                    break;
                }
                case FS_IPC::Type::Exit:
                {
                    return NULL;
                    break;
                }
            }
        }
    }
    return NULL;
}

void FSP::Open(int p_idx, FS_IPC::OpenParameters p)
{
    const char* path = (char*)shmat(p.path_shmid, NULL, 0);
    if(path == (char*)-1)
    {
        FS_RETURN(-1);
    }
    int fd = inf.Open(path);
    shmdt(path);
    if(fd == -1)
    {
        FS_RETURN(-1);
    }

    int f_idx = processes[p_idx].opened.size();
    processes[p_idx].opened.push_back(fd);
    FS_RETURN(f_idx);
}

void FSP::Close(int p_idx, FS_IPC::CloseParameters p)
{
    if(p.f_idx < 0 || p.f_idx >= processes[p_idx].opened.size())
    {
        FS_RETURN(-1);
    }

    inf.Close(processes[p_idx].opened[p.f_idx]);
    FS_RETURN(0);
}

void FSP::Read(int p_idx, FS_IPC::ReadParameters p)
{
    if(p.f_idx < 0 || p.f_idx >= processes[p_idx].opened.size())
    {
        FS_RETURN(-1);
    }

    char* buf = (char*)shmat(p.buf_shmid, NULL, 0);
    if(buf == (char*)-1)
    {
        FS_RETURN(-1);
    }

    int num = inf.Read(processes[p_idx].opened[p.f_idx], buf, 0, p.size);
    shmdt(buf);
    FS_RETURN(num);
}

void FSP::Write(int p_idx, FS_IPC::WriteParameters p)
{
    if(p.f_idx < 0 || p.f_idx >= processes[p_idx].opened.size())
    {
        FS_RETURN(-1);
    }

    const char* buf = (char*)shmat(p.buf_shmid, NULL, 0);
    if(buf == (char*)-1)
    {
        FS_RETURN(-1);
    }

    int num = inf.Write(processes[p_idx].opened[p.f_idx], buf, 0, sizeof(buf));
    shmdt(buf);
    FS_RETURN(num);
}

void FSP::Create(int p_idx, FS_IPC::CreateParameters p)
{
    if(p.f_idx < 0 || p.f_idx >= processes[p_idx].opened.size())
    {
        FS_RETURN(-1);
    }

    const char* name = (char*)shmat(p.name_shmid, NULL, 0);
    if(name == (char*)-1)
    {
        FS_RETURN(-1);
    }

    FS::ElementType e;
    if(p.etype == 'F')
    {
        e = FS::ElementType::File;
    }
    else if(p.etype == 'D')
    {
        e = FS::ElementType::Directory;
    }
    else
    {
        FS_RETURN(-1);
    }

    int res = (int)inf.Add(processes[p_idx].opened[p.f_idx], name, e, processes[p_idx].uid, p.permissions);
    FS_RETURN(res);
}

void FSP::Remove(int p_idx, FS_IPC::RemoveParameters p)
{
    if(p.f_idx < 0 || p.f_idx >= processes[p_idx].opened.size())
    {
        FS_RETURN(-1);
    }

    const char* name = (char*)shmat(p.name_shmid, NULL, 0);
    if(name == (char*)-1)
    {
        FS_RETURN(-1);
    }

    inf.Remove(processes[p_idx].opened[p.f_idx], name);
    FS_RETURN(0);
}

void FSP::ReturnValue(int p_idx, int val)
{
    FS_IPC::ReturnBuf rbuf = { .mtype = 10, .retval = val };
    if(msgsnd(processes[p_idx].qid, &rbuf, sizeof(rbuf.retval), 0) == -1)
    {
        perror("msgsnd");
    }
}

void* RegistrationRedirect(void* params)
{
    FSP* _this = (FSP*)params;
    return _this->Registration(params);
}

void* ServiceRedirect(void* params)
{
    FSP* _this = ((FSP::ServiceThreadParams*)params)->_this;
    return _this->Service(params);
}