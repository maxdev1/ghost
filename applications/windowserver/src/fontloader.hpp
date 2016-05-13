#ifndef FONTLOADER_HPP_
#define FONTLOADER_HPP_

#include <ghostuser/graphics/text/font.hpp>
#include <string>

/**
 *
 */
class font_loader_t {
private:
	static g_font* getFontAtPath(std::string path, std::string name);
	static g_font* getSystemFont(std::string name);
public:
	static g_font* get(std::string name);
	static g_font* getDefault();
};

#endif
