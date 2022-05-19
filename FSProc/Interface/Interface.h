#pragma once

#include <Disk.h>
#include <BlockManager.h>
#include <Directory.h>
#include <semaphore.h>

namespace FS
{
    class Interface
    {
        struct MasterFCB
        {
            std::string path_str;
            FS::FSElementPtr ptr;
            int num_opened;
            //std::mutex mtx;

            MasterFCB(const std::string& path, FS::FSElementPtr&& ptr_in, int num_opened = 1)
                :
                path_str(path),
                ptr(std::move(ptr_in)),
                num_opened(num_opened)
            {}
            //MasterFCB(MasterFCB&& rhs)
            //{
            //    //sem_wait(&s);
            //    std::unique_lock<std::mutex> lock;
            //    path_str = std::move(rhs.path_str);
            //    ptr = std::move(rhs.ptr);
            //    num_opened = rhs.num_opened;
            //}
            //MasterFCB& operator=(MasterFCB&& rhs)
            //{
            //    if(&rhs == this)
            //        return *this;
            //    std::unique_lock<std::mutex> lock;
            //    path_str = std::move(rhs.path_str);
            //    ptr = std::move(rhs.ptr);
            //    num_opened = rhs.num_opened;
            //    return *this;
            //}
        };
    public:
        Interface(const std::string& disk_filename);
        Interface(const Interface&) = delete;
        Interface& operator=(const Interface&) = delete;
        ~Interface() = default;

        // general functions
        int Open(const std::string& path);
        int Open(const std::vector<std::string>& split_path);
        void Close(int idx);

        // directory functions
        bool Add(const std::string& path, 
            ElementType t, int owner, int perissions);
        bool Remove(const std::string& path);
        std::vector<std::string> List(int idx);

        // file functions
        int Read(int idx, char* data, int offset, int data_size);
        int Write(int idx, const char* data, int offset, int data_size);

        std::string GetLastError() const;
        std::string GetPathString(int idx) const;
        FS::ElementType GetType(int idx) const;
        
        int GetFreeSpace() const;
        int GetNumFreeBlocks() const;
    
    private:
        /* Thread safe accessors from opened file data */

        template<typename FSElementType> 
        FSElementType* GetPtr(int idx)
        {
            mtx.lock();
            auto ptr = (FSElementType*)opened[idx].ptr.get();
            mtx.unlock();
            return ptr;
        }
        void AddFSElement(const std::string& path, FSElementPtr&& ptr_in, int num_opened = 1);
        int GetNumFSElements() const;
        void IncrementNumOpened(int idx);
        void DecrementNumOpened(int idx);
        void ClearOpened();

        static std::vector<std::string> SplitPath(const std::string& path_str);
        int GetIdx(const std::string& path);

    private:
        const std::string filename;
        Disk d;
        BlockManager bm;
        std::vector<MasterFCB> opened;
        mutable std::mutex mtx;
        //std::mutex mtx;
        std::string last_error;
    };
}