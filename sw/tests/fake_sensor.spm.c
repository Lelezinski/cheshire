#include "printf.h"
#include "semihost.h"
#include "chessy.h"
#include <string.h>
#include <stdint.h>
#include "regs/cheshire.h"
#include "util.h"
#include "params.h"

__attribute__((optimize("O0"))) uint64_t read_sensor(volatile uint64_t* sensor_address)
{
    // Initialize return data with invalid value
    volatile uint64_t sensor_data = 0xdeadbeef;
    // Force a break to let the host modify data
    __asm__ volatile("ebreak");

    return sensor_data;
}

__attribute__((optimize("O0"))) void write_sensor(volatile uint64_t* sensor_address, volatile uint64_t sensor_data)
{
    // Force a break to let the host read data
    __asm__ volatile("ebreak");
}

/* ---------------------------------- MAIN ---------------------------------- */
int main()
{
    int ret;
    volatile uint32_t rtc_freq = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
    
    volatile const uint32_t magic_number = 420;
    volatile uint32_t data;
    volatile uint64_t* sensor_address = (volatile uint64_t*)((uint8_t*)&__base_regs + CHESHIRE_SCRATCH_2_REG_OFFSET);

    last_cycle_counter = clint_get_mtime();

    // Write data to sensor
    write_sensor(sensor_address, magic_number);
    semihost_printf("Wrote data to sensor: %ld\n", magic_number);

    // Read data from sensor
    data = read_sensor(sensor_address);
    fence();
    semihost_printf("Read data from sensor: %ld\n", data);

    // Print elapsed time
    uint64_t current_cycle_counter = clint_get_mtime();
    uint64_t elapsed_time = current_cycle_counter - last_cycle_counter;
    uint64_t elapsed_time_ms = (elapsed_time * 1000) / rtc_freq;
    semihost_printf("Elapsed time: %ld cycles\n", elapsed_time);
    semihost_printf("Elapsed time: %ld ms\n", elapsed_time_ms);
    
    while (1)
    {
        wfi(); 
    }
    
    return 0;
}
