#include "regs/cheshire.h"
#include "dif/clint.h"
#include "dif/uart.h"
#include "params.h"
#include "util.h"
#include "printf.h"
#include <string.h>

// PLIC Register Offsets
#define PLIC_PRIORITY_OFFSET (4 * 10)   // Priority register for UART IRQ 10
#define PLIC_ENABLE_OFFSET (0x2000)     // Enable register for CPU 0
#define PLIC_CLAIM_OFFSET (0x200004)    // Claim register for CPU 0
#define PLIC_COMPLETE_OFFSET (0x200004) // Complete register for CPU 0

#define PLIC_PRIORITY *reg32(&__base_plic, PLIC_PRIORITY_OFFSET)
#define PLIC_ENABLE *reg32(&__base_plic, PLIC_ENABLE_OFFSET)
#define PLIC_CLAIM *reg32(&__base_plic, PLIC_CLAIM_OFFSET)
#define PLIC_COMPLETE *reg32(&__base_plic, PLIC_COMPLETE_OFFSET)

volatile int test = 7;

void uart_print(char *str)
{
    uart_write_str(&__base_uart, str, strlen(str));
    uart_write_str(&__base_uart, "\r", 1);
    uart_write_flush(&__base_uart);
}

/* ---------------------------- ENABLE INTERRUPTS --------------------------- */
// Enable UART RX Interrupt
void enable_uart_interrupt()
{
    *reg8(&__base_uart, UART_INTR_ENABLE_REG_OFFSET) = 0x01;
    fence();
}

// Enable UART Interrupt in PLIC
void enable_plic_uart_interrupt()
{
    PLIC_PRIORITY = 1;       // Set priority > 0 for UART IRQ
    PLIC_ENABLE |= (1 << 1); // TODO: CHECK Enable UART IRQ 1 in PLIC
    fence();
}

/* ---------------------------- MANAGE INTERRPTS ---------------------------- */
void uart_isr(void)
{
    // Just send char back
    uint8_t received = uart_read(&__base_uart);
    uart_write(&__base_uart, received);
}

// On trap, report relevant CSRs and spin
void trap_vector()
{
    uint64_t mcause, mepc, mip, mie, mstatus, mtval;
    asm volatile("csrr %0, mcause" : "=r"(mcause));

    // if (mcause == 0x8000000B)
    // {
    uint32_t irq_id = PLIC_CLAIM; // Claim the interrupt

    if (irq_id == 10)
    {               // UART interrupt ID
        uart_isr(); // Handle the UART interrupt
    }

    PLIC_COMPLETE = irq_id; // Mark interrupt as handled
    fence();                // Ensure memory consistency
    // }
}

/* ---------------------------------- MAIN ---------------------------------- */
int main()
{
    // Initialize UART
    uint32_t rtc_freq = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
    uint64_t reset_freq = clint_get_core_freq(rtc_freq, 2500);
    uart_init(&__base_uart, reset_freq, __BOOT_BAUDRATE);

    enable_uart_interrupt();      // Enable UART RX interrupt
    enable_plic_uart_interrupt(); // Enable UART interrupt in PLIC
    set_mie(1);                   // Enable machine-level global interrupts

    // Set variable to be read from GDB
    volatile int test = 7;
    printf("test before trap = %d\n", test);
    
    // Try to access something outside of the memory map
    // This should trigger a trap
    *reg8(0, 0) = 1; 
    
    // At this point GDB should have modified the value of test to 42
    printf("test after trap = %d\n", test);


    // Test UART ISR
    printf("UART ISR Test\n\r");
    
    while (1)
    {
        /* code */
    }
    
    


    uart_isr(); // Test UART ISR

    printf("\n\rNow waiting for interrupts...\n\r");
    while (1)
    {
        wfi(); // Wait for interrupt (low-power mode)
    }

    return 0;
}
