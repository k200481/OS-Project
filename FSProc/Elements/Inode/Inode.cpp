#include "Inode.h"
using namespace FS;

#include <string.h>
#include <cmath>

Inode Inode::Load(const BlockManager& bm, unsigned int block_num)
{
    // create a default inode
	Inode in;
    // create an empty block to write to disk
	char buf[Disk::BlockSize];
    // read thwe inode from the disk into the block
	bm.Read(block_num, &buf);
    // copy the inode data into the inode
	memcpy(&in, buf, sizeof(Inode));
	return in;
}

Inode Inode::Create(BlockManager& bm, unsigned int block_num, 
	ElementType type, int owner, int permissions)
{
    // create a default inode
	Inode in;
    // create an empty block
	char buf[Disk::BlockSize] = {};
	
	// init default metadata struct
	const time_t t = time(NULL);
	in.mtd = { type, owner, permissions, 0, t, t, t };

    // copy the inode onto the block and write it to disk
	memcpy(&buf, &in, sizeof(Inode));
	bm.Write(block_num, buf);
	return in;
}

unsigned int FS::Inode::Write(BlockManager& bm, unsigned int inode_block,
	unsigned int offset, const void* data, unsigned int data_size)
{
	// sanity check
	if (offset > mtd.size)
		offset = mtd.size;

	// allocate as many blocks as needed (if any) for this new data and update size
	if (offset + data_size > mtd.size)
	{
		const unsigned int new_size = offset + data_size;
		const unsigned int new_num_blocks = (unsigned int)std::ceil(float(new_size) / Disk::BlockSize);
		if (new_num_blocks > num_blocks)
			AddNewBlocks(bm, inode_block, new_num_blocks - num_blocks);
		if (num_blocks == new_num_blocks)
			mtd.size = new_size;
		else // in case enough blocks were not allocated (because disk ran out of space)
		{
			mtd.size = new_num_blocks * Disk::BlockSize;
			data_size = mtd.size - offset;
		}
		Save(bm, inode_block);
	}

	// some needed initializations
	char buf[Disk::BlockSize] = {};
	unsigned int data_offset = 0; // index for the data array
	const unsigned int block_idx = (unsigned int)std::floor(float(offset) / Disk::BlockSize); // the block idx to start writing from
	const unsigned int block_num = GetBlockNum(bm, block_idx); // actual block num on disk to start writing from

	if (block_num == 0) // well rip, no space left
		return 0;

	// load the block to start writing from
	bm.Read(block_num, buf);

	// write this first block
	const unsigned int block_offset = offset % Disk::BlockSize;
	const unsigned int cpy_size = std::min(Disk::BlockSize - block_offset, data_size);
	memcpy(buf + block_offset, data, cpy_size);
	bm.Write(block_num, buf);
	data_offset += cpy_size;

	// write the remaining blocks
	for (unsigned int i = block_idx + 1; i < num_blocks && data_offset < data_size; i++, data_offset += Disk::BlockSize)
	{
		memcpy(buf, (char*)data + data_offset,
			std::min(Disk::BlockSize, data_size - data_offset)
		);
		bm.Write(GetBlockNum(bm, i), buf);
	}

	return data_size;
}

unsigned int FS::Inode::Read(const BlockManager& bm, unsigned int offset, void* data, unsigned int data_size) const
{
	if (offset >= mtd.size)
		return 0;

	// some needed initializations
	char buf[Disk::BlockSize] = {};
	unsigned int data_offset = 0; // index for the data array
	const unsigned int block_idx = (unsigned int)std::floor(float(offset) / Disk::BlockSize); // the block idx to start writing from
	const unsigned int block_num = GetBlockNum(bm, block_idx); // actual block num on disk to start writing from

	// copy this first block
	const unsigned int block_offset = offset % Disk::BlockSize;
	const unsigned int cpy_size = std::min(Disk::BlockSize - block_offset, data_size);
	bm.Read(block_num, buf);
	memcpy(data, buf + block_offset, cpy_size);
	data_offset += cpy_size;

	// copy the remaining blocks
	for (unsigned int i = block_idx + 1; i < num_blocks && data_offset < data_size; i++, data_offset += Disk::BlockSize)
	{
		bm.Read(GetBlockNum(bm, i), buf);
		memcpy((char*)data + data_offset, buf,
			std::min(Disk::BlockSize, data_size - data_offset)
		);
	}

	return data_size;
}

unsigned int FS::Inode::GetSize() const
{
	return mtd.size;
}

unsigned int FS::Inode::GetSizeOnDisk() const
{
	return num_blocks * Disk::BlockSize;
}

ElementType Inode::GetType() const
{
    return mtd.type;
}

Inode::Metadata Inode::GetMetadata() const
{
	return mtd;
}

void FS::Inode::AddNewBlock(BlockManager& bm, unsigned int inode_block)
{
    // allocate a block
	const unsigned int block_num = bm.AlloateFreeBlock();
	
    // if there are direct blocks left to be allocated allocate one
	if (num_blocks < NumDirectBlocks)
	{
		blocks[num_blocks++] = block_num;
	}
	else if (num_blocks < NumDirectBlocks + NumIndirectBlocks) // otherwise allocate an indirect block
	{
		// allocate indir block if it doesn't exist
		if (indir == 0)
			indir = bm.AlloateFreeBlock();
		
		// load indir block
		unsigned int buf[Disk::BlockSize / sizeof(int)] = {};
		bm.Read(indir, buf);

		// update indir block
		buf[num_blocks - NumDirectBlocks] = block_num;
		bm.Write(indir, buf);
		num_blocks++;
	}
    // update the inode on the disk
	Save(bm, inode_block);
}

void FS::Inode::AddNewBlocks(BlockManager& bm, unsigned int inode_block, unsigned int num_blocks)
{
	for (unsigned int i = 0; i < num_blocks; i++)
		AddNewBlock(bm, inode_block);
}

void Inode::RemoveLastBlock(BlockManager& bm, unsigned int inode_block)
{
	if (num_blocks <= 0)
		return;
	else if (num_blocks <= 12)
	{
		bm.FreeBlock(blocks[--num_blocks]);
		return;
	}
	Save(bm, inode_block);
}

unsigned int Inode::GetBlockNum(const BlockManager& bm, unsigned int idx) const
{
	if (idx < 12)
	{
		return blocks[idx];
	}
	else if (idx < NumDirectBlocks + NumIndirectBlocks)
	{
		// load indir block
		unsigned int buf[Disk::BlockSize / sizeof(int)] = {};
		bm.Read(indir, buf);
		// return the block_num at idx - NumDirectBlocks
		return buf[idx - NumDirectBlocks];
	}
	return 0;
}

void Inode::Save(BlockManager& bm, unsigned int block_num)
{
	char buf[Disk::BlockSize] = {};
	memcpy(buf, this, sizeof(Inode));
	bm.Write(block_num, buf);
}

void FS::Inode::UpdateTimeModified(BlockManager& bm, unsigned int inode_block)
{
	mtd.accessed = time(NULL);
	Save(bm, inode_block);
}

void FS::Inode::UpdateTimeAccessed(BlockManager& bm, unsigned int inode_block)
{
	mtd.accessed = time(NULL);
	Save(bm, inode_block);
}
