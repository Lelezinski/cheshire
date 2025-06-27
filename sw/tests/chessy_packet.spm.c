#include "printf.h"
#include "semihost.h"
#include "chessy.h"
#include <string.h>
#include <stdint.h>
#include "regs/cheshire.h"
#include "util.h"
#include "params.h"

/* ---------------------------------- MAIN ---------------------------------- */
int main()
{
    int ret;
    uint32_t rtc_freq = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
    uint64_t core_freq = clint_get_core_freq(rtc_freq, 2500);
    uint64_t current_cycle_counter, last_cycle_counter, elapsed_time_ms;
    
    // Initialize Chessy
    int fd_to_messy, fd_from_messy;
    ret = chessy_init(&fd_to_messy, &fd_from_messy);
    if (ret < 0) {
        semihost_printf("Error initializing Chessy: %d\n", ret);
        return 1;
    }

    uint64_t sensor_address = 0xdeadbeef; // Example address
    uint8_t data[MAX_PACKET_SIZE];
    memset(data, 0, sizeof(data));

    // Fill data with some example data
    const char *data_to_write = "Ankara Mesi ";
    size_t data_to_write_len = strlen(data_to_write);
    for (size_t i = 0; i < MAX_PACKET_SIZE; i++) {
        data[i] = data_to_write[i % data_to_write_len];
    }
    data[MAX_PACKET_SIZE - 1] = '\0'; // Null-terminate the string

    // Write data to sensor
    last_cycle_counter = clint_get_mtime();
    ret = chessy_request_write(fd_to_messy, fd_from_messy, sensor_address, (const char *)data);
    current_cycle_counter = clint_get_mtime();
    elapsed_time_ms = (current_cycle_counter - last_cycle_counter) * 1000 / core_freq;
    semihost_printf("Elapsed time for write: %ld cycles (%ld ms)\n", current_cycle_counter - last_cycle_counter, elapsed_time_ms);
    if (ret < 0) {
        semihost_printf("Error writing to sensor: %d\n", ret);
        return 1;
    }
    semihost_printf("Data written to address 0x%lx\n", sensor_address);

    // Read data from sensor
    last_cycle_counter = clint_get_mtime();
    ret = chessy_request_read(fd_to_messy, fd_from_messy, &sensor_address, data);
    current_cycle_counter = clint_get_mtime();
    elapsed_time_ms = (current_cycle_counter - last_cycle_counter) * 1000 / core_freq;
    semihost_printf("Elapsed time for read: %ld cycles (%ld ms)\n", current_cycle_counter - last_cycle_counter, elapsed_time_ms);

    if (ret < 0) {
        semihost_printf("Error reading from sensor: %d\n", ret);
        return 1;
    }
    semihost_printf("Data read from address 0x%lx:\n%s\n", sensor_address, data);

    // Close Chessy
    ret = chessy_close(fd_to_messy, fd_from_messy);
    if (ret < 0) {
        semihost_printf("Error closing Chessy: %d\n", ret);
        return 1;
    }

    // Print success message
    semihost_printf("Chessy closed successfully\n");

    while (1)
    {
        // spin
    }
    return 0;
}
