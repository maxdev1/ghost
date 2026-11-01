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

#ifndef GHOST_INSTALL_UI_MODEL
#define GHOST_INSTALL_UI_MODEL

#include <cstdint>
#include <cstring>

enum class InstallUiStage
{
	Discovering,
	Choose,
	Confirm,
	Installing,
	Complete,
	Failed
};

/**
 * Keeps the confirmed target fixed until the install operation finishes.
 */
class InstallUiModel
{
public:
	InstallUiStage stage = InstallUiStage::Discovering;
	int selected = -1;
	unsigned progress = 0;

	bool installUiSelect(int index, int count)
	{
		if(stage != InstallUiStage::Choose)
			return false;
		selected = index >= 0 && index < count ? index : -1;
		return selected >= 0;
	}

	bool installUiRequestConfirmation()
	{
		if(stage != InstallUiStage::Choose || selected < 0)
			return false;
		stage = InstallUiStage::Confirm;
		return true;
	}

	bool installUiConfirm()
	{
		if(stage != InstallUiStage::Confirm)
			return false;
		stage = InstallUiStage::Installing;
		progress = 0;
		return true;
	}

	void installUiCancel()
	{
		if(stage == InstallUiStage::Confirm)
			stage = InstallUiStage::Choose;
	}

	void installUiProgress(const char* phase, uint64_t done, uint64_t total)
	{
		if(stage != InstallUiStage::Installing)
			return;
		struct Range
		{
			const char* name;
			unsigned first, span;
		};
		const Range ranges[] = {{"Scanning the loaded filesystem", 0, 2},
								{"Unmounting target disk volumes", 2, 3},
								{"Replacing disk partition table", 5, 3},
								{"Creating target partition", 8, 2},
								{"Formatting FAT32", 10, 10},
								{"Waiting for filesystem", 20, 1},
								{"Mounting target volume", 21, 1},
								{"Checking free space", 22, 6},
								{"Checking destination files", 28, 2},
								{"Copying", 30, 35},
								{"Flushing target volume", 65, 3},
								{"Verifying", 68, 27},
								{"Unmounting target volume", 95, 1},
								{"Installing bootloader", 96, 2},
								{"Committing boot sector", 98, 1},
								{"Refreshing volumes", 99, 0}};
		for(const auto& range: ranges)
			if(!strcmp(phase, range.name))
			{
				auto value = range.first + (total ? unsigned((done < total ? done : total) * range.span / total) : 0);
				if(value > progress)
					progress = value;
				return;
			}
	}

	void installUiFinish(bool success)
	{
		stage = success ? InstallUiStage::Complete : InstallUiStage::Failed;
		if(success)
			progress = 100;
	}
};

#endif
