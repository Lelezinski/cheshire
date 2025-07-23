#include "printf.h"
#include "semihost.h"
#include <string.h>
#include <stdint.h>
#include "regs/cheshire.h"
#include "util.h"
#include "params.h"
#include "dif/clint.h"
#include "chessy.h"

/* ---------------------------------- MAIN ---------------------------------- */
int main()
{
    volatile uint64_t start_time, end_time;
    volatile const uint32_t magic_number = 420;
    volatile uint64_t data;
    volatile uint64_t* sensor_address = (volatile uint64_t*)((uint8_t*)&__base_regs + CHESHIRE_SCRATCH_2_REG_OFFSET);
    
    // Write data to sensor
    start_time = clint_get_mtime();
    write_sensor(sensor_address, magic_number);
    end_time = clint_get_mtime();
    semihost_printf("Wrote data to sensor: %ld\n", magic_number);
    semihost_printf("Elapsed time for write: %ld ms\n", (end_time - start_time) / 1000);

    // Read data from sensor
    start_time = clint_get_mtime();
    data = read_sensor(sensor_address);
    end_time = clint_get_mtime();
    semihost_printf("Read data from sensor: %ld\n", data);
    semihost_printf("Elapsed time for read: %ld ms\n", (end_time - start_time) / 1000);

    while (1)
    {
        wfi(); 
    }
    
    return 0;
}
