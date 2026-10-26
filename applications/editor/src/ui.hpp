#ifndef GHOST_EDITOR_UI_HPP
#define GHOST_EDITOR_UI_HPP

#include "notepad.hpp"
#include <libfenster/application.hpp>
#include <libfenster/component_update_batch.hpp>
#include <libfenster/components/button.hpp>
#include <libfenster/components/canvas.hpp>
#include <libfenster/components/image.hpp>
#include <libfenster/components/label.hpp>
#include <libfenster/components/panel.hpp>
#include <libfenster/components/scrollpane.hpp>
#include <libfenster/components/text_box.hpp>
#include <libfenster/components/window.hpp>
#include <libfenster/layout/fill_layout.hpp>
#include <libfenster/layout/flex_layout.hpp>
#include <libfenster/layout/stack_layout.hpp>
#include <atomic>
#include <memory>
#include <map>

namespace editor
{
enum class UiKind { MainCommand, Menu, MenuAction, MenuHover, MainKey, DialogCommand, DialogKey, DialogClose, FileSelect, FileActivate, Place, FileResize, RootResize };
struct UiEvent
{
    UiKind kind = UiKind::MainCommand;
    Command command = Command::None;
    int index = 0, menu = 0;
    uint32_t epoch = 0;
    std::string text;
    fenster::KeyInfoBasic key{};
    fenster::Rectangle bounds;
};

class EditorUi
{
    using Component = fenster::Component;
    using Panel = fenster::Panel;
    using Button = fenster::Button;
    using Label = fenster::Label;
    using TextBox = fenster::TextBox;
    using Window = fenster::Window;
    using Flex = fenster::FlexLayout;
    using Fill = fenster::FillLayout;
    using Stack = fenster::StackLayout;
    using Icon = fenster::Icon;
    using Property = fenster::ComponentProperty;
    using Orientation = fenster::Orientation;
    Notepad& app;
    Window *window, *dialogWindow = nullptr;
    std::function<void(UiEvent)> post;
    std::atomic<uint32_t> epoch{0};
    Panel *root, *menuLayer, *footer, *browser, *form, *fieldRows[2], *options, *dialogSpacer;
    Panel *fileContent, *fileArea;
    fenster::ScrollPane* fileScroll;
    TextBox *fields[2], *location, *filter;
    Label *fieldLabels[2], *intro, *notice, *fileSummary, *preview, *emptyFiles, *position, *format;
    Flex *dialogLayout;
    Button* matchCase;
    Dialog shownDialog = Dialog::None;
    std::string lastDirectory, fileQuery, lastFields[2], lastNotice, lastFileSummary, lastPosition, lastFormat, lastIntro, lastPreview;
    int rootWidth = 760, rootHeight = 500, lastMenu = -2, lastSelection = -2, fileWidth = -1, previousFont = -1;
    bool previousStatus = true, previousMatchCase = false, noticeShown = false;
    std::vector<Component*> focusOrder;
    std::vector<std::pair<Button*, std::string>> places;
    std::map<Command, Button*> actions, toolbar;
    std::map<Button*, bool> enabled;
    struct MenuRow { Panel* panel; Label* title; fenster::Image* icon; };
    struct Menu { Button* button; Panel* panel; fenster::ScrollPane* scroll; std::vector<MenuRow> rows; int height; };
    std::vector<Menu> menus;
    struct FileRow {
        Panel* panel; Label *name, *kind; fenster::Image* icon;
        std::string path, title, tooltip, type;
        Icon image = Icon::None;
        bool active = false, selected = false;
    };
    std::vector<std::unique_ptr<FileRow>> rows;
    std::vector<int> visible;
    SYS_MUTEX_T rowLock = fenster::platformInitializeMutex(true);

    void emit(UiKind kind, Command command = Command::None, int index = 0, const std::string& text = "")
    {
        UiEvent event; event.kind = kind; event.command = command; event.index = index; event.text = text; event.epoch = epoch.load(); post(std::move(event));
    }
    void keys(Component* component, bool dialog)
    {
        auto id = component->getId();
        component->addKeyListener([this, id, dialog](fenster::KeyEvent& key) {
            if(!key.info.pressed) return;
            UiEvent event; event.kind = dialog ? UiKind::DialogKey : UiKind::MainKey; event.key = key.info; event.index = id; event.epoch = epoch.load(); post(std::move(event));
        });
    }
    static void fixed(Component* component, int width, int height)
    {
        component->setMinimumSize({std::max(0, width), std::max(0, height)}); component->setMaximumSize({width, height});
    }
    static Panel* panel(Component* parent, fenster::ColorArgb color = ARGB(0, 0, 0, 0))
    {
        auto result = Panel::create(); result->setBackground(color); parent->addChild(result); return result;
    }
    static Flex* flex(Component* component, Orientation orientation, fenster::Insets padding = {}, int gap = 0)
    {
        auto layout = Flex::create(component); layout->setOrientation(orientation); layout->setPadding(padding); layout->setSpace(gap); return layout;
    }
    static void part(Flex* layout, Component* child, int basis, float grow = 0, float shrink = 0) { layout->setComponentInfo(child, grow, shrink, basis); }
    static void spacer(Component* parent, Flex* layout, int size = 0, float grow = 1) { part(layout, panel(parent), size, grow); }
    static Label* label(Component* parent, const std::string& title = "", int size = 14, fenster::ColorArgb color = _RGB(48, 57, 74))
    {
        auto value = Label::create(); value->setTitle(title); value->setNumericProperty(Property::FontSize, size); value->setColor(color); parent->addChild(value); return value;
    }
    Button* button(Component* parent, const std::string& title, Icon icon, UiKind kind, Command command = Command::None, int index = 0)
    {
        auto value = Button::create(); value->setTitle(title); value->setIcon(icon); value->setVariant(fenster::ButtonVariant::Ghost); parent->addChild(value);
        value->setActionListener([this, kind, command, index] { emit(kind, command, index); });
        keys(value, kind == UiKind::DialogCommand); return value;
    }
    static TextBox* field(Component* parent, const std::string& hint, Icon icon = Icon::None)
    {
        auto value = TextBox::create(); parent->addChild(value); value->setStringProperty(Property::Placeholder, hint); value->setNumericProperty(Property::Icon, uint32_t(icon)); return value;
    }
    static std::string wrapped(const std::string& value, int columns = 58)
    {
        std::string result; int column = 0;
        for(size_t start = 0; start < value.size();) {
            size_t end = value.find_first_of(" \n", start); if(end == std::string::npos) end = value.size();
            auto word = value.substr(start, end - start);
            if(column && column + int(word.size()) > columns) { result += '\n'; column = 0; }
            result += word; column += word.size();
            if(end < value.size()) { result += value[end]; column = value[end] == '\n' ? 0 : column + 1; }
            start = end + 1;
        }
        return result;
    }
    static void title(Label* label, const std::string& text, std::string& previous)
    {
        if(text != previous) { label->setTitle(text); previous = text; }
    }
    void setEnabled(Button* button, bool value)
    {
        if(!enabled.count(button) || enabled[button] != value) { enabled[button] = value; button->setEnabled(value); }
    }
    static Icon commandIcon(Command command)
    {
        switch(command) {
            case Command::New: return Icon::Document;
            case Command::Open: return Icon::Folder;
            case Command::Save: case Command::SaveAs: case Command::ConfirmSave: return Icon::Save;
            case Command::Undo: return Icon::Undo;
            case Command::Redo: return Icon::Redo;
            case Command::Find: case Command::FindNext: case Command::Replace: return Icon::Search;
            case Command::About: return Icon::Info;
            case Command::Exit: case Command::Cancel: return Icon::Close;
            default: return Icon::None;
        }
    }
    void buildMenus(Component* column, Flex* frame)
    {
        auto bar = panel(column, _RGB(250, 251, 253)); part(frame, bar, 30);
        auto layout = Stack::create(bar); layout->setOrientation(Orientation::Horizontal); layout->setPadding({1, 12, 1, 0}); layout->setSpace(2);
        auto overlay = panel(root); Fill::create(overlay)->setPadding({30, 0, 0, 0}); menuLayer = panel(overlay); menuLayer->setFocusable(true); keys(menuLayer, false);
        menuLayer->addMouseListener([this](fenster::ComponentMouseEvent* mouse) { if(mouse->type == fenster::MouseEventType::Press) emit(UiKind::Menu, Command::None, -1); });
        const char* names[] = {"File", "Edit", "Format", "View", "Help"};
        for(int i = 0; i < 5; ++i) {
            Menu menu{}; menu.button = button(bar, names[i], Icon::None, UiKind::Menu, Command::None, i); fixed(menu.button, i == 2 ? 68 : 52, 28);
            menus.push_back(std::move(menu));
        }
        menuLayer->setVisible(false);
    }
    void buildMenu(Menu& menu, int i)
    {
        menu.panel = panel(menuLayer, _RGB(255, 255, 255)); menu.panel->setVisible(false); menu.panel->setNumericProperty(Property::CornerRadius, 9); menu.panel->setNumericProperty(Property::BorderColor, _RGB(222, 228, 238));
        Fill::create(menu.panel)->setPadding({6, 6, 6, 6}); menu.scroll = fenster::ScrollPane::create(); menu.scroll->setFixed(true, false); menu.panel->addChild(menu.scroll);
        auto content = Panel::create(); auto stack = Stack::create(content); stack->setOrientation(Orientation::Vertical); stack->setSpace(2); menu.scroll->setContent(content);
        menu.panel->setFocusable(true); keys(menu.panel, false); menu.height = 10;
        auto items = app.menuItems(i);
        for(size_t n = 0; n < items.size(); ++n) {
            auto item = items[n]; MenuRow row{}; row.panel = panel(content); fixed(row.panel, -1, item.command == Command::None ? 10 : 30); menu.height += item.command == Command::None ? 12 : 32;
            if(item.command == Command::None) { Fill::create(row.panel)->setPadding({5, 30, 4, 4}); panel(row.panel, _RGB(233, 237, 244)); menu.rows.push_back(row); continue; }
            row.panel->setNumericProperty(Property::CornerRadius, 5); row.panel->setNumericProperty(Property::HoverBackground, _RGB(241, 244, 251)); row.panel->setNumericProperty(Property::SelectedBackground, _RGB(233, 237, 255));
            auto line = flex(row.panel, Orientation::Horizontal, {0, 10, 0, 4}, 10);
            auto iconHost = panel(row.panel); Fill::create(iconHost)->setPadding({7, 0, 7, 0}); part(line, iconHost, 16); row.icon = fenster::Image::create(); iconHost->addChild(row.icon);
            row.title = label(row.panel, item.title); part(line, row.title, 0, 1, 1);
            auto shortcut = label(row.panel, item.shortcut, 12, _RGB(115, 128, 148)); shortcut->setAlignment(fenster::TextAlignment::RIGHT); part(line, shortcut, 86);
            row.panel->addMouseListener([this, i, n](fenster::ComponentMouseEvent* mouse) {
                if(mouse->type == fenster::MouseEventType::Press && (mouse->buttons & fenster::MouseButton::Button1)) { UiEvent event; event.kind = UiKind::MenuAction; event.menu = i; event.index = n; post(event); }
                if(mouse->type == fenster::MouseEventType::Enter) { UiEvent event; event.kind = UiKind::MenuHover; event.menu = i; event.index = n; post(event); }
            }); menu.rows.push_back(row);
        }
    }
    void buildDialog()
    {
        dialogWindow = Window::create(); dialogWindow->setShowInTaskbar(false); dialogWindow->setTitle("Editor");
        dialogWindow->onClose([this] { emit(UiKind::DialogClose); }); keys(dialogWindow, true);
        dialogLayout = flex(dialogWindow, Orientation::Vertical, {16, 20, 16, 20}, 8);
        intro = label(dialogWindow); part(dialogLayout, intro, 84);
        browser = panel(dialogWindow); auto browsing = flex(browser, Orientation::Vertical, {}, 10); part(dialogLayout, browser, 0, 1, 1);
        auto navigation = panel(browser); auto navigationLayout = flex(navigation, Orientation::Horizontal, {}, 8); part(browsing, navigation, 36);
        location = field(navigation, "Folder path", Icon::Folder); part(navigationLayout, location, 0, 1, 1); keys(location, true);
        auto up = button(navigation, "", Icon::Up, UiKind::DialogCommand, Command::BrowseUp); up->setTooltip("Parent folder  (Alt+Up)"); part(navigationLayout, up, 32);
        filter = field(navigation, "Filter by name", Icon::Search); part(navigationLayout, filter, 176); keys(filter, true);
        auto body = panel(browser); auto bodyLayout = flex(body, Orientation::Horizontal, {}, 12); part(browsing, body, 0, 1, 1);
        auto sidebar = panel(body, _RGB(247, 249, 252)); sidebar->setNumericProperty(Property::CornerRadius, 8); auto side = flex(sidebar, Orientation::Vertical, {8, 4, 8, 4}, 4); part(bodyLayout, sidebar, 136);
        auto heading = label(sidebar, "LOCATIONS", 11, _RGB(139, 150, 168)); part(side, heading, 24);
        const char* names[] = {"Home", "Desktop", "Computer"}; const char* paths[] = {"/user", "/user/desktop", "/"}; const Icon icons[] = {Icon::Home, Icon::Desktop, Icon::Computer};
        for(int i = 0; i < 3; ++i) {
            auto place = Button::create(); place->setTitle(names[i]); place->setIcon(icons[i]); place->setVariant(fenster::ButtonVariant::Ghost); place->setNumericProperty(Property::Alignment, uint32_t(fenster::TextAlignment::LEFT)); sidebar->addChild(place); part(side, place, 34);
            std::string path = paths[i]; place->setActionListener([this, path] { emit(UiKind::Place, Command::None, 0, path); }); keys(place, true); places.push_back({place, path});
        }
        spacer(sidebar, side);
        fileArea = panel(body, _RGB(255, 255, 255)); Fill::create(fileArea); part(bodyLayout, fileArea, 0, 1, 1);
        fileScroll = fenster::ScrollPane::create(); fileScroll->setFixed(true, false); fileArea->addChild(fileScroll);
        fileContent = Panel::create(); fileContent->setFocusable(true); auto files = Stack::create(fileContent); files->setOrientation(Orientation::Vertical); files->setPadding({6, 0, 6, 0}); files->setSpace(2); keys(fileContent, true); fileScroll->setContent(fileContent);
        fileContent->setBoundsListener([this](fenster::Rectangle bounds) { UiEvent event; event.kind = UiKind::FileResize; event.bounds = bounds; event.epoch = epoch.load(); post(event); });
        emptyFiles = label(fileArea, "This folder is empty", 14, _RGB(139, 150, 168)); emptyFiles->setAlignment(fenster::TextAlignment::CENTER);
        form = panel(dialogWindow); auto forms = Stack::create(form); forms->setOrientation(Orientation::Vertical); forms->setSpace(8); part(dialogLayout, form, 36);
        for(int i = 0; i < 2; ++i) {
            fieldRows[i] = panel(form); fixed(fieldRows[i], -1, 36); auto row = flex(fieldRows[i], Orientation::Horizontal, {}, 10);
            fieldLabels[i] = label(fieldRows[i]); part(row, fieldLabels[i], 96);
            fields[i] = field(fieldRows[i], ""); part(row, fields[i], 0, 1, 1); keys(fields[i], true);
        }
        options = panel(dialogWindow); auto choices = flex(options, Orientation::Horizontal); part(dialogLayout, options, 32);
        matchCase = button(options, "Match case", Icon::None, UiKind::DialogCommand, Command::MatchCase); part(choices, matchCase, 136); spacer(options, choices);
        preview = label(dialogWindow, "", 18); part(dialogLayout, preview, 44);
        notice = label(dialogWindow, "", 12, _RGB(115, 128, 148)); notice->setVisible(false); part(dialogLayout, notice, 40);
        dialogSpacer = panel(dialogWindow); part(dialogLayout, dialogSpacer, 0, 1, 1);
        auto buttons = panel(dialogWindow); auto buttonsLayout = flex(buttons, Orientation::Horizontal, {}, 8); part(dialogLayout, buttons, 34);
        fileSummary = label(buttons, "", 12, _RGB(115, 128, 148)); part(buttonsLayout, fileSummary, 0, 1, 1);
        const std::pair<Command, const char*> commands[] = {{Command::Smaller, "Smaller"}, {Command::Larger, "Larger"}, {Command::Accept, "OK"}, {Command::ReplaceOne, "Replace"}, {Command::ReplaceAll, "Replace all"}, {Command::ConfirmSave, "Save"}, {Command::Discard, "Don't save"}, {Command::ConfirmOverwrite, "Replace"}, {Command::Cancel, "Cancel"}};
        for(auto item : commands) {
            auto action = button(buttons, item.second, Icon::None, UiKind::DialogCommand, item.first); action->setVariant(fenster::ButtonVariant::Default);
            part(buttonsLayout, action, item.first == Command::ReplaceAll || item.first == Command::Discard ? 100 : 88); actions[item.first] = action;
        }
    }
    bool fileDialog() const { return app.dialog == Dialog::Open || app.dialog == Dialog::Save; }
    void readFields()
    {
        for(int i = 0; i < app.fieldCount(); ++i) { auto value = fields[i]->getTitle(); app.fields[i].reset(value); lastFields[i] = value; }
    }
    void syncFields(bool force = false)
    {
        for(int i = 0; i < app.fieldCount(); ++i) if(force || app.fields[i].text != lastFields[i]) {
            fields[i]->setTitle(app.fields[i].text); lastFields[i] = app.fields[i].text;
            if(force) fields[i]->setNumericProperty(Property::SelectAll, true);
        }
    }
    bool ensureRows(size_t count)
    {
        if(count <= rows.size()) return true;
        size_t missing = count - rows.size();
        fenster::ComponentUpdateBatch batch; fenster::ComponentUpdateBatch::Scope scope(batch);
        for(size_t i = 0; i < missing; ++i) {
            auto row = std::make_unique<FileRow>(); row->panel = Panel::create(); auto iconHost = Panel::create();
            row->icon = fenster::Image::create(); row->name = Label::create(); row->kind = Label::create();
            if(!row->panel || !iconHost || !row->icon || !row->name || !row->kind) return false;
            batch.addChild(fileContent, row->panel); batch.addChild(row->panel, iconHost); batch.addChild(iconHost, row->icon);
            batch.addChild(row->panel, row->name); batch.addChild(row->panel, row->kind);
            batch.setNumericProperty(row->panel, Property::Visible, false);
            batch.setMinimumSize(row->panel, {0, 34});
            batch.setNumericProperty(row->panel, Property::CornerRadius, 6);
            batch.setNumericProperty(row->panel, Property::HoverBackground, _RGB(244, 246, 251));
            batch.setNumericProperty(row->panel, Property::SelectedBackground, _RGB(233, 237, 255));
            auto line = flex(row->panel, Orientation::Horizontal, {0, 10, 0, 10}, 10);
            Fill::create(iconHost)->setPadding({7, 0, 7, 0});
            part(line, iconHost, 20); part(line, row->name, 0, 1, 1); part(line, row->kind, 82);
            batch.setNumericProperty(row->name, Property::Color, _RGB(48, 57, 74));
            batch.setNumericProperty(row->kind, Property::FontSize, 12); batch.setNumericProperty(row->kind, Property::Color, _RGB(115, 128, 148));
            auto stable = row.get(); batch.addMouseListener(row->panel, [this, stable](fenster::ComponentMouseEvent* mouse) {
                if(mouse->type != fenster::MouseEventType::Press || !(mouse->buttons & fenster::MouseButton::Button1)) return;
                fenster::platformAcquireMutex(rowLock); auto path = stable->path; fenster::platformReleaseMutex(rowLock);
                if(!path.empty()) emit(mouse->clickCount == 2 ? UiKind::FileActivate : UiKind::FileSelect, Command::None, 0, path);
            }); rows.push_back(std::move(row));
        }
        return batch.commit();
    }

    void syncFiles(bool reset = false)
    {
        if(!fileDialog()) return;
        fenster::ComponentUpdateBatch batch; fenster::ComponentUpdateBatch::Scope scope(batch);
        bool changedDirectory = lastDirectory != app.directory;
        if(changedDirectory) { lastDirectory = app.directory; location->setTitle(app.directory); fileQuery.clear(); filter->setTitle(""); reset = true; }
        visible.clear();
        for(size_t i = 0; i < app.entries.size(); ++i) if(fileQuery.empty() || fold(app.entries[i].name).find(fold(fileQuery)) != std::string::npos) visible.push_back(i);
        if(!ensureRows(visible.size())) { visible.clear(); app.message = "Unable to show files. Enter a full path instead."; }
        fenster::platformAcquireMutex(rowLock);
        for(size_t i = 0; i < rows.size(); ++i) {
            auto& row = *rows[i]; bool active = i < visible.size();
            if(row.active != active) { batch.setNumericProperty(row.panel, Property::Visible, active); row.active = active; }
            if(!active) { row.path.clear(); continue; }
            auto& entry = app.entries[visible[i]]; row.path = resolvePath(app.directory, entry.name);
            int limit = std::max(8, (std::max(200, fileWidth) - 160) / 8); std::string name; int length = 0;
            for(size_t p = 0; p < entry.name.size() && length < limit; p = next(entry.name, p), ++length) name.append(entry.name, p, next(entry.name, p) - p);
            if(name.size() < entry.name.size()) name += "...";
            if(row.title != name) { batch.setStringProperty(row.name, Property::Title, name); row.title = name; }
            if(row.tooltip != entry.name) { batch.setStringProperty(row.panel, Property::Tooltip, entry.name); row.tooltip = entry.name; }
            bool application = !entry.directory && entry.name.size() >= 4 && entry.name.substr(entry.name.size() - 4) == ".bin";
            auto image = entry.directory ? Icon::Folder : application ? Icon::Applications : Icon::Document;
            std::string type = entry.directory ? "Folder" : application ? "Application" : "File";
            bool selected = app.fileSelected == visible[i];
            if(row.image != image) { batch.setNumericProperty(row.icon, Property::Icon, uint32_t(image)); row.image = image; }
            if(row.type != type) { batch.setStringProperty(row.kind, Property::Title, type); row.type = type; }
            if(row.selected != selected) { batch.setNumericProperty(row.panel, Property::Selected, selected); row.selected = selected; }
        }
        batch.commit(); fenster::platformReleaseMutex(rowLock);
        emptyFiles->setTitle(fileQuery.empty() ? "This folder is empty" : "No matching files"); emptyFiles->setVisible(visible.empty());
        for(auto& place : places) place.first->setSelected(place.second == app.directory);
        if(reset) fileScroll->setNumericProperty(Property::ScrollOffsetY, 0);
    }
    void revealSelection()
    {
        for(size_t i = 0; i < visible.size(); ++i) if(visible[i] == app.fileSelected) {
            auto bounds = rows[i]->panel->getBounds(); uint32_t offset = 0; fileScroll->getNumericProperty(Property::ScrollOffsetY, &offset);
            int height = fileScroll->getBounds().height;
            if(bounds.y < int(offset)) fileScroll->setNumericProperty(Property::ScrollOffsetY, bounds.y);
            else if(bounds.y + bounds.height > int(offset) + height) fileScroll->setNumericProperty(Property::ScrollOffsetY, bounds.y + bounds.height - height);
        }
    }
    void syncMenus()
    {
        bool changed = app.menu != lastMenu;
        if(changed) menuLayer->setVisible(app.menu >= 0);
        for(size_t i = 0; i < menus.size(); ++i) {
            auto& menu = menus[i]; bool open = int(i) == app.menu;
            if(open && !menu.panel) buildMenu(menu, i);
            if(changed) { if(menu.panel) menu.panel->setVisible(open); menu.button->setSelected(open); }
            if(!open) continue;
            auto anchor = menu.button->getBounds();
            menu.panel->setBounds({std::min(anchor.x, std::max(0, rootWidth - 310)), 0, 310, std::min(menu.height, std::max(40, rootHeight - 36))});
            auto items = app.menuItems(i);
            for(size_t n = 0; n < items.size(); ++n) {
                auto& row = menu.rows[n]; auto item = items[n]; if(item.command == Command::None) continue;
                row.title->setColor(item.enabled ? _RGB(48, 57, 74) : _RGB(165, 175, 190));
                row.icon->setNumericProperty(Property::Icon, uint32_t(item.checked ? Icon::Check : commandIcon(item.command)));
                row.panel->setNumericProperty(Property::Selected, int(n) == app.menuSelection && item.enabled);
            }
            if(changed) { menu.scroll->setNumericProperty(Property::ScrollOffsetY, 0); focus(menu.panel); }
            if(lastSelection != app.menuSelection && app.menuSelection >= 0) {
                auto bounds = menu.rows[app.menuSelection].panel->getBounds(); menu.scroll->setNumericProperty(Property::ScrollOffsetY, std::max(0, bounds.y + bounds.height - menu.panel->getBounds().height + 12));
            }
        }
        if(changed && app.menu < 0 && app.dialog == Dialog::None) focus(canvas);
        lastMenu = app.menu; lastSelection = app.menuSelection;
    }
    void syncDialog()
    {
        bool changed = app.dialog != shownDialog;
        if(!changed && app.dialog == Dialog::None) return;
        if(changed) {
            if(!dialogWindow) buildDialog();
            ++epoch; dialogWindow->setVisible(false); shownDialog = app.dialog;
            if(app.dialog == Dialog::None) { focus(canvas); return; }
            const char* title = app.dialog == Dialog::Open ? "Open file" : app.dialog == Dialog::Save ? "Save as" : app.dialog == Dialog::Find ? "Find" : app.dialog == Dialog::Replace ? "Replace" : app.dialog == Dialog::GoTo ? "Go to line" : app.dialog == Dialog::Font ? "Font size" : app.dialog == Dialog::About ? "About Editor" : app.dialog == Dialog::Overwrite ? "Replace file?" : app.dialog == Dialog::Unsaved ? "Save changes?" : "Editor";
            dialogWindow->setTitle(std::string(title) + " — Editor"); bool file = fileDialog();
            int width = file ? 740 : app.dialog == Dialog::Replace ? 580 : 540;
            int height = file ? 540 : app.dialog == Dialog::Replace ? 310 : app.dialog == Dialog::About || app.dialog == Dialog::Error ? 300 : 270;
            auto parent = window->getBounds(); dialogWindow->setMinimumSize(file ? fenster::Dimension(640, 440) : fenster::Dimension(width, height)); dialogWindow->setResizable(file);
            dialogWindow->setBounds({std::max(0, parent.x + (parent.width - width) / 2), std::max(0, parent.y + (parent.height - height) / 2), width, height});
            browser->setVisible(file); form->setVisible(app.fieldCount() > 0);
            fieldRows[0]->setVisible(app.fieldCount() > 0); fieldRows[1]->setVisible(app.fieldCount() == 2);
            part(dialogLayout, form, app.fieldCount() == 2 ? 80 : 36);
            fieldLabels[0]->setTitle(file ? "File name" : app.dialog == Dialog::GoTo ? "Line number" : "Find"); fieldLabels[1]->setTitle("Replace with");
            options->setVisible(app.dialog == Dialog::Find || app.dialog == Dialog::Replace);
            preview->setVisible(app.dialog == Dialog::Font); intro->setVisible(app.fieldCount() == 0);
            part(dialogLayout, intro, app.dialog == Dialog::Font ? 28 : app.dialog == Dialog::About ? 112 : app.dialog == Dialog::Error ? 100 : 64);
            dialogSpacer->setVisible(!file);
            if(file) {
                if(app.dialog == Dialog::Save && !app.fields[0].text.empty() && app.fields[0].text[0] == '/')
                    app.browseDirectory(parentPath(app.fields[0].text));
                app.fields[0].reset(app.dialog == Dialog::Save ? baseName(app.fields[0].text) : "");
                lastDirectory.clear(); syncFiles(true);
            }
            auto commands = app.dialogCommands();
            for(auto& item : actions) item.second->setVisible(std::find(commands.begin(), commands.end(), item.first) != commands.end());
            std::string accept = app.dialog == Dialog::Open ? "Open" : app.dialog == Dialog::Save ? "Save" : app.dialog == Dialog::Find || app.dialog == Dialog::Replace ? "Find next" : app.dialog == Dialog::GoTo ? "Go to" : "OK";
            actions[Command::Accept]->setTitle(accept);
            for(auto& item : actions) item.second->setSelected(item.first == Command::Accept || item.first == Command::ConfirmSave || item.first == Command::ConfirmOverwrite);
            focusOrder.clear(); if(file) { focusOrder.push_back(location); focusOrder.push_back(filter); focusOrder.push_back(fileContent); }
            for(int i = 0; i < app.fieldCount(); ++i) focusOrder.push_back(fields[i]);
            for(auto command : commands) {
                if(command == Command::MatchCase) focusOrder.push_back(matchCase);
                else if(actions.count(command)) focusOrder.push_back(actions[command]);
            }
            syncFields(true);
        }
        if(fileDialog() && lastDirectory != app.directory) {
            if(app.dialog == Dialog::Open && app.fields[0].text == (app.directory == "/" ? "/" : app.directory + "/")) app.fields[0].reset("");
            syncFiles(true);
        }
        std::string description = app.dialog == Dialog::Unsaved ? "Save changes to " + (app.path.empty() ? std::string("Untitled") : baseName(app.path)) + " before continuing?" : app.dialog == Dialog::Overwrite ? "This file already exists. Replace it with the current document?" : app.dialog == Dialog::About ? "Ghost Editor\nA plain text editor for Ghost.\nUTF-8 and UTF-16 · Windows and Unix line endings\nClipboard text is shared within this Editor." : app.dialog == Dialog::Font ? "Consolas · " + std::to_string(app.fontSize) + " pt" : app.dialog == Dialog::Error ? app.message : "";
        title(intro, wrapped(description), lastIntro);
        title(preview, "The quick brown fox jumps over the lazy dog.", lastPreview); if(previousFont != app.fontSize) { preview->setNumericProperty(Property::FontSize, app.fontSize); previousFont = app.fontSize; }
        title(fileSummary, fileDialog() ? "All files · " + std::to_string(visible.size()) + " items" : "", lastFileSummary);
        bool showNotice = (!fileDialog() && app.fieldCount() > 0) || (!app.message.empty() && app.dialog != Dialog::Error);
        if(showNotice != noticeShown) { notice->setVisible(showNotice); noticeShown = showNotice; }
        std::string help = app.message.empty() && (app.dialog == Dialog::Find || app.dialog == Dialog::Replace) ? "Search wraps around the document." : app.message;
        title(notice, wrapped(help, 70), lastNotice); if(previousMatchCase != app.matchCase) { matchCase->setSelected(app.matchCase); matchCase->setIcon(app.matchCase ? Icon::Check : Icon::None); previousMatchCase = app.matchCase; } syncFields();
        if(changed) { dialogWindow->setVisible(true); focusDialog(); }
    }
public:
    fenster::Canvas* canvas;
    EditorUi(Notepad& state, Window* mainWindow, std::function<void(UiEvent)> sink) : app(state), window(mainWindow), post(std::move(sink))
    {
        fenster::ComponentUpdateBatch batch; fenster::ComponentUpdateBatch::Scope scope(batch);
        Fill::create(window); root = panel(window, _RGB(255, 255, 255)); Fill::create(root);
        auto column = panel(root, _RGB(255, 255, 255)); auto frame = flex(column, Orientation::Vertical);
        buildMenus(column, frame);
        auto tools = panel(column, _RGB(250, 251, 253)); auto toolLayout = flex(tools, Orientation::Horizontal, {7, 12, 7, 12}, 4); part(frame, tools, 46);
        const std::pair<Command, const char*> commands[] = {{Command::New, "New  (Ctrl+N)"}, {Command::Open, "Open  (Ctrl+O)"}, {Command::Save, "Save  (Ctrl+S)"}, {Command::Undo, "Undo  (Ctrl+Z)"}, {Command::Redo, "Redo  (Ctrl+Y)"}, {Command::Find, "Find  (Ctrl+F)"}};
        for(auto item : commands) {
            if(item.first == Command::Undo || item.first == Command::Find) spacer(tools, toolLayout, 8, 0);
            auto control = button(tools, "", commandIcon(item.first), UiKind::MainCommand, item.first); control->setTooltip(item.second); part(toolLayout, control, 32); toolbar[item.first] = control;
        }
        spacer(tools, toolLayout); auto type = label(tools, "Plain text", 12, _RGB(139, 150, 168)); part(toolLayout, type, 78);
        canvas = fenster::Canvas::create(); canvas->setFocusable(true); column->addChild(canvas); part(frame, canvas, 0, 1, 1);
        footer = panel(column, _RGB(249, 250, 253)); auto status = flex(footer, Orientation::Horizontal, {0, 14, 0, 14}, 12); part(frame, footer, 26);
        position = label(footer, "", 12, _RGB(115, 128, 148)); part(status, position, 0, 1, 1); format = label(footer, "", 12, _RGB(115, 128, 148)); part(status, format, 260);
        root->setBoundsListener([this](fenster::Rectangle bounds) { UiEvent event; event.kind = UiKind::RootResize; event.bounds = bounds; post(event); });
    }
    static void focus(Component* component)
    {
        fenster::CommandFocusRequest request; request.header.id = fenster::ProtocolCommandId::Focus; request.id = component->getId();
        auto tx = fenster::platformCreateMessageTransaction(); if(!fenster::sendUiMessage(fenster::DelegateTaskId, &request, sizeof(request), tx)) return;
        uint8_t response[SYS_MESSAGE_HEADER_SIZE + sizeof(fenster::CommandFocusResponse)]; fenster::platformReceiveMessage(response, sizeof(response), tx);
    }
    void focusDialog()
    {
        if(app.dialog == Dialog::None || shownDialog != app.dialog) return;
        if(app.fieldCount()) focus(fields[0]);
        else if(app.dialog == Dialog::Unsaved) focus(actions[Command::ConfirmSave]);
        else if(app.dialog == Dialog::Overwrite) focus(actions[Command::Cancel]);
        else focus(actions[Command::Accept]);
    }
    void sync()
    {
        fenster::ComponentUpdateBatch batch; fenster::ComponentUpdateBatch::Scope scope(batch);
        if(previousStatus != app.showStatus) { footer->setVisible(app.showStatus); previousStatus = app.showStatus; }
        title(position, "Ln " + std::to_string(app.document.lineNumber()) + ", Col " + std::to_string(app.document.column(app.document.cursor) + 1), lastPosition);
        title(format, std::string(app.format.endingName()) + "    ·    " + app.format.encodingName(), lastFormat);
        setEnabled(toolbar[Command::Undo], app.document.canUndo()); setEnabled(toolbar[Command::Redo], app.document.canRedo());
        syncMenus(); syncDialog();
    }
    bool handle(const UiEvent& event)
    {
        fenster::ComponentUpdateBatch batch; fenster::ComponentUpdateBatch::Scope scope(batch);
        using fenster::ComponentProperty;
        if(event.kind == UiKind::RootResize) { rootWidth = event.bounds.width; rootHeight = event.bounds.height; syncMenus(); return false; }
        if(event.kind == UiKind::MainCommand || event.kind == UiKind::Menu || event.kind == UiKind::MenuAction || event.kind == UiKind::MenuHover) {
            if(app.dialog != Dialog::None) { focusDialog(); return false; }
            if(event.kind == UiKind::MainCommand) { app.command(event.command); if(app.dialog == Dialog::None) focus(canvas); }
            else if(event.kind == UiKind::Menu) { if(event.index < 0 || app.menu == event.index) app.menu = -1; else app.openMenu(event.index); }
            else if(event.menu == app.menu && app.menu >= 0) {
                auto items = app.menuItems(app.menu); if(event.index < 0 || event.index >= int(items.size())) return false;
                if(event.kind == UiKind::MenuHover) app.menuSelection = event.index;
                else if(items[event.index].enabled) app.command(items[event.index].command);
            }
            return true;
        }
        if(event.kind == UiKind::MainKey) return false; // Main key translation lives in editor.cpp.
        if(event.epoch != epoch.load() || app.dialog == Dialog::None) return false;
        if(event.kind == UiKind::FileResize) { if(event.bounds.width != fileWidth) { fileWidth = event.bounds.width; syncFiles(); } return false; }
        if(event.kind == UiKind::DialogClose) { app.command(Command::Cancel); return true; }
        if(event.kind == UiKind::Place) { readFields(); app.browseDirectory(event.text); syncFiles(true); syncFields(); return true; }
        if(event.kind == UiKind::FileSelect || event.kind == UiKind::FileActivate) {
            if(!fileDialog()) return false;
            for(size_t i = 0; i < app.entries.size(); ++i) if(resolvePath(app.directory, app.entries[i].name) == event.text) {
                app.fileSelected = i;
                if(event.kind == UiKind::FileActivate && app.entries[i].directory) { readFields(); app.browseDirectory(event.text); syncFiles(true); }
                else {
                    if(!app.entries[i].directory || app.dialog == Dialog::Open) app.fields[0].reset(event.text);
                    if(event.kind == UiKind::FileActivate) app.command(Command::Accept);
                }
                syncFields(); syncFiles(); focus(fileContent); return true;
            }
            return false;
        }
        if(event.kind == UiKind::DialogCommand) {
            readFields();
            if(event.command == Command::BrowseUp && fileDialog()) { app.browseDirectory(parentPath(app.directory)); syncFiles(true); }
            else {
                auto commands = app.dialogCommands();
                if(std::find(commands.begin(), commands.end(), event.command) == commands.end()) return false;
                app.command(event.command);
            }
            syncFields(); return true;
        }
        if(event.kind == UiKind::DialogKey) {
            int scan = event.key.scancode & 0xff;
            if(scan == 1 || (event.key.alt && scan == 62) || (event.key.ctrl && scan == 17)) { app.command(Command::Cancel); return true; }
            if(scan == 15 && !focusOrder.empty()) {
                int current = -1; for(size_t i = 0; i < focusOrder.size(); ++i) if(int(focusOrder[i]->getId()) == event.index) current = i;
                auto next = focusOrder[(current + int(focusOrder.size()) + (event.key.shift ? -1 : 1)) % focusOrder.size()];
                focus(next); return false;
            }
            if(fileDialog()) {
                if(event.key.ctrl && scan == 38) { focus(location); location->setNumericProperty(Property::SelectAll, true); return false; }
                if(event.key.ctrl && scan == 33) { focus(filter); filter->setNumericProperty(Property::SelectAll, true); return false; }
                if(event.key.alt && scan == 72) { readFields(); app.browseDirectory(parentPath(app.directory)); syncFiles(true); syncFields(); return true; }
                if(event.index == int(filter->getId())) {
                    auto value = filter->getTitle(); if(value != fileQuery) { fileQuery = value; syncFiles(true); }
                    if(scan == 28) focus(fileContent); return true;
                }
                if(scan == 28 && event.index == int(location->getId())) { readFields(); app.browseDirectory(location->getTitle()); location->setTitle(app.directory); syncFiles(true); syncFields(); return true; }
                if((scan == 72 || scan == 80) && event.index != int(location->getId()) && !visible.empty()) {
                    auto found = std::find(visible.begin(), visible.end(), app.fileSelected); int index = found == visible.end() ? 0 : std::max(0, std::min(int(visible.size()) - 1, int(found - visible.begin()) + (scan == 72 ? -1 : 1)));
                    app.fileSelected = visible[index]; app.fields[0].reset(resolvePath(app.directory, app.entries[app.fileSelected].name)); syncFields(); syncFiles(); revealSelection(); focus(fileContent); return true;
                }
            }
            if(scan == 28) {
                readFields();
                if(fileDialog() && app.fileSelected >= 0 && event.index == int(fileContent->getId()) && app.entries[app.fileSelected].directory) { app.browseDirectory(resolvePath(app.directory, app.entries[app.fileSelected].name)); syncFiles(true); syncFields(); }
                else app.command(Command::Accept);
                return true;
            }
        }
        return false;
    }
};
}
#endif
