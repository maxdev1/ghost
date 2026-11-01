#ifndef CORE_DISK
#define CORE_DISK

#include "../src-common/storage.hpp"
#include <algorithm>
#include <vector>

void diskPrint(const g_disk_info& disk);
int diskList();
void diskPrintPartition(const g_disk_partition_info& part);
int diskPartitionList(const g_disk_info& disk);
bool diskPartitionOptions(int argc, char** argv, const g_disk_info& disk, g_volume_admin_request* request);
int diskToolRun(int argc, char** argv);

#endif
