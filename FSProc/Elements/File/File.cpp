#include <File.h>
#include <unistd.h>

using namespace FS;

File::File(BlockManager& bm, unsigned int inode_block)
    :
    FSElement(bm, inode_block)
{
    inode.UpdateTimeAccessed(bm, inode_block);
}

File::File(BlockManager& bm, int owner, int permissions)
    :
    FSElement(bm, ElementType::File, owner, permissions)
{}

int File::Read(BlockManager& bm, char* data, int offset, int size) const
{
    BeginRead(bm);
    int num = inode.Read(bm, offset, data, size);
    EndRead();

    return num;
}

int File::Write(BlockManager& bm, const char* data, int offset, int size)
{
    BeginWrite(bm);
    int num = inode.Write(bm, inode_block, offset, data, size);
    EndWrite();
    
    return num;
}
