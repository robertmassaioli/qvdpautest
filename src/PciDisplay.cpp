// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;

extern "C" {
#include <pci/pci.h>
}

#include <iostream>
#include "PciDisplay.h"

PciDisplay::PciDisplay()
	: _pacc(pci_alloc())
{
	pci_init(_pacc);
	pci_scan_bus(_pacc);
}

PciDisplay::~PciDisplay()
{
	pci_cleanup(_pacc);
}

std::string PciDisplay::getFirstDisplay(void) const
{
	for ( pci_dev *d = _pacc->devices; d; d = d->next )
	{
		if ( (d->device_class >> 8) == PCI_BASE_CLASS_DISPLAY )
		{
			char buf[256];

			return pci_lookup_name(_pacc
					, buf
					, sizeof(buf)
					, PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE
					, d->vendor_id, d->device_id);
		}
	}

	return "Unknown GPU";
}

