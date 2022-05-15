#include "Directory.h"
#include "File.h"
#include <string.h>

using namespace FS;

Directory::Directory(BlockManager& bm, unsigned int inode_block)
    :
    FSElement(bm, inode_block)
{
    inode.Read(bm, 0, &num_entries, sizeof(int));
    inode.UpdateTimeAccessed(bm, inode_block);
}

Directory::Directory(BlockManager& bm, int owner, int permissions)
    :
    FSElement(bm, ElementType::Directory, owner, permissions)
{
    inode.Write(bm, inode_block, 0, &num_entries, sizeof(int));
}

Directory::Directory(Directory&& rhs)
{
    std::unique_lock<std::mutex> lock1(mtx);
    std::unique_lock<std::mutex> lock2(rw_mtx);
    inode = std::move(rhs.inode);
    inode_block = rhs.inode_block;
    num_entries = rhs.num_entries;
    reader_count = rhs.reader_count;
}

DirPtr Directory::Load(BlockManager& bm, unsigned int inode_block)
{
    return std::make_unique<Directory>(Directory(bm, inode_block));
}

DirPtr Directory::LoadRoot(BlockManager& bm)
{
    if(bm.FirstAllocatableBlock() == bm.GetFreeBlock())
        return DirPtr();
    return Load(bm, bm.FirstAllocatableBlock());
}

DirPtr Directory::CreateRoot(BlockManager& bm, int root_uid, int rootdir_permissions)
{
    return std::make_unique<Directory>(Directory(bm, root_uid, rootdir_permissions));
}

bool Directory::AddDirectory(BlockManager& bm, const char* name, int owner, int permissions)
{
    if(EntryExists(bm, name))
        return false;
    // create directory
    Directory new_dir(bm, owner, permissions);
    // add entry in current directory
    Entry e;
    strncpy(e.name, name, MaxNameLen);
    e.block_num = new_dir.inode_block;
    inode.Write(bm, inode_block, inode.GetSize(), &e, sizeof(Entry));
    // update num_entries
    num_entries++;
    inode.Write(bm, inode_block, 0, &num_entries, sizeof(int));
    return true;
}

bool Directory::AddFile(BlockManager& bm, const char* name, int owner, int permissions)
{
    if(EntryExists(bm, name))
        return false;
    // create file
    File new_file(bm, owner, permissions);
    // add entry in current directory
    Entry e;
    strncpy(e.name, name, MaxNameLen);
    e.block_num = new_file.inode_block;
    inode.Write(bm, inode_block, inode.GetSize(), &e, sizeof(Entry));
    // update num_entries
    num_entries++;
    inode.Write(bm, inode_block, 0, &num_entries, sizeof(int));
    return true;
}

unsigned int Directory::GetNumEntries() const
{
    return num_entries;
}

std::vector<data_pair> FS::Directory::List(BlockManager& bm) const
{
    inode.UpdateTimeAccessed(bm, inode_block);
    
    std::vector<data_pair> entries(num_entries);
    int offset = sizeof(num_entries);

    for (int i = 0; i < num_entries; i++)
    {
        Entry e = GetEntry(bm, i);
        Inode in = Inode::Load(bm, e.block_num);

        entries[i].first = e.name;
        entries[i].second = in.GetMetadata();

        offset += sizeof(Entry);
    }
    return entries;
}

DirPtr Directory::OpenSubdir(BlockManager& bm, const std::string& filename)
{
    for (int i = 0; i < num_entries; i++)
    {
        Entry e = GetEntry(bm, i);
        if (filename == e.name)
        {
            Inode in = Inode::Load(bm, e.block_num);
            if (in.GetMetadata().type == ElementType::Directory)
                return Directory::Load(bm, e.block_num);
        }
    }
    return DirPtr();
}

FilePtr FS::Directory::OpenFile(BlockManager& bm, const std::string& filename)
{
    for (int i = 0; i < num_entries; i++)
    {
        Entry e = GetEntry(bm, i);
        if (filename == e.name)
        {
            Inode in = Inode::Load(bm, e.block_num);
            if (in.GetMetadata().type == ElementType::File)
                return File::Load(bm, inode_block);
        }
    }
    throw FilePtr();
}

Directory::Entry Directory::GetEntry(const BlockManager& bm, unsigned int idx) const
{
    Entry e;
    inode.Read(bm, sizeof(int) + idx * sizeof(Entry), &e, sizeof(Entry));
    return e;
}

bool Directory::EntryExists(const BlockManager& bm, const std::string& filename)
{
    return GetEntryIdx(bm, filename) != 0;
}

int Directory::GetEntryIdx(const BlockManager& bm, const std::string& filename) const
{
    for(int i = 0; i < num_entries; i++)
    {
        if(GetEntry(bm, i).name == filename)
        {
            return i;
        }
    }
    return 0;
}