#ifndef COMPONENTSTORE_HPP_
#define COMPONENTSTORE_HPP_

#include <WindowManager.hpp>

/**
 *
 */
class ComponentStore {
public:
	static uint32_t add(Component* comp);
	static Component* get(uint32_t ident);
	static void remove(uint32_t ident);
};

#endif
