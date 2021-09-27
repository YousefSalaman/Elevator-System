#include "cobs.h"
#include "../scheduler_config.h"

// TODO: Credit Jacques Fortier for the COBS stuff

/**COBS encode a packet
 * 
 * From the original library,the code was modified to be a bit more 
 * compact and it also adds the delimiter byte and the end of the
 * encoded packet.
 * 
 * Originally made by Jacques Fortier.
*/
size_t cobs_encode(const uint8_t * input, size_t length, uint8_t * output)
{
    uint8_t code = 1;        // Encoded byte in the output (indicates where the next 0 is)
    size_t code_index = 0;   // Index where the last zero was found
    size_t read_index = 0;   // Index for byte to read in input
    size_t write_index = 1;  // Index for byte to write in output (indirectly tells length at the end)

    while (read_index < length)
    {
        uint8_t byte = input[read_index++];

        // If byte is not 0, pass data to output
		if(byte)
        {
			output[write_index++] = byte;
            code++;
		}

        // If byte is 0 or code reached 255 (block completed), then restart code count
		if (!byte || code == 0xFF)
        {
			output[code_index] = code;
			code = 1;
			code_index = write_index++;
		}
    }

    output[code_index] = code;     // Place the location of the COBS delimeter byte 
    output[write_index++] = '\0';  // Place the COBS delimiter byte at the end

    return write_index;
}


// COBS decode a packet
size_t cobs_decode(const uint8_t * input, size_t length, uint8_t * output)
{
	uint8_t code;            // Encoded byte in the input (indicates where the next 0 is)
    size_t read_index = 0;   // Index for byte to read in input
    size_t write_index = 0;  // Index for byte to write in output (indirectly tells length at the end)

    while (read_index < length)
    {
        code = input[read_index];

        // If encoded byte (code) points to outside the range of the input 
        if(read_index + code > length && code != 1)
        {
            return 0;
        }

        read_index++;

        // Directly pass other values that are not the encoded zero (i.e. the values that remained the same after encoding)
        for (uint8_t i = 1; i < code; i++)
        {
            output[write_index++] = input[read_index++];
        }

        // Translate encoded 0 to byte
        if (code != 0xFF && read_index != length)
        {
            output[write_index++] = '\0';
        }
    }

    return write_index;
}