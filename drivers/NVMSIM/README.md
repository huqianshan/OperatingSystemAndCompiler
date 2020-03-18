# A Block driver of Non-volatile Memory

## Version 1.0

### File Contains

- `ramdecive.h/c` the implementention of driver

- `mem.c` the implementention of  `memcpy` like function

- `nvmconfig.h` contains all of `#define` configuration (Current Not Used)

### Architecture

#### The simulation of `Non-volatile Memory`

- Write Delay

- Write Bandwidth

#### The Block Driver For `NVM`

- The Memory Management
  - Highmemory and Mapping to Kernel
  - `DONE`

- Write/Read Function
  - The Test of I/O throughput
  - `DONE`

#### Free Block Manaement

- `BitMap`

  - clear set get `Done`

#### The Address Transformer Mechanism

#### The Access Information Summary Table

- 32 sectors `Done`

- Todo **lock**

#### Data consistency and Fault-Tolerance Mechaism
