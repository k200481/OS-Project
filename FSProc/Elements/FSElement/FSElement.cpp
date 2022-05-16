#include <FSElement.h>
#include <Inode.h>
using namespace FS;

FSElement::FSElement()
    :
    inode_block(0)
{}

FSElement::FSElement(BlockManager& bm, unsigned int inode_block)
    :
    inode_block(inode_block)
{
    // load an existing inode
    inode = Inode::Load(bm, inode_block);
    inode.UpdateTimeAccessed(bm, inode_block);
}

FSElement::FSElement(BlockManager& bm, ElementType type, int owner, int permissions)
{
    // create a new inode
    inode_block = bm.AlloateFreeBlock();
    inode = Inode::Create(bm, inode_block, type, owner, permissions);
}

FSElement::FSElement(FSElement&& rhs) noexcept
{
    // locking the mutexes ensures the object is not moved during a read/write operation
    // although this is unlikely to happen as the only time a move occurs is during construction
    std::unique_lock<std::mutex> lock1(rw_mtx);
    std::unique_lock<std::mutex> lock2(mtx);
    reader_count = rhs.reader_count;
    inode = std::move(rhs.inode);
    inode_block = rhs.inode_block;
}

ElementType FSElement::GetType() const
{
    return inode.GetType();
}

Inode::Metadata FSElement::GetMetadata() const
{
    return inode.mtd;
}

unsigned int FSElement::FSElement::GetSize() const
{
    return inode.mtd.size;
}

unsigned int FSElement::GetSizeOnDisk() const
{
    return inode.mtd.size * Disk::BlockSize + Disk::BlockSize;
}

int FSElement::GetOwner() const
{
    return inode.mtd.owner;
}

int FSElement::GetPermissions() const
{
    return inode.mtd.permissions;
}

int FSElement::GetTimeCreated() const
{
    return inode.mtd.created;
}

int FSElement::GetTimeModified() const
{
    return inode.mtd.modified;
}

int FSElement::GetTimeAccessed() const
{
    return inode.mtd.accessed;
}

void FSElement::BeginRead(BlockManager& bm) const
{
    mtx.lock(); // lock mtx before accessing reader count and access time
    if(reader_count == 0) // lock rw_mtx id this is the first reader
        rw_mtx.lock();
    reader_count++;
    inode.UpdateTimeAccessed(bm, inode_block);
    mtx.unlock();
}

void FSElement::BeginRead() const
{
    mtx.lock(); // lock mtx before accessing reader count
    if(reader_count == 0) // lock rw_mtx id this is the first reader
        rw_mtx.lock();
    reader_count++;
    mtx.unlock();
}

void FSElement::EndRead() const
{
    mtx.lock(); // lock mtx before accessing reader count and access time
    reader_count--;
    if(reader_count == 0) // unlock rw_mtx id this is the last reader
        rw_mtx.unlock();
    mtx.unlock();
}

void FSElement::BeginWrite(BlockManager& bm) const
{
    rw_mtx.lock(); // just need to lock the rw_mtx
    inode.UpdateTimeModified(bm, inode_block);
}

void FSElement::BeginWrite() const
{
    rw_mtx.lock(); // just need to lock the rw_mtx
}

void FSElement::EndWrite() const
{
    rw_mtx.unlock(); // unlock the rw_mtx to allow the next writer or first reader to enter
}