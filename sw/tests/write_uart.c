#include "regs/cheshire.h"
#include "dif/clint.h"
#include "dif/uart.h"
#include "params.h"
#include "util.h"

#define BUFFER_SIZE 128

int main(void) {
    char str[] = "Hello World!\r\nType something and press Enter:\r\n";
    char buffer[BUFFER_SIZE];
    int i = 0;

    uint32_t rtc_freq = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
    uint64_t reset_freq = clint_get_core_freq(rtc_freq, 2500);

    uart_init(&__base_uart, reset_freq, __BOOT_BAUDRATE);

    // Write the hello message
    uart_write_str(&__base_uart, str, sizeof(str));
    uart_write_flush(&__base_uart);

    // Read from UART until newline or buffer is full
    while (i < BUFFER_SIZE - 1) {
        char c = uart_read(&__base_uart);
        buffer[i++] = c;

        // Echo the character back
        uart_write(&__base_uart, c);

        if (c == '\r' || c == '\n') {
            break;
        }
    }

    buffer[i] = '\0'; // Null-terminate the string

    // Write a newline
    uart_write_str(&__base_uart, "\r\n", 2);

    // Echo back the received string
    uart_write_str(&__base_uart, "You typed: ", 11);
    uart_write_str(&__base_uart, buffer, i);
    uart_write_str(&__base_uart, "\r\n", 2);
    uart_write_flush(&__base_uart);

    return 0;
}