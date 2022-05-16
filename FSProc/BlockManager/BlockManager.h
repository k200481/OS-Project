#pragma once
#include <Disk.h>

class BlockManager
{
	struct SuperBlock
	{
		char bitmap[Disk::BlockSize];
	};

	static constexpr unsigned int SuperBlockNum = 0;
	static constexpr unsigned int NumBlocks = sizeof(SuperBlock) * 8;

public:
    // paremeterized ctor only, needs a disk object and a file to load as the disk
	BlockManager(Disk& d, const std::string& filename);
    // unmounts the loaded file
	~BlockManager();
    // no copy ctors or = operators
	BlockManager(const BlockManager&) = delete;
	BlockManager& operator=(const BlockManager&) = delete;

    // get the index to a block the has not been marked as allocated
    // returns 0 if no block is free
	unsigned int GetFreeBlock() const;
    // mark a given block as allocated
	void AllocateBlock(unsigned int block_num);
    // mark the first free block as allocated and get its index
    // returns zero if no block is free
	unsigned int AlloateFreeBlock();
    // get the index to the first block that is allowed to be allocated by this block manager
    // this is the first block after the superblock
	unsigned int FirstAllocatableBlock() const;
    // free the block at the given index
	void FreeBlock(int block_num);
    // check if the block at the given idx is free
	bool BlockIsFree(unsigned int block_num) const;
    // read data from the given blockl (can only read a full block)
	void Read(unsigned int block_num, void* buf) const;
    // write data to a given block (can only write a full block)
	void Write(unsigned int block_num, const void* buf);
    // get the number of free blocks left in the file/disk
	unsigned int GetNumFreeBlocks() const;
    // get thwe total free space left in the file (free blocks * block size)
	unsigned int GetFreeSpace() const;

private:
    // writes superblock to the file
	void UpdateSuperblock();

private:
	Disk& d;
	SuperBlock sb = { { 0x1 } };
};

