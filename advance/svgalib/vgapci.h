extern int __svgalib_pci_find_vendor_vga(unsigned int vendor, unsigned long *conf, int cont);
extern int __svgalib_pci_find_vendor_vga_pos(unsigned int vendor, unsigned long *conf, int cont);
extern int __svgalib_pci_idev;
extern void __svgalib_pci_write_config_byte(int pos, int address, unsigned char data);
extern void __svgalib_pci_write_config_word(int pos, int address, unsigned short data);
extern void __svgalib_pci_write_config_dword(int pos, int address, unsigned int data);
extern int __svgalib_pci_read_config_byte(int pos, int address);
extern int __svgalib_pci_read_config_word(int pos, int address);
extern int __svgalib_pci_read_config_dword(int pos, int address);
extern int __svgalib_pci_read_aperture_len(int pos, int address);
extern int memorytest(unsigned char *m, int max_mem);

