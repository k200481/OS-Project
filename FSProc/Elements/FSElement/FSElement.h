#pragma once

#include <BlockManager.h>
#include <Inode.h>
#include <mutex>

namespace FS
{
	class FSElement
	{
    public:
        /* Public interface to all the inode getters as the inode itself is hidden */

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
        /* Pritected constructors so only derived classes can make an FSElement */
        // basically turns this into an abstract class without using virtual

        // default ctor so derived classes can make default ctors
		FSElement();
        // load the FSElement using the given inode
		FSElement(BlockManager& bm, unsigned int inode_block);
        // create a new FSElement
		FSElement(BlockManager& bm, ElementType type, int owner, int permissions);
        // move ctor to make the derived types movable
        // needed due to the mutexes being used in this class
        FSElement(FSElement&& rhs) noexcept;
        // good idea to make virtual destructors for classes meant to be inherited
        virtual ~FSElement() = default;
        // explicitly deleting these two for the sake of thread safety
        // copies of FSElements can lead to... not fun situations
        FSElement(const FSElement&) = delete;
        FSElement& operator=(const FSElement&) = delete;

        /* Reader-Writer Thread safety mechanisms */

        void BeginRead(BlockManager& bm) const;
        void BeginRead() const;
        void EndRead() const;
        void BeginWrite(BlockManager& bm) const;
        void BeginWrite() const;
        void EndWrite() const;

	protected:
        // inodes are modified even during read operations
        // it felt right to make them mutable
		mutable Inode inode;
        // the block where the inode is stored
		unsigned int inode_block;
    private:
        /* reader-writer problem stuff */

        mutable int reader_count = 0;
		mutable std::mutex rw_mtx;
		mutable std::mutex mtx;
	};
}
