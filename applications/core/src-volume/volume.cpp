#include "volume.hpp"

int main(int argc, char** argv)
{
	int result = volumeToolRun(argc, argv);
	if(fflush(stdout) == EOF || ferror(stdout))
	{
		fprintf(stderr, "volume: cannot write standard output\n");
		return 1;
	}
	return result;
}
