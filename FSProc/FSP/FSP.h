#pragma once

#include <Disk.h>
#include <BlockManager.h>
#include <Inode.h>
#include <Directory.h>

class FSP
{
    class Path : public std::vector<int>
    {
    public:
        using std::vector<int>::vector;
        
        bool operator==(const Path& rhs)
        {
            for(size_t i = 0; i < size(); i++)
            {
                if(at(i) != rhs.at(i))
                    return false;
            }
            return true;
        }
    };
    
    struct FCB
    {
        std::string path_str;
        std::string name;
        Path full_path;
        FS::FSElementPtr ptr;
        int num_opened;
    };
public:
	FSP();
	~FSP();
	FSP(const FSP&) = delete;
	FSP& operator=(const FSP&) = delete;

	void Run();

private:
	void Init();
    void MakeDirectory(const std::string& path, const std::string& name);
    void MakeFile(const std::string& path, const std::string& name);

    void List(const std::string& path, int uid, bool recurse = false);
    void List(const FS::DirPtr& dir, int uid, bool recurse = false);

    int OpenFile(const std::string& path);
    int OpenDir(const std::string& path);
    void CloseFile(int idx);
    void closeDirectory(int idx);

    int GetIdx(const std::string& path);
    static std::vector<std::string> SplitPath(const std::string& path);

private:
	const std::string filename = "TestFile";
	Disk d;
	BlockManager bm;
	std::vector<FCB> opened;
};
