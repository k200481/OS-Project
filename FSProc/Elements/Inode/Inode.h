#pragma once

#include <BlockManager.h>
#include <Disk.h>
#include <FS.h>

namespace FS
{
	class Inode
	{
		static constexpr unsigned int NumDirectBlocks = 12;
		static constexpr unsigned int NumIndirectBlocks = Disk::BlockSize / sizeof(int);
		// with a heavy heart
		friend class FSElement;
	public:
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
		static Inode Load(const BlockManager& bm, unsigned int block_num);

		static Inode Create(BlockManager& bm, unsigned int block_num, 
			ElementType type, int owner, int permissions);

		unsigned int Write(BlockManager& bm, unsigned int inode_block, 
			unsigned int offset, const void* data, unsigned int data_size);
		unsigned int Read(const BlockManager& bm, unsigned int offset, void* buf, unsigned int data_size) const;
		
		void UpdateTimeModified(BlockManager& bm, unsigned int block_num);
		void UpdateTimeAccessed(BlockManager& bm, unsigned int block_num);

		unsigned int GetSize() const;
		unsigned int GetSizeOnDisk() const;
        ElementType GetType() const;
		Metadata GetMetadata() const;

	private:
		Inode() = default;
		void AddNewBlock(BlockManager& bm, unsigned int inode_block);
		void AddNewBlocks(BlockManager& bm, unsigned int inode_block, unsigned int num_blocks);
		void RemoveLastBlock(BlockManager& bm, unsigned int inode_block);
		unsigned int GetBlockNum(const BlockManager& bm, unsigned int idx) const;
		// writes the inode to disk
		void Save(BlockManager& disk, unsigned int block_num);
	private:
		Metadata mtd = {};
		unsigned int num_blocks = 0;
		unsigned int blocks[NumDirectBlocks] = {};
		unsigned int indir = 0;
	};

	struct data_pair : std::pair<std::string, Inode::Metadata> {};
	std::ostream& operator<<(std::ostream& out, const FS::data_pair& dp);
}

