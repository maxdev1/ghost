#include <interface/ComponentStore.hpp>
#include <map>

static std::map<uint32_t, Component*> components;
static uint32_t idCounter = 0;

/**
 *
 */
uint32_t ComponentStore::add(Component* c) {
	++idCounter;
	components[idCounter] = c;
	return idCounter;
}

/**
 *
 */
Component* ComponentStore::get(uint32_t ident) {
	if(components.count(ident) > 0) {
		return components[ident];
	}
	return 0;
}

/**
 *
 */
void ComponentStore::remove(uint32_t ident) {
	components.erase(ident);
}
