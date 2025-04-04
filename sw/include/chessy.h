#ifndef CHESSY_H
#define CHESSY_H

// This file is a collection of functions used to communicate with Messy running on the host system

#include "semihost.h"
#include "jsmn.h"
#include "dif/clint.h"
#include <stdint.h>
#include <string.h>

#define FILE_TO_MESSY "/tmp/chessy/to_messy"
#define FILE_FROM_MESSY "/tmp/chessy/from_messy"
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
 * @param fd_to_messy Pointer to store the file descriptor for writing.
 * @param fd_from_messy Pointer to store the file descriptor for reading.
 * @return 0 on success, -1 on failure.
 */
int chessy_init(int *fd_to_messy, int *fd_from_messy)
{
    // Open the file for writing
    *fd_to_messy = semihost_open(FILE_TO_MESSY, SEMIHOST_OPEN_WB);
    if (*fd_to_messy < 0)
    {
        return -1; // Failed to open file for writing
    }

    // Open the file for reading
    *fd_from_messy = semihost_open(FILE_FROM_MESSY, SEMIHOST_OPEN_RB);
    if (*fd_from_messy < 0)
    {
        semihost_close(*fd_to_messy); // Close the first file
        return -1;                    // Failed to open file for reading
    }

    // Read the initial timestamp from clint_get_mtime()
    last_cycle_counter = clint_get_mtime();
    if (last_cycle_counter == 0)
    {
        semihost_close(*fd_to_messy);   // Close the first file
        semihost_close(*fd_from_messy); // Close the second file
        return ERR_INVALID_TIMESTAMP;   // Invalid timestamp
    }

    return 0; // Success
}

/**
 * @brief Send a Chessy packet to the file.
 *
 * @param fd File descriptor of the opened file.
 * @param packet Pointer to the Chessy packet to send.
 * @return 0 on success, -1 on failure.
 */
int chessy_send(int fd, struct chessy_packet *packet)
{
    // Create a JSON string from the packet data, leaving the first 4 bytes for length
    char json_string[MAX_PACKET_SIZE];
    snprintf(json_string + 4, sizeof(json_string) - 4, "{\"timestamp\":%ld,\"command\":\"%s\",\"data\":\"%s\"}",
             packet->timestamp, packet->command, packet->data);

    // Calculate the length of the JSON string
    int length = strlen(json_string + 4);

    // Write the length into the first 4 bytes
    json_string[0] = (length >> 24) & 0xFF;
    json_string[1] = (length >> 16) & 0xFF;
    json_string[2] = (length >> 8) & 0xFF;
    json_string[3] = length & 0xFF;

    // Write the JSON string to the file
    if (semihost_write(fd, json_string, length + 4) < 0)
    {
        return -1; // Failed to write to file
    }
    return 0; // Success
}

/**
 * @brief Read a Chessy packet from the file.
 *
 * @param fd File descriptor of the opened file.
 * @param packet Pointer to the Chessy packet to read into.
 * @return 0 on success, -1 on failure.
 */
int chessy_receive(int fd, struct chessy_packet *packet)
{
    // TODO: clean this, fix the json_string + 4 thing...
    // Read the length of the packet
    char length_buffer[4];
    if (semihost_read(fd, length_buffer, 4) < 0)
    {
        return -1; // Failed to read length
    }

    // Calculate the length of the packet
    int length = (length_buffer[0] << 24) | (length_buffer[1] << 16) | (length_buffer[2] << 8) | length_buffer[3];

    // Check if the length is valid
    if (length > MAX_PACKET_SIZE - 4)
    {
        return ERR_PACKET_TOO_LARGE; // Packet too large
    }

    // Read the packet data
    char json_string[MAX_PACKET_SIZE];
    if (semihost_read(fd, json_string + 4, length) < 0)
    {
        return -1; // Failed to read packet data
    }

    // Null-terminate the string
    json_string[length + 4] = '\0';

#ifdef DEBUG_LEVEL
    // Print the received JSON string for debugging
    semihost_printf("Received JSON: %s\n", json_string + 4);
#endif

    // Parse the JSON string into the packet structure
    jsmn_parser parser;
    jsmntok_t tokens[10];
    jsmn_init(&parser);

    int r = jsmn_parse(&parser, json_string + 4, strlen(json_string + 4), tokens, sizeof(tokens) / sizeof(tokens[0]));

    if (r < 0)
    {
        return ERR_INVALID_PACKET; // Invalid JSON packet
    }

    // Extract values from tokens and fill the packet structure
    for (int i = 1; i < r; i++)
    {
#ifdef DEBUG_LEVEL
        // Print token content
        semihost_printf("Token %d: %.*s, Value: %.*s\n", i,
                       tokens[i].end - tokens[i].start, json_string + 4 + tokens[i].start,
                       tokens[i + 1].end - tokens[i + 1].start, json_string + 4 + tokens[i + 1].start);
#endif

        if (tokens[i].type == JSMN_STRING && tokens[i].size == 1)
        {
            if (strncmp(json_string + 4 + tokens[i].start, "timestamp", tokens[i].end - tokens[i].start) == 0)
            {
            packet->timestamp = string_to_long(json_string + 4 + tokens[i + 1].start);
            }
            else if (strncmp(json_string + 4 + tokens[i].start, "command", tokens[i].end - tokens[i].start) == 0)
            {
            int length = tokens[i + 1].end - tokens[i + 1].start;
            static char command_buffer[MAX_PACKET_SIZE];
            strncpy(command_buffer, json_string + 4 + tokens[i + 1].start, length);
            command_buffer[length] = '\0';
            packet->command = command_buffer;
            }
            else if (strncmp(json_string + 4 + tokens[i].start, "data", tokens[i].end - tokens[i].start) == 0)
            {
            int length = tokens[i + 1].end - tokens[i + 1].start;
            static char data_buffer[MAX_PACKET_SIZE];
            strncpy(data_buffer, json_string + 4 + tokens[i + 1].start, length);
            data_buffer[length] = '\0';
            packet->data = data_buffer;
            }
            i++; // Skip the value token
        }
    }

#ifdef DEBUG_LEVEL
    // Print the parsed packet for debugging
    semihost_printf("Parsed packet: timestamp=%ld, command=%s, data=%s\n",
                   packet->timestamp, packet->command, packet->data);
#endif

    // Check if the packet is valid
    if (packet->timestamp == 0 || packet->command == NULL || packet->data == NULL)
    {
        return ERR_INVALID_PACKET; // Invalid packet
    }

    return 0;
}

/**
 * @brief Close the Chessy communication by closing both file descriptors.
 *
 * @param fd_to_messy File descriptor for writing.
 * @param fd_from_messy File descriptor for reading.
 * @return 0 on success, -1 on failure.
 */
int chessy_close(int fd_to_messy, int fd_from_messy)
{
    int result = 0;

    // Close the file descriptor for writing
    if (semihost_close(fd_to_messy) < 0)
    {
        result = -1; // Failed to close the write file
    }

    // Close the file descriptor for reading
    if (semihost_close(fd_from_messy) < 0)
    {
        result = -1; // Failed to close the read file
    }

    return result; // Return 0 if both succeeded, -1 if any failed
}

#endif // CHESSY_H