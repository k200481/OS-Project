#include <FSP.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sstream>
#include <sys/shm.h>

#define FS_RETURN(val) ReturnValue((p_idx), (val)); return;

void* RegistrationRedirect(void* params);
void* ServiceRedirect(void* params);

FSP::FSP()
	:
	inf(filename)
{
    qid = msgget(FSIPC::regq_key, IPC_CREAT | FSIPC::regq_permissions);
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
}

void* FSP::Registration(void* params)
{
    FSIPC::RequestBuf rbuf = {};

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

    FSIPC::CommandBuf cbuf;
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
                case FSIPC::Type::Open:
                {
                    FSIPC::OpenParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    Open(p_idx, p);
                    break;
                }
                case FSIPC::Type::Close:
                {
                    FSIPC::CloseParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    Close(p_idx, p);
                    break;
                }
                case FSIPC::Type::Read:
                {
                    FSIPC::ReadParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    Read(p_idx, p);
                    break;
                }
                case FSIPC::Type::Write:
                {
                    FSIPC::WriteParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    Write(p_idx, p);
                    break;
                }
                case FSIPC::Type::Create:
                {
                    FSIPC::CreateParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    Create(p_idx, p);
                    break;
                }
                case FSIPC::Type::Remove:
                {
                    FSIPC::RemoveParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    Remove(p_idx, p);
                    break;
                }
                case FSIPC::Type::List:
                {
                    FSIPC::ListParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));
                    List(p_idx, p);
                    break;
                }
                case FSIPC::Type::ErrorInfo:
                {
                    FSIPC::ErrorInfoParameters p;
                    memcpy(&p, cbuf.params, sizeof(p));

                    break;
                }
                case FSIPC::Type::Exit:
                {
                    return NULL;
                    break;
                }
            }
        }
    }
    return NULL;
}

void FSP::Open(int p_idx, FSIPC::OpenParameters p)
{
    std::ostringstream log_stream;
    log_stream << "[" << p_idx << "] Open ";

    const char* path = (char*)shmat(p.path_shmid, NULL, 0);
    if(path == (char*)-1)
    {
        FS_RETURN(-1);
    }

    log_stream << "[Path] " << path << " ";

    const int fd = inf.Open(path);
    if(fd == -1)
    {
        shmdt(path);
        FS_RETURN(-1);
    }
    shmdt(path);

    const int f_idx = processes[p_idx].opened.size();
    processes[p_idx].opened.push_back(fd);

    log_stream << "[FD] " << fd << " [F_IDX] " << f_idx << std::endl;
    std::cout << log_stream.str();

    FS_RETURN(f_idx);
}

void FSP::Close(int p_idx, FSIPC::CloseParameters p)
{
    std::ostringstream log_stream;
    log_stream << "[" << p_idx << "] Close ";

    if(p.f_idx < 0 || p.f_idx >= processes[p_idx].opened.size())
    {
        FS_RETURN(-1);
    }

    log_stream << "[F_IDX] " << p.f_idx << std::endl;
    std::cout << log_stream.str();

    inf.Close(processes[p_idx].opened[p.f_idx]);
    FS_RETURN(0);
}

void FSP::Read(int p_idx, FSIPC::ReadParameters p)
{
    std::ostringstream log_stream;
    log_stream << "[" << p_idx << "] Read ";

    if(p.f_idx < 0 || p.f_idx >= processes[p_idx].opened.size())
    {
        FS_RETURN(-1);
    }

    char* buf = (char*)shmat(p.buf_shmid, NULL, 0);
    if(buf == (char*)-1)
    {
        FS_RETURN(-1);
    }

    const int num = inf.Read(processes[p_idx].opened[p.f_idx], buf, 0, p.size);
    
    log_stream << "[F_IDX] " << p.f_idx << " [Num] " << num << std::endl;
    std::cout << log_stream.str();

    shmdt(buf);
    FS_RETURN(num);
}

void FSP::Write(int p_idx, FSIPC::WriteParameters p)
{
    std::ostringstream log_stream;
    log_stream << "[" << p_idx << "] Write [F_IDX] " << p.f_idx << " [size] " << p.size << " ";

    if(p.f_idx < 0 || p.f_idx >= processes[p_idx].opened.size())
    {
        FS_RETURN(-1);
    }

    const char* buf = (char*)shmat(p.buf_shmid, NULL, 0);
    if(buf == (char*)-1)
    {
        FS_RETURN(-1);
    }

    const int num = inf.Write(processes[p_idx].opened[p.f_idx], buf, 0, p.size);

    log_stream << "[Num] " << num << std::endl;
    std::cout << log_stream.str();

    shmdt(buf);
    FS_RETURN(num);
}

void FSP::Create(int p_idx, FSIPC::CreateParameters p)
{
    std::ostringstream log_stream;
    log_stream << "[" << p_idx << "] Create [Type] " << p.etype << " ";

    const char* path = (char*)shmat(p.path_shmid, NULL, 0);
    if(path == (char*)-1)
    {
        FS_RETURN(-1);
    }

    log_stream << "[Path] " << path << std::endl;
    std::cout << log_stream.str();

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
        shmdt(path);
        FS_RETURN(-1);
    }

    int res = (int)inf.Add(path, e, processes[p_idx].uid, p.permissions);
    shmdt(path);
    FS_RETURN(res);
}

void FSP::Remove(int p_idx, FSIPC::RemoveParameters p)
{
    const char* path = (char*)shmat(p.path_shmid, NULL, 0);
    if(path == (char*)-1)
    {
        FS_RETURN(-1);
    }
    bool res = inf.Remove(path);
    shmdt(path);
    FS_RETURN(res);
}

void FSP::List(int p_idx, FSIPC::ListParameters p)
{
    std::ostringstream log_stream;
    log_stream << "[" << p_idx << "] List [F_IDX] " << p.f_idx << " ";

    if(p.f_idx < 0 || p.f_idx >= processes[p_idx].opened.size())
    {
        FS_RETURN(-1);
    }
    
    char* shm = (char*)shmat(p.listing_shmid, NULL, 0);
    if(shm == (char*)-1)
    {
        FS_RETURN(-1);
    }

    const auto list = inf.List(processes[p_idx].opened[p.f_idx]);
    std::ostringstream list_stream;
    const std::string path = inf.GetPathString(processes[p_idx].opened[p.f_idx]);
    for(size_t i = 0; i < list.size(); i++)
    {
        std::string child_path = path;
        if(child_path.back() != '/')
            child_path += "/";
        child_path += list[i];
        int fd = inf.Open(child_path);
        list_stream << "[" << (inf.GetType(fd) == FS::ElementType::Directory ? 'D' : 'F')  << "]"<< list[i] << " ";
        inf.Close(fd);
    }

    const std::string& res = list_stream.str();
    strncpy(shm, res.c_str(), p.size);
    shmdt(shm);
    int retval = std::min(int(res.size()), p.size);

    log_stream << "[Num] " << retval << std::endl;
    std::cout << log_stream.str();

    FS_RETURN(retval + 1);
}

void FSP::ErrorInfo(int p_idx, FSIPC::ErrorInfoParameters p)
{
    
}

void FSP::ReturnValue(int p_idx, int val)
{
    FSIPC::ReturnBuf rbuf = { .mtype = 10, .retval = val };
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