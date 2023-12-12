
#include "RTE_Components.h"
#include CMSIS_device_header

extern int lib1 (void);
extern int lib2 (void);

int main (void) {
  if ((lib1() != 1) || (lib2() != 2))
    return 1;
  return 0;
}
