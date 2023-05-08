#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "pci.h"
#include "rc_manager.h"

#define PCI_VENDOR_NVIDIA "10de"
#define PCI_CLASS_VGA	  "0300"

#define PCI_FILTER_STR	  PCI_VENDOR_NVIDIA "::" PCI_CLASS_VGA

const char program_name[] = "gpu_speed";

void die(char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	fprintf(stderr, "%s: ", program_name);
	vfprintf(stderr, msg, args);
	fputc('\n', stderr);
	exit(1);
}

uint8_t get_conf_byte(uint8_t *data, unsigned int pos) { return data[pos]; }

uint16_t get_conf_word(uint8_t *data, unsigned int pos) { return data[pos] | (data[pos + 1] << 8); }

uint32_t get_conf_long(uint8_t *data, unsigned int pos)
{
	return data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
}

static char *link_speed(int speed)
{
	switch(speed) {
	case 1:
		return "2.5GT/s";
	case 2:
		return "5GT/s";
	case 3:
		return "8GT/s";
	case 4:
		return "16GT/s";
	case 5:
		return "32GT/s";
	case 6:
		return "64GT/s";
	default:
		return "unknown";
	}
}

static char *link_compare(int type, int sta, int cap)
{
	if(sta > cap)
		return " (overdriven)";
	if(sta == cap)
		return "";
	if((type == PCI_EXP_TYPE_ROOT_PORT) || (type == PCI_EXP_TYPE_DOWNSTREAM)
	   || (type == PCI_EXP_TYPE_PCIE_BRIDGE))
		return "";
	return " (downgraded)";
}

static const char *cap_express_link2_speed_cap(int vector)
{
	/*
	 * Per PCIe r5.0, sec 8.2.1, a device must support 2.5GT/s and is not
	 * permitted to skip support for any data rates between 2.5GT/s and the
	 * highest supported rate.
	 */
	if(vector & 0x60)
		return "RsvdP";
	if(vector & 0x10)
		return "2.5-32GT/s";
	if(vector & 0x08)
		return "2.5-16GT/s";
	if(vector & 0x04)
		return "2.5-8GT/s";
	if(vector & 0x02)
		return "2.5-5GT/s";
	if(vector & 0x01)
		return "2.5GT/s";

	return "Unknown";
}

static const char *cap_express_link2_speed(int type)
{
	switch(type) {
	case 0: /* hardwire to 0 means only the 2.5GT/s is supported */
	case 1:
		return "2.5GT/s";
	case 2:
		return "5GT/s";
	case 3:
		return "8GT/s";
	case 4:
		return "16GT/s";
	case 5:
		return "32GT/s";
	case 6:
		return "64GT/s";
	default:
		return "Unknown";
	}
}

int main(void)
{
	// unpack all required resources
	if(rc_manager_init() != 0) {
		die("rc_manager_init");
	}

	struct pci_filter filter = { 0 };
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
		char devbuf[256] = { 0 };

		pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_LABEL | PCI_FILL_CAPS | PCI_FILL_EXT_CAPS);

		printf("%04x:%04x %s\n", dev->vendor_id, dev->device_id,
			   pci_lookup_name(pacc, devbuf, sizeof(devbuf), PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE,
							   dev->vendor_id, dev->device_id));

		struct pci_cap *cap = pci_find_cap(dev, PCI_CAP_ID_EXP, PCI_CAP_NORMAL);
		if(!cap) {
			printf("Warning: Failed to find PCI Express capability\n");
			continue;
		}

		uint8_t cap_express_data[58] = { 0 };

		// get PCIe config data
		if(!dev->no_config_access
		   && !pci_read_block(dev, cap->addr, cap_express_data, sizeof(cap_express_data))) {
			printf("Warning: no_config_access\n");
			continue;
		}

		uint32_t lnk_cap_raw = get_conf_long(cap_express_data, PCI_EXP_LNKCAP);
		uint32_t lnk_cap_speed = lnk_cap_raw & PCI_EXP_LNKCAP_SPEED;
		uint32_t lnk_cap_width = (lnk_cap_raw & PCI_EXP_LNKCAP_WIDTH) >> 4;

		printf("\tLnkCap:\tSpeed %s, Width x%d\n", link_speed(lnk_cap_speed), lnk_cap_width);

		uint16_t lnk_sta_raw = get_conf_word(cap_express_data, PCI_EXP_LNKSTA);
		uint32_t lnk_sta_speed = lnk_sta_raw & PCI_EXP_LNKSTA_SPEED;
		uint32_t lnk_sta_width = (lnk_sta_raw & PCI_EXP_LNKSTA_WIDTH) >> 4;
		printf("\tLnkSta:\tSpeed %s%s, Width x%d%s\n", link_speed(lnk_sta_speed),
			   link_compare(PCI_EXP_TYPE_LEG_END, lnk_sta_speed, lnk_cap_speed), lnk_sta_width,
			   link_compare(PCI_EXP_TYPE_LEG_END, lnk_sta_width, lnk_cap_width));

		uint32_t lnk_cap2_raw = get_conf_long(cap_express_data, PCI_EXP_LNKCAP2);
		if(lnk_cap2_raw) {
			printf("\tLnkCap2: Supported Link Speeds: %s\n",
				   cap_express_link2_speed_cap(PCI_EXP_LNKCAP2_SPEED(lnk_cap2_raw)));
		}

		uint16_t lnk_ctl2_raw = get_conf_word(cap_express_data, PCI_EXP_LNKCTL2);
		printf("\tLnkCtl2: Target Link Speed: %s\n",
			   cap_express_link2_speed(PCI_EXP_LNKCTL2_SPEED(lnk_ctl2_raw)));
	}

	pci_cleanup(pacc);
	return 0;
}
