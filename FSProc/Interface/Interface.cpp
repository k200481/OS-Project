#include "Interface.h"
using namespace FS;

#include <algorithm>
#include <iostream>
#include <sstream>

Interface::Interface(const std::string& disk_filename)
    :
    filename(disk_filename),
    bm(d, filename)
{
    auto root = FS::Directory::LoadRoot(bm);
	if (root.get() != nullptr) // create root dir if it does not exist
	{
        opened.push_back(MasterFCB{
            .ptr = std::move(root),
            .num_opened = 1
        });
        return;
	}
    Init();
}

void Interface::Init()
{
    auto root = FS::Directory::CreateRoot(bm, 0, 0x22);
	root->AddDirectory(bm, "home", 0, 0x66);
    auto home = root->OpenSubdir(bm, "home");
    home->AddDirectory(bm, "default", 0, 0x66);
    opened.push_back(MasterFCB{
        .path_str = "/",
        .ptr = std::move(root),
        .num_opened = 1
    });
}

int Interface::Open(const std::string& path)
{
    // split path into its individual directories
    const auto split_path = SplitPath(path);
    // regenerate path starting at root
    std::string new_path = "/";
    // the current index in the opened list that is opened
    int cur_idx = 0;
    bool error = false;
    // loop over the elements in the path
    for(auto& e : split_path)
    {
        // check if currently opened object is a directory
        // in case a user provides an invalid path and treats a file as a dir
        if(opened[cur_idx].ptr->GetType() != FS::ElementType::Directory)
        {
            std::ostringstream oss;
            oss << new_path << " is not a directory";
            last_error = oss.str();
            error = true;
            break;
        }
        // add the new element into the generated path
        new_path += e + "/";
        // check if file/dir has already been opened
        int new_idx = GetIdx(new_path);
        // open the file/dir if not already opened
        if(new_idx == opened.size())
        {
            auto ptr = ((FS::Directory*)(opened[cur_idx].ptr.get()))->Open(bm, e);
            if(ptr.get() == nullptr)
            {
                std::ostringstream oss;
                oss << new_path << " does not exist";
                last_error = oss.str();
                error = true;
                break;
            }
            opened.push_back(MasterFCB{
                .path_str = new_path,
                //.name = e,
                .ptr = std::move(ptr),
                .num_opened = 1
            });
        }
        else // update num opened if already opened
            opened[new_idx].num_opened++;
        cur_idx = new_idx;
    }

    opened[cur_idx].ptr->GetMetadata();

    if(!error)
        return cur_idx;
    Close(cur_idx);
    return -1;
}

void Interface::Close(int idx)
{
    if(idx == 0)
        return;
    
    auto split_path = SplitPath(opened[idx].path_str);
    std::string new_path = "/";
    for(auto& e : split_path)
    {
        new_path += e + "/";
        int i = GetIdx(new_path);
        opened[i].num_opened--;
    }
    
    // remove any file or directory that is not currently opened by anyone
    opened.erase(
        std::remove_if(opened.begin(), opened.end(), 
            [](const MasterFCB& e){ return e.num_opened == 0; }
        ), 
        opened.end()
    );
}

void Interface::Add(int idx, const std::string& name, 
    ElementType t, int owner, int perissions)
{
    if(opened[idx].ptr->GetType() != ElementType::Directory)
    {
        std::ostringstream oss;
        oss << opened[idx].path_str << " is not a directory";
        last_error = oss.str();
        return;
    }
    
    auto dir_ptr = ((Directory*)(opened[idx].ptr.get()));
    dir_ptr->Add(bm, name.c_str(), t, owner, perissions);
}

void Interface::Remove(int idx, const std::string& name)
{
    // yet to be implemented on a lower level
}

std::vector<data_pair> Interface::List(int idx)
{
    if(opened[idx].ptr->GetType() != ElementType::Directory)
    {
        std::ostringstream oss;
        oss << opened[idx].path_str << " is not a directory";
        last_error = oss.str();
        return std::vector<data_pair>();
    }

    auto dir_ptr = (Directory*)(opened[idx].ptr.get());
    return dir_ptr->List(bm);
}

int Interface::Read(int idx, char* data, int offset, int data_size)
{
    if(opened[idx].ptr->GetType() != ElementType::File)
    {
        std::ostringstream oss;
        oss << opened[idx].path_str << " is not a file\n";
        last_error = oss.str();
        return -1;
    }
    
    auto file_ptr = (File*)(opened[idx].ptr.get());
    return file_ptr->Read(bm, data, offset, data_size);
}

int Interface::Write(int idx, const char* data, int offset, int data_size)
{
    if(opened[idx].ptr->GetType() != ElementType::File)
    {
        std::ostringstream oss;
        oss << opened[idx].path_str << " is not a file\n";
        last_error = oss.str();
        return -1;
    }
    
    auto file_ptr = (File*)(opened[idx].ptr.get());
    return file_ptr->Write(bm, data, offset, data_size);
}

int Interface::GetIdx(std::string& path)
{
    auto it = std::find_if(
        opened.begin(), opened.end(), [&path](const MasterFCB& e){ return e.path_str == path; }
    );

    return std::distance(opened.begin(), it);
}

std::vector<std::string> Interface::SplitPath(const std::string& path_str)
{
    std::vector<std::string> split_path(1);
    int count = 0;
    for(int i = 0; i < path_str.length(); i++)
    {
        if(path_str[i] == '/')
        {
            if(i != 0 && i != path_str.length() - 1)
            {
                split_path.emplace_back();
                count++;
            }
            continue;
        }
        split_path[count] += path_str[i];
    }
    return split_path;
}
