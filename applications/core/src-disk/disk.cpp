#include "disk.hpp"

int main(int argc, char** argv)
{
	int result = diskToolRun(argc, argv);
	if(fflush(stdout) == EOF || ferror(stdout))
		return storageError("disk", G_VOLUME_IO_ERROR);
	return result;
}
