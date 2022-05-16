#pragma once

#include <Interface.h>

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
	const std::string filename = "TestFile";
	FS::Interface inf;
};
