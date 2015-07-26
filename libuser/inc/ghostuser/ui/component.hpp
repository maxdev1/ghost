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

#ifndef GHOSTLIBRARY_UI_COMPONENT
#define GHOSTLIBRARY_UI_COMPONENT

#include <cstdint>
#include <ghost.h>
#include <ghost/utils/local.hpp>
#include <ghostuser/graphics/metrics/rectangle.hpp>
#include <ghostuser/ui/interface_specification.hpp>
#include <ghostuser/ui/ui.hpp>
#include <ghostuser/utils/logger.hpp>
#include <ghostuser/utils/value_placer.hpp>

/**
 *
 */
class g_component {
protected:
	uint32_t id;

	g_component(uint32_t id) :
			id(id) {
	}

	/**
	 *
	 */
	template<typename T>
	struct g_concrete: T {
		g_concrete(uint32_t id) :
				T(id) {
		}
	};

	/**
	 *
	 */
	template<typename COMPONENT_TYPE, uint32_t COMPONENT_CONSTANT>
	static COMPONENT_TYPE* createComponent() {

		if (!g_ui_ready) {
			return 0;
		}

		// write request
		uint32_t request_length = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_CREATE_COMPONENT_REQUEST_LENGTH;
		uint8_t request[request_length];
		g_value_placer request_builder(request);
		request_builder.put<g_ui_protocol_command_id>(G_UI_PROTOCOL_CREATE_COMPONENT);
		request_builder.put<uint32_t>(COMPONENT_CONSTANT);
		g_ui_transaction_id transaction = g_ui::send(request, request_length);

		// wait for response
		uint32_t response_length;
		uint8_t* response;
		if (g_ui::receive(transaction, &response, &response_length)) {
			g_local<uint8_t> response_deleter(response);

			g_value_placer response_reader(response);
			g_ui_protocol_command_id command = response_reader.get<g_ui_protocol_command_id>();
			g_ui_protocol_status status = response_reader.get<g_ui_protocol_status>();
			uint32_t component_id = response_reader.get<uint32_t>();

			if (command == G_UI_PROTOCOL_CREATE_COMPONENT && status == G_UI_PROTOCOL_SUCCESS) {
				return new g_concrete<COMPONENT_TYPE>(component_id);
			}
		}
		return 0;
	}

public:
	/**
	 *
	 */
	bool addChild(g_component* c);

	/**
	 *
	 */
	bool setBounds(g_rectangle rect);

	/**
	 *
	 */
	bool setVisible(bool visible);

};

#endif
