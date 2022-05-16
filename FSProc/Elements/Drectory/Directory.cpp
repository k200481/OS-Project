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
    if(bm.FirstAllocatableBlock() == bm.GetFreeBlock()) // root is kept at the first allocatable block
        return DirPtr(); // if the first allocatable block is free, there is no root. return empty ptr to indicate this
    return Load(bm, bm.FirstAllocatableBlock()); // load the root block and return it
}

DirPtr Directory::CreateRoot(BlockManager& bm, int root_uid, int rootdir_permissions)
{
    // create a root block and return it
    return std::make_unique<Directory>(Directory(bm, root_uid, rootdir_permissions));
}

bool Directory::Add(BlockManager& bm, const char* name, ElementType t, int owner, int permissions)
{
    if(EntryExists(bm, name)) // stop if the entry alrady exists
        return false;

    BeginWrite(bm); // write mode

    Entry e;
    // check the given element type and create the appropriate element
    // and store the block number it was saved to
    if(t == ElementType::Directory)
        e.block_num = Directory(bm, owner, permissions).inode_block;
    else
        e.block_num = File(bm, owner, permissions).inode_block;
    
    // if the block num is 0, it failed
    if(e.block_num == 0)
    {
        EndWrite();
        return false;
    }
    
    // copy the entry name and save it
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
    BeginRead(bm);

    std::vector<data_pair> entries(num_entries);
    int offset = sizeof(num_entries); // start at offset num entries
    // iterate over all entries and add them to the vector
    for (int i = 0; i < num_entries; i++)
    {
        Entry e = GetEntry(bm, i);
        Inode in = Inode::Load(bm, e.block_num); // load the indoe to read its metadata

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
    FSElementPtr ptr;
    
    BeginRead(bm);
    // read the entire entry list
    inode.Read(bm, sizeof(int), entry_list, sizeof(Entry) * num_entries);
    // iterate over entry list to find the correct one
    for(int i = 0; i < num_entries; i++)
    {
        if(entry_list[i].name == filename)
        {
            Inode child_inode = Inode::Load(bm, entry_list[i].block_num);
            // load the appropriate type of element based on the element type
            if(child_inode.GetType() == ElementType::File)
            {
                ptr = File::Load(bm, entry_list[i].block_num);
                break;
            }
            else if(child_inode.GetType() == ElementType::Directory)
            {
                ptr = Directory::Load(bm, entry_list[i].block_num);
                break;
            }
        }
    }
    EndRead();

    return ptr; // returns null if nothing was found
}

Directory::Entry Directory::GetEntry(const BlockManager& bm, unsigned int idx) const
{
    Entry e;
    // read the entry by generating its offset using the index
    inode.Read(bm, sizeof(int) + idx * sizeof(Entry), &e, sizeof(Entry));
    return e;
}

bool Directory::EntryExists(const BlockManager& bm, const std::string& filename)
{
    return GetEntryIdx(bm, filename) != -1;
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
    return -1;
}