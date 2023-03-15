#ifndef LIGHTMODBUS_BASE_IMPL_H
#define LIGHTMODBUS_BASE_IMPL_H

#include "base.h"
#include <stdlib.h>

/**
	\file base.impl.h
	\brief Common types and functions (implementation)
*/

/**
	\brief The default memory allocator based on realloc()
	\param buffer a pointer to the buffer to be reallocated
	\param size new desired buffer size in bytes
	\param context user's context pointer
	\returns MODBUS_ERROR_ALLOC on allocation failure
	\returns MODBUS_OK on success
	\see allocators
*/
LIGHTMODBUS_WARN_UNUSED ModbusError modbusDefaultAllocator(ModbusBuffer *buffer, uint16_t size, void *context)
{
	// Make sure to handle the case when *ptr = NULL and size = 0
	// We don't want to allocate any memory then, but realloc(NULL, 0) would
	// result in malloc(0)
	if (!size)
	{
		free(buffer->data);
		buffer->data = NULL;
	}
	else
	{
		uint8_t *old_ptr = buffer->data;
		buffer->data = (uint8_t*)realloc(buffer->data, size);
		
		if (!buffer->data)
		{
			free(old_ptr);
			return MODBUS_ERROR_ALLOC;
		}
	}

	return MODBUS_OK;
}

/**
	\brief Initializes a buffer for use
	\param allocator Memory allocator to be used by the buffer
	\returns MODBUS_NO_ERROR() on success 
*/
LIGHTMODBUS_RET_ERROR modbusBufferInit(ModbusBuffer *buffer, ModbusAllocator allocator)
{
	*buffer = (ModbusBuffer){
		.allocator = allocator,
		.data = NULL,
		.pdu = NULL,
		.length = 0,
		.padding = 0,
		.pduOffset = 0,
	};
	return MODBUS_NO_ERROR();
}

/**
	\brief Frees memory allocated inside the buffer
	\param context context pointer passed on to the allocator
*/
void modbusBufferFree(ModbusBuffer *buffer, void *context)
{
	ModbusError err = modbusBufferAllocateADU(buffer, 0, context);
	(void) err;
}

/**
	\brief Equivalent of modbusBufferFree()
	\copydetail modbusBufferFree()
*/
void modbusBufferDestroy(ModbusBuffer *buffer, void *context)
{
	modbusBufferFree(buffer, context);
}

/**
	\brief Allocates memory to hold Modbus ADU
	\param pduSize size of the PDU in bytes
	\param context context pointer passed on to the allocator
	\returns MODBUS_OK on success
	\returns MODBUS_ERROR_ALLOC on allocation failure

	If called with pduSize == 0, the buffer is freed. Otherwise a buffer
	for `(pduSize + buffer->padding)` bytes is allocated. This guarantees
	that the buffer is big enough to hold the entire ADU.

	This function is responsible for managing `data`, `pdu` and `length` fields
	in the buffer struct. The `pdu` pointer is set up to point `pduOffset` bytes
	after the `data` pointer unless `data` is a null pointer.
*/
LIGHTMODBUS_WARN_UNUSED ModbusError modbusBufferAllocateADU(ModbusBuffer *buffer, uint16_t pduSize, void *context)
{
	uint16_t size = pduSize;
	if (pduSize) size += buffer->padding;

	ModbusError err = buffer->allocator(buffer, size, context);

	if (err == MODBUS_ERROR_ALLOC || size == 0)
	{
		buffer->data = NULL;
		buffer->pdu  = NULL;
		buffer->length = 0;
	}
	else
	{
		buffer->pdu = buffer->data + buffer->pduOffset;
		buffer->length = size;
	}

	return err;
}

/**
	\brief Calculates 16-bit Modbus CRC of provided data
	\param data A pointer to the data to be processed
	\param length Number of bytes, starting at the `data` pointer, to process
	\returns 16-bit Modbus CRC value
*/
LIGHTMODBUS_WARN_UNUSED uint16_t modbusCRC(const uint8_t *data, uint16_t length)
{
	uint16_t crc = 0xFFFF;

	for (uint16_t i = 0; i < length; i++)
	{
		crc ^= (uint16_t) data[i];
		for (uint8_t j = 8; j != 0; j--)
		{
			if ((crc & 0x0001) != 0)
			{
				crc >>= 1;
				crc ^= 0xA001;
			}
			else
				crc >>= 1;
		}
	}

	return crc;
}

#endif
