#include "printf.h"
#include "semihost.h"
#include <string.h>
#include <stdint.h>
#include "regs/cheshire.h"
#include "util.h"
#include "params.h"
#include "dif/clint.h"

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
    uint64_t core_freq = clint_get_core_freq(rtc_freq, 2500);
    uint64_t current_cycle_counter, last_cycle_counter, elapsed_time_ms;
    
    volatile const uint32_t magic_number = 420;
    volatile uint32_t data;
    volatile uint64_t* sensor_address = (volatile uint64_t*)((uint8_t*)&__base_regs + CHESHIRE_SCRATCH_2_REG_OFFSET);
    
    // Write data to sensor
    last_cycle_counter = clint_get_mtime();
    write_sensor(sensor_address, magic_number);
    current_cycle_counter = clint_get_mtime();
    elapsed_time_ms = (current_cycle_counter - last_cycle_counter) * 1000 / core_freq;
    semihost_printf("Wrote data to sensor: %ld\n", magic_number);
    semihost_printf("Elapsed time for write: %ld ms\n", elapsed_time_ms);

    // Read data from sensor
    last_cycle_counter = clint_get_mtime();
    data = read_sensor(sensor_address);
    current_cycle_counter = clint_get_mtime();
    elapsed_time_ms = (current_cycle_counter - last_cycle_counter) * 1000 / core_freq;
    fence();
    semihost_printf("Read data from sensor: %ld\n", data);
    semihost_printf("Elapsed time for read: %ld ms\n", elapsed_time_ms);
    
    while (1)
    {
        wfi(); 
    }
    
    return 0;
}
