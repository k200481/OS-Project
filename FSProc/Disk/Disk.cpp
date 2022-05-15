#include <Disk.h>

#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <string.h>

Disk::~Disk()
{
	if (fd != -1)
		Unmount();
}

bool Disk::Mount(const std::string& filename)
{
	fd = open(filename.c_str(), O_RDWR);
	if (fd == -1)
		return false;
	return true;
}

void Disk::Unmount()
{
	if (fd != -1)
		close(fd);
	fd = -1;
}

void Disk::Read(int block_num, void* buf) const
{
	const int offset = block_num * BlockSize;
	lseek(fd, offset, SEEK_SET);
	if (read(fd, buf, BlockSize) == -1)
		throw std::exception();
}

void Disk::Write(int block_num, const void* buf)
{
	const int offset = block_num * BlockSize;
	lseek(fd, offset, SEEK_SET);
	if (write(fd, buf, BlockSize) == -1)
		throw std::exception();
}

template<int NUM_BLOCKS>
void Disk::Create(const std::string& filename)
{
	int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRWXU);
	if (fd == -1)
	{
		std::ostringstream oss;
		oss << "Error Creating Disk: " << strerror(errno);
		throw std::exception();
	}
    

	char buf[BlockSize] = {};
	for (int i = 0; i < NUM_BLOCKS; i++)
	{
		write(fd, buf, BlockSize);
	}
}

template void Disk::Create<4096>(const std::string&);