#include <Fonts.hpp>

/**
 *
 */
g_font* Fonts::getFontAtPath(std::string path, std::string name) {
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
g_font* Fonts::getSystemFont(std::string name) {
	return getFontAtPath("/ramdisk/system/graphics/fonts/" + name + ".ttf", name);
}

/**
 *
 */
g_font* Fonts::get(std::string name) {
	g_font* font = getSystemFont(name);

	if (font == 0) {
		font = getDefault();
	}

	return font;
}

/**
 *
 */
g_font* Fonts::getDefault() {
	return getFontAtPath("/ramdisk/system/graphics/fonts/default.ttf", "default");
}
