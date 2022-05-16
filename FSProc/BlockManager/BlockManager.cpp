#include <BlockManager.h>
#include <cassert>

BlockManager::BlockManager(Disk& d, const std::string& filename)
	:
	d(d)
{
    // if the file doesnt exist create it and then initialize it
	if (!d.Mount(filename))
	{
		Disk::Create(filename);
		d.Mount(filename);
		UpdateSuperblock();
		return;
	}
    // read the superblock otherwise
	d.Read(SuperBlockNum, &sb);
}

BlockManager::~BlockManager()
{
    // unmount the disk
	d.Unmount();
}

unsigned int BlockManager::GetFreeBlock() const
{
    // check each block to see if it is free
	for (int i = 0; i < NumBlocks; i++)
	{
        // return the idx of the first free block found
		if (BlockIsFree(i))
			return i;
	}
    // return zero if none are found
	return 0;
}

void BlockManager::AllocateBlock(unsigned int block_num)
{
    // update the bit representing the block in the superblock
    // and write the block to the disk
	const int char_num = block_num / 8;
	const int bit_num = block_num % 8;
	sb.bitmap[char_num] |= (1 << bit_num);
	UpdateSuperblock();
}

unsigned int BlockManager::AlloateFreeBlock()
{
    // iterate over every block to find a free block, allocate it, and return its index
	for (int i = 0; i < NumBlocks; i++)
	{
		if (BlockIsFree(i))
		{
			AllocateBlock(i);
			return i;
		}
	}
    // return 0 if no new block is found
	return 0;
}

unsigned int BlockManager::FirstAllocatableBlock() const
{
    // the first block after the superblock is allocatable
	return SuperBlockNum + 1;;
}

void BlockManager::FreeBlock(int block_num)
{
    // set the bit representing block_num in the superblock to 0 and save it to disk
	const int char_num = block_num / 8;
	const int bit_num = block_num % 8;
	sb.bitmap[char_num] &= ~(1 << bit_num);
	UpdateSuperblock();
}

bool BlockManager::BlockIsFree(unsigned int block_num) const
{
    // check if the bit representing nlock num is set to 1
	const int char_num = block_num / 8;
	const int bit_num = block_num % 8;
	return !(sb.bitmap[char_num] & (1 << bit_num));
}

void BlockManager::Read(unsigned int block_num, void* buf) const
{
    // ensure write operations are only performed on allocatable blocks
	assert(block_num >= FirstAllocatableBlock());
	if (block_num >= NumBlocks)
		return;
    // read the given block
	d.Read(block_num, buf);
}

void BlockManager::Write(unsigned int block_num, const void* buf)
{
    // ensure read operations are only performed on allocatable blocks
	assert(block_num >= FirstAllocatableBlock());
	if (block_num >= NumBlocks)
		return;
    // write the given block
	d.Write(block_num, buf);
}

unsigned int BlockManager::GetNumFreeBlocks() const
{
    // iterate over every block to count the number of free blocks
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
    // free space = numbr of free blocks * block size
	return GetNumFreeBlocks() * Disk::BlockSize;
}

void BlockManager::UpdateSuperblock()
{
    // write the superblock to disk
	d.Write(SuperBlockNum, &sb);
}
