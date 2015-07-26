#ifndef BUTTON_HPP_
#define BUTTON_HPP_

#include <components/Component.hpp>
#include <ghostuser/graphics/text/Font.hpp>
#include <ghostuser/graphics/metrics/insets.hpp>
#include <components/Label.hpp>
#include <string>
#include "components/TitledComponent.hpp"
#include "components/ActionComponent.hpp"

/**
 *
 */
struct ButtonState {
	ButtonState() :
			hovered(false), pressed(false), focused(false) {
	}
	bool hovered;
	bool pressed;
	bool focused;
};

/**
 *
 */
class Button: public Component, public TitledComponent, public ActionComponent {
private:
	ButtonState state;
	Label label;
	g_insets insets;

public:
	Button();
	virtual ~Button() {
	}

	virtual void layout();
	virtual void paint();
	virtual bool handle(Event& e);
	virtual void handleBoundChange(g_rectangle oldBounds);

	virtual void setTitle(std::string title);
	virtual std::string getTitle();

	Label& getLabel() {
		return label;
	}
};

#endif
