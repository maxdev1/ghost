/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <calculator.hpp>
#include <libfenster/application.hpp>
#include <libfenster/component_update_batch.hpp>
#include <libfenster/components/button.hpp>
#include <libfenster/components/label.hpp>
#include <libfenster/components/panel.hpp>
#include <libfenster/components/window.hpp>
#include <libinput/keyboard/keyboard.hpp>
#include <vector>

using namespace fenster;

namespace
{
class CalculatorDisplay : public Label, public FocusableComponent
{
public:
    explicit CalculatorDisplay(ComponentId id) : Component(id), TitledComponent(id), Label(id), FocusableComponent(id) {}
    static CalculatorDisplay* create() { return createComponent<CalculatorDisplay, ComponentType::Label>(); }
};

Calculator calculator;
Window* window;
CalculatorDisplay* display;
Label* history;
std::vector<std::pair<char, Button*>> operators;

void input(char key)
{
    ComponentUpdateBatch batch;
    ComponentUpdateBatch::Scope scope(batch);
    calculator.input(key);
    display->setTitle(calculator.display());
    display->setNumericProperty(ComponentProperty::FontSize, calculator.display().size() > 12 ? 26 : 36);
    history->setTitle(calculator.history());
    for(auto& item : operators) item.second->setSelected(item.first == calculator.pendingOperation());
    // Return keyboard input to the readout after a keypad click.
    batch.commit();
    display->setFocused(true);
}

void keyPressed(KeyEvent& event)
{
    const auto& key = event.info;
    if(!key.pressed || key.ctrl || key.alt) return;
    // The shared layout omits these extended/numeric keypad keys.
    if(key.scancode == 0x37) { input('*'); return; }
    if(key.scancode == 0x6035) { input('/'); return; }
    if(key.scancode == 0x601c) { input('='); return; }
    if(key.scancode == 0x6053) { input('c'); return; }
    g_key_info_basic basic;
    basic.pressed = key.pressed; basic.ctrl = key.ctrl; basic.alt = key.alt;
    basic.shift = key.shift; basic.scancode = key.scancode;
    auto full = g_keyboard::fullKeyInfo(basic);
    char c = g_keyboard::charForKey(full);
    if(full.key.compare(0, 8, "KEY_PAD_") == 0) {
        if(full.key.size() == 9 && full.key[8] >= '0' && full.key[8] <= '9') input(full.key[8]);
        else if(full.key == "KEY_PAD_PLUS") input('+');
        else if(full.key == "KEY_PAD_MINUS") input('-');
        else if(full.key == "KEY_PAD_DOT") input('.');
    }
    else if(full.key == "KEY_ENTER") input('=');
    else if(full.key == "KEY_ESC" || full.key == "KEY_DELETE" || c == 'c' || c == 'C') input('c');
    else if(full.key == "KEY_BACKSPACE") input('b');
    else if(c == ',') input('.');
    else if((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-' || c == '*' || c == '/' || c == '=') input(c);
}
}

int main()
{
    if(Application::open() != ApplicationOpenStatus::Success) return -1;
    if(!g_keyboard::loadLayout("de-DE")) return -1;
    ComponentUpdateBatch batch;
    ComponentUpdateBatch::Scope scope(batch);
    window = Window::create();
    window->setTitle("Calculator");
    window->setResizable(false);
    window->setMinimizeButtonVisible(false);
    window->setMaximizeButtonVisible(false);
    window->setBounds(Rectangle(70, 70, 348, 540));
    window->onClose([] { g_exit(0); });

    auto readout = Panel::create();
    readout->setBounds(Rectangle(16, 16, 296, 96));
    readout->setBackground(_RGB(241, 244, 250));
    readout->setNumericProperty(ComponentProperty::CornerRadius, 9);
    readout->setNumericProperty(ComponentProperty::BorderColor, _RGB(222, 228, 238));
    window->addChild(readout);
    history = Label::create();
    history->setBounds(Rectangle(12, 6, 272, 24));
    history->setAlignment(TextAlignment::RIGHT);
    history->setColor(_RGB(108, 120, 140));
    history->setNumericProperty(ComponentProperty::FontSize, 13);
    readout->addChild(history);
    display = CalculatorDisplay::create();
    display->setBounds(Rectangle(12, 32, 272, 56));
    display->setTitle("0");
    display->setAlignment(TextAlignment::RIGHT);
    display->setColor(_RGB(37, 48, 68));
    display->setNumericProperty(ComponentProperty::FontSize, 36);
    display->setFocusable(true);
    display->addKeyListener(keyPressed);
    readout->addChild(display);

    auto button = [&](const std::string& title, char key, int column, int row, int span = 1) {
        auto value = Button::create();
        value->setTitle(title);
        value->setBounds(Rectangle(16 + column * 76, 128 + row * 64, 68 * span + 8 * (span - 1), 56));
        value->setNumericProperty(ComponentProperty::FontSize, 24);
        value->setActionListener([key] { input(key); });
        window->addChild(value);
        return value;
    };
    button("C", 'c', 0, 0)->setTooltip("Clear all  (Esc)");
    button("±", 's', 1, 0)->setTooltip("Change sign");
    auto erase = button("", 'b', 2, 0);
    erase->setIcon(Icon::Back); erase->setTooltip("Delete last digit  (Backspace)");
    const char operations[] = {'/', '*', '-', '+'};
    const char* symbols[] = {"÷", "×", "−", "+"};
    for(int row = 0; row < 4; ++row) operators.push_back({operations[row], button(symbols[row], operations[row], 3, row)});
    for(int row = 0; row < 3; ++row)
        for(int column = 0; column < 3; ++column) {
            char digit = '7' - row * 3 + column;
            button(std::string(1, digit), digit, column, row + 1);
        }
    button("0", '0', 0, 4, 2);
    button(".", '.', 2, 4);
    auto equals = button("=", '=', 3, 4);
    equals->setSelected(true); equals->setTooltip("Calculate  (Enter)");
    auto hint = Label::create();
    hint->setTitle("Enter =    Esc clear    Backspace delete");
    hint->setBounds(Rectangle(16, 448, 296, 20));
    hint->setNumericProperty(ComponentProperty::FontSize, 11);
    hint->setAlignment(TextAlignment::CENTER);
    hint->setColor(_RGB(108, 120, 140));
    window->addChild(hint);
    window->addFocusListener([](bool focused) {
        if(focused) display->setFocused(true);
    });
    window->setVisible(true);
    if(!batch.commit()) return -1;
    display->setFocused(true);

    g_user_mutex lock = g_mutex_initialize();
    g_mutex_acquire(lock);
    g_mutex_acquire(lock);
}
