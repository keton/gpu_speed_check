#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "console.h"
#include "pcie_speed.h"

#define PCI_CLASS_VGA  "0300"
#define PCI_FILTER_STR "::" PCI_CLASS_VGA

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	console_init();

	struct pcie_speed_data pci = {0};

	if(pcie_speed_get(PCI_FILTER_STR, &pci) != EXIT_SUCCESS) {
		fprintf(stderr, "Failed to get PCIe device data\n");
		return EXIT_FAILURE;
	}

	for(size_t i = 0; i < pci.len; i++) {

		struct pcie_speed_entry *entry = &pci.data[i];

		printf("%s\n", entry->name);
		printf("\tLnkCap:\tMaximum Link Speed %s, Maximum Link Width x%d\n",
			   pcie_speed_to_str(entry->lnk_cap_speed), entry->lnk_cap_width);
		printf("\tLnkSta:\tNegotiated Link Speed %s, Negotiated Link Width x%d\n",
			   pcie_speed_to_str(entry->lnk_sta_speed), entry->lnk_sta_width);
		printf("\tLnkCap2: Supported Link Speed: 2.5GT/s - %s\n",
			   pcie_speed_to_str(entry->lnk_cap2_speed));
		printf("\tLnkCtl2: Target Link Speed: %s\n", pcie_speed_to_str(entry->lnk_ctl2_speed));

		// device is operating at suboptimal speed
		if(entry->lnk_cap_speed < entry->lnk_cap2_speed) {
			printf("Warning: device is operating at suboptimal speed %s instead of %s\n",
				   pcie_speed_to_str(entry->lnk_cap_speed),
				   pcie_speed_to_str(entry->lnk_cap2_speed));
		}
	}

	pcie_speed_data_free(&pci);

	if(console_is_allocated()) {
		printf("Press any key...\n");
		console_wait_anykey();
	}

	console_free();

	return EXIT_SUCCESS;
}
