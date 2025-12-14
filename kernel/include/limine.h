#ifndef LIMINE_HEADER_INCLUDED
#define LIMINE_HEADER_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIMINE_PTR(TYPE) TYPE

#define LIMINE_BASE_REVISION(N) \
	uint64_t limine_base_revision[3] = { 0xf9562b2d5c95a6c8, 0x6a7b384944536bdc, (N) }

#define LIMINE_BASE_REVISION_SUPPORTED (limine_base_revision[2] == 0)

#define LIMINE_REQUESTS_START_MARKER \
	uint64_t limine_requests_start_marker[4] = { 0xf6b8f4b39de7d1ae, 0xfab91a6940fcb9cf, 0x785c6ed015d3e316, 0x181e920a7852b9d9 }

#define LIMINE_REQUESTS_END_MARKER \
	uint64_t limine_requests_end_marker[2] = { 0xadc0e0531bb10d03, 0x9572709f31764c62 }

#define LIMINE_COMMON_MAGIC 0xc7b1dd30df4c8b88, 0x0a82e883a194f07b

#define LIMINE_FRAMEBUFFER_REQUEST { LIMINE_COMMON_MAGIC, 0x9d5827dcd881dd75, 0xa3148604f6fab11b }
#define LIMINE_MEMMAP_REQUEST      { LIMINE_COMMON_MAGIC, 0x67cf3d9d378a806f, 0xe304acdfc50c3c62 }
#define LIMINE_PAGING_MODE_REQUEST { LIMINE_COMMON_MAGIC, 0x95c1a0edab0944cb, 0xa4e5cb3842f7488a }
#define LIMINE_MODULE_REQUEST      { LIMINE_COMMON_MAGIC, 0x3e7e279702be32af, 0xca1c4f3bd1280cee }
#define LIMINE_RSDP_REQUEST        { LIMINE_COMMON_MAGIC, 0xc5e77b6b397e7b43, 0x27637845accdcf3c }
#define LIMINE_HHDM_REQUEST        { LIMINE_COMMON_MAGIC, 0x48dcf1cb8ad2b852, 0x63984e959a98244b }

struct limine_uuid
{
	uint32_t a;
	uint16_t b;
	uint16_t c;
	uint8_t d[8];
};

#define LIMINE_MEDIA_TYPE_GENERIC 0
#define LIMINE_MEDIA_TYPE_OPTICAL 1
#define LIMINE_MEDIA_TYPE_TFTP    2

struct limine_file
{
	uint64_t revision;
	LIMINE_PTR(void*) address;
	uint64_t size;
	LIMINE_PTR(char*) path;
	LIMINE_PTR(char*) string;
	uint32_t media_type;
	uint32_t unused;
	uint32_t tftp_ip;
	uint32_t tftp_port;
	uint32_t partition_index;
	uint32_t mbr_disk_id;
	struct limine_uuid gpt_disk_uuid;
	struct limine_uuid gpt_part_uuid;
	struct limine_uuid part_uuid;
};

struct limine_framebuffer;
struct limine_video_mode;

struct limine_framebuffer_request
{
	uint64_t id[4];
	uint64_t revision;
	LIMINE_PTR(struct limine_framebuffer_response*) response;
};

struct limine_framebuffer_response
{
	uint64_t revision;
	uint64_t framebuffer_count;
	LIMINE_PTR(struct limine_framebuffer**) framebuffers;
};

#define LIMINE_FRAMEBUFFER_RGB 1

struct limine_framebuffer
{
	LIMINE_PTR(void*) address;
	uint64_t width;
	uint64_t height;
	uint64_t pitch;
	uint16_t bpp;
	uint8_t memory_model;
	uint8_t red_mask_size;
	uint8_t red_mask_shift;
	uint8_t green_mask_size;
	uint8_t green_mask_shift;
	uint8_t blue_mask_size;
	uint8_t blue_mask_shift;
	uint8_t unused[7];
	uint64_t edid_size;
	LIMINE_PTR(void*) edid;

	/* Revision 1 fields */
	uint64_t mode_count;
	LIMINE_PTR(struct limine_video_mode**) modes;
};

struct limine_video_mode
{
	uint64_t pitch;
	uint64_t width;
	uint64_t height;
	uint16_t bpp;
	uint8_t memory_model;
	uint8_t red_mask_size;
	uint8_t red_mask_shift;
	uint8_t green_mask_size;
	uint8_t green_mask_shift;
	uint8_t blue_mask_size;
	uint8_t blue_mask_shift;
};

struct limine_memmap_request
{
	uint64_t id[4];
	uint64_t revision;
	LIMINE_PTR(struct limine_memmap_response*) response;
};

struct limine_memmap_response
{
	uint64_t revision;
	uint64_t entry_count;
	LIMINE_PTR(struct limine_memmap_entry**) entries;
};

struct limine_memmap_entry
{
	uint64_t base;
	uint64_t length;
	uint64_t type;
};

#define LIMINE_MEMMAP_USABLE                 0
#define LIMINE_MEMMAP_RESERVED               1
#define LIMINE_MEMMAP_ACPI_RECLAIMABLE       2
#define LIMINE_MEMMAP_ACPI_NVS               3
#define LIMINE_MEMMAP_BAD_MEMORY             4
#define LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define LIMINE_MEMMAP_EXECUTABLE_AND_MODULES 6
#define LIMINE_MEMMAP_FRAMEBUFFER            7

struct limine_paging_mode_request
{
	uint64_t id[4];
	uint64_t revision;
	LIMINE_PTR(struct limine_paging_mode_response*) response;
	uint64_t mode;
	uint64_t max_mode;
	uint64_t min_mode;
};

struct limine_paging_mode_response
{
	uint64_t revision;
	uint64_t mode;
};

#define LIMINE_PAGING_MODE_X86_64_4LVL 0
#define LIMINE_PAGING_MODE_X86_64_5LVL 1

struct limine_internal_module
{
	LIMINE_PTR(const char*) path;
	LIMINE_PTR(const char*) string;
	uint64_t flags;
};

#define LIMINE_INTERNAL_MODULE_REQUIRED   (1ull << 0)
#define LIMINE_INTERNAL_MODULE_COMPRESSED (1ull << 1)

struct limine_module_request
{
	uint64_t id[4];
	uint64_t revision;
	LIMINE_PTR(struct limine_module_response*) response;
	uint64_t internal_module_count;
	LIMINE_PTR(struct limine_internal_module**) internal_modules;
};

struct limine_module_response
{
	uint64_t revision;
	uint64_t module_count;
	LIMINE_PTR(struct limine_file**) modules;
};

struct limine_rsdp_request
{
	uint64_t id[4];
	uint64_t revision;
	LIMINE_PTR(struct limine_rsdp_response*) response;
};

struct limine_rsdp_response
{
	uint64_t revision;
	uint64_t address;
};

struct limine_hhdm_request
{
	uint64_t id[4];
	uint64_t revision;
	LIMINE_PTR(struct limine_hhdm_response*) response;
};

struct limine_hhdm_response
{
	uint64_t revision;
	uint64_t offset;
};

#ifdef __cplusplus
}
#endif

#endif
