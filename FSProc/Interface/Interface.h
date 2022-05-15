#pragma once

#include <Disk.h>
#include <BlockManager.h>
#include <Inode.h>
#include <Directory.h>
#include <unordered_map>

namespace FS
{
    class Interface
    {
        struct MasterFCB
        {
            std::string path_str;
            //std::string name;
            FS::FSElementPtr ptr;
            int num_opened;
        };
    public:
        Interface(const std::string& disk_filename);
        Interface(const Interface&) = delete;
        Interface& operator=(const Interface&) = delete;
        ~Interface() = default;

        // general functions
        int Open(const std::string& path);
        void Close(int idx);

        // directory functions
        void Add(int idx, const std::string& name, 
            ElementType t, int owner, int perissions);
        void Remove(int idx, const std::string& name);
        std::vector<data_pair> List(int idx);

        // file functions
        int Read(int idx, char* data, int offset, int data_size);
        int Write(int idx, const char* data, int offset, int data_size);
    
    private:
        void Init();
        static std::vector<std::string> SplitPath(const std::string& path_str);
        int GetIdx(std::string& path);

    private:
        const std::string filename;
        Disk d;
        BlockManager bm;
        std::vector<MasterFCB> opened;
        std::unordered_map<std::string, MasterFCB> list;
        std::mutex openclose_mtx;
        std::string last_error;
    };
}