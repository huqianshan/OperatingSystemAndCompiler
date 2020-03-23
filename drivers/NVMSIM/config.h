int nvm_capacity_mb = 4096; //Size of each NVM disk in MB
uint64_t g_highmem_phys_addr = 0x100000000; /* beginning of the reserved phy mem space (bytes)*/

// sector size 512Bytes
#define SECTOR_SHIFT 9