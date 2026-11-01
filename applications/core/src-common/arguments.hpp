#ifndef CORE_ARGUMENTS
#define CORE_ARGUMENTS

#include <stdint.h>
#include <string.h>

static inline bool coreArgumentIsHelp(const char* text)
{
	return text && (!strcmp(text, "-h") || !strcmp(text, "--help"));
}

static inline bool coreIsDecimalNumber(const char* text)
{
	if(!text || !*text)
		return false;
	for(auto digit = text; *digit; ++digit)
		if(*digit < '0' || *digit > '9')
			return false;
	return true;
}

/**
 * Parses a decimal prefix without consuming its suffix. Failed parses leave
 * the cursor and result unchanged, including on overflow of the chosen limit.
 */
static inline bool coreParseUnsignedPrefix(const char*& text, uint64_t& result, uint64_t maximum = UINT64_MAX)
{
	if(!text || *text < '0' || *text > '9')
		return false;
	auto cursor = text;
	uint64_t value = 0;
	while(*cursor >= '0' && *cursor <= '9')
	{
		auto digit = unsigned(*cursor - '0');
		if(value > maximum / 10 || (value == maximum / 10 && digit > maximum % 10))
			return false;
		value = value * 10 + digit;
		++cursor;
	}
	text = cursor;
	result = value;
	return true;
}

static inline bool coreParseUnsigned(const char* text, uint64_t* result, uint64_t maximum = UINT64_MAX)
{
	uint64_t value;
	if(!result || !coreParseUnsignedPrefix(text, value, maximum) || *text)
		return false;
	*result = value;
	return true;
}

#endif
