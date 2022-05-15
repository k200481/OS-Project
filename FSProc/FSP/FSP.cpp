#include <FSP.h>
#include <iostream>
#include <sstream>

#include <File.h>
#include <thread>

#include <algorithm>
#include <iterator>

FSP::FSP()
	:
	bm(d, filename)
{
	auto root = FS::Directory::LoadRoot(bm);
	if (root.get() != nullptr) // create root dir if it does not exist
	{
        opened.push_back(FCB{
            .ptr = std::move(root)
        });
        return;
	}
    Init();
}

FSP::~FSP()
{}

void FSP::Run()
{
    OpenDir("/home/");
    OpenDir("/home/default");
}

void FSP::Init()
{
    auto root = FS::Directory::CreateRoot(bm, 0, 0x22);
	root->AddDirectory(bm, "home", 0, 0x66);
    auto home = root->OpenSubdir(bm, "home");
    home->AddDirectory(bm, "dafault", 0, 0x66);
    opened.push_back(FCB{
        .path_str = "/",
        .name = "/",
        .ptr = std::move(root)
    });
}

void FSP::List(const std::string& path, int uid, bool recurse)
{
    auto split_path = SplitPath(path);

    for(auto& e : split_path)
    {

    }
}

void FSP::List(const FS::DirPtr& dir, int uid, bool recurse)
{
    auto list = dir->List(bm);
    for(auto& e : list)
    {
        std::cout << "[" << (int)e.second.type << "]" << e.first << " ";
    }
    std::cout << std::endl;

    if(!recurse)
        return;
    
    std::cout << std::endl;

    for(auto& e : list)
    {
        if(e.second.type == FS::ElementType::Directory)
        {
            if(e.second.owner == uid)
            {
                auto child = dir->OpenSubdir(bm, e.first);
                std::cout << e.first << ":\n";
                List(child, uid, true);
            }
            else
            {
                std::cout << e.first << ":\n";
                std::cout << "Permission denied\n";
            }
        }
    }
}

int FSP::OpenFile(const std::string& path)
{
    const auto split_path = SplitPath(path);
    for(auto& e : split_path)
    {
        for(int i = 0; i < opened.size(); i++)
        {
            if(opened[i].path_str == path)
            {
                return i;
            }
        }
    }
}

int FSP::OpenDir(const std::string& path)
{
    const auto split_path = SplitPath(path);
    std::string new_path = "/";
    int cur_idx = 0;
    for(auto& e : split_path)
    {
        new_path += e + "/";
        int new_idx = GetIdx(new_path);
        if(new_idx == opened.size())
        {
            opened.push_back(FCB{
                .path_str = new_path,
                .name = e,
                .ptr = ((FS::Directory*)(opened[cur_idx].ptr.get()))->Open(bm, e)
            });
        }
        cur_idx = new_idx;
    }
}

int FSP::GetIdx(const std::string& path)
{
    auto res = find_if(opened.begin(), opened.end(), 
        [&path](const FCB& a)
        {
            return a.path_str == path;
        }
    );
    return std::distance(opened.begin(), res);
}

std::vector<std::string> FSP::SplitPath(const std::string& path)
{
    std::vector<std::string> split_path(1);
    int count = 0;
    for(int i = 0; i < path.length(); i++)
    {
        if(path[i] == '/')
        {
            if(i != 0 && i != path.length() - 1)
            {
                split_path.emplace_back();
                count++;
            }
            continue;
        }
        split_path[count] += path[i];
    }
    return split_path;
}
