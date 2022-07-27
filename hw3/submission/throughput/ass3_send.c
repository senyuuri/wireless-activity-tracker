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

#include "contiki.h"
#include "net/rime/rime.h"

#include "lib/list.h"
#include "lib/memb.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"

#include "board-peripherals.h"

#define MAX_RETRANSMISSIONS 15
#define NUM_HISTORY_ENTRIES 4
#define TOTAL_FILE_SIZE 32 * 1024
#define PACKET_SIZE 50
#define HEADER_SIZE 4

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
static int dataIndex = 0;
static int data_num = (PACKET_SIZE - HEADER_SIZE) / 2;
// void print_hex_memory(void *mem) {
//   int i;
//   unsigned char *p = (unsigned char *)mem;
//   for (i=0;i<64;i++) {
//     printf("0x%02x ", p[i]);
//     if ((i%16==0) && i)
//       printf("\n");
//   }
//   printf("\n");
// }

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
      // printf("runicast message received from %d.%d, seqno %d (DUPLICATE)\n",
	    //  from->u8[0], from->u8[1], seqno);
      return;
    }
    /* Update existing history entry */
    e->seq = seqno;
  }

  // printf("runicast message received from %d.%d, seqno %d\n",
	//  from->u8[0], from->u8[1], seqno);
}
static void
sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  // printf("runicast message sent to %d.%d, retransmissions %d\n",
	//  to->u8[0], to->u8[1], retransmissions);
  dataIndex += data_num * 4;
}
static void
timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  // printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
	//  to->u8[0], to->u8[1], retransmissions);
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

  int executed = ext_flash_open();

  if(!executed) {
    printf("Cannot open flash\n");
    ext_flash_close();
    return 0;
  }

  static int sensor_data_int[4];
  static int address_index_writing = -sizeof(sensor_data_int);

  for (int i = 0; i < TOTAL_FILE_SIZE; i +=sizeof(sensor_data_int)) {
      sensor_data_int[0]= i%100;
      sensor_data_int[1]= (i+1)%100;
      sensor_data_int[2]= (i+2)%100;
      sensor_data_int[3]= (i+3)%100;

      if (ext_flash_write(0 + address_index_writing, sizeof(sensor_data_int), (int *)&sensor_data_int)) {
        // printf("writing at index %d\n", i);
        address_index_writing += sizeof(sensor_data_int);
      }
  }

  ext_flash_close();

  
  static int address_offset = 0;
  static int send_buf[PACKET_SIZE / 2];
 
  while(1) {
    static struct etimer et;

    etimer_set(&et, CLOCK_SECOND / 10);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    if(!runicast_is_transmitting(&runicast)) {
      linkaddr_t recv;

      if (dataIndex > TOTAL_FILE_SIZE) {
        printf("Finished transmitting: %d of 32 kb\n", dataIndex);
        break;
      }
      // printf("dataindex:%d\n", dataIndex);
      if (dataIndex % (data_num * 4) == 0) {
        ext_flash_open();
        memset(send_buf, '\0', sizeof(send_buf));
        if (ext_flash_read(dataIndex, (data_num * 4), (int *)&send_buf)) {
          // printf("DATA READ PASS: %d \n", dataIndex, send_buf);
          // for(int i=0;i<12;i++){
          //   printf("%d,",send_buf[i]);
          // }
          
          // printf("\n");
        } else {
          printf("DATA READ FAIL");
        }
        ext_flash_close();
      }


      
      recv.u8[0] = 14;
      recv.u8[1] = 231;

      // printf("%u.%u: sending runicast to address %u.%u\n",
	    //  linkaddr_node_addr.u8[0],
	    //  linkaddr_node_addr.u8[1],
	    //  recv.u8[0],
	    //  recv.u8[1]);
      
      static char numberbuf[2];
      static char finalbuf[PACKET_SIZE + 1];
      // printf("read success: %d \n", dataIndex);
      for(int i=0;i < data_num; i++){
        printf("%d,",send_buf[i]);
        sprintf(numberbuf, "%02d", send_buf[i]);
        memcpy(finalbuf + i*2, numberbuf, 2);
      }

      static char seqbuf[4];
      int packetSeq = dataIndex / (data_num * 4);
      sprintf(seqbuf, "%04d", packetSeq);

      memcpy(finalbuf+data_num*2, seqbuf, 4);
      printf("%04d", packetSeq);
      printf("\n");

      packetbuf_copyfrom(finalbuf, PACKET_SIZE);
      // print_hex_memory(finalbuf);
      // print_hex_memory(packetbuf_dataptr());
      runicast_send(&runicast, &recv, MAX_RETRANSMISSIONS);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
