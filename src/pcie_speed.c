#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pcie_speed.h"

#include "pci.h"
#include "rc_manager.h"

static uint16_t get_conf_word(uint8_t *data, unsigned int pos)
{
	return data[pos] | (data[pos + 1] << 8);
}

static uint32_t get_conf_long(uint8_t *data, unsigned int pos)
{
	return data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
}

static pcie_speed_t lnk_cap1_speed_to_enum(int link2speed)
{
	switch(link2speed) {
	case 1:
		return PCIE_SPEED_2_5_GTS;
	case 2:
		return PCIE_SPEED_5_GTS;
	case 3:
		return PCIE_SPEED_8_GTS;
	case 4:
		return PCIE_SPEED_16_GTS;
	case 5:
		return PCIE_SPEED_32_GTS;
	case 6:
		return PCIE_SPEED_64_GTS;
	default:
		return PCIE_SPEED_UNKNOWN;
	}
}

static pcie_speed_t lnk_cap2_speed_to_enum(int vector)
{
	/*
	 * Per PCIe r5.0, sec 8.2.1, a device must support 2.5GT/s and is not
	 * permitted to skip support for any data rates between 2.5GT/s and the
	 * highest supported rate.
	 */
	if(vector & 0x60)
		return PCIE_SPEED_64_GTS;
	if(vector & 0x10)
		return PCIE_SPEED_32_GTS;
	if(vector & 0x08)
		return PCIE_SPEED_16_GTS;
	if(vector & 0x04)
		return PCIE_SPEED_8_GTS;
	if(vector & 0x02)
		return PCIE_SPEED_5_GTS;
	if(vector & 0x01)
		return PCIE_SPEED_2_5_GTS;

	return PCIE_SPEED_UNKNOWN;
}

static pcie_speed_t lnk_ctl2_speed_to_enum(int type)
{
	switch(type) {
	case 0: /* hardwire to 0 means only the 2.5GT/s is supported */
	case 1:
		return PCIE_SPEED_2_5_GTS;
	case 2:
		return PCIE_SPEED_5_GTS;
	case 3:
		return PCIE_SPEED_8_GTS;
	case 4:
		return PCIE_SPEED_16_GTS;
	case 5:
		return PCIE_SPEED_32_GTS;
	case 6:
		return PCIE_SPEED_64_GTS;
	default:
		return PCIE_SPEED_UNKNOWN;
	}
}

char *pcie_speed_to_str(pcie_speed_t speed)
{
	switch(speed) {
	case PCIE_SPEED_2_5_GTS:
		return "2.5GT/s";
	case PCIE_SPEED_5_GTS:
		return "5GT/s";
	case PCIE_SPEED_8_GTS:
		return "8GT/s";
	case PCIE_SPEED_16_GTS:
		return "16GT/s";
	case PCIE_SPEED_32_GTS:
		return "32GT/s";
	case PCIE_SPEED_64_GTS:
		return "64GT/s";
	case PCIE_SPEED_UNKNOWN:
	default:
		return "unknown";
	}
}

int pcie_speed_get(char * pci_method_str, char *pci_filter_str, struct pcie_speed_data *pcie_speed_data)
{

	if(!pci_filter_str || !pcie_speed_data) {
		fprintf(stderr, "NULL\n");
		return EXIT_FAILURE;
	}

	// unpack all required resources
	if(rc_manager_init() != 0) {
		fprintf(stderr, "rc_manager_init\n");
		return EXIT_FAILURE;
	}

	struct pci_filter filter = {0};
	struct pci_access *pacc = pci_alloc();
	if(!pacc) {
		fprintf(stderr, "pci_alloc\n");
		return EXIT_FAILURE;
	}

	int pci_method_id = pci_lookup_method(pci_method_str);
	if(pci_method_id < 0) {
		fprintf(stderr, "No such PCI access method: %s\n", pci_method_str);
		return EXIT_FAILURE;
	}
	pacc->method = pci_method_id;

	pci_init(pacc);
	pci_filter_init(pacc, &filter);
	if(pci_filter_parse_id(&filter, pci_filter_str)) {
		fprintf(stderr, "pci_filter_parse_id error\n");
		return EXIT_FAILURE;
	}
	pci_scan_bus(pacc);

	for(struct pci_dev *dev = pacc->devices; dev; dev = dev->next) {
		if(!pci_filter_match(&filter, dev)) {
			continue;
		}

		pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_LABEL | PCI_FILL_CAPS | PCI_FILL_EXT_CAPS);

		struct pci_cap *cap = pci_find_cap(dev, PCI_CAP_ID_EXP, PCI_CAP_NORMAL);
		if(!cap) {
			printf("Warning: Failed to find PCI Express capability\n");
			continue;
		}

		// per PCIe spec 'devices with links' have first 56 bytes of PCI Express capability register
		// populated
		uint8_t cap_express_data[56] = {0};

		// get PCIe config data
		if(!dev->no_config_access &&
		   !pci_read_block(dev, cap->addr, cap_express_data, sizeof(cap_express_data))) {
			printf("Warning: no_config_access\n");
			continue;
		}

		char devbuf[256] = {0};
		char namebuff[1024] = {0};

		struct pcie_speed_entry *entry = &pcie_speed_data->data[pcie_speed_data->len];

		snprintf(namebuff, sizeof(namebuff), "%04x:%04x %s", dev->vendor_id, dev->device_id,
				 pci_lookup_name(pacc, devbuf, sizeof(devbuf),
								 PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE, dev->vendor_id,
								 dev->device_id));
		entry->name = _strdup(namebuff);

		uint32_t lnk_cap_raw = get_conf_long(cap_express_data, PCI_EXP_LNKCAP);
		entry->lnk_cap_speed = lnk_cap1_speed_to_enum(lnk_cap_raw & PCI_EXP_LNKCAP_SPEED);
		entry->lnk_cap_width = (lnk_cap_raw & PCI_EXP_LNKCAP_WIDTH) >> 4;

		uint16_t lnk_sta_raw = get_conf_word(cap_express_data, PCI_EXP_LNKSTA);
		entry->lnk_sta_speed = lnk_cap1_speed_to_enum(lnk_sta_raw & PCI_EXP_LNKSTA_SPEED);
		entry->lnk_sta_width = (lnk_sta_raw & PCI_EXP_LNKSTA_WIDTH) >> 4;

		uint32_t lnk_cap2_raw = get_conf_long(cap_express_data, PCI_EXP_LNKCAP2);
		entry->lnk_cap2_speed = lnk_cap2_speed_to_enum(PCI_EXP_LNKCAP2_SPEED(lnk_cap2_raw));

		uint16_t lnk_ctl2_raw = get_conf_word(cap_express_data, PCI_EXP_LNKCTL2);
		entry->lnk_ctl2_speed = lnk_ctl2_speed_to_enum(PCI_EXP_LNKCTL2_SPEED(lnk_ctl2_raw));

		pcie_speed_data->len++;

		if(pcie_speed_data->len >= PCIE_SPEED_MAX_DEV) {
			printf("Warning: pcie_speed_data out of space\n");
			break;
		}
	}

	pci_cleanup(pacc);
	return EXIT_SUCCESS;
}

int pcie_speed_data_free(struct pcie_speed_data *pcie_speed_data)
{
	if(!pcie_speed_data) {
		return EXIT_FAILURE;
	}

	for(size_t i = 0; i < pcie_speed_data->len; i++) {
		free(pcie_speed_data->data->name);
	}

	return EXIT_SUCCESS;
}
