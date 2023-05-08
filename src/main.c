#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "pci.h"
#include "rc_manager.h"

#define PCI_CLASS_VGA  "0300"
#define PCI_FILTER_STR "::" PCI_CLASS_VGA

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

void die(char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	fprintf(stderr, "fatal error: ");
	vfprintf(stderr, msg, args);
	fputc('\n', stderr);
	exit(1);
}

static uint8_t get_conf_byte(uint8_t *data, unsigned int pos)
{
	return data[pos];
}

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

static char *pcie_speed_to_str(pcie_speed_t speed)
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

int main(void)
{
	// unpack all required resources
	if(rc_manager_init() != 0) {
		die("rc_manager_init");
	}

	struct pci_filter filter = {0};
	struct pci_access *pacc = pci_alloc();
	if(!pacc) {
		die("pci_alloc");
	}

	pacc->error = die;
	pci_init(pacc);
	pci_filter_init(pacc, &filter);
	if(pci_filter_parse_id(&filter, PCI_FILTER_STR)) {
		die("pci_filter_parse_id error\n");
	}
	pci_scan_bus(pacc);

	for(struct pci_dev *dev = pacc->devices; dev; dev = dev->next) {
		if(!pci_filter_match(&filter, dev)) {
			continue;
		}
		char devbuf[256] = {0};

		pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_LABEL | PCI_FILL_CAPS | PCI_FILL_EXT_CAPS);

		printf("%04x:%04x %s\n", dev->vendor_id, dev->device_id,
			   pci_lookup_name(pacc, devbuf, sizeof(devbuf), PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE,
							   dev->vendor_id, dev->device_id));

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

		uint32_t lnk_cap_raw = get_conf_long(cap_express_data, PCI_EXP_LNKCAP);
		pcie_speed_t lnk_cap_speed = lnk_cap1_speed_to_enum(lnk_cap_raw & PCI_EXP_LNKCAP_SPEED);

		uint32_t lnk_cap_width = (lnk_cap_raw & PCI_EXP_LNKCAP_WIDTH) >> 4;

		printf("\tLnkCap:\tMaximum Link Speed %s, Maximum Link Width x%d\n",
			   pcie_speed_to_str(lnk_cap_speed), lnk_cap_width);

		uint16_t lnk_sta_raw = get_conf_word(cap_express_data, PCI_EXP_LNKSTA);
		pcie_speed_t lnk_sta_speed = lnk_cap1_speed_to_enum(lnk_sta_raw & PCI_EXP_LNKSTA_SPEED);

		uint32_t lnk_sta_width = (lnk_sta_raw & PCI_EXP_LNKSTA_WIDTH) >> 4;

		printf("\tLnkSta:\tNegotiated Link Speed %s, Negotiated Link Width x%d\n",
			   pcie_speed_to_str(lnk_sta_speed), lnk_sta_width);

		uint32_t lnk_cap2_raw = get_conf_long(cap_express_data, PCI_EXP_LNKCAP2);
		pcie_speed_t lnk_cap2_speed = lnk_cap2_speed_to_enum(PCI_EXP_LNKCAP2_SPEED(lnk_cap2_raw));

		if(lnk_cap2_speed != PCIE_SPEED_UNKNOWN) {
			printf("\tLnkCap2: Supported Link Speed: 2.5GT/s - %s\n",
				   pcie_speed_to_str(lnk_cap2_speed));
		}

		uint16_t lnk_ctl2_raw = get_conf_word(cap_express_data, PCI_EXP_LNKCTL2);
		pcie_speed_t lnk_ctl2_speed = lnk_ctl2_speed_to_enum(PCI_EXP_LNKCTL2_SPEED(lnk_ctl2_raw));

		printf("\tLnkCtl2: Target Link Speed: %s\n", pcie_speed_to_str(lnk_ctl2_speed));
	}

	pci_cleanup(pacc);
	return 0;
}
