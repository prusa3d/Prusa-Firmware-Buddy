/*
	A utility for generating random Modbus requests

	make && ./genrq > generated.txt && ./test < ./generated.txt > gen-result.txt && ~/Desktop/llm-old/t < generated.txt > gen-result-old.txt && diff -u gen-result-old.txt gen-result.txt | tee diff.diff
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LIGHTMODBUS_FULL
#define LIGHTMODBUS_IMPL
#include <lightmodbus/lightmodbus.h>

#define RANGE_MAX 16

void dump(uint8_t *data, int len)
{
	if (len > 252) len = 252;
	for (int i = 0; i < len; i++)
		printf("%02x ", data[i]);
	printf("\n");
}

void gen01020304(int count)
{
	uint16_t index = rand() % RANGE_MAX;
	uint8_t buf[5];
	buf[0] = 1 + rand() % 4;
	modbusWBE(&buf[1], index);
	modbusWBE(&buf[3], count);
	dump(buf, 5);
}

void gen0506(void)
{
	uint16_t index = rand() % RANGE_MAX;
	uint8_t buf[5];
	uint8_t coil = rand() % 2;
	uint16_t value = rand();;

	// Sometimes provide invalid coil value
	if (coil && (rand() % 100 < 90))
		value = (value % 2) ? 0xff00 : 0;

	buf[0] = coil ? 5 : 6;
	modbusWBE(&buf[1], index);
	modbusWBE(&buf[3], value);
	dump(buf, 5);
}

void gen1516(int count)
{
	uint16_t index = rand() % RANGE_MAX;
	uint8_t buf[256];
	uint8_t coils = rand() % 2;
	int datalen;

	modbusWBE(&buf[1], index);
	modbusWBE(&buf[3], count);

	if (coils)
	{
		buf[0] = 15;
		datalen = buf[5] = modbusBitsToBytes(count);
	}
	else
	{
		buf[0] = 16;
		datalen = buf[5] = count * 2;
	}

	if (datalen > 246) return;

	// Sometimes provide invalid data length
	if (rand() % 100 < 5)
		buf[5] = rand();

	for (int i = 0; i < datalen; i++)
		buf[6 + i] = rand();

	dump(buf, datalen + 4 + 1 + 1);
}

void gen22(void)
{
	uint16_t index = rand() % RANGE_MAX;
	uint8_t buf[7];

	buf[0] = 22;
	modbusWBE(&buf[1], index);
	modbusWBE(&buf[3], rand());
	modbusWBE(&buf[5], rand());
	dump(buf, 7);
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	
	int N = 1000;
	if (argc > 1)
		sscanf(argv[1], "%d", &N);

	for (int i = 0; i < N; i++)
		gen0506();

	for (int i = 0; i < N; i++)
		for (int c = 1; c < 100; c++)
			gen1516(rand() % (20 * c));

	for (int i = 0; i < N; i++)
		gen22();

	for (int i = 0; i < N; i++)
		for (int c = 1; c < 100; c++)
			gen01020304(rand() % (20 * c));

	return 0;
}