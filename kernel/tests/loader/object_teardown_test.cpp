#include "support/fixture.hpp"

namespace
{
#include "support/tracked_heap.hpp"

struct Elf64_Ehdr {};
struct Elf64_Dyn {};
struct Elf64_Sym {};
using Elf64_Word = uint32_t;
#include "production/hashmap_hpp.inc"
#include "production/hashmap_string_hpp.inc"
#include "production/hashmap_string_cpp.inc"
#include "production/elf_object_layout.inc"

g_elf_object* makeObject(const char* name) {
    auto object = static_cast<g_elf_object*>(heapAllocateClear(sizeof(g_elf_object)));
    object->name = stringDuplicate(name);
    object->tlsPart.content = static_cast<uint8_t*>(heapAllocate(32));
    auto dependency = static_cast<g_elf_dependency*>(heapAllocate(sizeof(g_elf_dependency)));
    dependency->name = stringDuplicate("dependency");
    dependency->next = nullptr;
    object->dependencies = dependency;
    return object;
}

#include "production/elfObjectDestroy.inc"

TEST(ElfObjects, ReclaimsDependenciesNamesTlsAndPartiallyLoadedObjects)
{
    for(int round = 0; round < 100; ++round) {
        auto root = makeObject("root");
        root->loadedObjects = hashmapCreateString<g_elf_object*>(16);
        hashmapPut(root->loadedObjects, "root", root);
        auto library = makeObject("library");
        auto failed = makeObject("partially-loaded");
        hashmapPut(root->loadedObjects, "library", library);
        hashmapPut(root->loadedObjects, "partially-loaded", failed);
        elfObjectDestroy(root);
        EXPECT_TRUE(heapBlocks.empty());
    }
}
}
