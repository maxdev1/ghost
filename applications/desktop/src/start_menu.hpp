#ifndef DESKTOP_START_MENU_HPP
#define DESKTOP_START_MENU_HPP
#include "taskbar.hpp"
#include "start_menu_model.hpp"
#include <functional>
#include <map>

class start_menu_t : public Canvas {
    taskbar_t* dock = nullptr;
    desktop::StartMenuModel model;
    Font* font = nullptr;
    LayoutedText* textLayout = nullptr;
    std::map<std::string, cairo_surface_t*> icons;
    std::function<bool(const desktop::StartEntry&)> launch;
    std::string error;
    bool open = false;
    void activate(int index);
    void onMouse(ComponentMouseEvent* event);
    void onKey(KeyEvent& event);
protected:
    void init(taskbar_t* dock, std::vector<desktop::StartEntry> applications,
              std::function<bool(const desktop::StartEntry&)> launch);
public:
    explicit start_menu_t(ComponentId id) : Component(id), Canvas(id), FocusableComponent(id) {}
    ~start_menu_t() override;
    static start_menu_t* create(taskbar_t* dock, std::vector<desktop::StartEntry> applications,
                               std::function<bool(const desktop::StartEntry&)> launch);
    void show(bool visible);
    void paint();
};
#endif
