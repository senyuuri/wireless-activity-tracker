#include "contiki.h"
#include <stdio.h> 
#include "dev/leds.h"
/*---------------------------------------------------------------------------*/
#define EXT_FLASH_BASE_ADDR     0
#define EXT_FLASH_SIZE          4*1024*1024
/*---------------------------------------------------------------------------*/
PROCESS(erasing_flash_process, "Erasing flash");
AUTOSTART_PROCESSES(&erasing_flash_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(erasing_flash_process, ev, data)
{
  PROCESS_BEGIN();

  printf("ERASING FLASH: from %d, size of %d\n", EXT_FLASH_BASE_ADDR, EXT_FLASH_SIZE);
  
  leds_on(LEDS_ALL);
  int executed = ext_flash_open();

  if(!executed) {
    printf("Cannot open flash\n");
    ext_flash_close();
    return 0;
  }
  executed = ext_flash_erase(EXT_FLASH_BASE_ADDR, EXT_FLASH_SIZE);

  ext_flash_close();
  printf("DONE!\n");  
  leds_off(LEDS_ALL);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/