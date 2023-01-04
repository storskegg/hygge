#ifndef _RMS_H

#include <Arduino.h>
#include "float.h"

#define _RMS_H

typedef struct {
  float vals[60];
  size_t cur;
  size_t sz;
  size_t n;
} rms_t;

rms_t new_rms(void);
void push(float, rms_t *);
float rms(rms_t *);

#endif
