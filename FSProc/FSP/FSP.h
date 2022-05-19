#pragma once

#include <Interface.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <FSIPC_Structures.h>

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
        std::string last_error;
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

    void Open(int p_idx, FSIPC::OpenParameters p);
    void Close(int p_idx, FSIPC::CloseParameters p);
    void Read(int p_idx, FSIPC::ReadParameters p);
    void Write(int p_idx, FSIPC::WriteParameters p);
    void Create(int p_idx, FSIPC::CreateParameters p);
    void Remove(int p_idx, FSIPC::RemoveParameters p);
    void List(int p_idx, FSIPC::ListParameters p);
    void ErrorInfo(int p_idx, FSIPC::ErrorInfoParameters p);
    
    void ReturnValue(int p_idx, int val);

private:
	const std::string filename = "TestFile";
	FS::Interface inf;
    int qid;
    sem_t mtx;
    sem_t oc_mtx;
    sem_t copy_sem;
    bool running = true;
    std::vector<Process> processes;
    pthread_t registration_thread;
};
