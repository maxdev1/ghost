/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "components/tree.hpp"
#include "components/tree_node.hpp"
#include "components/window.hpp"
#include "layout/stack_layout_manager.hpp"

#include <libjson/json.hpp>

tree_t::tree_t()
{
	auto layout = new stack_layout_manager_t();
	this->component_t::setLayoutManager(layout);
}

void tree_t::setModelFromJson(std::string& model)
{
	for(auto child: this->acquireChildren())
	{
		this->removeChild(child.component);
		delete child.component;
	}
	this->releaseChildren();

	g_json parser;
	auto json = parser.parse(model);

	if(!json.isObject())
	{
		klog("Tree model JSON was not an object");
		return;
	}
	auto jsonObject = json.asObject();

	auto rootNodes = jsonObject["rootNodes"];
	if(!rootNodes.isArray())
	{
		klog("Property 'rootNodes' in JSON model must be an array");
		return;
	}

	for(auto node: rootNodes.asArray())
	{
		auto nodeComponent = this->createNodeComponent(node);
		if(nodeComponent)
			this->addChild(nodeComponent);
	}
}

tree_node_t* tree_t::createNodeComponent(g_json_node& node)
{
	if(!node.isObject())
	{
		klog("Skipping non-object node in 'rootNodes'");
		return nullptr;
	}

	auto nodeObject = node.asObject();

	auto nodeComponent = new tree_node_t();

	auto titleValue = nodeObject["title"];
	if(titleValue.isString())
		nodeComponent->setTitle(titleValue.asString());

	// TODO Add ID

	auto childrenValue = nodeObject["children"];
	if(childrenValue.isArray())
	{
		for(auto child: childrenValue.asArray())
		{
			if(!child.isObject())
				continue;

			auto childComponent = this->createNodeComponent(child);
			if(childComponent)
				nodeComponent->addChild(childComponent);
		}
	}


	return nodeComponent;
}
