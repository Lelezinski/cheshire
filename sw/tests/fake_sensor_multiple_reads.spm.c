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

#define NUM_REPS 10
#define CHESSY ///< Enable Chessy protocol, otherwise use dummy sensor accesses

// Sensor registers addresses
#define CONTROL_REG_BASE 0x00 ///< Base address for the control register (start/stop bit)
#define MODULE_REG_BASE 0x04  ///< Base address for the MODULE amount register
#define STATUS_REG_BASE 0x08  ///< Base address for the status register (new data present bit)
#define DATA_REG_BASE 0x0C    ///< Base address for the data register

/* ---------------------------------- MAIN ---------------------------------- */
int main()
{
    volatile uint64_t start_cc, end_cc, start_time, end_time;
    volatile uint64_t data                 = 0;
    volatile uint64_t sum                  = 0;
    volatile uint64_t *sensor_address      = (volatile uint64_t *)(DATA_REG_BASE);
    volatile uint64_t *sensor_ctrl_address = (volatile uint64_t *)(CONTROL_REG_BASE);

#ifdef CHESSY
    semihost_printf("Starting sensor test. Chessy protocol enabled.\n");
#else
    uint32_t rtc_freq   = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
    uint64_t reset_freq = clint_get_core_freq(rtc_freq, 2500);
    uart_init(&__base_uart, reset_freq, __BOOT_BAUDRATE);

    char str[] = "Starting sensor test...\n";
    uart_write_str(&__base_uart, str, sizeof(str) - 1);
    uart_write_flush(&__base_uart);
#endif

    // Reset mtime register to zero
    *(uint64_t *)reg32(&__base_clint, CLINT_MTIME_LOW_REG_OFFSET) = 0;

    // Start sensor
    write_sensor(sensor_ctrl_address, 1);

    // Start timer
    start_time = clint_get_mtime();
    start_cc   = get_mcycle();

    // Main loop
    for (int i = 0; i < NUM_REPS; i++) {
        data = read_sensor(sensor_address);
        sum += data; // Accumulate the data
        clint_spin_ticks(100000); // Simulate some computation (e.g., 100ms)
    }

    // Stop timer
    end_cc   = get_mcycle();
    end_time = clint_get_mtime();

    // Print results

#ifdef CHESSY
    semihost_printf("----- Test completed successfully! -----\n");
    semihost_printf("Number of repetitions: %d\n", NUM_REPS);
    semihost_printf("Number of cycles: %ld\n", end_cc - start_cc);
    semihost_printf("Elapsed time: %ld ms\n", (end_time - start_time) / 1000);
    semihost_printf("Sum of sensor data: %ld\n", sum);
    semihost_printf("----------------------------------------\n");

    while (1) {
        wfi();
    }
#else
    /*
        char buf[64];

        uart_write_str(&__base_uart, "----- Test completed successfully! -----\n", 37);
        uart_write_flush(&__base_uart);

        uart_write_str(&__base_uart, "Number of repetitions: ", 24);
        uart_write_flush(&__base_uart);

        snprintf(buf, sizeof(buf), "%d\n", NUM_REPS);
        uart_write_str(&__base_uart, buf, strlen(buf));
        uart_write_flush(&__base_uart);

        uart_write_str(&__base_uart, "Number of cycles: ", 18);
        uart_write_flush(&__base_uart);

        snprintf(buf, sizeof(buf), "%ld\n", end_cc - start_cc);
        uart_write_str(&__base_uart, buf, strlen(buf));
        uart_write_flush(&__base_uart);

        uart_write_str(&__base_uart, "Elapsed time: ", 15);
        uart_write_flush(&__base_uart);

        snprintf(buf, sizeof(buf), "%ld ms\n", (end_time - start_time) / 1000);
        uart_write_str(&__base_uart, buf, strlen(buf));
        uart_write_flush(&__base_uart);

        uart_write_str(&__base_uart, "Sum of sensor data: ", 20);
        uart_write_flush(&__base_uart);

        snprintf(buf, sizeof(buf), "%ld\n", sum);
        uart_write_str(&__base_uart, buf, strlen(buf));
        uart_write_flush(&__base_uart);

        uart_write_str(&__base_uart, "----------------------------------------\n", 42);
        uart_write_flush(&__base_uart);
         */
#endif
    return 0;
}
