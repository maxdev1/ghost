#include "start_menu.hpp"
#include "helper.hpp"
#include <libfenster/application.hpp>
#include <libfenster/font/font_loader.hpp>
#include <libfenster/icons.hpp>

start_menu_t* start_menu_t::create(taskbar_t* dock, std::vector<desktop::StartEntry> applications,
                                  std::function<bool(const desktop::StartEntry&)> launch)
{
    auto menu = createCanvasComponent<start_menu_t>();
    if(menu) menu->init(dock, std::move(applications), std::move(launch));
    return menu;
}

void start_menu_t::init(taskbar_t* taskbar, std::vector<desktop::StartEntry> applications,
                        std::function<bool(const desktop::StartEntry&)> handler)
{
    dock = taskbar; launch = std::move(handler);
    model.configure(std::move(applications));
    font = FontLoader::getDefault(); textLayout = TextLayouter::initializeLayout();
    for(const auto& entry : model.entries) if(!entry.icon.empty() && !icons.count(entry.icon)) {
        auto surface = cairo_image_surface_create_from_png(entry.icon.c_str());
        if(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) icons[entry.icon] = surface;
        else cairo_surface_destroy(surface);
    }
    setVisible(false);
    setBufferListener([this] { paint(); });
    addMouseListener([this](ComponentMouseEvent* event) { onMouse(event); });
    addKeyListener([this](KeyEvent& event) { onKey(event); });
    addVisibleListener([this](bool visible) {
        open = visible;
        if(!visible) model.reset();
        dock->setStartMenuOpen(visible);
    });
    dock->addChild(this);
    dock->setStartMenuHandler([this](bool visible) { show(visible); });
}

start_menu_t::~start_menu_t()
{
    if(dock) dock->setStartMenuHandler({});
    for(auto& entry : icons) cairo_surface_destroy(entry.second);
    if(textLayout) TextLayouter::destroyLayout(textLayout);
}

void start_menu_t::show(bool visible)
{
    if(visible) {
        Dimension screen;
        if(!Application::getScreenDimension(screen)) return;
        auto taskbar = dock->getBounds();
        model.reset(); model.fit(screen.width, taskbar.y - 16);
        if(model.width < 100 || model.height < 100) return;
        if(!setBounds({8, taskbar.y - model.height - 8, model.width, model.height})) return;
    }
    if(setNumericProperty(ComponentProperty::DesktopPopup, visible)) {
        open = visible;
        dock->setStartMenuOpen(visible);
        if(visible) paint();
    }
}

void start_menu_t::activate(int index)
{
    if(index < 0 || index >= int(model.entries.size())) return;
    auto entry = model.entries[index];
    show(false);
    if(launch && !launch(entry)) {
        error = "Could not open " + entry.name;
        show(true);
    } else error.clear();
}

void start_menu_t::onMouse(ComponentMouseEvent* event)
{
    if(!open) return;
    if(event->type == MouseEventType::Move || event->type == MouseEventType::Enter || event->type == MouseEventType::Drag) {
        int selected = model.hit(event->position);
        if(selected != model.selected) { model.selected = selected; paint(); }
    } else if(event->type == MouseEventType::Leave) {
        model.selected = -1; paint();
    } else if(event->type == MouseEventType::Scroll) {
        model.wheel(event->scroll); model.selected = model.hit(event->position); paint();
    } else if(event->type == MouseEventType::Press && (event->buttons & MouseButton::Button1)) {
        model.pressed = model.selected = model.hit(event->position); paint();
    } else if(event->type == MouseEventType::Release && !(event->buttons & MouseButton::Button1)) {
        int pressed = model.pressed; model.pressed = -1;
        if(pressed >= 0 && pressed == model.hit(event->position)) activate(pressed);
        else paint();
    } else if(event->type == MouseEventType::DragRelease) {
        model.pressed = -1; paint();
    }
}

void start_menu_t::onKey(KeyEvent& event)
{
    if(!open || !event.info.pressed) return;
    switch(event.info.scancode & 0xff) {
        case 0x01: show(false); break;
        case 0x48: model.move(-1); paint(); break;
        case 0x50: model.move(1); paint(); break;
        case 0x0f: model.move(event.info.shift ? -1 : 1); paint(); break;
        case 0x1c: activate(model.selected); break;
        default: break;
    }
}

void start_menu_t::paint()
{
    if(!textLayout) return;
    auto bounds = getBounds();
    auto cr = acquireGraphics(); if(!cr) return;
    if(bounds.width > currentBuffer.width || bounds.height > currentBuffer.height) { releaseGraphics(); return; }
    cairo_save(cr); cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE); cairo_set_source_rgba(cr, 0, 0, 0, 0); cairo_paint(cr); cairo_restore(cr);
    for(int i = 0; i < 5; ++i) {
        cairo_set_source_rgba(cr, 0, 0, 0, .035);
        cairo_rounded_rectangle(cr, i + 1, i + 3, model.width - 2 * (i + 1), model.height - 2 * (i + 1), 17 - i); cairo_fill(cr);
    }
    auto gradient = cairo_pattern_create_linear(0, 0, 0, model.height);
    cairo_pattern_add_color_stop_rgb(gradient, 0, .10, .14, .215);
    cairo_pattern_add_color_stop_rgb(gradient, 1, .065, .09, .145);
    cairo_set_source(cr, gradient);
    cairo_rounded_rectangle(cr, 6, 6, model.width - 12, model.height - 12, 14); cairo_fill_preserve(cr);
    cairo_pattern_destroy(gradient);
    cairo_set_source_rgba(cr, .50, .65, .85, .28); cairo_set_line_width(cr, 1); cairo_stroke(cr);
    auto text = [&](const std::string& value, Rectangle box, int size, ColorArgb color) {
        if(!font || box.width <= 0) return;
        cairo_save(cr); cairo_rectangle(cr, box.x, box.y, box.width, box.height); cairo_clip(cr);
        TextLayouter::layout(cr, value.c_str(), font, size, {0, 0, box.width, box.height}, TextAlignment::LEFT, textLayout, false);
        for(auto& glyph : textLayout->positions) {
            if(glyph.line != 0 || glyph.controlChar != -1) continue;
            cairo_save(cr); cairo_set_source_rgba(cr, ARGB_TO_FPARAMS(color));
            TextLayouter::paintChar(cr, glyph, {box.x, box.y + (box.height - size - 4) / 2}); cairo_restore(cr);
        }
        cairo_restore(cr);
    };
    text("Applications", {26, 15, model.width - 52, 28}, 11, _RGB(144, 164, 194));
    if(!error.empty()) text(error, {26, 36, model.width - 52, 18}, 10, _RGB(255, 172, 177));
    cairo_save(cr); cairo_rectangle(cr, 12, 54, model.width - 24, model.height - 68); cairo_clip(cr);
    int places = 58 + model.applicationCount * 42 - model.scroll;
    cairo_set_source_rgba(cr, .5, .65, .85, .16); cairo_rectangle(cr, 24, places + 4, model.width - 48, 1); cairo_fill(cr);
    text("Places", {26, places + 9, model.width - 52, 24}, 11, _RGB(144, 164, 194));
    for(int i = 0; i < int(model.entries.size()); ++i) {
        auto box = model.row(i); if(box.y + box.height <= 54 || box.y >= model.height - 14) continue;
        const auto& entry = model.entries[i];
        if(model.selected == i) {
            cairo_set_source_rgb(cr, model.pressed == i ? .21 : .16, model.pressed == i ? .32 : .25, model.pressed == i ? .47 : .39);
            cairo_rounded_rectangle(cr, box.x, box.y, box.width, box.height, 8); cairo_fill(cr);
        }
        auto image = icons.find(entry.icon);
        if(image != icons.end()) {
            cairo_save(cr); cairo_translate(cr, box.x + 12, box.y + 8);
            cairo_scale(cr, 24. / cairo_image_surface_get_width(image->second), 24. / cairo_image_surface_get_height(image->second));
            cairo_set_source_surface(cr, image->second, 0, 0); cairo_paint(cr); cairo_restore(cr);
        } else {
            auto icon = i >= model.applicationCount ? (entry.arguments == "/user" ? Icon::Home : entry.arguments == "/" ? Icon::Computer : Icon::Desktop)
                        : entry.application.find("editor") != std::string::npos ? Icon::Document : Icon::Applications;
            paintIcon(cr, icon, box.x + 12, box.y + 8, 24, _RGB(148, 192, 229));
        }
        auto name = entry.name;
        std::replace(name.begin(), name.end(), '\n', ' '); std::replace(name.begin(), name.end(), '\r', ' ');
        text(name, {box.x + 48, box.y, box.width - 76, box.height}, 14, _RGB(230, 238, 251));
        if(i >= model.applicationCount) paintIcon(cr, Icon::ChevronRight, box.x + box.width - 26, box.y + 12, 16, _RGB(119, 147, 180));
    }
    cairo_restore(cr);
    blit({0, 0, model.width, model.height}); releaseGraphics();
}
