#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <lightmodbus/lightmodbus.h>

#define REG_COUNT 16
static uint16_t registers[REG_COUNT] = {0};
// static uint16_t inputs[REG_COUNT] = {0};
static uint8_t coils[REG_COUNT] = {0};
// static uint8_t discrete[REG_COUNT] = {0};

int16_t modbusWLE(uint8_t *p, uint16_t val)
{
	*p = val;
	*(p + 1) = val >> 8;
	return val;
}

void dumpregs(void)
{
	printf("|R ");
	for (int i = 0; i < REG_COUNT; i++)
		printf("%04x ", registers[i]);

	printf("|C ");
	for (int i = 0; i < REG_COUNT / 8; i++)
		printf("%02x ", coils[i]);
}

void parse(ModbusSlave *s, const uint8_t *data, int n)
{	
	s->request.frame = data;
	s->request.length = n;
	ModbusError err = modbusParseRequest(s);
	
	dumpregs();
	printf("\t|F ");

	if (!err || err == MODBUS_ERROR_EXCEPTION)
	{
		for (int i = 0; i < s->response.length; i++)
			printf("%02x ", s->response.frame[i]);
	}
	else
	{
		printf("ERR");
		// printf("ERR %d", err);
	}

	putchar('\n');
}

/*
	Single binary request, delimited by EOF
*/
void raw(ModbusSlave *s)
{
	uint8_t data[1024];
	data[0] = 1;
	int n = fread(&data[1], 1, sizeof(data) - 3, stdin);
	modbusWLE(&data[n + 1], modbusCRC(data, n + 1));
	int len = n + 3;
	parse(s, data, len);
}

/*
	Accepts multiple requests - one per line in hex format
*/
void hex(ModbusSlave *s)
{
	char *line = NULL;
	size_t linesize = 0;
	while (getline(&line, &linesize, stdin) > 0)
	{
		// printf("got line '%s'\n", line);

		uint8_t data[1024];

		int offset = 0;
		unsigned int len = 1;
		while (len < sizeof(data) - 2)
		{
			int c = 0, n = 0;
			if (sscanf(&line[offset], "%02x%n", &c, &n) != 1)
				break;
			offset += n;
			data[len++] = c;
		}

		data[0] = 1;
		modbusWLE(&data[len], modbusCRC(data, len));
		len += 2;

		parse(s, data, len);

		// for (int i = 0; i < len; i++)
		// 	printf("%02x ", data[i]);
		// putchar('\n');


	}
	free(line);
}

int main()
{
	ModbusSlave slave = {0};
	ModbusError err;
	slave.registers = registers;
	slave.registerCount = REG_COUNT;
// 	slave.inputRegisters = inputs;
	slave.inputRegisters = registers;
	slave.inputRegisterCount = REG_COUNT;
	slave.coils = coils;
	slave.coilCount = REG_COUNT;
// 	slave.discreteInputs = discrete;
	slave.discreteInputs = coils;
	slave.discreteInputCount = REG_COUNT;
	slave.address = 1;
	err = modbusSlaveInit(&slave);
	assert(err == MODBUS_OK && "Init failed!");

	#ifdef FUZZ
	raw(&slave);
	#else
	hex(&slave);
	#endif

	modbusSlaveEnd(&slave);
}
