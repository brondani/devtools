
#include "RTE_Components.h"
#include CMSIS_device_header

extern int lib_ac6 (void);
extern int lib_gcc (void);

int main (void) {
  if ((lib_ac6() != 1) || (lib_gcc() != 1))
    return 1;
  return 0;
}
