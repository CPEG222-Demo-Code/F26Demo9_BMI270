#ifndef NEIPIXEL_H
#define NEIPIXEL_H  

#include <stdint.h>

void NeoPixel_Init_PB11(void);
void NeoPixel_SetColor(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void NeoPixel_Show_PB11(void);
#endif // NEIPIXEL_H 