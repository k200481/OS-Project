#pragma once

#include <Interface.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <FS_IPC.h>

class FSP
{
    struct ServiceThreadParams
    {
        FSP* _this;
        size_t proc_idx;
    };

    struct Process
    {
        int pid;
        int uid;
        key_t qid;
        int shmid;
        pthread_t t;
        std::vector<int> opened;
    };


    class ProcFileManager
    {
        struct FCB
        {
            int idx;
            int offset;
        };
    public:
        int Open(const std::string& path);
        void Close(int idx);
        int Read(int idx, char* buf, int size);
        int Write(int idx, char* buf, int size);
        int Add(int idx, const std::string& name);
        void Remove(int idx, const std::string& name);
        std::string List(int idx);
    private:
        std::vector<FCB> opened;
    };

public:
	FSP();
	~FSP();
	FSP(const FSP&) = delete;
	FSP& operator=(const FSP&) = delete;

	void Run();

private:
    friend void* RegistrationRedirect(void* params);
    friend void* ServiceRedirect(void* params);

    void* Registration(void* params);
    void* Service(void* params);

    void Open(int p_idx, FS_IPC::OpenParameters p);
    void Close(int p_idx, FS_IPC::CloseParameters p);
    void Read(int p_idx, FS_IPC::ReadParameters p);
    void Write(int p_idx, FS_IPC::WriteParameters p);
    void Create(int p_idx, FS_IPC::CreateParameters p);
    void Remove(int p_idx, FS_IPC::RemoveParameters p);
    
    void ReturnValue(int p_idx, int val);

private:
	const std::string filename = "TestFile";
	FS::Interface inf;
    int qid;
    sem_t mtx;
    sem_t copy_sem;
    bool running = true;
    std::vector<Process> processes;
    pthread_t registration_thread;
};
