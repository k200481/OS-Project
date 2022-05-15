#pragma once

#include <BlockManager.h>
#include <Inode.h>

namespace FS
{
	class FSElement
	{
    public:
        Inode::Metadata GetMetadata() const;
        ElementType GetType() const;
        unsigned int GetSize() const;
        unsigned int GetSizeOnDisk() const;
        int GetOwner() const;
        int GetPermissions() const;
        int GetTimeCreated() const;
        int GetTimeModified() const;
        int GetTimeAccessed() const;

	protected:
		FSElement();
		FSElement(BlockManager& bm, unsigned int inode_block);
		FSElement(BlockManager& bm, ElementType type, int owner, int permissions);

	protected:
		mutable Inode inode;
		unsigned int inode_block;
	};
}
