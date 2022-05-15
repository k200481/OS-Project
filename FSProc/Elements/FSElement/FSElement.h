#pragma once

#include <BlockManager.h>
#include <Inode.h>
#include <mutex>

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
        FSElement(FSElement&& rhs) noexcept;

        void BeginRead(BlockManager& bm) const;
        void BeginRead() const;
        void EndRead() const;
        void BeginWrite(BlockManager& bm) const;
        void BeginWrite() const;
        void EndWrite() const;

	protected:
		mutable Inode inode;
		unsigned int inode_block;
    private:
        mutable int reader_count = 0;
		mutable std::mutex rw_mtx;
		mutable std::mutex mtx;
	};
}
