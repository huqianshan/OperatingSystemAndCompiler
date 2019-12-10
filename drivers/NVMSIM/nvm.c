/*
 * NVM.c
 * NVM Simulator: NVM Model
 *
 * Copyright 2010
 *      The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/highmem.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/timex.h>

#include <asm/uaccess.h>

#include "mem.h"
#include "nvm.h"
#include "ramdevice.h"

//#include "util.h"

#define  NVMSIM_GROUND_TRUTH
/**
 * The original NVM timings
 */
unsigned nvm_org_tRCD = 22;
unsigned nvm_org_tRP  = 60;

/**
 * The original NVM frequency
 */
unsigned nvm_org_mhz  = 400;

/**
 * The extrapolated NVM timings
 */
unsigned nvm_tRCD;
unsigned nvm_tRP;

/**
 * NVM logical row width (bytes)
 */
unsigned nvm_row_width = 256;

/**
 * The NVM latency for reading and writing
 */
unsigned nvm_latency[2 /* 0 = read, 1 = write */][NVMSIM_MEM_SECTORS + 1];

/**
 * The NVM delta latency for reading and writing
 */
int nvm_latency_delta[2 /* 0 = read, 1 = write */][NVMSIM_MEM_SECTORS + 1];


/**
 * Calibrate the NVM model. This function can be called only after
 * the memory subsystem has been initialized.
 
void nvm_calibrate(void)
{
	unsigned sectors, n;
	unsigned mem_rows, nvm_rows, mem_t;
	unsigned d_read, d_write;

	WARN_ON(sizeof(unsigned) != 4);

	
	// Extrapolate NVM timing information to the current bus frequency

	nvm_tRCD = 10 * nvm_org_tRCD * memory_bus_mhz / nvm_org_mhz;
	nvm_tRP  = 10 * nvm_org_tRP  * memory_bus_mhz / nvm_org_mhz;

	if (nvm_tRCD % 10 >= 5) nvm_tRCD += 10;
	if (nvm_tRP  % 10 >= 5) nvm_tRP  += 10;
	nvm_tRCD /= 10;
	nvm_tRP  /= 10;

	
	// Compute the NVM latencies

	for (sectors = 1; sectors <= NVMSIM_MEM_SECTORS; sectors++) {

		mem_rows = (sectors << 9) / memory_row_width;
		nvm_rows = (sectors << 9) /    nvm_row_width;

		mem_t    = memory_overhead_read[NVMSIM_MEM_UNCACHED][sectors];
		d_read   = nvm_rows * nvm_tRCD - mem_rows * memory_tRCD;
		d_write  = nvm_rows * nvm_tRP  - mem_rows * memory_tRP ;

		nvm_latency[NVM_READ ][sectors] = mem_t + d_read  * memory_bus_scale;
		nvm_latency[NVM_WRITE][sectors] = mem_t + d_write * memory_bus_scale;
	}


	// Compute the deltas

	for (sectors = 1; sectors <= NVMSIM_MEM_SECTORS; sectors++) {

		mem_t    = memory_overhead_read[NVMSIM_MEM_UNCACHED][sectors];

		nvm_latency_delta[NVM_READ ][sectors] = (int) nvm_latency[NVM_READ ][sectors] - (int) mem_t;
		nvm_latency_delta[NVM_WRITE][sectors] = (int) nvm_latency[NVM_WRITE][sectors] - (int) mem_t;
	}

	
	// Print a report

	printk("\n");
	printk("  NVMSIM NVM Settings  \n");
	printk("-----------------------\n");
	printk("\n");
	printk("tRCD          : %4d bus cycles\n", nvm_tRCD);
	printk("tRP           : %4d bus cycles\n", nvm_tRP);
	printk("\n");
	printk("nvm\n");
	for (n = 1; n <= NVMSIM_MEM_SECTORS; n++) {
		printk("%4d sector%s  : %5d cycles read, %6d cycles write\n",
			   n, n == 1 ? " " : "s", nvm_latency[NVM_READ ][n], nvm_latency[NVM_WRITE][n]);
	}
	printk("\n");
	printk("nvm delta\n");
	for (n = 1; n <= NVMSIM_MEM_SECTORS; n++) {
		printk("%4d sector%s  : %5d cycles read, %6d cycles write\n",
			   n, n == 1 ? " " : "s", nvm_latency_delta[NVM_READ ][n], nvm_latency_delta[NVM_WRITE][n]);
	}
	printk("\n");
}
*/

/**
 * Allocate NVM model data
 */
struct nvm_model* nvm_model_allocate(unsigned sectors)
{
	struct nvm_model* model;


	// Allocate the model struct
	model = (struct nvm_model*) kzalloc(sizeof(struct nvm_model), GFP_KERNEL);
	if (model == NULL) 
        goto out;


	// Allocate the dirty bits array
	model->dirty = (unsigned*) kzalloc(sectors / (sizeof(unsigned) << 3) + sizeof(unsigned), GFP_KERNEL);
	if (model->dirty == NULL) 
        goto out_free;

	return model;


	// Cleanup on error

out_free:
	kfree(model);
out:
	return NULL;
}


/**
 * Free NVM model data
 */
void nvm_model_free(struct nvm_model* model)
{
	unsigned total_reads;
	unsigned total_writes;
	unsigned cached_reads;
	unsigned cached_writes;


	// Compute some statistics

	total_reads  = model->stat_reads [0] + model->stat_reads [1];
	total_writes = model->stat_writes[0] + model->stat_writes[1];

	cached_reads = 0;
	cached_writes = 0;

	if (total_reads > 0) {
		cached_reads = (10000 * model->stat_reads[1]) / total_reads;
	}

	if (total_writes > 0) {
		cached_writes = (10000 * model->stat_writes[1]) / total_writes;
	}


	// Print the statistics

	printk("\n");
	printk("  NVMSIM Statistics  \n");
	printk("---------------------\n");
	printk("\n");
	printk("Reads         : %6d (%2d.%02d%% cached)\n", total_reads , cached_reads  / 100, cached_reads  % 100);
	printk("Writes        : %6d (%2d.%02d%% cached)\n", total_writes, cached_writes / 100, cached_writes % 100);
	printk("\n");
	

	// Free the data structures
	
	kfree(model->dirty);
	kfree(model);
}


/**
 * Perform a NVM read access
 */
void nvm_read(struct nvm_model* model, void* dest, const void* src, size_t length, sector_t sector)
{
    unsigned sectors;
    sectors = length >> SECTOR_SHIFT;
    WARN_ON(sectors > NVMSIM_MEM_SECTORS);
    // Perform the operation
    memory_copy(dest, src, length);		// This does mfence, so we do not need pipeline flush
}


/**
 * Perform a NVM write access
 */
void nvm_write(struct nvm_model* model, void* dest, const void* src, size_t length, sector_t sector)
{

	unsigned sectors;

	sectors = length >> SECTOR_SHIFT;
	WARN_ON(sectors > NVMSIM_MEM_SECTORS);

	// Perform the operation

	memory_copy(dest, src, length);		// This does mfence, so we do not need pipeline flush

}
