#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define LIGHTMODBUS_FULL
#define LIGHTMODBUS_DEBUG
#define LIGHTMODBUS_IMPL
#include <lightmodbus/lightmodbus.h>

#define REG_COUNT 8
static uint16_t registers[REG_COUNT] = {0};
static uint8_t coils[REG_COUNT] = {0};

ModbusError regCallback(
	ModbusSlave *status,
	const ModbusRegisterCallbackArgs *args,
	uint16_t *result)
{
	if (args->query == MODBUS_REGQ_R_CHECK || args->query == MODBUS_REGQ_W_CHECK)
	{
		*result = args->index < REG_COUNT ? MODBUS_EXCEP_NONE : MODBUS_EXCEP_ILLEGAL_ADDRESS;
	}
	else if (args->query == MODBUS_REGQ_R)
	{
		switch (args->type)
		{
			case MODBUS_HOLDING_REGISTER: *result = registers[args->index]; break;
			case MODBUS_INPUT_REGISTER:   *result = registers[args->index]; break;
			case MODBUS_COIL:             *result = modbusMaskRead(coils, args->index); break;
			case MODBUS_DISCRETE_INPUT:   *result = modbusMaskRead(coils, args->index); break;
		}
	}
	else if (args->query == MODBUS_REGQ_W)
	{
		switch (args->type)
		{
			case MODBUS_HOLDING_REGISTER: registers[args->index] = args->value; break;
			case MODBUS_COIL:             modbusMaskWrite(coils, args->index, args->value); break;
			default: break;
		}
	}
	else
		return MODBUS_ERROR_OTHER;

	return MODBUS_OK;
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
	ModbusErrorInfo err = modbusParseRequestRTU(s, data, n);
	dumpregs();
	printf("\t|F ");

	if (modbusIsOk(err))
	{
		for (int i = 0; i < s->response.length; i++)
			printf("%02x ", s->response.data[i]);
	}
	else
	{
		printf("Error: {source: %s, error: %s}", modbusErrorSourceStr(err.source), modbusErrorStr(err.error));
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
	ModbusSlave slave;
	ModbusErrorInfo err;
	err = modbusSlaveInit(&slave, 1, modbusSlaveDefaultAllocator, regCallback);
	assert(modbusIsOk(err) && "Init failed!");
	hex(&slave);
	modbusSlaveDestroy(&slave);
}