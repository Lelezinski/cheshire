#ifndef CHESSY_H
#define CHESSY_H

#include <stdint.h>
#include "regs/cheshire.h"
#include "dif/clint.h"
#include "util.h"
#include "regs/clint.h"

//
// Enable Chessy protocol?
//
#define CHESSY

//
// Messy managed peripherals
//

// Pheripherals base addresses in Messy register map
#define GESTURE_SENSOR_BASE 0x000
#define ROBOTIC_ARM_BASE 0x1000

// Gesture sensor registers addresses
#define GESTURE_CONTROL_REG_BASE 0x00 ///< Base address for the control register (start/stop bit)
#define GESTURE_MODULE_REG_BASE 0x04  ///< Base address for the MODULE amount register
#define GESTURE_STATUS_REG_BASE 0x08  ///< Base address for the status register (new data present bit)
#define GESTURE_DATA_REG_BASE 0x0C    ///< Base address for the data register

// Robotic arm registers addresses
#define ROBOTIC_ARM_CONTROL_REG_BASE 0x00 ///< Base address for the control register (start/stop bit)
#define ROBOTIC_ARM_STATUS_REG_BASE 0x04  ///< Base address for the status register
#define ROBOTIC_ARM_MOVEMENT_REG_BASE 0x08 ///< Base address for the input movement register

// This file is a collection of functions used to communicate with Messy running on the host system
// If CHESSY is defined, the following functions will be used to interact with the Messy host
// Otherwise, they will be mocked by fake sensor accesses
#ifdef CHESSY

// Chessy access function, triggers an ebreak to notify the host of a new request
__attribute__((optimize("O0"))) void __chessy_access(volatile uint8_t req_is_read, volatile uint64_t *req_addr, volatile uint8_t *req_data, volatile unsigned int req_size)
{
    // Save the current request timestamp
    volatile uint64_t req_timestamp = clint_get_mtime();

    // Force a break to give control to the host
    __asm__ volatile("ebreak");

    // Set mtime to req_timestamp to synchronize with host
    *(uint64_t *)reg32(&__base_clint, CLINT_MTIME_LOW_REG_OFFSET) = req_timestamp;
}

// Wrappers for read and write sensor functions
void read_sensor(volatile uint64_t *address, volatile unsigned int size_bytes, volatile uint8_t *data)
{
    __chessy_access(1, address, data, size_bytes);
}

void write_sensor(volatile uint64_t *address, volatile unsigned int size_bytes, volatile uint8_t *data)
{
    __chessy_access(0, address, data, size_bytes);
}

__attribute__((optimize("O0"))) void chessy_close()
{
    // Notify the host that the program is exiting
    __asm__ volatile("ebreak");
}

#else

void read_sensor(volatile uint64_t *address, volatile unsigned int size_bytes)
{
    return get_mcycle(); // Read random data
}

void write_sensor(volatile uint64_t *address, volatile unsigned int size_bytes, volatile uint8_t *data)
{
    __asm__ volatile("nop"); // Do nothing
}

#endif

#endif // CHESSY_H