/*
 * helpers.c
 *
 *  Created on: 11.12.2022
 *      Author: Yannick
 */
#include "helpers.h"

float bytesToFloat(uint8_t *bytes, uint8_t big_endian) {
    float f;
    uint8_t *f_ptr = (uint8_t *) &f;
    if (big_endian) {
        f_ptr[3] = bytes[0];
        f_ptr[2] = bytes[1];
        f_ptr[1] = bytes[2];
        f_ptr[0] = bytes[3];
    } else {
        f_ptr[3] = bytes[3];
        f_ptr[2] = bytes[2];
        f_ptr[1] = bytes[1];
        f_ptr[0] = bytes[0];
    }
    return f;
}
