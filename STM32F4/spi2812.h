#ifndef _SPI_2812_H
#define _SPI_2812_H

#define SPI2812_MAX_LEDS 300


#define SPI2812_BUFFSIZE (SPI2812_MAX_LEDS*24/2)


void InitSPI2812();
void SendSPI2812( unsigned char * lightarray, int leds );

#endif


