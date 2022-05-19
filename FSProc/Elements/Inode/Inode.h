#pragma once

#include <BlockManager.h>
#include <Disk.h>
#include <FS.h>

namespace FS
{
	class Inode
	{
        // maximum number of direct block pointers
		static constexpr unsigned int NumDirectBlocks = 12;
        // max number of indirect block pointers
		static constexpr unsigned int NumIndirectBlocks = Disk::BlockSize / sizeof(int);
		// with a heavy heart
		friend class FSElement;
	public:
        // metadata about the file system element this inode belongs to
		struct Metadata
		{
			ElementType type;
			int owner;
			int permissions;
			unsigned int size;
			time_t created;
			time_t modified;
			time_t accessed;
		};

	public:
        // loads an inode from the given block idx
		static Inode Load(const BlockManager& bm, unsigned int block_num);
        // creates a new inode at the given block idx
		static Inode Create(BlockManager& bm, unsigned int block_num, 
			ElementType type, int owner, int permissions);
        // writes data to the blocks tracked by the inode using the offset
        // calculation for which block an offset falls in is done automatically
		unsigned int Write(BlockManager& bm, unsigned int inode_block, 
			unsigned int offset, const void* data, unsigned int data_size);
        // reads data from the blocks tracked by the inode using the offset
        // calculation for which block an offset falls in is done automatically
		unsigned int Read(const BlockManager& bm, 
            unsigned int offset, void* buf, unsigned int data_size) const;
        // frees all allocated blocks to the inode
        void FreeAll(BlockManager& bm, unsigned int inode_block);
		// update the modification time in the metadata and save the inode
		void UpdateTimeModified(BlockManager& bm, unsigned int block_num);
		// update the access time in the metadata and save the inode
		void UpdateTimeAccessed(BlockManager& bm, unsigned int block_num);
        // get the size of the data tracked by the inode
        // does not include the wasted space at the end of the last data block
        // does not include the inode block itself
		unsigned int GetSize() const;
        // get the actual effective size of the data tracked by the inode
        // accounts forthe space wasted at the end of the last data block
        // does not include the inode block itself
		unsigned int GetSizeOnDisk() const;
        // get the type of the file system element pointed to by the inode
        ElementType GetType() const;
        // get the entire metadata of the file system element pointed to by the inode
		Metadata GetMetadata() const;

	private:
        // the default constructor
        // to ensure no inode is ever unitialized, this is kept private
        // and is only accessible by the FSElement base class
		Inode() = default;
        // allocate a new block to the inode
		void AddNewBlock(BlockManager& bm, unsigned int inode_block);
        // allocate multiple blocks to the inode
		void AddNewBlocks(BlockManager& bm, unsigned int inode_block, unsigned int num_blocks);
        // remove the last alocated block
		void RemoveLastBlock(BlockManager& bm, unsigned int inode_block);
        // get the actual block number of the block on the given index
		unsigned int GetBlockNum(const BlockManager& bm, unsigned int idx) const;
		// writes the inode to disk
		void Save(BlockManager& disk, unsigned int block_num);
	private:
		Metadata mtd = {};
        // total number of data blocks aside form the inode block itself
		unsigned int num_blocks = 0;
        // indices of the direct blocks
		unsigned int blocks[NumDirectBlocks] = {};
        // index of the block with the indices to indirect blocks
		unsigned int indir = 0;
	};

	struct data_pair : std::pair<std::string, Inode::Metadata> {};
	std::ostream& operator<<(std::ostream& out, const FS::data_pair& dp);
}

