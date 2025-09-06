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

#define SIZE_BYTES 2400

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
    volatile uint8_t data[SIZE_BYTES] = {0};
    volatile uint64_t *sensor_data_address      = (volatile uint64_t *)(GESTURE_DATA_REG_BASE + GESTURE_SENSOR_BASE);
    volatile uint64_t *sensor_ctrl_address      = (volatile uint64_t *)(GESTURE_CONTROL_REG_BASE + GESTURE_SENSOR_BASE);
    volatile uint64_t *robotic_arm_ctrl_address = (volatile uint64_t *)(ROBOTIC_ARM_CONTROL_REG_BASE + ROBOTIC_ARM_BASE);
    volatile uint64_t *robotic_arm_data_address = (volatile uint64_t *)(ROBOTIC_ARM_MOVEMENT_REG_BASE + ROBOTIC_ARM_BASE);

#ifdef CHESSY
    semihost_printf("Starting sensor test. Chessy protocol enabled.\n");
#else
    uint32_t rtc_freq   = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
    uint64_t reset_freq = clint_get_core_freq(rtc_freq, 2500);
    uart_init(&__base_uart, reset_freq, __BOOT_BAUDRATE);

    uart_print("Starting sensor test...\n");
#endif

    // Reset mtime register to zero
    *(uint64_t *)reg32(&__base_clint, CLINT_MTIME_LOW_REG_OFFSET) = 0;

    // Start timer
    start_time = clint_get_mtime();
    start_cc   = get_mcycle();

    uint8_t start_bit = 1;

    // Start sensor
    write_sensor(sensor_ctrl_address, 1, &start_bit);
    // Start robotic arm
    write_sensor(robotic_arm_ctrl_address, 1, &start_bit);

    //clint_spin_ticks(100000); // Simulate some computation (e.g., 100ms)

    // Read data
    read_sensor(sensor_data_address, SIZE_BYTES, (uint8_t *)&data); // Read SIZE_BYTES bytes from sensor data register

    // Stop timer
    end_cc   = get_mcycle();
    end_time = clint_get_mtime();

    // Print results

#ifdef CHESSY
    semihost_printf("\n----- Test completed successfully! -----\n");
    semihost_printf("Sensor data (first byte): %d\n", data[0]);
    semihost_printf("Sensor data (last byte): %d\n", data[SIZE_BYTES - 1]);
    semihost_printf("Number of cycles: %ld\n", end_cc - start_cc);
    semihost_printf("Elapsed time: %ld ms\n", (end_time - start_time) / 1000);
    semihost_printf("----------------------------------------\n\n");

    while (1) {
        wfi();
    }
#else
    uart_print("----- Test completed successfully! -----");
#endif
    return 0;
}
