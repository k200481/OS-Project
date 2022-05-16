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

bool Directory::Add(BlockManager& bm, const char* name, ElementType t, int owner, int permissions)
{
    if(EntryExists(bm, name))
        return false;

    BeginWrite(bm);

    Entry e;
    if(t == ElementType::Directory)
        e.block_num = Directory(bm, owner, permissions).inode_block;
    else
        e.block_num = File(bm, owner, permissions).inode_block;
    
    if(e.block_num == 0)
        return false;
    
    strncpy(e.name, name, MaxNameLen);
    inode.Write(bm, inode_block, inode.GetSize(), &e, sizeof(Entry));
    num_entries++;
    inode.Write(bm, inode_block, 0, &num_entries, sizeof(int));

    EndWrite();
    return true;
}

unsigned int Directory::GetNumEntries() const
{
    BeginRead();
    int num = num_entries;
    EndRead();

    return num;
}

std::vector<data_pair> FS::Directory::List(BlockManager& bm) const
{
    // updating access time is a write operation
    BeginRead(bm);

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

    EndRead();

    return entries;
}

FSElementPtr Directory::Open(BlockManager& bm, const std::string& filename)
{
    Entry* entry_list = new Entry[num_entries];
    BeginRead(bm);
    inode.Read(bm, sizeof(int), entry_list, sizeof(Entry) * num_entries);
    for(int i = 0; i < num_entries; i++)
    {
        if(entry_list[i].name == filename)
        {
            Inode inode = Inode::Load(bm, entry_list[i].block_num);
            if(inode.GetType() == ElementType::File)
                return File::Load(bm, entry_list[i].block_num);
            else if(inode.GetType() == ElementType::Directory)
                return Directory::Load(bm, entry_list[i].block_num);
        }
    }
    EndRead();
    return FSElementPtr();
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