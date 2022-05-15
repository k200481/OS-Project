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
	BlockManager(Disk& d, const std::string& filename);
	BlockManager(const BlockManager&) = delete;
	BlockManager& operator=(const BlockManager&) = delete;
	~BlockManager();

	unsigned int GetFreeBlock() const;
	void AllocateBlock(unsigned int block_num);
	unsigned int AlloateFreeBlock();
	unsigned int FirstAllocatableBlock() const;

	void FreeBlock(int block_num);

	bool BlockIsFree(unsigned int block_num) const;

	void Read(unsigned int block_num, void* buf) const;
	void Write(unsigned int block_num, const void* buf);

	unsigned int GetNumFreeBlocks() const;
	unsigned int GetFreeSpace() const;

private:
	void UpdateSuperblock();

private:
	Disk& d;
	SuperBlock sb = { { 0x1 } };
};

