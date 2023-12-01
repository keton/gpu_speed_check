#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "console.h"
#include "pcie_speed.h"
#include "toast.h"

#define PCI_CLASS_VGA			   "0300"
#define PCI_FILTER_STR			   "::" PCI_CLASS_VGA
#define PCI_METHOD_STR			   "win32-kldbg"

#define TOAST_SLEEP_MS			   (60 * 1000)

#define COMMAND_QUIET			   L"--quiet"
#define COMMAND_QUIET_SHORT		   L"-q"
#define COMMAND_ALWAYS_TOAST	   L"--always"
#define COMMAND_ALWAYS_TOAST_SHORT L"-a"

static int check_gpu_speed(bool *const toast_shown, const bool always_show_toast)
{
	struct pcie_speed_data pci = {0};

	if(pcie_speed_get(PCI_METHOD_STR, PCI_FILTER_STR, &pci) != EXIT_SUCCESS) {
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
		if((entry->lnk_cap_speed < entry->lnk_cap2_speed) ||
		   (entry->lnk_sta_speed < entry->lnk_cap2_speed) || always_show_toast) {
			char buff[1024] = {0};
			snprintf(buff, sizeof(buff), "PCIe speed %s instead of %s",
					 pcie_speed_to_str(min(entry->lnk_cap_speed, entry->lnk_sta_speed)),
					 pcie_speed_to_str(entry->lnk_cap2_speed));

			toast_show("GPU Speed error", entry->name, buff, TOAST_SLEEP_MS);

			*toast_shown = true;

			printf("Warning: device is operating at suboptimal speed %s instead of %s\n",
				   pcie_speed_to_str(min(entry->lnk_cap_speed, entry->lnk_sta_speed)),
				   pcie_speed_to_str(entry->lnk_cap2_speed));
		}
	}

	pcie_speed_data_free(&pci);

	return EXIT_SUCCESS;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);

	int ret = EXIT_SUCCESS;
	bool toast_shown = false;
	bool always_show_toast = false;
	bool hide_console = false;

	int argc = 0;
	LPWSTR *argv = CommandLineToArgvW(pCmdLine, &argc);

	for(int i = 0; i < argc; i++) {
		if(!wcscmp(COMMAND_QUIET, argv[i]) || !wcscmp(COMMAND_QUIET_SHORT, argv[i])) {
			hide_console = true;
		} else if(!wcscmp(COMMAND_ALWAYS_TOAST, argv[i]) ||
				  !wcscmp(COMMAND_ALWAYS_TOAST_SHORT, argv[i])) {
			always_show_toast = true;
		}
	}

	console_init();

	if(hide_console && console_is_allocated()) {
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	}

	if(toast_init() != TOAST_RESULT_SUCCESS) {
		fprintf(stderr, "Failed to initialize toast\n");
		ret = EXIT_FAILURE;
		goto exit;
	}

	if(check_gpu_speed(&toast_shown, always_show_toast) != EXIT_SUCCESS) {
		ret = EXIT_FAILURE;
		goto exit;
	}

	if(!toast_shown && !hide_console && console_is_allocated()) {
		printf("Press any key...\n");
		console_wait_anykey();
	}

	if(toast_shown) {
		Sleep(TOAST_SLEEP_MS);
	}

exit:

	console_free();
	return ret;
}
