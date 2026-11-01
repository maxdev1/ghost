#pragma once

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static inline void coreReportError(const char* tool, const char* action, const char* path)
{
	fprintf(stderr, "%s: cannot %s '%s': %s\n", tool, action, path, strerror(errno));
}

/**
 * Writes a complete byte range, retrying interrupted calls and short writes.
 * A zero-byte write is an output failure rather than forward progress.
 */
static inline bool coreWriteAll(int output, const void* data, size_t size, const char* tool, const char* path)
{
	auto bytes = static_cast<const uint8_t*>(data);
	size_t offset = 0;
	while(offset < size)
	{
		ssize_t written = write(output, bytes + offset, size - offset);
		if(written < 0 && errno == EINTR)
			continue;
		if(written <= 0)
		{
			if(written == 0)
				errno = EIO;
			coreReportError(tool, "write", path);
			return false;
		}
		offset += written;
	}
	return true;
}

/**
 * Streams bytes without waiting to fill a buffer. Returns 1 for an input failure
 * and 2 for an output failure, so callers can stop after a broken output stream.
 */
static inline int coreTransfer(int input, int output, const char* tool, const char* inputPath, const char* outputPath)
{
	uint8_t buffer[16384];
	for(;;)
	{
		ssize_t count = read(input, buffer, sizeof(buffer));
		if(count < 0 && errno == EINTR)
			continue;
		if(count < 0)
		{
			coreReportError(tool, "read", inputPath);
			return 1;
		}
		if(count == 0)
			return 0;

		if(!coreWriteAll(output, buffer, count, tool, outputPath))
			return 2;
	}
}

static inline int coreClose(int descriptor, const char* tool, const char* path)
{
	if(close(descriptor) == 0)
		return 0;
	coreReportError(tool, "close", path);
	return 1;
}
