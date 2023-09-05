#pragma once

#include <stdint.h>

// ordered so faster speed == higher enum value
typedef enum
{
	PCIE_SPEED_UNKNOWN = 0,
	PCIE_SPEED_2_5_GTS,
	PCIE_SPEED_5_GTS,
	PCIE_SPEED_8_GTS,
	PCIE_SPEED_16_GTS,
	PCIE_SPEED_32_GTS,
	PCIE_SPEED_64_GTS,
} pcie_speed_t;

struct pcie_speed_entry
{
	// vendor and device id string
	char *name;

	// maximum currently possible link speed (ALPM ignored)
	pcie_speed_t lnk_cap_speed;

	// maximum currently possible link width
	uint32_t lnk_cap_width;

	// current link speed, may be reduced by ALPM
	pcie_speed_t lnk_sta_speed;

	// current link width, may be reduced by ALPM
	uint32_t lnk_sta_width;

	// maximum possible link speed for given device
	pcie_speed_t lnk_cap2_speed;
	pcie_speed_t lnk_ctl2_speed;
};

#define PCIE_SPEED_MAX_DEV 10

struct pcie_speed_data
{
	struct pcie_speed_entry data[PCIE_SPEED_MAX_DEV];
	size_t len;
};

int pcie_speed_get(char *pci_filter_str, struct pcie_speed_data *pcie_speed_data);
int pcie_speed_data_free(struct pcie_speed_data *pcie_speed_data);
char *pcie_speed_to_str(pcie_speed_t speed);
