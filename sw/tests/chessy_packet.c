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
    int errno;
    uint32_t rtc_freq = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
    
    // Initialize Chessy
    int fd_to_messy, fd_from_messy;
    errno = chessy_init(&fd_to_messy, &fd_from_messy);
    if (errno < 0) {
        semihost_printf("Error initializing Chessy: %d\n", errno);
        return 1;
    }

    // Create a Chessy packet
    chessy_packet packet;
    packet.timestamp = clint_get_mtime();
    packet.command = "test_command";
    packet.data = "test_data";

    // Send the packet
    errno = chessy_send(fd_to_messy, &packet);
    if (errno < 0) {
        semihost_printf("Error sending packet: %d\n", errno);
        return 1;
    }

    // Prepare to receive a packet
    chessy_packet received_packet;

    // Receive a packet (the same packet for testing)
    errno = chessy_receive(fd_from_messy, &received_packet);
    if (errno < 0) {
        semihost_printf("Error receiving packet: %d\n", errno);
        return 1;
    }
    // Print the received packet
    semihost_printf("Received packet: timestamp=%ld, command=%s, data=%s\n",
                    received_packet.timestamp, received_packet.command, received_packet.data);

    // Print elapsed time
    uint64_t current_cycle_counter = clint_get_mtime();
    uint64_t elapsed_time = current_cycle_counter - last_cycle_counter;
    uint64_t elapsed_time_ms = (elapsed_time * 1000) / rtc_freq;
    semihost_printf("Elapsed time: %ld cycles\n", elapsed_time);
    semihost_printf("Elapsed time: %ld ms\n", elapsed_time_ms);
    
    // Close Chessy
    errno = chessy_close(fd_to_messy, fd_from_messy);
    if (errno < 0) {
        semihost_printf("Error closing Chessy: %d\n", errno);
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
