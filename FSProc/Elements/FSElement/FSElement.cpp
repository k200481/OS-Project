#include "FSElement.h"
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
    inode = Inode::Load(bm, inode_block);
    inode.UpdateTimeAccessed(bm, inode_block);
}

FSElement::FSElement(BlockManager& bm, ElementType type, int owner, int permissions)
{
    inode_block = bm.AlloateFreeBlock();
    inode = Inode::Create(bm, inode_block, type, owner, permissions);
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