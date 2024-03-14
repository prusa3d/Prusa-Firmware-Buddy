#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>

#define LIGHTMODBUS_MASTER_FULL
#define LIGHTMODBUS_DEBUG
#define LIGHTMODBUS_IMPL
#include <lightmodbus/lightmodbus.h>

int serialopen(const char *path, int speed, int parity, int blocking)
{
	int fd = open(path, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd == -1) return -1;

	struct termios tty;
	memset(&tty, 0, sizeof tty);

	// Lock serial port
	if (lockf(fd, F_TLOCK, 0) == -1) return -1;

	// Get configuration
	if (tcgetattr(fd, &tty) != 0) return -1;
	
	//Set speed
	cfsetospeed(&tty, speed); 
	cfsetispeed(&tty, speed);
	cfmakeraw(&tty);

	//Config (8-bit characters, 0.1 sec character timeout)
	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
	tty.c_iflag &= ~IGNBRK;
	tty.c_lflag = 0;
	tty.c_oflag = 0;
	tty.c_cc[VMIN] = (blocking ? 1 : 0);
	tty.c_cc[VTIME] = 1;
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);
	tty.c_cflag |= (CLOCAL | CREAD);
	tty.c_cflag &= ~(PARENB | PARODD);
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;
	if (tcsetattr(fd, TCSANOW, &tty) != 0) return -1;

	return fd;
}

int serialrecv(int fd, uint8_t *buf, int buflen)
{
	// Number of bytes received
	int len = 0;

	// Time of last received byte
	struct timeval last;
	gettimeofday(&last, NULL);

	while (len < buflen)
	{
		// Check timeout
		struct timeval current;
		gettimeofday(&current, NULL);
		struct timeval tdiff;
		timersub(&current, &last, &tdiff);
		struct timeval timeout = {.tv_sec = 0, .tv_usec = 10e-3 * 10e9}; // 10ms
		if (timercmp(&tdiff, &timeout, >))
			break;

		// Attempt to read
		int n = read(fd, buf + len, buflen - len);
		if (n == -1)
			return -1;

		if (n > 0)
			gettimeofday(&last, NULL);

		len += n;
	}

	return len;
}

int convbaud(int baudrate)
{
	switch (baudrate)
	{
		case 300: return B300;
		case 600: return B600;
		case 1200: return B1200;
		case 2400: return B2400;
		case 4800: return B4800;
		case 9600: return B9600;
		case 19200: return B19200;
		case 38400: return B38400;
		case 57600: return B57600;
		case 115200: return B115200;
		default: return -1;
	}
}

int serialclose(int fd)
{
	// Remove lock & close
	if (lockf(fd, F_ULOCK, 0) == -1) return -1;
	return close(fd);
}

ModbusError dataCallback(const ModbusMaster *master, const ModbusDataCallbackArgs *args)
{
	char typechar = '?';
	switch (args->type)
	{
		case MODBUS_HOLDING_REGISTER: typechar = 'R'; break;
		case MODBUS_INPUT_REGISTER: typechar = 'I'; break;
		case MODBUS_COIL: typechar = 'C'; break;
		case MODBUS_DISCRETE_INPUT: typechar = 'D'; break;
	}
	printf(
		"F: %03d, T: %c, ID: %03d, VAL: 0x%04x (%d)\n",
		args->function,
		typechar,
		args->index,
		args->value,
		args->value);
	return MODBUS_OK;
}

ModbusError exceptionCallback(const ModbusMaster *master, uint8_t address, uint8_t function, ModbusExceptionCode code)
{
	printf(
		"EXCEPTION SLAVE: %03d, F: %03d, CODE: %03d (%s)\n",
		address,
		function,
		(int) code,
		modbusExceptionCodeStr(code));
	return MODBUS_OK;
}

void buildreq(ModbusMaster *master, int address, int function, int *args, int argc)
{
	ModbusErrorInfo err = MODBUS_NO_ERROR();

	switch (function)
	{
		case 1:
		case 2:
		case 3:
		case 4:
			if (argc != 2)
			{
				fprintf(stderr, "Functions 01, 02, 03 and 04 expect exactly 2 arguments! (got %d)\n", argc);
				exit(EXIT_FAILURE);
			}
			
			err = modbusBeginRequestRTU(master);
			if (!modbusIsOk(err)) break;
			err = modbusBuildRequest01020304(master, function, args[0], args[1]);
			if (!modbusIsOk(err)) break;
			err = modbusEndRequestRTU(master, address);
			break;

		case 5:
		case 6:
			if (argc != 2)
			{
				fprintf(stderr, "Functions 05 and 06 expect exactly 2 arguments! (got %d)\n", argc);
				exit(EXIT_FAILURE);
			}
			
			err = modbusBeginRequestRTU(master);
			if (!modbusIsOk(err)) break;
			err = modbusBuildRequest0506(master, function, args[0], args[1]);
			if (!modbusIsOk(err)) break;
			err = modbusEndRequestRTU(master, address);
			break;

		case 15:
			{
				if (argc < 2)
				{
					fprintf(stderr, "Function 15 expects at least 2 arguments! (got %d)\n", argc);
					exit(EXIT_FAILURE);
				}

				int index = args[0];
				int count = argc - 1;
				uint8_t *data = calloc(1 + count / 8, sizeof(uint8_t));
				for (int i = 0; i < count; i++)
					modbusMaskWrite(data, i, args[1 + i]);
				err = modbusBuildRequest15RTU(master, address, index, count, data);
				free(data);
			}
			break;

		case 16:
			{
				if (argc < 2)
				{
					fprintf(stderr, "Function 16 expects at least 2 arguments! (got %d)\n", argc);
					exit(EXIT_FAILURE);
				}

				int index = args[0];
				int count = argc - 1;
				uint16_t *data = calloc(count, sizeof(uint16_t));
				for (int i = 0; i < count; i++)
					data[i] = args[1 + i];
				err = modbusBuildRequest16RTU(master, address, index, count, data);
				free(data);
			}
			break;

		case 22:
			if (argc != 3)
			{
				fprintf(stderr, "Function 22 expects exactly 3 arguments! (got %d)\n", argc);
				exit(EXIT_FAILURE);
			}
			
			err = modbusBuildRequest22RTU(master, address, args[0], args[1], args[2]);
			break;

		default:
			fprintf(stderr, "Unknown function %d!\n", function);
			break;
	}

	if (!modbusIsOk(err))
	{
		fprintf(
			stderr,
			"Error building request: %s(%s)\n",
			modbusErrorSourceStr(modbusGetErrorSource(err)),
			modbusErrorStr(modbusGetErrorCode(err)));
		exit(EXIT_FAILURE);
	}
}

void help(const char *exename)
{
	fprintf(
		stderr,
		"Usage:\n"
		"\t%s <TTY> <BAUDRATE> <ADDRESS> <FUNCTION> [args]\n"
		"\n"
		"Function usage:\n"
		"\tFunction 01: <index> <count>\n"
		"\tFunction 02: <index> <count>\n"
		"\tFunction 03: <index> <count>\n"
		"\tFunction 04: <index> <count>\n"
		"\tFunction 05: <index> <value>\n"
		"\tFunction 06: <index> <value>\n"
		"\tFunction 15: <index> [values]\n"
		"\tFunction 16: <index> [values]\n"
		"\tFunction 22: <index> <andmask> <ormask>\n",
		exename
	);
}

int main(int argc, char *argv[])
{
	if (argc < 5)
	{
		help(argv[0]);
		return 1;
	}

	// Serial port
	const char *ttypath = argv[1];
	
	// Integer values of the args
	int argi[argc];
	for (int i = 2; i < argc; i++)
	{
		if (sscanf(argv[i], "%i", &argi[i]) != 1)
		{
			fprintf(stderr, "Invalid integer argument: '%s'\n", argv[i]);
			exit(EXIT_FAILURE);
		}
	}

	// Init master
	ModbusMaster master;
	ModbusErrorInfo err = modbusMasterInit(
		&master,
		dataCallback,
		exceptionCallback,
		modbusDefaultAllocator,
		modbusMasterDefaultFunctions,
		modbusMasterDefaultFunctionCount);
	assert(modbusIsOk(err) && "modbusMasterInit() failed!");

	// Try to build request
	buildreq(&master, argi[3], argi[4], &argi[5], argc - 5);

	// Convert baudrate
	int baud = convbaud(argi[2]);
	if (baud < 0)
	{
		fprintf(stderr, "Invalid baudrate: %d\n", argi[2]);
		exit(EXIT_FAILURE);
	}

	// Open serial port
	int serialfd = serialopen(ttypath, baud, 0, 0);
	if (serialfd < 0)
	{
		fprintf(stderr, "Could not open '%s' - %s\n", ttypath, strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Send request
	if (write(serialfd, modbusMasterGetRequest(&master), modbusMasterGetRequestLength(&master)) < 0)
	{
		fprintf(stderr, "write() failed - %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Wait 50ms
	{
		struct timespec ts = {.tv_sec = 0, .tv_nsec = 0.05 * 1e9};
		nanosleep(&ts, NULL);
	}

	// Receive response
	uint8_t response[256];
	int len = serialrecv(serialfd, response, sizeof(response));
	if (len < 0)
	{
		fprintf(stderr, "read() error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Print out response length
	printf("RESP LEN: %03d\n", len);

	// Parse response
	err = modbusParseResponseRTU(
		&master,
		modbusMasterGetRequest(&master),
		modbusMasterGetRequestLength(&master),
		response,
		len);
	
	// Handle parsing error
	if (!modbusIsOk(err))
	{
		fprintf(
			stderr,
			"Response parsing error: %s(%s)\n",
			modbusErrorSourceStr(modbusGetErrorSource(err)),
			modbusErrorStr(modbusGetErrorCode(err)));
	}

	// Close serial
	if (serialclose(serialfd) < 0)
	{
		fprintf(stderr, "Error closing tty: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	return 0;
}