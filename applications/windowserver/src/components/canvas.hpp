/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __WINDOWSERVER_COMPONENTS_CANVAS__
#define __WINDOWSERVER_COMPONENTS_CANVAS__

#include "components/component.hpp"

struct buffer_info_t
{
    uint8_t* localMapping = nullptr;
    uint8_t* remoteMapping = nullptr;
    uint16_t pages;

    cairo_surface_t* surface = nullptr;
    uint16_t paintableWidth;
    uint16_t paintableHeight;
};

class canvas_t;

struct async_resizer_info_t
{
    bool alive;
    g_user_mutex lock;
    g_user_mutex checkAtom;
    canvas_t* canvas;
};

class canvas_t : public component_t
{
public:
    g_pid partnerProcess;
    g_tid partnerThread;

    async_resizer_info_t* asyncInfo;

    g_user_mutex bufferLock = g_mutex_initialize_r(true);
    buffer_info_t buffer{};
    g_rectangle bufferDirty = g_rectangle();

    canvas_t(g_tid partnerThread);
    virtual ~canvas_t();

    virtual bool handle()
    {
        return false;
    }

    virtual void handleBoundChange(g_rectangle oldBounds);

    static void asyncBufferResizer(async_resizer_info_t* info);

    void blit(graphics_t* out, const g_rectangle& absClip, const g_point& positionOnParent) override;

    void requestBlit(g_rectangle& area);

private:
    void createNewBuffer(g_rectangle& bounds, uint32_t size);
    void notifyClientAboutNewBuffer();

    void checkBuffer();
};

#endif
