#ifndef PMM_H
#define PMM_H
#include <stdint.h>
#define BLOCK_SIZE 4096 //default page/block size
void pmm_init(uint32_t kernel_end_addr, uint32_t total_ram_size);
uint32_t pmm_get_used_blocks();
uint32_t pmm_get_total_blocks();
void pmm_free_memory(uint32_t block_address);
uint32_t pmm_alloc_block();
void pmm_lock_memory(uint32_t physical_address);
#endif
