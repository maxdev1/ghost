#include <ghostuser/graphics/text/font_loader.hpp>

/**
 *
 */
g_font* g_font_loader::getFontAtPath(std::string path, std::string name) {
	FILE* file = fopen(path.c_str(), "r");
	if (file != NULL) {
		g_font* font = g_font::fromFile(file, name);
		fclose(file);
		return font;
	}
	return 0;
}

/**
 *
 */
g_font* g_font_loader::getSystemFont(std::string name) {
	return getFontAtPath("/system/graphics/fonts/" + name + ".ttf", name);
}

/**
 *
 */
g_font* g_font_loader::get(std::string name) {
	g_font* font = getSystemFont(name);

	if (font == 0) {
		font = getDefault();
	}

	return font;
}

/**
 *
 */
g_font* g_font_loader::getDefault() {
	return getFontAtPath("/system/graphics/fonts/default.ttf", "default");
}
