#ifndef _MP45DT02
#define _MP45DT02

#include "config.h"


#define FS DFREQ

#define VOLUME 10


#define DECIMATION 64

#define MIC_IN_BUF_SIZE ((((FS/1000)*DECIMATION)/8)/2)
#define MIC_OUT_BUF_SIZE (FS/1000)

#define I2S_FS ((FS*DECIMATION)/(16*2))


void InitMP45DT02();
void GotSample( int s );

#endif

