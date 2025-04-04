#include "printf.h"
#include "semihost.h"
#include "jsmn.h"
#include <string.h>

#define FILE_PATH "/home/zcu102/git/chessy_test.txt"

/* ---------------------------------- MAIN ---------------------------------- */

int main()
{
    char write_buf[256];
    char read_buf[256];

    // Ask what to write to the file
    semihost_printf("Enter a string to write to %s: ", FILE_PATH);
    semihost_read(0, write_buf, sizeof(write_buf));
    
    // Open the file
    long fd = semihost_open(FILE_PATH, SEMIHOST_OPEN_W);
    if (fd < 0) {
        semihost_printf("Error opening file\n");
        return 1;
    }

    // Write the file
    if (semihost_write(fd, write_buf, strlen(write_buf)) < 0) {
        semihost_printf("Error writing file\n");
        return 1;
    }

    // Close the file
    if (semihost_close(fd) < 0) {
        semihost_printf("Error closing file\n");
        return 1;
    }

    // Open the file
    fd = semihost_open(FILE_PATH, SEMIHOST_OPEN_R);
    if (fd < 0) {
        semihost_printf("Error opening file\n");
        return 1;
    }

    // Read the file
    long read_len = semihost_read(fd, read_buf, sizeof(read_buf));
    if (read_len < 0) {
        semihost_printf("Error reading file\n");
        return 1;
    }

    // Print the file content
    semihost_printf("Read from %s: \"%s\"\n", FILE_PATH, read_buf);

    // Close the file
    if (semihost_close(fd) < 0) {
        semihost_printf("Error closing file\n");
        return 1;
    }


    while (1)
    {
        // spin
    }
    return 0;
}
