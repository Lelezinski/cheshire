#ifndef SEMIHOST_H
#define SEMIHOST_H

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// RISC-V semihosting call number
#define RISCV_SEMIHOSTING_CALL_NUMBER 7

// Buffer size for printf
#define PRINTF_BUFFER_SIZE 256

enum semihost_instr {
	/*
	 * File I/O operations
	 */

	/** Open a file or stream on the host system. */
	SEMIHOST_OPEN   = 0x01,
	/** Check whether a file is associated with a stream/terminal */
	SEMIHOST_ISTTY  = 0x09,
	/** Write to a file or stream. */
	SEMIHOST_WRITE  = 0x05,
	/** Read from a file at the current cursor position. */
	SEMIHOST_READ   = 0x06,
	/** Closes a file on the host which has been opened by SEMIHOST_OPEN. */
	SEMIHOST_CLOSE  = 0x02,
	/** Get the length of a file. */
	SEMIHOST_FLEN   = 0x0C,
	/** Set the file cursor to a given position in a file. */
	SEMIHOST_SEEK   = 0x0A,
	/** Get a temporary absolute file path to create a temporary file. */
	SEMIHOST_TMPNAM = 0x0D,
	/** Remove a file on the host system. Possibly insecure! */
	SEMIHOST_REMOVE = 0x0E,
	/** Rename a file on the host system. Possibly insecure! */
	SEMIHOST_RENAME = 0x0F,

	/*
	 * Terminal I/O operations
	 */

	/** Write one character to the debug terminal. */
	SEMIHOST_WRITEC         = 0x03,
	/** Write a NULL terminated string to the debug terminal. */
	SEMIHOST_WRITE0         = 0x04,
	/** Read one character from the debug terminal. */
	SEMIHOST_READC          = 0x07,

	/*
	 * Time operations
	 */
	SEMIHOST_CLOCK          = 0x10,
	SEMIHOST_ELAPSED        = 0x30,
	SEMIHOST_TICKFREQ       = 0x31,
	SEMIHOST_TIME           = 0x11,

	/*
	 * System/Misc. operations
	 */

	/** Retrieve the errno variable from semihosting operations. */
	SEMIHOST_ERRNO          = 0x13,
	/** Get commandline parameters for the application to run with */
	SEMIHOST_GET_CMDLINE    = 0x15,
	SEMIHOST_HEAPINFO       = 0x16,
	SEMIHOST_ISERROR        = 0x08,
	SEMIHOST_SYSTEM         = 0x12
};

// Structs for arguments
struct semihost_poll_in_args {
	long zero;
} __packed;

struct semihost_open_args {
	const char *path;
	long mode;
	long path_len;
} __packed;

struct semihost_close_args {
	long fd;
} __packed;

struct semihost_flen_args {
	long fd;
} __packed;

struct semihost_seek_args {
	long fd;
	long offset;
} __packed;

struct semihost_read_args {
	long fd;
	char *buf;
	long len;
} __packed;

struct semihost_write_args {
	long fd;
	const char *buf;
	long len;
} __packed;

/**
 * @brief Modes to open a file with
 *
 * Behaviour corresponds to equivalent fopen strings.
 * i.e. SEMIHOST_OPEN_RB_PLUS == "rb+"
 */
enum semihost_open_mode {
	SEMIHOST_OPEN_R         = 0,
	SEMIHOST_OPEN_RB        = 1,
	SEMIHOST_OPEN_R_PLUS    = 2,
	SEMIHOST_OPEN_RB_PLUS   = 3,
	SEMIHOST_OPEN_W         = 4,
	SEMIHOST_OPEN_WB        = 5,
	SEMIHOST_OPEN_W_PLUS    = 6,
	SEMIHOST_OPEN_WB_PLUS   = 7,
	SEMIHOST_OPEN_A         = 8,
	SEMIHOST_OPEN_AB        = 9,
	SEMIHOST_OPEN_A_PLUS    = 10,
	SEMIHOST_OPEN_AB_PLUS   = 11,
};

/* --------------------------- INTERNAL FUNCTIONS --------------------------- */

/**
 * @brief Manually execute a semihosting instruction
 *
 * @param instr instruction code to run
 * @param args instruction specific arguments
 *
 * @return integer return code of instruction
 */
static inline long __attribute__((always_inline)) semihost_exec(enum semihost_instr instr, void *args)
{
    register int value asm("a0") = instr;
    register void *ptr asm("a1") = args;
    asm volatile(
        // Workaround for RISC-V lack of multiple EBREAKs: magic sequence
        " .option push \n"
        " .option norvc \n"
        // TODO: check if this is necessary
        // Force 16-byte alignment to make sure that the 3 instruction fall
        // within the same virtual page. If you the instruction straddle a page boundary
        // the debugger fetching the instructions could lead to a page fault.
        // Note: align 4 means, align by 2 to the power of 4!
        //" .align 4 \n"
        " slli x0, x0, 0x1f \n"
        " ebreak \n"
        " srai x0, x0, %[swi] \n"
        " .option pop \n"

        : "=r"(value)                                                    /* Outputs */
        : "0"(value), "r"(ptr), [swi] "i"(RISCV_SEMIHOSTING_CALL_NUMBER) /* Inputs */
        : "memory"                                                       /* Clobbers */
    );
    return value;
}

/* -------------------------------------------------------------------------- */
/*                                  WRAPPERS                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Read a byte from the console
 *
 * @return char byte read from the console.
 */
char semihost_poll_in(void)
{
	struct semihost_poll_in_args args = {
		.zero = 0
	};

	return (char)semihost_exec(SEMIHOST_READC, &args);
}

/**
 * @brief Write a byte to the console
 *
 * @param c byte to write to console
 */
void semihost_poll_out(char c)
{
	/* WRITEC takes a pointer directly to the character */
	(void)semihost_exec(SEMIHOST_WRITEC, &c);
}

/**
 * @brief Open a file on the host system
 *
 * @param path file path to open. Can be absolute or relative to current
 *             directory of the running process.
 * @param mode value from @see semihost_open_mode.
 *
 * @retval handle positive handle on success.
 * @retval -1 on failure.
 */
long semihost_open(const char *path, long mode)
{
	struct semihost_open_args args = {
		.path = path,
		.mode = mode,
		.path_len = strlen(path)
	};

	return semihost_exec(SEMIHOST_OPEN, &args);
}

/**
 * @brief Close a file
 *
 * @param fd handle returned by @see semihost_open.
 *
 * @retval 0 on success.
 * @retval -1 on failure.
 */
long semihost_close(long fd)
{
	struct semihost_close_args args = {
		.fd = fd
	};

	return semihost_exec(SEMIHOST_CLOSE, &args);
}

/**
 * @brief Query the size of a file
 *
 * @param fd handle returned by @see semihost_open.
 *
 * @retval positive file size on success.
 * @retval -1 on failure.
 */
long semihost_flen(long fd)
{
	struct semihost_flen_args args = {
		.fd = fd
	};

	return semihost_exec(SEMIHOST_FLEN, &args);
}

/**
 * @brief Seeks to an absolute position in a file.
 *
 * @param fd handle returned by @see semihost_open.
 * @param offset offset from the start of the file in bytes.
 *
 * @retval 0 on success.
 * @retval -errno negative error code on failure.
 */
long semihost_seek(long fd, long offset)
{
	struct semihost_seek_args args = {
		.fd = fd,
		.offset = offset
	};

	return semihost_exec(SEMIHOST_SEEK, &args);
}

/**
 * @brief Read the contents of a file into a buffer.
 *
 * @param fd handle returned by @see semihost_open.
 * @param buf buffer to read data into.
 * @param len number of bytes to read.
 *
 * @retval read number of bytes read on success.
 * @retval -errno negative error code on failure.
 */
long semihost_read(long fd, char *buf, long len)
{
	struct semihost_read_args args = {
		.fd = fd,
		.buf = buf,
		.len = len
	};
	long ret;

	ret = semihost_exec(SEMIHOST_READ, &args);
	/* EOF condition */
	if (ret == len) {
		ret = 0;
	}
	/* All bytes read */
	else if (ret == 0) {
		ret = len;
	}
	return ret;
}

/**
 * @brief Write the contents of a buffer into a file.
 *
 * @param fd handle returned by @see semihost_open.
 * @param buf buffer to write data from.
 * @param len number of bytes to write.
 *
 * @retval 0 on success.
 * @retval -errno negative error code on failure.
 */
long semihost_write(long fd, const char *buf, long len)
{
	struct semihost_write_args args = {
		.fd = fd,
		.buf = buf,
		.len = len
	};

	return semihost_exec(SEMIHOST_WRITE, &args);
}

/**
 * @brief Print a formatted string to the console
 *
 * @param format format string
 * @param ... additional arguments
 *
 * @retval number of characters printed
 */
int semihost_printf(const char *format, ...)
{
    char buffer[PRINTF_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (ret > 0) {
        semihost_write(1, buffer, ret);
    }

    return ret;
}

/**
 * @brief Read a line from a file.
 *
 * @param fd handle returned by @see semihost_open.
 * @param buf buffer to read data into.
 * @param max_len maximum number of bytes to read.
 *
 * @retval number of bytes read on success.
 * @retval -errno negative error code on failure.
 */
long semihost_readline(long fd, char *buf, long max_len)
{
	long total_read = 0;
	while (total_read < max_len - 1) {
		char c;
		long ret = semihost_read(fd, &c, 1);
		if (ret <= 0) {
			break;
		}
		buf[total_read++] = c;
		if (c == '\n') {
			break;
		}
	}
	buf[total_read] = '\0';
	return total_read;
}

/**
 * @brief Force a break in the GDB execution.
 *
 * This function triggers a semihosting breakpoint that halts the execution
 * in GDB. Execution can be resumed by continuing in the debugger.
 */
__attribute__((used)) void semihost_break(void)
{
	semihost_exec(SEMIHOST_SYSTEM, NULL);
}

#endif /* SEMIHOST_H */