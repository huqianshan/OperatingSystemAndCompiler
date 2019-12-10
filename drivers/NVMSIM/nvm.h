/*
 * nvm.h
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

#ifndef __NVMSIM_NVM_H
#define __NVMSIM_NVM_H

/**
 * NVM operation codes
 */
#define NVM_READ	0
#define NVM_WRITE	1

#define NVMSIM_MEM_SECTORS 8



/**
 * The NVM model data
 */
struct nvm_model {

	/// The dirty bits
	unsigned* dirty;

	/// The cycle budget
	int budget;

	/// Number of reads
	unsigned stat_reads[2 /* cached? */];

	/// Number of writes
	unsigned stat_writes[2 /* cached? */];
};


/**
 * Calibrate the NVM model. This function can be called only after
 * the memory subsystem has been initialized.
 */
//void nvm_calibrate(void);

/**
 * Allocate NVM model data
 */
struct nvm_model* nvm_model_allocate(unsigned sectors);

/**
 * Free NVM model data
 */
void nvm_model_free(struct nvm_model* model);

/**
 * Perform a NVM read access
 */
void nvm_read(struct nvm_model* model, void* dest, const void* src, size_t length, sector_t sector);

/**
 * Perform a NVM write access
 */
void nvm_write(struct nvm_model* model, void* dest, const void* src, size_t length, sector_t sector);

#endif