#include "support/fixture.hpp"

using g_ramdisk_entry_type = uint8_t;
using g_ramdisk_id = uint32_t;
constexpr int G_RAMDISK_ENTRY_TYPE_FILE = 1, G_RAMDISK_ENTRY_TYPE_FOLDER = 0;
constexpr unsigned G_RAMDISK_MAXIMUM_PATH_LENGTH = 4096;
#include "production/ramdisk_entry.inc"
struct g_ramdisk { g_ramdisk_entry* firstEntry; g_ramdisk_entry* root; uint32_t nextUnusedId; };
struct limine_file { void* address; uint64_t size; const char* path; };
struct limine_module_response { uint64_t module_count; limine_file** modules; };
static g_ramdisk* ramdiskMain;
void* heapAllocate(size_t bytes) { return std::malloc(bytes); }
int stringLength(const char* value) { return std::strlen(value); }
int stringCopy(char* target, const char* source) { std::strcpy(target, source); return 0; }
bool stringEquals(const char* a, const char* b) { return std::strcmp(a, b) == 0; }
#include "production/boot_modules.inc"

TEST(BootModules, PublishesIndividualFilesWithoutCopyingTheirContents)
{
	uint8_t a[]{1,2,3}, b[]{4,5};
	limine_file first{a, sizeof(a), "/applications/pcidriver.bin"};
	limine_file second{b, sizeof(b), "/system/lib/libc.so"};
	limine_file* files[]{&first, &second};
	limine_module_response response{2, files};
	ramdiskLoadBootModules(&response);
	auto application = ramdiskFindChild(ramdiskMain->root, "applications");
	ASSERT_NE(nullptr, application);
	auto file = ramdiskFindChild(application, "pcidriver.bin");
	ASSERT_NE(nullptr, file);
	EXPECT_EQ(a, file->data);
	EXPECT_EQ(sizeof(a), file->dataSize);
	EXPECT_TRUE(file->dataOnRamdisk);
	auto system = ramdiskFindChild(ramdiskMain->root, "system");
	ASSERT_NE(nullptr, system);
	auto library = ramdiskFindChild(ramdiskFindChild(system, "lib"), "libc.so");
	ASSERT_NE(nullptr, library);
	EXPECT_EQ(b, library->data);
	while(ramdiskMain->firstEntry)
	{
		auto entry = ramdiskMain->firstEntry;
		ramdiskMain->firstEntry = entry->next;
		std::free(entry->name);
		std::free(entry);
	}
	delete ramdiskMain->root;
	delete ramdiskMain;
	ramdiskMain = nullptr;
}
