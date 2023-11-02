#include <Arduino.h>

#ifdef MASTER_BUOY
  #include "MasterBuoy.h"
#elif SLAVE_BUOY
  #include "SlaveBuoy.h"
#elif COAST_BOARD
  #include "CoastBoard.h"
#else
  #error "Unsuported board was provided in platformIO config!"
#endif