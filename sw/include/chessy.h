#ifndef CHESSY_H
#define CHESSY_H

// This file is a collection of functions used to communicate with Messy running on the host system

__attribute__((optimize("O0"))) uint64_t read_sensor(volatile uint64_t *sensor_address)
{
    // Initialize return data with invalid value
    volatile uint64_t sensor_data = 0xdeadbeef;
    // Force a break to let the host modify data
    __asm__ volatile("ebreak");

    return sensor_data;
}

__attribute__((optimize("O0"))) void write_sensor(volatile uint64_t *sensor_address, volatile uint64_t sensor_data)
{
    // Force a break to let the host read data
    __asm__ volatile("ebreak");
}

#endif // CHESSY_H