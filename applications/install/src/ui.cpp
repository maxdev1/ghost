/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "ui.hpp"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <ghost.h>
#include <libfenster/application.hpp>
#include <libfenster/component_update_batch.hpp>
#include <libfenster/components/button.hpp>
#include <libfenster/components/label.hpp>
#include <libfenster/components/list.hpp>
#include <libfenster/components/panel.hpp>
#include <libfenster/components/progress_bar.hpp>
#include <libfenster/components/window.hpp>
#include "install.hpp"
#include "ui_model.hpp"

using namespace fenster;

enum InstallUiAction : unsigned
{
	InstallUiSelect = 1,
	InstallUiNext = 2,
	InstallUiBack = 4,
	InstallUiClose = 8,
	InstallUiRefresh = 16,
	InstallUiConfirmAction = 32
};

class InstallUiButton : public Button, public FocusableComponent
{
public:
	explicit InstallUiButton(ComponentId id) :
		Component(id), TitledComponent(id), ActionComponent(id), Button(id), FocusableComponent(id)
	{
	}

	static InstallUiButton* installUiCreateButton()
	{
		return createComponent<InstallUiButton, ComponentType::Button>();
	}
};

int installUiRun()
{
	if(Application::open() != ApplicationOpenStatus::Success)
	{
		fprintf(stderr, "install: could not connect to the window server\n");
		return 1;
	}

	InstallUiModel model;
	std::atomic<unsigned> actions{0};
	auto post = [&](unsigned action) { actions.fetch_or(action); };
	auto window = Window::create();
	if(!window)
		return 1;

	window->setTitle("Install Ghost");
	window->setBounds({230, 140, 680, 520});
	window->setResizable(false);
	window->setMaximizeButtonVisible(false);
	auto panel = [&](Component* parent, Rectangle bounds)
	{
		auto result = Panel::create();
		result->setBounds(bounds);
		result->setBackground(_RGB(250, 251, 253));
		parent->addChild(result);
		return result;
	};

	auto root = panel(window, {0, 0, 660, 460});
	auto chooseView = panel(root, {0, 0, 660, 460});
	auto installView = panel(root, {0, 0, 660, 460});
	installView->setVisible(false);

	auto label = [&](Panel* parent, const char* text, Rectangle bounds, int size)
	{
		auto result = Label::create();
		result->setTitle(text);
		result->setBounds(bounds);
		result->setNumericProperty(ComponentProperty::FontSize, size);
		result->setColor(_RGB(43, 53, 72));
		parent->addChild(result);
		return result;
	};

	label(chooseView, "Choose a disk or volume", {28, 20, 604, 38}, 24);
	label(chooseView, "Choose an entire disk or an existing FAT32 volume.", {28, 61, 604, 28}, 14);

	auto drives = List::create();
	drives->setBounds({28, 104, 604, 192});
	drives->setEnabled(false);
	chooseView->addChild(drives);

	auto discovery = label(chooseView, "Looking for available drives...", {28, 310, 604, 28}, 14);
	auto warning = label(chooseView, "Disk targets are reformatted. Volume targets keep their filesystem.",
						 {28, 342, 604, 24}, 12);

	auto heading = label(installView, "Confirm installation", {28, 20, 604, 38}, 24);
	label(installView, "Destination for Ghost and your files.", {28, 61, 604, 28}, 14);

	auto destination = panel(installView, {28, 106, 604, 106});
	destination->setBackground(_RGB(255, 255, 255));
	destination->setNumericProperty(ComponentProperty::CornerRadius, 8);
	destination->setNumericProperty(ComponentProperty::BorderColor, _RGB(218, 224, 234));
	label(destination, "DESTINATION", {20, 12, 564, 22}, 11);
	auto driveName = label(destination, "", {20, 37, 564, 30}, 20);
	auto driveInfo = label(destination, "", {20, 72, 564, 22}, 13);

	auto status = label(installView, "All existing data on this drive will be lost.", {28, 232, 604, 28}, 14);
	auto detail = label(installView, "Choose Install to format this drive, or Back to choose another.",
						{28, 266, 604, 24}, 12);

	auto progress = ProgressBar::create();
	progress->setBounds({28, 312, 604, 14});
	installView->addChild(progress);
	auto phase = label(installView, "Ready to begin.", {28, 342, 604, 24}, 13);

	auto button = [&](Panel* parent, const char* title, Rectangle bounds)
	{
		auto result = InstallUiButton::installUiCreateButton();
		result->setTitle(title);
		result->setBounds(bounds);
		parent->addChild(result);
		return result;
	};

	auto refresh = button(chooseView, "Refresh", {28, 396, 110, 36});
	auto cancel = button(chooseView, "Cancel", {366, 396, 126, 36});
	auto next = button(chooseView, "Continue", {506, 396, 126, 36});

	auto back = button(installView, "Back", {366, 396, 126, 36});
	auto confirm = button(installView, "Install", {506, 396, 126, 36});
	next->setVariant(ButtonVariant::Primary);
	confirm->setVariant(ButtonVariant::Primary);
	next->setEnabled(false);
	refresh->setEnabled(false);

	drives->setActionListener([&] { post(InstallUiSelect); });
	next->setActionListener([&] { post(InstallUiNext); });
	confirm->setActionListener([&] { post(InstallUiConfirmAction); });
	back->setActionListener([&] { post(InstallUiBack); });
	cancel->setActionListener([&] { post(InstallUiClose); });
	refresh->setActionListener([&] { post(InstallUiRefresh); });
	window->onClose([&] { post(InstallUiClose); });

	window->setVisible(true);
	window->setFocused(true);

	std::vector<g_install_destination> targets;
	auto discover = [&]
	{
		model.stage = InstallUiStage::Discovering;
		model.selected = -1;

		drives->setEnabled(false);
		next->setEnabled(false);
		refresh->setEnabled(false);

		discovery->setTitle("Looking for available drives...");
		targets = installDiscoverDestinations();
		std::vector<std::string> rows;
		for(const auto& target: targets)
		{
			char row[256];
			if(target.volume.id)
				snprintf(row, sizeof(row), "    Volume %lu   partition %u   %lu MiB   %s", target.volume.id,
						 target.volume.partition, target.volume.sectorCount / 2048,
						 target.volume.filesystem[0] ? target.volume.filesystem : "unknown filesystem");
			else
				snprintf(row, sizeof(row), "Disk %u   %s   %lu MiB   (reformat)", target.disk.id,
						 target.disk.info.model, target.disk.info.sectorCount / 2048);
			rows.emplace_back(row);
		}

		if(!drives->setItems(rows))
		{
			drives->setItems({});
			targets.clear();
			discovery->setTitle("Could not display the drives. Try Refresh.");
		}
		else
			discovery->setTitle(targets.empty() ? "No storage targets found. Attach a disk and refresh."
												: "Select a disk or volume, then click Continue.");

		model.stage = InstallUiStage::Choose;
		drives->setEnabled(true);
		refresh->setEnabled(true);
	};

	discover();
	for(;;)
	{
		auto pending = actions.exchange(0);
		if(pending & InstallUiClose)
			return model.stage == InstallUiStage::Failed ? 1 : 0;
		if(pending & InstallUiBack)
		{
			if(model.stage != InstallUiStage::Confirm)
				return model.stage == InstallUiStage::Failed ? 1 : 0;

			model.installUiCancel();
			installView->setVisible(false);
			chooseView->setVisible(true);
			drives->setFocused(true);
			pending &= ~(InstallUiNext | InstallUiConfirmAction);
		}

		if((pending & InstallUiRefresh) && model.stage == InstallUiStage::Choose)
			discover();
		if((pending & InstallUiSelect) && model.stage == InstallUiStage::Choose)
		{
			bool selected = model.installUiSelect(drives->getSelectedIndex(), int(targets.size()));
			next->setEnabled(selected);
			if(selected)
				warning->setTitle(targets[model.selected].volume.id
										  ? "Matching files will be replaced. Other files and partitions are preserved."
										  : "All partitions and data on this disk will be replaced.");
		}

		if(pending & InstallUiNext)
		{
			if(model.installUiRequestConfirmation())
			{
				const auto& target = targets[model.selected];
				const auto& disk = target.disk;
				char information[96];
				if(target.volume.id)
					snprintf(information, sizeof(information), "Volume %lu   disk %u partition %u   %lu MiB",
							 target.volume.id, disk.id, target.volume.partition, target.volume.sectorCount / 2048);
				else
					snprintf(information, sizeof(information), "Disk %u   %lu MiB", disk.id,
							 disk.info.sectorCount / 2048);
				status->setTitle(target.volume.id ? "The selected volume will be used without formatting."
												  : "All partitions and data on this disk will be lost.");
				detail->setTitle(target.volume.id ? "Matching files and the disk bootloader will be replaced."
												  : "One MBR partition will be created and formatted to FAT32.");
				driveName->setTitle(disk.info.model);
				driveInfo->setTitle(information);
				chooseView->setVisible(false);
				installView->setVisible(true);
				back->setFocused(true);
			}
		}

		if(pending & InstallUiConfirmAction)
		{
			if(model.installUiConfirm())
			{
				window->setCloseButtonEnabled(false);
				confirm->setEnabled(false);
				back->setEnabled(false);
				heading->setTitle("Installing Ghost");
				status->setTitle("Installation in progress.");
				detail->setTitle("Keep the computer running until installation finishes.");
				std::string failure;
				unsigned previous = UINT32_MAX;
				std::string previousPhase;
				int result = installRun(targets[model.selected], true, &failure,
										[&](const char* current, uint64_t done, uint64_t total)
										{
											model.installUiProgress(current, done, total);
											if(model.progress == previous && previousPhase == current)
												return;

											previous = model.progress;
											previousPhase = current;
											ComponentUpdateBatch batch;
											ComponentUpdateBatch::Scope scope(batch);
											char message[128];
											snprintf(message, sizeof(message), "%s... %u%%", current, model.progress);
											phase->setTitle(message);
											progress->setValue(model.progress);
											batch.commit();
										});
				model.installUiFinish(result == 0);
				window->setCloseButtonEnabled(true);
				progress->setValue(model.progress);
				heading->setTitle(result == 0 ? "Installation complete" : "Installation failed");
				status->setTitle(result == 0 ? "Ghost is installed and verified."
											 : "Installation could not be completed.");
				detail->setTitle(result == 0 ? "Shut down, remove the ISO, then boot from this drive." : failure);
				phase->setTitle(result == 0 ? "Complete. 100%" : "Installation stopped.");
				confirm->setVisible(false);
				back->setTitle("Close");
				back->setBounds({506, 396, 126, 36});
				back->setVariant(ButtonVariant::Primary);
				back->setEnabled(true);
				back->setFocused(true);
				actions.store(0);
			}
		}

		g_sleep(20);
	}
}
