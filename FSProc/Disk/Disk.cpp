#include <Disk.h>

#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <string.h>

Disk::~Disk()
{
    // only unmount if something was mounted to begin with
	if (fd != -1)
		Unmount();
}

bool Disk::Mount(const std::string& filename)
{
    // open the given file
	fd = open(filename.c_str(), O_RDWR);
	if (fd == -1)
		return false;
	return true;
}

void Disk::Unmount()
{
    // close the opened file
	if (fd != -1)
		close(fd);
	fd = -1;
}

void Disk::Read(int block_num, void* buf) const
{
    // calculate the file offset using the block num
	const int offset = block_num * BlockSize;
    // seek to calculated offset
	lseek(fd, offset, SEEK_SET);
    // read the block at this location
	if (read(fd, buf, BlockSize) == -1)
		throw std::exception(); // was intended to be a dedicated exception type but no time left
}

void Disk::Write(int block_num, const void* buf)
{
    // calculate the file offset using the block num
	const int offset = block_num * BlockSize;
    // seek to calculated offset
	lseek(fd, offset, SEEK_SET);
    // write the block at this location
	if (write(fd, buf, BlockSize) == -1)
		throw std::exception(); // was intended to be a dedicated exception type but no time left
}

template<int NUM_BLOCKS>
void Disk::Create(const std::string& filename)
{
    // create a file with read-write-execute permissions
	int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRWXU);
	if (fd == -1)
	{
		std::ostringstream oss;
		oss << "Error Creating Disk: " << strerror(errno);
		throw std::exception();
	}

    // create an empty buffer to write empty blocks to file
	char buf[BlockSize] = {};
    // write NUM_BLOCKS number of blocks
	for (int i = 0; i < NUM_BLOCKS; i++)
	{
		write(fd, buf, BlockSize);
	}
}

// template instantiation needed to make this code work
template void Disk::Create<4096>(const std::string&);