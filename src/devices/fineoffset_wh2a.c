/* Fine Offset Electronics sensor protocol
 *
 * The protocol is for the wireless Temperature/Humidity sensor
 * Fine Offset Electronics WH2A
 *
 * The sensor sends three identical packages of 48 bits each ~48s. The bits are PWM modulated with On Off Keying
 *
 * The data is grouped in 6 bytes / 12 nibbles
 * [pre] [pre] [type] [id] [id] [temp] [temp] [temp] [humi] [humi] [crc] [crc]
 *
 * pre is always 0xFF
 * type is always 0x4 (may be different for different sensor type?)
 * id is a random id that is generated when the sensor starts
 * temp is 12 bit signed magnitude scaled by 10 celcius
 * humi is 8 bit relative humidity percentage
 * 
 * Based on reverse engineering with gnu-radio and the nice article here:
 *  http://lucsmall.com/2012/04/29/weather-station-hacking-part-2/
 *
 * Copyright (C) 2015 Tommy Vestermark
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "decoder.h"

static int fineoffset_WH2A_callback(r_device *decoder, bitbuffer_t *bitbuffer) {
    bitrow_t *bb = bitbuffer->bb;
    data_t *data;

    uint8_t id;
    int16_t temp;
    float temperature;
    uint8_t humi;
    float humidity;

    const uint8_t polynomial = 0x31;    // x8 + x5 + x4 + 1 (x8 is implicit)

    // Validate package
    if (bitbuffer->bits_per_row[0] >= 59 &&         // Don't waste time on a short packages
        bb[0][0] == 0xFE && bb[0][7] == 0x00
    //    bb[0][5] == crc8(&bb[0][1], 4, polynomial, 0)	// CRC (excluding preamble)
    )
    {
         // Nibble 3,4 contains id
        id = ((bb[0][1]&0x0F) << 4) | ((bb[0][2]&0xF0) >> 4);

        // Nibble 5,6,7 contains 12 bits of temperature
        // The temperature is signed magnitude and scaled by 20
        temp = bb[0][3];
        temp |= (int16_t)(bb[0][2] & 0x0F) << 8;
        if(temp & 0x800) {
            temp &= 0x7FF;	// remove sign bit
            temp = -temp;	// reverse magnitude
        }
        temperature = (float)temp / 20;

        // Nibble 8,9 contains humidity scaled by 2
        humi = bb[0][4];
	    humidity = (float)humi / 2;

        data = data_make("model",         "",            DATA_STRING, "Fine Offset Electronics, WH2A Temperature/Humidity sensor",
                         "id",            "",            DATA_INT, id,
                         "temperature_C", "Temperature", DATA_FORMAT, "%.02f C", DATA_DOUBLE, temperature,
                         "humidity",      "Humidity",    DATA_FORMAT, "%.1f %%", DATA_DOUBLE, humidity,
                          NULL);
        decoder_output_data(decoder, data);
        return 1;
    }
    return 0;
}

static char *output_fields[] = {
    "time",
    "model",
    "id",
    "temperature_C",
    "humidity",
    NULL
};

r_device fineoffset_WH2A = {
    .name           = "Fine Offset Electronics, WH-2A Sensor",
    /*
    .modulation     = OOK_PULSE_PWM,
    .short_width    = 800,	// Short pulse 544µs, long pulse 1524µs, fixed gap 1036µs
    .long_width     = 2800,	// Maximum pulse period (long pulse + fixed gap)
    .reset_limit    = 2800,	// We just want 1 package
    .decode_fn      = &fineoffset_WH2A_callback,
    .disabled       = 0,
    .fields         = output_fields
    */
    .modulation     = OOK_PULSE_PWM,
    .short_width    = 500, // Short pulse 544µs, long pulse 1524µs, fixed gap 1036µs
    .long_width     = 1500, // Maximum pulse period (long pulse + fixed gap)
    .reset_limit    = 1200, // We just want 1 package
    .tolerance      = 160, // us  
    .decode_fn      = &fineoffset_WH2A_callback,
    .disabled       = 0,
    .fields         = output_fields     
};



