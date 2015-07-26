#ifndef CHECKBOX_HPP_
#define CHECKBOX_HPP_

#include <components/Component.hpp>
#include <components/Label.hpp>

#define DEFAULT_BOX_SIZE		20
#define DEFAULT_BOX_TEXT_GAP	5

/**
 *
 */
class CheckBox: public Component {
private:
	Label label;
	bool checked;
	int boxSize;
	int boxTextGap;

	bool hovered;
	bool pressed;

public:
	CheckBox();

	virtual void layout();
	virtual void paint();
	virtual bool handle(Event& e);
	virtual void handleBoundChange(g_rectangle oldBounds);

	Label& getLabel() {
		return label;
	}
};

#endif
