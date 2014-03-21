#pragma once
// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;

#include <string>

extern "C" {
struct pci_access;
}

class PciDisplay
{
public:
	PciDisplay();
	~PciDisplay();
	std::string getFirstDisplay(void) const;

private:
	struct pci_access *_pacc;
};

