#include "printf.h"
#include "semihost.h"
#include <string.h>
#include <stdint.h>
#include "regs/cheshire.h"
#include "util.h"
#include "params.h"
#include "dif/clint.h"
#include "chessy.h"

#define NUM_REPS 100
#define CHESSY
#define SENSOR_ADDRESS ((volatile uint64_t *)((uint8_t *)&__base_regs + CHESHIRE_SCRATCH_2_REG_OFFSET))

/* ---------------------------------- MAIN ---------------------------------- */
int main()
{
    volatile uint64_t start_cc, end_cc, start_time, end_time;
    volatile uint64_t data            = 0;
    volatile uint64_t *sensor_address = SENSOR_ADDRESS;

#ifdef CHESSY
    // Initialize sensor
    write_sensor(sensor_address, 0);
#endif

    // Start timer
    start_time = clint_get_mtime();
    start_cc   = get_mcycle();

    // Main loop
    do {
#ifdef CHESSY
        data = read_sensor(sensor_address);
#endif
        data += 1;
#ifdef CHESSY
        write_sensor(sensor_address, data);
#else
#endif
    } while (data < NUM_REPS);

    // Stop timer
    end_cc   = get_mcycle();
    end_time = clint_get_mtime();

    // Check for correct value
    if (data != NUM_REPS) {
        semihost_printf("Test failed! Final data in sensor is %ld, expected %d\n", data, NUM_REPS);
    } else {
        semihost_printf("----- Test completed successfully! -----\n");
        semihost_printf("Number of repetitions: %d\n", NUM_REPS);
        semihost_printf("Number of cycles: %ld\n", end_cc - start_cc);
        semihost_printf("Elapsed time: %ld ms\n", (end_time - start_time) / 1000);
        semihost_printf("----------------------------------------\n");
    }

    while (1) {
        wfi();
    }

    return 0;
}
