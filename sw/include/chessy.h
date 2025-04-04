#ifndef CHESSY_H
#define CHESSY_H

// This file is a collection of functions used to communicate with Messy running on the host system

#include "semihost.h"
#include "jsmn.h"
#include "dif/clint.h"
#include <stdint.h>
#include <string.h>

#define FILE_REQ_ADDRESS "/tmp/chessy/req_address"
#define FILE_REQ_DATA "/tmp/chessy/req_data"
#define MAX_PACKET_SIZE 1024

#define ERR_INVALID_PACKET -2
#define ERR_INVALID_FD -3
#define ERR_PACKET_TOO_LARGE -4
#define ERR_INVALID_TIMESTAMP -5
// TODO: check for errors

#define DEBUG_LEVEL 1

typedef struct chessy_packet
{
    long timestamp;
    const char *command;
    const char *data;
} chessy_packet;

static uint64_t last_cycle_counter = 0;

/**
 * @brief Convert a string to a long integer.
 *
 * @param str The string to convert.
 * @return The converted long integer.
 */
static long string_to_long(const char *str)
{
    long result = 0;
    int sign = 1;

    // Handle optional sign
    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    else if (*str == '+')
    {
        str++;
    }

    // Convert digits to integer
    while (*str >= '0' && *str <= '9')
    {
        result = result * 10 + (*str - '0');
        str++;
    }

    return result * sign;
}

/**
 * @brief Initialize the Chessy communication by opening files and reading the initial timestamp.
 *
 * @param fd_req_address Pointer to store the file descriptor for request addresses.
 * @param fd_req_data Pointer to store the file descriptor for request data.
 * @return 0 on success, negative error code on failure.
 */
int chessy_init(int *fd_req_address, int *fd_req_data)
{
    // Open the request address file
    *fd_req_address = semihost_open(FILE_REQ_ADDRESS, SEMIHOST_OPEN_WB_PLUS);
    if (*fd_req_address < 0)
    {
        return ERR_INVALID_FD; // Invalid file descriptor
    }
    // Open the request data file
    *fd_req_data = semihost_open(FILE_REQ_DATA, SEMIHOST_OPEN_WB_PLUS);
    if (*fd_req_data < 0)
    {
        semihost_close(*fd_req_address); // Close the first file
        return ERR_INVALID_FD;          // Invalid file descriptor
    }

    // Read the initial timestamp from clint_get_mtime()
    last_cycle_counter = clint_get_mtime();

    return 0; // Success
}

/**
 * @brief Request a read at a given address to Messy.
 *
 * @param fd_req_address File descriptor for request addresses.
 * @param fd_req_data File descriptor for request data.
 * @param address Pointer to store the address to read from.
 * @param data Pointer to store the read data.
 * @return Length of data read on success, negative error code on failure.
 */
int chessy_request_read(int fd_req_address, int fd_req_data, uint64_t *address, uint8_t *data)
{
    // Update the last cycle counter
    last_cycle_counter = clint_get_mtime();
    uint64_t buffer[2] = {last_cycle_counter, *address};

    // Write the timestamp and the address concatenated to the request address file
    if (semihost_write(fd_req_address, (char *)buffer, sizeof(buffer)) < 0)
    {
        return -1; // Failed to write to file
    }

    // Force a break to wait for the host to respond
    semihost_break();

    // Read the data from the request data file until EOF or MAX_PACKET_SIZE
    int data_length = semihost_flen(fd_req_data);
    if (data_length < 0)
    {
        return -2; // Failed to get file length
    }
    if (data_length > MAX_PACKET_SIZE)
    {
        return -3; // Packet too large
    }
    if (semihost_read(fd_req_data, (char *)data, data_length) < 0)
    {
        return -4; // Failed to read from file
    }

    return data_length; // Return the length of data read
}

/**
 * @brief Request a write at a given address to Messy.
 *
 * @param fd_req_address File descriptor for request addresses.
 * @param fd_req_data File descriptor for request data.
 * @param address Address to write to.
 * @param data Data to write.
 * @return 0 on success, negative error code on failure.
 */
int chessy_request_write(int fd_req_address, int fd_req_data, uint64_t address, const char *data)
{
    // Update the last cycle counter
    last_cycle_counter = clint_get_mtime();
    uint64_t buffer[2] = {last_cycle_counter, address};

    // Write the timestamp and the address concatenated to the request address file
    if (semihost_write(fd_req_address, (char *)buffer, sizeof(buffer)) < 0)
    {
        return -1; // Failed to write to file
    }

    // Write the data to the request data file
    if (semihost_write(fd_req_data, data, strlen(data)) < 0)
    {
        return -2; // Failed to write to file
    }

    // Force a break to wait for the host to respond
    semihost_break();

    return 0; // Success
}

/**
 * @brief Close the Chessy communication by closing both file descriptors.
 *
 * @param fd_req_address File descriptor for request addresses.
 * @param fd_req_data File descriptor for request data.
 * @return 0 on success, -1 on failure.
 */
int chessy_close(int fd_req_address, int fd_req_data)
{
    int result = 0;

    // Close the file descriptor for request addresses
    if (semihost_close(fd_req_address) < 0)
    {
        result = -1; // Failed to close the request address file
    }

    // Close the file descriptor for request data
    if (semihost_close(fd_req_data) < 0)
    {
        result = -1; // Failed to close the request data file
    }

    return result; // Return 0 if both succeeded, -1 if any failed
}

#endif // CHESSY_H