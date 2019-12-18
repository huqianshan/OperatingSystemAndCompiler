# A Block driver of Non-volatile Memory

## Version 1.0

### File Contains

- `module.c` : the initition of kernel module for driver

- `ramdecive.h/c` the implementention of driver

- `nvm.c` the non-volatile memory simmulation layer (Current Not Used)

- `mem.c` the implementention of  `memcpy` like function

- `nvmconfig.h` contains all of `#define` configuration (Current Not Used)

### Architecture

#### The simulation of `Non-volatile Memory`

- Write Delay

- Write Bandwidth

#### The Block Driver For `NVM`

- The Memory Management
  - Highmemory and Mapping to Kernel

- Write/Read Function
  - The Test of I/O throughput

#### The Address Transformer Mechanism

#### The Access Information Summary Table

#### Data consistency and Fault-Tolerance Mechaism
