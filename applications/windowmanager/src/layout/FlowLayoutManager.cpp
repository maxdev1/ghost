#include <layout/FlowLayoutManager.hpp>
#include <vector>

#include <components/Component.hpp>
#include <ghostuser/utils/Logger.hpp>

#include <typeinfo>

/**
 *
 */
void FlowLayoutManager::layout() {

	if (component == 0) {
		return;
	}

	std::vector<Component*>& children = component->getChildren();

	int x = 0;
	int y = 0;
	int lineHeight = 0;

	g_rectangle parentBounds = component->getBounds();
	for (Component* c : children) {

		g_dimension preferredSize = c->getPreferredSize();

		if (x + preferredSize.width > parentBounds.width) {
			x = 0;
			y += lineHeight;
			lineHeight = 0;
		}

		c->setBounds(g_rectangle(x, y, preferredSize.width, preferredSize.height));
		x += preferredSize.width;

		if (preferredSize.height > lineHeight) {
			lineHeight = preferredSize.height;
		}

	}
}
