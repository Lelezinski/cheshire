#include "printf.h"
#include "semihost.h"
#include <string.h>
#include <stdint.h>
#include "regs/cheshire.h"
#include "util.h"
#include "params.h"
#include "dif/clint.h"
#include "chessy.h"
#include "dif/uart.h"

#define CEIL_BYTES 400
#define STEP_BYTES 8

//#define PROFILE_CHESSY ///< Enable profiling of Cheshire adapter, WILL BREAK SYNCHRONIZATION!

// UART helper
void uart_print(char *str)
{
    uart_write_str(&__base_uart, str, strlen(str));
    uart_write_str(&__base_uart, "\r", 1);
    uart_write_flush(&__base_uart);
}

/* ---------------------------------- MAIN ---------------------------------- */
int main()
{
    volatile uint64_t start_cc, end_cc, start_time, end_time;
    volatile uint64_t data                 = 0;
    volatile uint64_t sum                  = 0;
    volatile uint64_t *sensor_data_address      = (volatile uint64_t *)(GESTURE_DATA_REG_BASE + GESTURE_SENSOR_BASE);
    volatile uint64_t *sensor_ctrl_address      = (volatile uint64_t *)(GESTURE_CONTROL_REG_BASE + GESTURE_SENSOR_BASE);
    volatile uint64_t *robotic_arm_ctrl_address = (volatile uint64_t *)(ROBOTIC_ARM_CONTROL_REG_BASE + ROBOTIC_ARM_BASE);
    volatile uint64_t *robotic_arm_data_address = (volatile uint64_t *)(ROBOTIC_ARM_MOVEMENT_REG_BASE + ROBOTIC_ARM_BASE);

    // Reset mtime register to zero
    *(uint64_t *)reg32(&__base_clint, CLINT_MTIME_LOW_REG_OFFSET) = 0;

    uint8_t start_bit = 1;
    // Start sensor
    write_sensor(sensor_ctrl_address, 1, &start_bit);
    // Start robotic arm
    write_sensor(robotic_arm_ctrl_address, 1, &start_bit);

    semihost_printf("Starting profiling tests...\n");
    semihost_printf("Doing %d writes, then %d reads.\n\n", (CEIL_BYTES / STEP_BYTES), (CEIL_BYTES / STEP_BYTES));

    #ifdef TEST_TIMER
    // Test timer
    start_time = clint_get_mtime();
    clint_spin_ticks(5000000); // Delay for 5 seconds
    end_time   = clint_get_mtime();
    semihost_printf("Timer test for 5 sec: start %ld, end %ld, diff %ld us\n\n", start_time, end_time, end_time - start_time);
    #endif

    // WRITES
    uint64_t tot_write_time = 0;
    semihost_printf("---------------------- WRITE tests ----------------\n");
    semihost_printf("Starting WRITE tests: %d to %d bytes, step %d bytes\n", STEP_BYTES, CEIL_BYTES, STEP_BYTES);
    for (int i = STEP_BYTES; i <= CEIL_BYTES; i += STEP_BYTES) {
        // Start timer
        start_time = clint_get_mtime();
        #ifdef PROFILE_CHESSY
        write_sensor(sensor_data_address, STEP_BYTES, (uint8_t *)&i); // Write i to sensor data register
        #endif
        end_time = clint_get_mtime();
        semihost_printf("[%d bytes]:\t%ld us\n", i, (end_time - start_time));
        tot_write_time += (end_time - start_time);
    }

    // READS
    uint64_t tot_read_time = 0;
    semihost_printf("---------------------- READ tests -----------------\n");
    semihost_printf("Starting READ tests: %d to %d bytes, step %d bytes\n", STEP_BYTES, CEIL_BYTES, STEP_BYTES);
    for (int i = STEP_BYTES; i <= CEIL_BYTES; i += STEP_BYTES) {
        // Start timer
        start_time = clint_get_mtime();
        #ifdef PROFILE_CHESSY
        read_sensor(sensor_data_address, STEP_BYTES, (uint8_t *)&data); // Read i from sensor data register
        #endif
        end_time = clint_get_mtime();
        semihost_printf("[%d bytes]:\t%ld us\t(data: %ld)\n", i, (end_time - start_time), data);
        tot_read_time += (end_time - start_time);
    }

    // Print results

#ifdef CHESSY
    semihost_printf("\n----------- Test completed successfully! ----------\n");
    semihost_printf("Total WRITE time (%d samples): %ld us\n", CEIL_BYTES / STEP_BYTES, tot_write_time);
    semihost_printf("Total READ time  (%d samples): %ld us\n", CEIL_BYTES / STEP_BYTES, tot_read_time);
    semihost_printf("Average WRITE time: %ld us\n", tot_write_time / (CEIL_BYTES / STEP_BYTES));
    semihost_printf("Average READ time: %ld us\n", tot_read_time / (CEIL_BYTES / STEP_BYTES));
    semihost_printf("---------------------------------------------------\n\n");

    chessy_close();
#else
    uart_print("----- Test completed successfully! -----");
#endif
    return 0;
}
