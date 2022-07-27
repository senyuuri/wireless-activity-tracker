/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Reliable single-hop unicast example
 * \author
 *         Adam Dunkels <adam@sics.se>
 */
#include <stdio.h>
#include <stdlib.h> 
#include "contiki.h"
#include "net/rime/rime.h"
#include "board-peripherals.h"
#include "node-id.h"

#include "lib/list.h"
#include "lib/memb.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"

#define MAX_RETRANSMISSIONS 4
#define NUM_HISTORY_ENTRIES 4

/* Number of readings to be generated/sent. Size of 10000 samples is about 32KB*/
#define NUM_SAMPLE 10000
/* send 1024 bytes at a time */
#define PACKET_SIZE 1024 
#define EXT_FLASH_BASE_ADDR_SENSOR_DATA     0 
#define EXT_FLASH_MEMORY_END_ADDRESS        0x400010 
#define EXT_FLASH_BASE_ADDR                 0
#define EXT_FLASH_SIZE                      4*1024*1024

/*---------------------------------------------------------------------------*/
static int randint = -1;
static int address_index_writing = -sizeof(randint);
/*---------------------------------------------------------------------------*/
PROCESS(test_runicast_process, "runicast test");
AUTOSTART_PROCESSES(&test_runicast_process);
/*---------------------------------------------------------------------------*/
/* OPTIONAL: Sender history.
 * Detects duplicate callbacks at receiving nodes.
 * Duplicates appear when ack messages are lost. */
struct history_entry {
  struct history_entry *next;
  linkaddr_t addr;
  uint8_t seq;
};
LIST(history_table);
MEMB(history_mem, struct history_entry, NUM_HISTORY_ENTRIES);
/*---------------------------------------------------------------------------*/
static void
recv_runicast(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
  /* OPTIONAL: Sender history */
  struct history_entry *e = NULL;
  for(e = list_head(history_table); e != NULL; e = e->next) {
    if(linkaddr_cmp(&e->addr, from)) {
      break;
    }
  }
  if(e == NULL) {
    /* Create new history entry */
    e = memb_alloc(&history_mem);
    if(e == NULL) {
      e = list_chop(history_table); /* Remove oldest at full history */
    }
    linkaddr_copy(&e->addr, from);
    e->seq = seqno;
    list_push(history_table, e);
  } else {
    /* Detect duplicate callback */
    if(e->seq == seqno) {
      printf("runicast message received from %d.%d, seqno %d (DUPLICATE)\n",
	     from->u8[0], from->u8[1], seqno);
      return;
    }
    /* Update existing history entry */
    e->seq = seqno;
  }

  printf("runicast message received from %d.%d, seqno %d\n",
	 from->u8[0], from->u8[1], seqno);
}
static void
sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message sent to %d.%d, retransmissions %d\n",
	 to->u8[0], to->u8[1], retransmissions);
}
static void
timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
	 to->u8[0], to->u8[1], retransmissions);
}
static const struct runicast_callbacks runicast_callbacks = {recv_runicast,
							     sent_runicast,
							     timedout_runicast};
static struct runicast_conn runicast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_runicast_process, ev, data)
{
  PROCESS_EXITHANDLER(runicast_close(&runicast);)

  PROCESS_BEGIN();

  runicast_open(&runicast, 144, &runicast_callbacks);

  /* OPTIONAL: Sender history */
  list_init(history_table);
  memb_init(&history_mem);

  /* Receiver node: do nothing */
  if(linkaddr_node_addr.u8[0] == 1 &&
     linkaddr_node_addr.u8[1] == 0) {
    PROCESS_WAIT_EVENT_UNTIL(0);
  }

  /* generate 32KB random data and save to external storage */
  srand(1);
  leds_on(LEDS_YELLOW);

  if(EXT_FLASH_BASE_ADDR_SENSOR_DATA + address_index_writing + sizeof(randint) < EXT_FLASH_MEMORY_END_ADDRESS){
    int executed = ext_flash_open();
    if(!executed) {
      printf("Cannot open flash\n");
      ext_flash_close();
      return 0;
    }
    for(int i=0; i< NUM_SAMPLE; i++){
      randint = rand() % 100;
      printf('i:%d, random int: %d\n',i , randint);
      if(i % 1000 == 0){
        printf("WRITE PROGRESS:%d/10000\n", i); 
        printf("WRITING TO EXTERNAL FLASH @0x%08X sizeof: %d\n",EXT_FLASH_BASE_ADDR_SENSOR_DATA + address_index_writing, sizeof(randint));
      }
      executed = ext_flash_write(EXT_FLASH_BASE_ADDR_SENSOR_DATA + address_index_writing, sizeof(randint), (int *)&randint);
      printf('%d\n',randint);
      if(!executed) {
        printf("Error saving data to EXT flash\n");
      }
      address_index_writing += sizeof(randint);
    }

    ext_flash_close();

  } else{
    printf("POSSIBLE EXTERNAL FLASH OVERFLOW @0x%08X sizeof: %d\n",EXT_FLASH_BASE_ADDR_SENSOR_DATA + address_index_writing, sizeof(randint));  
  }
  leds_off(LEDS_YELLOW);


  /* Read test */
  static int address_offset = 0;
  static int buffer_offset = 0;
  static int sensor_data_int;
  char buffer[PACKET_SIZE] = {'\0'};

  int executed = ext_flash_open();

  if(!executed) {
    printf("Cannot open flash\n");
    ext_flash_close();
    return 0;
  }

  int count = 0;
  int pointer = EXT_FLASH_BASE_ADDR + address_offset;
  while(pointer < sizeof(sensor_data_int) * NUM_SAMPLE){
    executed = ext_flash_read(address_offset, sizeof(sensor_data_int),  () *)&sensor_data_int);
    count++;
    // if buffer if full, send packet and clear buffer
    if(buffer_offset >= PACKET_SIZE){
      printf("Buffer is full(lenth: %d), sending packet...", buffer_offset);

      printf("Buffer content: %s\n", buffer);
      // TODO send message
      // clear buffer
      memset(buffer, '\0', sizeof(buffer));
      buffer_offset = 0;
      printf("Buffer cleared!");
    }

    printf("%08X,%d\n", pointer,sensor_data_int);
    memcpy(buffer+buffer_offset, (char*) &sensor_data_int, sizeof(sensor_data_int));
    buffer_offset += sizeof(sensor_data_int);
    address_offset += sizeof(sensor_data_int);
    pointer = EXT_FLASH_BASE_ADDR + address_offset;
    //printf("count: %d, addr offset: %d, buffer size: %d,\n", count, address_offset, buffer_offset);
    //printf("buffer_content: %s\n", buffer);
  }
  ext_flash_close();
  printf("%d samples read successfully", count);

  // if buffer not empty, flush buffer and send last package
  // else send terminating char '\n'
  printf("last buffer length: %d\n", buffer_offset);
  printf("last buffer content: %s\n", buffer);


  while(1) {
    // static struct etimer et;

    // etimer_set(&et, 10*CLOCK_SECOND);
    // PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    // if(!runicast_is_transmitting(&runicast)) {
    //   linkaddr_t recv;

    //   packetbuf_copyfrom("abcdefghijklmnopqrstuvwxyzAabcdefghijklmnopqrstuvwxyzA\n", 54);
    //   recv.u8[0] = 169;
    //   recv.u8[1] = 134;

    //   printf("%u.%u: sending runicast to address %u.%u\n",
	   //   linkaddr_node_addr.u8[0],
	   //   linkaddr_node_addr.u8[1],
	   //   recv.u8[0],
	   //   recv.u8[1]);

    //   runicast_send(&runicast, &recv, MAX_RETRANSMISSIONS);
    // }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
