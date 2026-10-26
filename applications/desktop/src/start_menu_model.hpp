#ifndef DESKTOP_START_MENU_MODEL_HPP
#define DESKTOP_START_MENU_MODEL_HPP
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <libfenster/metrics/rectangle.hpp>

namespace desktop {
struct StartEntry {
    std::string name, application, arguments, icon;
};
inline bool launcherFromProperties(const std::map<std::string, std::string>& values, StartEntry& entry) {
    auto value = [&](const char* key) { auto found = values.find(key); return found == values.end() ? std::string{} : found->second; };
    entry = {value("name"), value("application"), value("arguments"), value("icon")};
    return !entry.name.empty() && !entry.application.empty() && entry.application.front() == '/';
}
inline std::vector<StartEntry> defaultApplications() {
    return {{"Navigator", "/applications/navigator.bin", "", "/system/graphics/app-icons/navigator.png"},
            {"Terminal", "/applications/terminal.bin", "", "/system/graphics/app-icons/terminal.png"},
            {"Editor", "/applications/editor.bin", "", ""},
            {"Calculator", "/applications/calculator.bin", "", "/system/graphics/app-icons/calculator.png"},
            {"Doom", "/applications/doom.bin", "", "/system/graphics/app-icons/doom.png"}};
}
class StartMenuModel {
public:
    std::vector<StartEntry> entries;
    int applicationCount = 0, selected = -1, pressed = -1, scroll = 0;
    int width = 344, height = 446;
    void configure(std::vector<StartEntry> applications) {
        if(applications.empty()) applications = defaultApplications();
        entries = std::move(applications); applicationCount = entries.size();
        for(const auto& place : std::vector<std::pair<std::string, std::string>>{{"Computer", "/"}, {"Home", "/user"}, {"Desktop", "/user/desktop"}})
            entries.push_back({place.first, "/applications/navigator.bin", place.second, ""});
        reset();
    }
    void reset() { selected = pressed = -1; scroll = 0; }
    int desiredHeight() const { return 58 + int(entries.size()) * 42 + 38 + 14; }
    int maximumScroll() const { return std::max(0, desiredHeight() - height); }
    fenster::Rectangle row(int index) const {
        return {14, 58 + index * 42 + (index >= applicationCount ? 38 : 0) - scroll, width - 28, 40};
    }
    int hit(fenster::Point position) const {
        if(position.y < 54 || position.y >= height - 14) return -1;
        for(int i = 0; i < int(entries.size()); ++i) if(row(i).contains(position)) return i;
        return -1;
    }
    void fit(int screenWidth, int availableHeight) {
        width = std::max(0, std::min(344, screenWidth - 16));
        height = std::max(0, std::min(desiredHeight(), availableHeight));
        scroll = std::clamp(scroll, 0, maximumScroll());
    }
    void move(int direction) {
        if(entries.empty()) return;
        selected = selected < 0 ? (direction > 0 ? 0 : int(entries.size()) - 1)
                               : (selected + int(entries.size()) + direction) % entries.size();
        auto box = row(selected);
        if(box.y < 54) scroll -= 54 - box.y;
        if(box.y + box.height > height - 14) scroll += box.y + box.height - height + 14;
        scroll = std::clamp(scroll, 0, maximumScroll());
    }
    void wheel(int amount) { scroll = std::clamp(scroll - amount * 42, 0, maximumScroll()); }
};
}
#endif
