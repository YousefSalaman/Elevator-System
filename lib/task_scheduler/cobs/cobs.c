

#include "cobs.h"

// TODO: Credit Jacques Fortier for the COBS stuff

// COBS encode a packet
size_t cobs_encode(const uint8_t * input, size_t length, uint8_t * output)
{
    uint8_t code = 1;
    size_t code_index = 0;
    size_t read_index = 0;
    size_t write_index = 1;

    while (read_index < length)
    {
        uint8_t byte = input[read_index++];

        // If byte is not 0, pass data to output
		if(byte)
        {
			output[write_index++] = byte;
            code++;
		}

        // If byte is 0 or code reached 255 (block completed), then restart
		if (!byte || code == 0xFF)
        {
			output[code_index] = code;
			code = 1;
			code_index = write_index++;
		}
    }

    output[code_index] = code;

    return write_index;
}


// COBS decode a packet
size_t cobs_decode(const uint8_t * input, size_t length, uint8_t * output)
{
	uint8_t code;
    size_t read_index = 0;
    size_t write_index = 0;

    while (read_index < length)
    {
        code = input[read_index];

        if(read_index + code > length && code != 1)
        {
            return 0;
        }

        read_index++;

        for (uint8_t i = 1; i < code; i++)
        {
            output[write_index++] = input[read_index++];
        }

        if (code != 0xFF && read_index != length)
        {
            output[write_index++] = '\0';
        }
    }

    return write_index;
}