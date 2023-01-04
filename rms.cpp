#include "rms.h"
#include "math.h"

rms_t new_rms() {
  rms_t set = {
    vals: {0},
    cur: 0,
    n: 0,
  };
  set.sz = sizeof(set.vals)/sizeof(float);

  for (size_t i = 0; i < set.sz; i++) {
    set.vals[i] = NAN;
  }

  return set;
} 

void push(float val, rms_t *set) {
  set->vals[set->cur] = val;
  set->cur++;
  if (set->cur >= set->sz) {
    set->cur = 0;
  }
}

float rms(rms_t *set) {
  float sqsum = 0;
  size_t less = 0;
  
  for (size_t i = 0; i < set->sz; i++) {
    if (isnan(set->vals[i])) {
      less++;
      continue;
    }

    sqsum += pow(set->vals[i], 2);
  }
  set->n = set->sz - less;

  return sqrtf((1 / (float)set->n) * sqsum);
}
