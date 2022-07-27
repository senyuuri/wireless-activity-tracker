#include "contiki.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "random.h"
#include "button-sensor.h"
#include "board-peripherals.h"
#include "ti-lib.h"

#include <stdio.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define CC26XX_DEMO_LOOP_INTERVAL       (CLOCK_SECOND * 0.01)
#define CC26XX_DEMO_LEDS_PERIODIC       LEDS_YELLOW
#define CC26XX_DEMO_LEDS_BUTTON         LEDS_RED
#define CC26XX_DEMO_LEDS_REBOOT         LEDS_ALL
#define CC26XX_DEMO_SENSOR_NONE         (void *)0xFFFFFFFF
#define CC26XX_DEMO_SENSOR_1     &button_left_sensor
#define CC26XX_DEMO_SENSOR_2     &button_right_sensor
#define EXT_FLASH_BASE_ADDR_SENSOR_DATA       0 
#define EXT_FLASH_MEMORY_END_ADDRESS          0x400010 // 4194320B
#define EXT_FLASH_BASE_ADDR       0
#define EXT_FLASH_SIZE            4*1024*1024
#define version         "3.0.0"
/*---------------------------------------------------------------------------*/

static struct etimer et;
struct  accel{ int x; int y; int z;};
static struct accel accel_data[1];
static bool write_to_flash = false;
static bool write_to_ext_flash = false;
static int samples = 0;
static int prev_second = 0;
static int now = -1;
static int time_started_writing = 0;
static void init_mpu_reading(void *not_used);

/*---------------------------------------------------------------------------*/
PROCESS(cc26xx_demo_process, "cc2650 accelerometer data process");
AUTOSTART_PROCESSES(&cc26xx_demo_process);
/*---------------------------------------------------------------------------*/
static void print_mpu_reading(int reading){
  if(reading < 0) {
    printf("-");
    reading = -reading;
  }
  printf("%d.%02d", reading / 100, reading % 100);
}

static void get_mpu_reading(){
  int seconds = clock_seconds();
  if(prev_second != seconds){
      printf("------------------------------------------%d %d :", prev_second, samples);
      prev_second = seconds;
      samples = 0;
  }

  accel_data[0].x = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
  print_mpu_reading(accel_data[0].x);
  printf(" G, ");
  accel_data[0].y = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
  print_mpu_reading(accel_data[0].y);
  printf(" G, ");
  accel_data[0].z = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);
  print_mpu_reading(accel_data[0].z);
  printf(" G\n");

  samples++;
  init_mpu_reading(NULL);
}

static void init_mpu_reading(void *not_used){
  mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ACC);
}


PROCESS_THREAD(cc26xx_demo_process, ev, data){

  PROCESS_BEGIN();

  printf("CS4222 DEMO for collecting accelerometer data (version %s)\n", version);
  printf("Long press the left pin to start the collection process (green led goes ON)\n");
  printf("Long press the right pin to stop the collection process (green led goes OFF)\n");
  printf("All accelerometer data are saved in the external flash\n");
  printf("During the collection process, the accelerometer data is displayed. Two lines are associated with each sample:\n");
  printf("    Line one shows the raw values of the collected data\n");
  printf("    Line two shows the respective values as written to the flash\n");
  printf("          Note that the first value is the time in second since data collection started.\n");

  etimer_set(&et, CC26XX_DEMO_LOOP_INTERVAL );
  init_mpu_reading(NULL);

  static int sensor_data_int[4];
  static int address_index_writing = -sizeof(sensor_data_int);
  static int time_started_writing = 0;
  
  etimer_set(&et, CC26XX_DEMO_LOOP_INTERVAL);
  while(1) {

    PROCESS_YIELD();

    if(ev == PROCESS_EVENT_TIMER) {
      if(data == &et) {
        etimer_set(&et, CC26XX_DEMO_LOOP_INTERVAL);
        if(write_to_ext_flash){
          get_mpu_reading();
          if(EXT_FLASH_BASE_ADDR_SENSOR_DATA + address_index_writing + sizeof(sensor_data_int) < EXT_FLASH_MEMORY_END_ADDRESS){
            int rv = ext_flash_open();
            if(!rv) {
              printf("Could not open EXT flash to save data...\n");
              ext_flash_close();
              return 0;
            }             
            now = clock_seconds() - time_started_writing; // *10 seconds
            sensor_data_int[0]= now;
            sensor_data_int[1]= accel_data[0].x;
            sensor_data_int[2]= accel_data[0].y;
            sensor_data_int[3]= accel_data[0].z;
            printf("%d,%d,%d,%d \n", now, accel_data[0].x, accel_data[0].y, accel_data[0].z);

            rv = ext_flash_write(EXT_FLASH_BASE_ADDR_SENSOR_DATA + address_index_writing, sizeof(sensor_data_int), (int *)&sensor_data_int);
            if(!rv) {
              printf("Error saving data to EXT flash\n");
            }
        
            address_index_writing += sizeof(sensor_data_int);
            ext_flash_close();
          }else{
            printf("POSSIBLE EXTERNAL FLASH OVERFLOW @0x%08X sizeof: %d\n",EXT_FLASH_BASE_ADDR_SENSOR_DATA + address_index_writing, sizeof(sensor_data_int));  
          }
        }      
      }
    } else if(ev == sensors_event) {
      if(data == CC26XX_DEMO_SENSOR_1) {
        printf("Left: Pin %d, press duration %d clock ticks\n",
               (CC26XX_DEMO_SENSOR_1)->value(BUTTON_SENSOR_VALUE_STATE),
               (CC26XX_DEMO_SENSOR_1)->value(BUTTON_SENSOR_VALUE_DURATION));

        if((CC26XX_DEMO_SENSOR_1)->value(BUTTON_SENSOR_VALUE_DURATION) >
           CLOCK_SECOND) {
          printf("Long button press!\n");
          if(!write_to_flash){
            write_to_flash=true;
            write_to_ext_flash=true;
            time_started_writing = clock_seconds(); 
            leds_on(CC26XX_DEMO_LEDS_PERIODIC);
            printf("WILL NOW WRITE TO FLASH!\n");
          }
        }

      } else if(data == CC26XX_DEMO_SENSOR_2) {
        printf("Right: Pin %d, press duration %d clock ticks\n",
       (CC26XX_DEMO_SENSOR_2)->value(BUTTON_SENSOR_VALUE_STATE),
       (CC26XX_DEMO_SENSOR_2)->value(BUTTON_SENSOR_VALUE_DURATION));

        if((CC26XX_DEMO_SENSOR_2)->value(BUTTON_SENSOR_VALUE_DURATION) >
            CLOCK_SECOND) {
          printf("Long button press!\n");
          if(write_to_flash){
            write_to_flash=false;
            write_to_ext_flash=false;
            // time_started_writing = clock_seconds(); 
            leds_off(CC26XX_DEMO_LEDS_PERIODIC);
            // leds_off(CC26XX_DEMO_LEDS_PERIODIC);
            printf("WILL STOP WRITING TO FLASH!\n");
          }
        }
      }
    }
  }

  PROCESS_END();            
}