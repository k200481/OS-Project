#include <BlockManager.h>
#include <cassert>

BlockManager::BlockManager(Disk& d, const std::string& filename)
	:
	d(d)
{
	if (!d.Mount(filename))
	{
		Disk::Create(filename);
		d.Mount(filename);
		UpdateSuperblock();
		return;
	}

	d.Read(SuperBlockNum, &sb);
}

BlockManager::~BlockManager()
{
	d.Unmount();
}

unsigned int BlockManager::GetFreeBlock() const
{
	for (int i = 0; i < NumBlocks; i++)
	{
		if (BlockIsFree(i))
			return i;
	}
	return 0;
}

void BlockManager::AllocateBlock(unsigned int block_num)
{
	const int char_num = block_num / 8;
	const int bit_num = block_num % 8;
	sb.bitmap[char_num] |= (1 << bit_num);
	UpdateSuperblock();
}

unsigned int BlockManager::AlloateFreeBlock()
{
	for (int i = 0; i < NumBlocks; i++)
	{
		if (BlockIsFree(i))
		{
			AllocateBlock(i);
			return i;
		}
	}
	return 0;
}

unsigned int BlockManager::FirstAllocatableBlock() const
{
	return SuperBlockNum + 1;;
}

void BlockManager::FreeBlock(int block_num)
{
	const int char_num = block_num / 8;
	const int bit_num = block_num % 8;
	sb.bitmap[char_num] &= ~(1 << bit_num);
	UpdateSuperblock();
}

bool BlockManager::BlockIsFree(unsigned int block_num) const
{
	const int char_num = block_num / 8;
	const int bit_num = block_num % 8;
	return !(sb.bitmap[char_num] & (1 << bit_num));
}

void BlockManager::Read(unsigned int block_num, void* buf) const
{
	assert(block_num >= FirstAllocatableBlock());
	if (block_num >= NumBlocks)
		return;
	d.Read(block_num, buf);
}

void BlockManager::Write(unsigned int block_num, const void* buf)
{
	assert(block_num >= FirstAllocatableBlock());
	if (block_num >= NumBlocks)
		return;
	d.Write(block_num, buf);
}

unsigned int BlockManager::GetNumFreeBlocks() const
{
	unsigned int count = 0;
	for (unsigned int i = 0; i < NumBlocks; i++)
	{
		if (BlockIsFree(i))
		{
			count++;
		}
	}
	return count;
}

unsigned int BlockManager::GetFreeSpace() const
{
	return GetNumFreeBlocks() * Disk::BlockSize;
}

void BlockManager::UpdateSuperblock()
{
	d.Write(SuperBlockNum, &sb);
}
