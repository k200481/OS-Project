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

FS::File::File(File&& rhs) noexcept
{
    std::unique_lock<std::mutex> lock1(mtx);
    std::unique_lock<std::mutex> lock2(rw_mtx); // might be unnecessary actually
    inode = std::move(rhs.inode);
    inode_block = rhs.inode_block;
    reader_count = rhs.reader_count;
}

int File::Read(BlockManager& bm, char* data, int offset, int size) const
{
    mtx.lock();
    if(reader_count == 0)
        rw_mtx.lock();
    reader_count++;
    inode.UpdateTimeAccessed(bm, inode_block);
    mtx.unlock();

    int num = inode.Read(bm, offset, data, size);

    mtx.lock();
    reader_count--;
    if (reader_count == 0)
        rw_mtx.unlock();
    mtx.unlock();

    return num;
}

int File::Write(BlockManager& bm, const char* data, int offset, int size)
{
    rw_mtx.lock();
    int num = inode.Write(bm, inode_block, offset, data, size);
    inode.UpdateTimeModified(bm, inode_block);
    rw_mtx.unlock();
    
    return num;
}
