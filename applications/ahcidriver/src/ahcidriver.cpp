#include <stdio.h>
#include <libpci/driver.hpp>

int main() {
	g_pci_identify_ahci_controller_entry* out;
	if(pciDriverIdentifyAhciController(&out))
	{
		klog("Successfully identified AHCI controller: %x", out);
	} else
	{
		klog("Failed to identify AHCI controller");
	}
}
