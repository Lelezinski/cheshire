#ifndef CHESSY_H
#define CHESSY_H

#include <stdint.h>
#include "regs/cheshire.h"
#include "dif/clint.h"
#include "util.h"
#include "regs/clint.h"

// Chessy request types
#define CHESSY_REQ_WRITE 0x00
#define CHESSY_REQ_READ 0x01
#define CHESSY_REQ_8BIT 0x01
#define CHESSY_REQ_32BIT 0x04
#define CHESSY_REQ_64BIT 0x08

// This file is a collection of functions used to communicate with Messy running on the host system
// If CHESSY is defined, the following functions will be used to interact with the Messy host
// Otherwise, they will be mocked by fake sensor accesses
#ifdef CHESSY

__attribute__((optimize("O0"))) uint64_t read_sensor(volatile uint64_t *sensor_address)
{
    // Save the current request timestamp
    volatile uint64_t req_timestamp = clint_get_mtime();
    // Initialize return data with invalid value
    volatile uint64_t sensor_data = 0xdeadbeef;
    // Force a break to let the host modify data
    __asm__ volatile("ebreak");

    // Set mtime to req_timestamp
    *(uint64_t *)reg32(&__base_clint, CLINT_MTIME_LOW_REG_OFFSET) = req_timestamp;

    return sensor_data;
}
__attribute__((optimize("O0"))) void write_sensor(volatile uint64_t *sensor_address, volatile uint64_t sensor_data)
{
    // Save the current request timestamp
    volatile uint64_t req_timestamp = clint_get_mtime();
    // Force a break to let the host read data
    __asm__ volatile("ebreak");

    // Set mtime to req_timestamp
    *(uint64_t *)reg32(&__base_clint, CLINT_MTIME_LOW_REG_OFFSET) = req_timestamp;
}

#else

__attribute__((optimize("O0"))) uint64_t read_sensor(volatile uint64_t *sensor_address)
{
    return get_mcycle(); // Read random data
}

__attribute__((optimize("O0"))) void write_sensor(volatile uint64_t *sensor_address, volatile uint64_t sensor_data)
{
    __asm__ volatile("nop"); // Do nothing
}

#endif

#endif // CHESSY_H