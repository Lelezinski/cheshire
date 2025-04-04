#include "regs/cheshire.h"
#include "dif/clint.h"
#include "dif/uart.h"
#include "params.h"
#include "util.h"
#include "printf.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>

////
// Testing pieces of code for future NN integration
////

#define max(a,b) ((a) > (b) ? (a) : (b))

/* --------------------------------- PARAMS --------------------------------- */
#define DEBUG
#define INPUT_HEIGHT 5
#define INPUT_WIDTH 5
#define FILTER_HEIGHT 3
#define FILTER_WIDTH 3
#define STRIDE 1
#define PADDING 0
#define OUTPUT_HEIGHT ((INPUT_HEIGHT - FILTER_HEIGHT + 2 * PADDING) / STRIDE + 1)
#define OUTPUT_WIDTH ((INPUT_WIDTH - FILTER_WIDTH + 2 * PADDING) / STRIDE + 1)
/* -------------------------------------------------------------------------- */

// UART utils
void uart_print(char *str) {
    uart_write_str(&__base_uart, str, strlen(str));
    uart_write_str(&__base_uart, "\n\r", 2);
    uart_write_flush(&__base_uart);
}

void uart_print_2d_matrix(int height, int width, int matrix[height][width]) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            char value_str[2];
            itoa(matrix[i][j], value_str, 10);
            uart_write_str(&__base_uart, value_str, 2);
            uart_write_str(&__base_uart, " ", 1);
        }
        uart_write_str(&__base_uart, "\n\r", 2);
    }
    uart_write_flush(&__base_uart);
}

// Function to perform 2D convolution
void conv2d(int input_height, int input_width, int filter_height, int filter_width,
            int input[input_height][input_width], int filter[filter_height][filter_width],
            int bias, int stride, int padding, int output_height, int output_width,
            int output[output_height][output_width]) {
    
    // Apply padding to the input (optional)
    int padded_height = input_height + 2 * padding;
    int padded_width = input_width + 2 * padding;
    int padded_input[padded_height][padded_width];
    
    for (int i = 0; i < padded_height; i++) {
        for (int j = 0; j < padded_width; j++) {
            if (i < padding || i >= padded_height - padding || j < padding || j >= padded_width - padding) {
                padded_input[i][j] = 0; // Padding with zeros
            } else {
                padded_input[i][j] = input[i - padding][j - padding];
            }
        }
    }
    
    // Initialize the output
    for (int i = 0; i < output_height; i++) {
        for (int j = 0; j < output_width; j++) {
            output[i][j] = 0;
        }
    }

    // Perform the convolution
    for (int i = 0; i <= padded_height - filter_height; i += stride) {
        for (int j = 0; j <= padded_width - filter_width; j += stride) {
            int conv_sum = 0;
            for (int fi = 0; fi < filter_height; fi++) {
                for (int fj = 0; fj < filter_width; fj++) {
                    conv_sum += padded_input[i + fi][j + fj] * filter[fi][fj];
                }
            }
            output[i / stride][j / stride] = conv_sum + bias;
        }
    }
}

// Function to apply ReLU activation
void relu(int output_height, int output_width, int output[output_height][output_width]) {
    for (int i = 0; i < output_height; i++) {
        for (int j = 0; j < output_width; j++) {
            output[i][j] = max(0, output[i][j]);
        }
    }
}

int main(void) {
    srand(0);
    uint64_t start_time, end_time, nn_start_time, nn_end_time;
    
    start_time = clint_get_mtime();
    printf("Current time: %lu\n", start_time);

    // Initialize UART
    uint32_t rtc_freq = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
    uint64_t reset_freq = clint_get_core_freq(rtc_freq, 2500);
    uart_init(&__base_uart, reset_freq, __BOOT_BAUDRATE);

    uart_print("UART connection established!");

    // Allocate memory for input, filter, and output
    int input[INPUT_HEIGHT][INPUT_WIDTH] = {0};
    int filter[FILTER_HEIGHT][FILTER_WIDTH] = {0};
    int output[OUTPUT_HEIGHT][OUTPUT_WIDTH] = {0};

    // Initialize filter with random values
    for (int i = 0; i < FILTER_HEIGHT; i++) {
        for (int j = 0; j < FILTER_WIDTH; j++) {
            filter[i][j] = rand() % 10; // Random values between 0 and 9
        }
    }

#ifdef DEBUG
    // Send the filter values over UART
    uart_print("Filter values:");
    uart_print_2d_matrix(FILTER_HEIGHT, FILTER_WIDTH, filter);
#endif

    // Receive input values stream over UART
    uart_print("Reading input values...");
    for (int i = 0; i < INPUT_HEIGHT; i++) {
        for (int j = 0; j < INPUT_WIDTH; j++) {
            char c = uart_read(&__base_uart);
            input[i][j] = c - '0'; // Convert ASCII to integer
        }
    }

    int json_length = 10;
    unsigned char length_buffer[4];
    length_buffer[0] = json_length & 0x000000FF;
    length_buffer[1] = (json_length >> 8) & 0x000000FF;
    length_buffer[2] = (json_length >> 16) & 0x000000FF;
    length_buffer[3] = (json_length >> 24) & 0x000000FF;                   

    uart_print("111111111111111111111");

    uart_write_str(&__base_uart, length_buffer, 4);
    uart_write_flush(&__base_uart);

#ifdef DEBUG
    // Send the input values over UART
    uart_print("Input values:");
    uart_print_2d_matrix(INPUT_HEIGHT, INPUT_WIDTH, input);
#endif

    // Perform 2D convolution
    nn_start_time = clint_get_mtime();
    conv2d(INPUT_HEIGHT, INPUT_WIDTH, FILTER_HEIGHT, FILTER_WIDTH, input, filter, 0, STRIDE, PADDING, OUTPUT_HEIGHT, OUTPUT_WIDTH, output);
    nn_end_time = clint_get_mtime();

#ifdef DEBUG
    // Send the output values after convolution over UART
    uart_print("Output values after convolution:");
    uart_print_2d_matrix(OUTPUT_HEIGHT, OUTPUT_WIDTH, output);
#endif

    end_time = clint_get_mtime();
    
    char time_str[100];
    snprintf(time_str, sizeof(time_str), "NN execution time: %lu cycles, %lu ms", 
             nn_end_time - nn_start_time, (nn_end_time - nn_start_time) * 1000 / rtc_freq);
    uart_print(time_str);
    
    snprintf(time_str, sizeof(time_str), "Total execution time: %lu cycles, %lu ms", 
             end_time - start_time, (end_time - start_time) * 1000 / rtc_freq);
    uart_print(time_str);

    return 0;
}