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
	if (root.get() == nullptr) // create root dir if it does not exist
	{
        auto root = FS::Directory::CreateRoot(bm, 0, 0x6);
        return;
	}
}

int Interface::Open(const std::string& path)
{
    // split path into its individual directories
    const auto split_path = SplitPath(path);
    // check path validity
    if(split_path.size() == 0 || !split_path[0].empty() || // empty path or doesnt start from root
        std::any_of(std::next(split_path.begin()), split_path.end(), 
        [](const std::string& str){ return str.empty(); }) // double '/' in path
    )
    {
        std::ostringstream oss;
        oss << path << ": invalid path";
        last_error = oss.str();

        return -1;
    }
    
    if(GetNumFSElements() == 0)
    {
        AddFSElement("/", Directory::LoadRoot(bm));
    }

    int idx = Open(split_path);
    if(idx != -1)
    {
        if(path.back() == '/' && GetType(idx) != ElementType::Directory)
        {
            std::ostringstream oss;
            oss << path << ": not a directory";
            Close(idx);
            return -1;
        }
    }

    return idx;
}

int Interface::Open(const std::vector<std::string>& split_path)
{
    // regenerate path starting at root (used to compare with entries in the opened list)
    std::string new_path;
    // the current index in the opened list that is opened
    int cur_idx = 0;
    // loop over the elements in the path
    for(size_t i = 1; i < split_path.size(); i++)
    {
        // check if currently opened object is a directory in case a user treats a file as a dir
        if(GetType(cur_idx) != FS::ElementType::Directory)
        {
            std::ostringstream oss;
            oss << new_path << ": not a directory";
            last_error = oss.str();
            Close(cur_idx);
            return -1;
        }
        // add the new element into the generated path
        new_path += "/" + split_path[i];
        // check if file/dir has already been opened
        int new_idx = GetIdx(new_path);
        // open the file/dir if not already opened
        if(new_idx == GetNumFSElements())
        {
            auto ptr = GetPtr<Directory>(cur_idx)->Open(bm, split_path[i]);
            if(ptr.get() == nullptr)
            {
                std::ostringstream oss;
                oss << new_path << ": no such file or directroy";
                last_error = oss.str();
                Close(cur_idx);
                return -1;
            }
            AddFSElement(new_path, std::move(ptr));
        }
        else // update num opened if already opened
        {
            IncrementNumOpened(new_idx);
        }
        cur_idx = new_idx;
    }

    return cur_idx;
}

void Interface::Close(int idx)
{
    if(idx == 0)
        return;
    
    auto split_path = SplitPath(GetPathString(idx));
    std::string new_path = "/";
    for(auto& e : split_path)
    {
        new_path += e + "/";
        int i = GetIdx(new_path);
        DecrementNumOpened(idx);
    }
    
    // remove any file or directory that is not currently opened by anyone
    ClearOpened();
}

bool Interface::Add(const std::string& path, 
    ElementType t, int owner, int perissions)
{
    auto i = path.rfind('/');
    if(i == path.size())
    {
        std::ostringstream oss;
        oss << path << ": no file or directory name entered";
        last_error = oss.str();
        return false;
    }
    else if(i == std::string::npos)
    {
        std::ostringstream oss;
        oss << path << ": invalid path";
        return false;
    }

    int idx = Open(path.substr(0, i + 1));
    if(idx == -1)
    {
        return false;
    }

    if(GetType(idx) != ElementType::Directory)
    {
        std::ostringstream oss;
        oss << GetPathString(idx) << ": not a directory";
        last_error = oss.str();
        Close(idx);
        return false;
    }
    
    auto dir_ptr = (GetPtr<Directory>(idx));
    std::string filename = path.substr(i + 1);
    if(dir_ptr->EntryExists(bm, filename))
    {
        std::ostringstream oss;
        oss << path << ": already exists";
        last_error = oss.str();
        Close(idx);
        return false;
    }
    bool res = dir_ptr->Add(bm, filename.c_str(), t, owner, perissions);
    Close(idx);
    return res;
}

bool Interface::Remove(const std::string& path)
{
    if(path == "/")
    {
        std::ostringstream oss;
        oss << "attempting to delete root";
        last_error = oss.str();
        return false;
    }

    auto i = path.rfind('/');
    if(i == path.size())
    {
        std::ostringstream oss;
        oss << path << ": no file or directory name entered";
        last_error = oss.str();
        return false;
    }
    else if(i == std::string::npos)
    {
        std::ostringstream oss;
        oss << path << ": invalid path";
        return false;
    }

    int target_idx = Open(path);
    if(target_idx == -1)
    {
        return false;
    }

    if(GetType(target_idx) == ElementType::Directory)
    {
        std::vector<std::string> children = List(target_idx);
        for(int i = 0; i < children.size(); i++)
        {
            std::string child_path = path;
            if(child_path.back() != '/')
                child_path += '/';
            child_path += children[i];
            Remove(child_path);
        }
    }
    
    GetPtr<FSElement>(target_idx)->FreeDatablocks(bm);
    GetPtr<FSElement>(target_idx)->FreeInodeBlock(bm);
    Close(target_idx);

    const std::string parent_path = path.substr(0, i + 1);
    int parent_idx = Open(parent_path);
    GetPtr<Directory>(parent_idx)->Remove(bm, path.substr(i + 1));
    Close(parent_idx);
    return true;
}

std::vector<std::string> Interface::List(int idx)
{
    if(GetType(idx) != ElementType::Directory)
    {
        std::ostringstream oss;
        oss << GetPathString(idx) << ": not a directory";
        last_error = oss.str();
        return std::vector<std::string>();
    }

    auto dir_ptr = GetPtr<Directory>(idx);
    return dir_ptr->List(bm);
}

int Interface::Read(int idx, char* data, int offset, int data_size)
{
    if(GetType(idx) != ElementType::File)
    {
        std::ostringstream oss;
        oss << GetPathString(idx) << ": not a file\n";
        last_error = oss.str();
        return -1;
    }
    
    auto file_ptr = GetPtr<File>(idx);
    return file_ptr->Read(bm, data, offset, data_size);
}

int Interface::Write(int idx, const char* data, int offset, int data_size)
{
    if(GetType(idx) != ElementType::File)
    {
        std::ostringstream oss;
        oss << GetPathString(idx) << ": not a file\n";
        last_error = oss.str();
        return -1;
    }
    
    auto file_ptr = GetPtr<File>(idx);
    return file_ptr->Write(bm, data, offset, data_size);
}

std::string Interface::GetPathString(int idx) const
{
    mtx.lock();
    auto path_str = opened[idx].path_str;
    mtx.unlock();
    return path_str;
}

FS::ElementType Interface::GetType(int idx) const
{
    mtx.lock();
    auto e = opened[idx].ptr->GetType();
    mtx.unlock();
    return e;
}

void Interface::AddFSElement(const std::string& path, FSElementPtr&& ptr_in, int num_opened)
{
    mtx.lock();
    opened.emplace_back(path, std::move(ptr_in), num_opened);
    mtx.unlock();
}

int Interface::GetNumFSElements() const
{
    mtx.lock();
    int s = opened.size();
    mtx.unlock();
    return s;
}

void Interface::IncrementNumOpened(int idx)
{
    mtx.lock();
    opened[idx].num_opened++;
    mtx.unlock();
}

void Interface::DecrementNumOpened(int idx)
{
    mtx.lock();
    opened[idx].num_opened++;
    mtx.unlock();
}

void Interface::ClearOpened()
{
    mtx.lock();
    opened.erase(
        std::remove_if(opened.begin(), opened.end(), 
            [](const MasterFCB& e){ return e.num_opened == 0; }
        ), 
        opened.end()
    );
    mtx.unlock();
}

int Interface::GetIdx(const std::string& path)
{
    mtx.lock();
    auto it = std::find_if(
        opened.begin(), opened.end(), [&path](const MasterFCB& e){ return e.path_str == path; }
    );
    int res = std::distance(opened.begin(), it);
    mtx.unlock();

    return res;
}

std::string Interface::GetLastError() const
{
    return last_error;
}

int Interface::GetFreeSpace() const
{
    return bm.GetFreeSpace();
}

int Interface::GetNumFreeBlocks() const
{
    return bm.GetNumFreeBlocks();
}

std::vector<std::string> Interface::SplitPath(const std::string& path_str)
{
    std::vector<std::string> split_path;
    std::stringstream ss(path_str);
    std::string str;
    while(std::getline(ss, str, '/'))
        split_path.push_back(std::move(str));
    return split_path;
}
