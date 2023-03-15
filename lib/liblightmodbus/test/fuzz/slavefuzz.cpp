#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iomanip>

#define LIGHTMODBUS_FULL
#define LIGHTMODBUS_DEBUG
#define LIGHTMODBUS_IMPL
#include <lightmodbus/lightmodbus.h>

std::array<uint16_t, 32768> regs;
std::array<uint8_t, 32768> coils;

ModbusError regCallback(
	const ModbusSlave *status,
	const ModbusRegisterCallbackArgs *args,
	ModbusRegisterCallbackResult *result)
{
	switch (args->query)
	{
		case MODBUS_REGQ_R_CHECK:
		case MODBUS_REGQ_W_CHECK:
		{
			int max;
			switch (args->type)
			{
				case MODBUS_HOLDING_REGISTER: max = regs.size(); break;
				case MODBUS_INPUT_REGISTER:   max = regs.size(); break;
				case MODBUS_COIL:             max = coils.size(); break;
				case MODBUS_DISCRETE_INPUT:   max = coils.size(); break;
				default: throw std::runtime_error{"invalid type in query"}; break;
			}

			result->exceptionCode = args->index < max ? MODBUS_EXCEP_NONE : MODBUS_EXCEP_ILLEGAL_ADDRESS;
		}
		break;

		case MODBUS_REGQ_R:
		{
			switch (args->type)
			{
				case MODBUS_HOLDING_REGISTER: result->value = regs.at(args->index); break;
				case MODBUS_INPUT_REGISTER:   result->value = regs.at(args->index); break;
				case MODBUS_COIL:             result->value = coils.at(args->index); break;
				case MODBUS_DISCRETE_INPUT:   result->value = coils.at(args->index); break;
				default: throw std::runtime_error{"invalid type in read query"}; break;
			}
		}
		break;

		case MODBUS_REGQ_W:
			{
				switch (args->type)
				{
					case MODBUS_HOLDING_REGISTER: regs.at(args->index) = args->value; break;
					case MODBUS_COIL:             coils.at(args->index) = args->value; break;
					default: throw std::runtime_error{"invalid write query"}; break;
				}
			}
			break;

		default:
			throw std::runtime_error{"invalid query"};
			break;
	}

	return MODBUS_OK;
}

std::string hexstr(const uint8_t *data, size_t len)
{
	std::stringstream ss;
	for (size_t i = 0; i < len; i++)
		ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << ' ';
	return ss.str();
}

void printerr(ModbusErrorInfo err)
{
	std::printf(
		"\t GEN ERR: %s\n"
		"\t REQ ERR: %s\n"
		"\tRESP ERR: %s\n",
		modbusErrorStr(modbusGetGeneralError(err)),
		modbusErrorStr(modbusGetRequestError(err)),
		modbusErrorStr(modbusGetResponseError(err))
	);
}

void printresp(const ModbusSlave *slave, ModbusErrorInfo err)
{
	if (modbusIsOk(err))
	{
		std::printf("\tRESP: %s\n", hexstr(
			modbusSlaveGetResponse(slave),
			modbusSlaveGetResponseLength(slave)).c_str());
	}
}

void parsePDU(ModbusSlave *s, const uint8_t *data, int n)
{
	n = std::min(255, n);

	ModbusErrorInfo err = modbusParseRequestPDU(s, data, n);
	printf("PDU\n");
	printerr(err);
	printresp(s, err);
	putchar('\n');

	if (modbusGetGeneralError(err) != MODBUS_OK)
		throw std::runtime_error{"PDU parse general error"};
}

void parseRTU(ModbusSlave *s, const uint8_t *data, int n)
{
	n = std::min(65535, n);

	ModbusErrorInfo err = modbusParseRequestRTU(s, 1, data, n);
	printf("RTU\n");
	printerr(err);
	printresp(s, err);
	putchar('\n');

	if (modbusGetGeneralError(err) != MODBUS_OK)
		throw std::runtime_error{"RTU parse general error"};
}

void parseTCP(ModbusSlave *s, const uint8_t *data, int n)
{
	n = std::min(65535, n);

	ModbusErrorInfo err = modbusParseRequestTCP(s, data, n);
	printf("TCP\n");
	printerr(err);
	printresp(s, err);
	putchar('\n');

	if (modbusGetGeneralError(err) != MODBUS_OK)
		throw std::runtime_error{"TCP parse general error"};
}

/*
	Single binary request, delimited by EOF
*/
void raw(ModbusSlave *s)
{
	uint8_t data[65536];
	int n = std::fread(&data[0], 1, sizeof(data), stdin);
	parsePDU(s, data, n);
	parseRTU(s, data, n);
	parseTCP(s, data, n);
}

int main()
{
	ModbusSlave slave;
	ModbusErrorInfo err;
	err = modbusSlaveInit(
		&slave,
		regCallback,
		NULL,
		modbusDefaultAllocator,
		modbusSlaveDefaultFunctions,
		modbusSlaveDefaultFunctionCount);
	assert(modbusIsOk(err) && "Init failed!");
	raw(&slave);
	modbusSlaveDestroy(&slave);
}