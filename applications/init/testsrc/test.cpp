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

#include <ghost.h>

#include <ghostuser/utils/Logger.hpp>
#include <ghostuser/ui/Ui.hpp>

#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <list>
#include <sstream>

/**
 *
 */
void run_tests() {

	// Start MagicFS-driver
	launch_via_spawner("/applications/example-fs-driver.bin", "", G_SECURITY_LEVEL_DRIVER);
	g_sleep(100);

	// g_open_directory test
	{
		g_fs_open_directory_status open_dir_stat;
		g_fs_directory_iterator* iter = g_open_directory_s("/", &open_dir_stat);
		g_logger::log("g_open_directory root: result: %i, node: %i, position: %i", open_dir_stat, iter->node_id, iter->position);

		g_fs_directory_entry* entry;
		while ((entry = g_read_directory(iter)) != 0) {
			g_logger::log(std::string("g_read_directory: ") + entry->name);
		}

		g_close_directory(iter);
	}
	{
		g_fs_open_directory_status open_dir_stat;
		g_fs_directory_iterator* iter = g_open_directory_s("/i/am/hidden", &open_dir_stat);
		g_logger::log("g_open_directory: result: %i, node: %i, position: %i", open_dir_stat, iter->node_id, iter->position);

		g_fs_directory_entry* entry;
		while ((entry = g_read_directory(iter)) != 0) {
			g_logger::log(std::string("g_read_directory: ") + entry->name);
		}

		g_close_directory(iter);
	}
	{
		g_fs_open_directory_status open_dir_stat;
		g_fs_directory_iterator* iter = g_open_directory_s("/i/am/hidden", &open_dir_stat);
		g_logger::log("g_open_directory: result: %i, node: %i, position: %i", open_dir_stat, iter->node_id, iter->position);

		g_fs_directory_entry* entry;
		while ((entry = g_read_directory(iter)) != 0) {
			g_logger::log(std::string("g_read_directory: ") + entry->name);
		}

		g_close_directory(iter);
	}
	{
		g_fs_open_directory_status open_dir_stat;
		g_fs_directory_iterator* iter = g_open_directory_s("/testfs/lol", &open_dir_stat);
		g_logger::log("g_open_directory magic: result: %i, node: %i, position: %i", open_dir_stat, iter->node_id, iter->position);

		g_fs_directory_entry* entry;
		while ((entry = g_read_directory(iter)) != 0) {
			g_logger::log(std::string("g_read_directory: ") + entry->name);
		}

		g_close_directory(iter);
	}
	{
		g_fs_open_directory_status open_dir_stat;
		g_fs_directory_iterator* iter = g_open_directory_s("/testfs/lol", &open_dir_stat);
		g_logger::log("g_open_directory magic: result: %i, node: %i, position: %i", open_dir_stat, iter->node_id, iter->position);

		g_fs_directory_entry* entry;
		while ((entry = g_read_directory(iter)) != 0) {
			g_logger::log(std::string("g_read_directory: ") + entry->name);
		}

		g_close_directory(iter);
	}

	// g_open test
	g_logger::log("g_open: return %i", g_open("/test"));
	g_logger::log("g_open: return %i", g_open("/test.image"));
	g_logger::log("g_open: return %i", g_open("/applications/idle.bin"));

	char* cwd = new char[G_PATH_MAX];
	g_get_working_directory(cwd);
	std::stringstream s1;
	s1 << "my 1. wd: '" << cwd << "'";
	g_logger::log(s1.str());

	g_set_working_directory("/applications");
	g_get_working_directory(cwd);
	std::stringstream s2;
	s2 << "my 2. wd: '" << cwd << "'";
	g_logger::log(s2.str());

	g_set_working_directory("/allahu/akbar");
	g_get_working_directory(cwd);
	std::stringstream s3;
	s3 << "my 3. wd: '" << cwd << "'";
	g_logger::log(s3.str());

	/* Ramdisk read test */{

		g_logger::log("ramdisk: starting read test");
		g_fs_open_status open_status;
		g_fd test_file = g_open_fs("/system/keyboard/de-DE.layout", 0, &open_status);
		g_logger::log("ramdisk: file opened: %i, status: %i", test_file, open_status);

		// and again
		int32_t len;
		uint8_t* buf = new uint8_t[128];
		while ((len = g_read(test_file, buf, 128)) > 0) {
			std::stringstream str;
			for (int i = 0; i < len; i++) {
				str << (char) buf[i];
			}
			g_logger::log("ramdisk: read %i content: '" + str.str() + "'", len);
			g_logger::log("position before seek: %i", g_tell(test_file));
			g_seek(test_file, 50, G_FS_SEEK_CUR);
			g_logger::log("position then: %i", g_tell(test_file));
		}
		delete buf;

		// get length of ramdisk file:
		g_logger::log("length: %i", g_length(test_file));
		g_logger::log("length: %i", g_flength("/system/keyboard/de-DE.layout"));
		g_logger::log("length: %i", g_flength_s("/system/keyboard/de-DE.layout", false));

		g_fs_close_status closestatus = g_close(test_file);
		g_logger::log("ramdisk: closed with status: %i", closestatus);
	}

	/* Ramdisk length-seeking test */{

		g_logger::log("ramdisk: starting seek-end test");
		g_fs_open_status open_status;
		g_fd test_file = g_open_fs("/system/keyboard/de-DE.layout", 0, &open_status);
		g_logger::log("ramdisk: file opened: %i, status: %i", test_file, open_status);
		g_seek(test_file, 0, G_FS_SEEK_END);
		g_logger::log("ramdisk: file length is %i", g_tell(test_file));

		g_fs_close_status closestatus = g_close(test_file);
		g_logger::log("ramdisk: closed with status: %i", closestatus);
	}

	/* Clone-FD test */{

		g_logger::log("ramdisk: ### starting clone-fd test");
		g_fs_open_status open_status;
		g_fd test_file = g_open_fs("/system/keyboard/de-DE.layout", 0, &open_status);
		g_logger::log("ramdisk: file opened: %i, status: %i", test_file, open_status);

		g_fd clone = g_clone_fd(test_file, g_get_pid(), g_get_pid());

		// read
		int32_t len;
		uint8_t* buf = new uint8_t[128];
		g_logger::log("ramdisk: buf is at %x", buf);
		std::stringstream str;
		while ((len = g_read(test_file, buf, 128)) > 0) {
			for (int i = 0; i < len; i++) {
				str << (char) buf[i];
			}
			g_logger::log("ramdisk: tell: %i", g_tell(test_file));
		}
		g_logger::log("ramdisk: content: '" + str.str() + "'", len);
		delete buf;

		// and again
		g_logger::log("now will try to read from cloned fd %i, which is at %i", clone, g_tell(clone));
		buf = new uint8_t[128];
		g_logger::log("ramdisk: buf is at %x", buf);
		std::stringstream str2;
		while ((len = g_read(clone, buf, 128)) > 0) {
			for (int i = 0; i < len; i++) {
				str2 << (char) buf[i];
			}
		}
		g_logger::log("ramdisk: content: '" + str2.str() + "'", len);
		delete buf;

		// get length of ramdisk file:
		g_logger::log("length: %i", g_length(test_file));
		g_logger::log("length: %i", g_flength("/system/keyboard/de-DE.layout"));
		g_logger::log("length: %i", g_flength_s("/system/keyboard/de-DE.layout", false));

		g_fs_close_status closestatus = g_close(test_file);
		g_logger::log("ramdisk: closed with status: %i", closestatus);
	}

	/* Magic-FS read test */{
		g_logger::log("magicfs: starting read test");

		g_fs_open_status open_status;
		g_fd test_file = g_open_fs("/testfs/magic", 0, &open_status);
		g_logger::log("magicfs: file opened: %i, status: %i", test_file, open_status);

		uint8_t* buf = new uint8_t[128];
		g_fs_read_status read_status;
		int32_t len = g_read_s(test_file, buf, 128, &read_status);
		g_logger::log("magicfs: was able to read %i bytes file, status: %i", len, read_status);

		std::stringstream str;
		for (int i = 0; i < len; i++) {
			str << (char) buf[i];
		}
		g_logger::log("magicfs: read content: '" + str.str() + "'");

		g_fs_close_status closestatus = g_close(test_file);
		g_logger::log("magicfs: closed with status: %i", closestatus);
	}

	/* Magic-FS write test */{
		g_logger::log("magicfs: starting write test");

		g_fs_open_status open_status;
		g_fd test_file = g_open_fs("/testfs/hidden/magic", 0, &open_status);
		g_logger::log("magicfs: file opened: %i, status: %i", test_file, open_status);

		uint8_t* buf = new uint8_t[128];
		const char* content = "These are the bytes that I'm writing to the magic driver!";
		int contentlen = strlen(content);
		memcpy(buf, content, contentlen);

		g_fs_length_status stat;
		int64_t resolvedlen = g_length_s(test_file, &stat);
		g_logger::log("length: %i %i, stat %i", resolvedlen, stat);
		g_logger::log("length: %i", g_flength_s("/testfs/hello/lol/yeah/amazing", true));
		g_logger::log("length: %i", g_flength_s("/testfs/hello/lol/yeah/amazing", false));

		g_fs_write_status write_status;
		int32_t len = g_write_s(test_file, buf, contentlen, &write_status);
		g_logger::log("magicfs: wrote %i bytes, status: %i", len, write_status);

		g_fs_close_status closestatus = g_close(test_file);
		g_logger::log("magicfs: closed with status: %i", closestatus);
	}

	/**
	 * Pipe test
	 */
	{
		// create a pipe
		g_fd write_end;
		g_fd read_end;
		g_fs_pipe_status stat = g_pipe(&write_end, &read_end);

		if (stat == G_FS_PIPE_SUCCESSFUL) {
			g_logger::log("successfully created pipe: %i, %i", write_end, read_end);

			// write to pipe
			const char* test_data = "Hello pipes!";
			int test_data_len = strlen(test_data);
			g_write(write_end, test_data, test_data_len);

			// read from pipe
			uint8_t* buf = new uint8_t[512];
			g_fs_read_status stat;
			int64_t len = g_read_s(read_end, buf, 512, &stat);
			g_logger::log("read %i bytes", len);
			std::stringstream msgstream;
			for (int i = 0; i < len; i++) {
				msgstream << buf[i];
			}
			g_logger::log("message in pipe: '" + msgstream.str() + "'");

		} else {
			g_logger::log("failed to create pipe: %i, %i", write_end, read_end);
		}
	}
}
