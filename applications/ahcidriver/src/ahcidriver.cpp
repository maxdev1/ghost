#include <libpci/driver.hpp>
#include <cstdio>

int main()
{
	while(true)
	{
		g_pci_identify_ahci_controller_entry* entries;
		int count;
		if(pciDriverIdentifyAhciController(&entries, &count))
		{
			klog("Successfully identified %i AHCI controllers", count);

			for(int i = 0; i < count; i++)
			{
				klog("AHCI controller: %x, INTR line: %i", entries[i].baseAddress, entries[i].interruptLine);
			}
			break;
		}
		else
		{
			klog("Failed to identify AHCI controller, retrying soon...");
		}
		g_sleep(5000);
	}
}
