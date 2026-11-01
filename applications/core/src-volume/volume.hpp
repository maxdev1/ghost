#ifndef CORE_VOLUME
#define CORE_VOLUME

#include <libstorage/volume.hpp>
#include "../src-common/storage.hpp"
#include <string>

const char* volumeToolStatusText(g_volume_status status);
const char* volumeToolStateText(g_volume_state state);
bool volumeToolParseId(const char* text, g_volume_id& id);
int volumeToolList();
void volumeToolPrintInfo(const g_volume_info& info);
bool volumeToolGet(g_volume_id id, g_volume_info& info);
bool volumeToolResolve(const char* target, g_volume_info& info);
int volumeToolRun(int argc, char** argv);

#endif
