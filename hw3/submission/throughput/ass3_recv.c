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

#define EXT_FLASH_BASE_ADDR_SENSOR_DATA     0 
#define EXT_FLASH_MEMORY_END_ADDRESS        0x400010 
#define EXT_FLASH_BASE_ADDR                 0
#define EXT_FLASH_SIZE                      4*1024*1024

#define PACKET_SIZE 50
static int recv_pkt_count = 0;
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

void print_hex_memory(void *mem) {
  int i;
  unsigned char *p = (unsigned char *)mem;
  for (i=0;i<64;i++) {
    printf("0x%02x ", p[i]);
    if ((i%16==0) && i)
      printf("\n");
  }
  printf("\n");
}

static void
recv_runicast(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
  recv_pkt_count++;
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
      //printf("%d: runicast message received from %d.%d, seqno %d (DUPLICATE)\n",
	    // recv_pkt_count, from->u8[0], from->u8[1], seqno);
      return;
    }
    /* Update existing history entry */
    e->seq = seqno;
  }

  uint16_t pkt_len = packetbuf_datalen();
  static uint8_t buf[PACKET_SIZE+1];
  memset(buf, '\0', PACKET_SIZE+1);
  strncpy(buf, packetbuf_dataptr(), PACKET_SIZE);
  buf[PACKET_SIZE] = '\0';
  printf("%s\n", (uint8_t *)buf);
  //printf("%d,runicast message received from %d.%d, seqno %d, length %u, content: %s\n",
	// recv_pkt_count, from->u8[0], from->u8[1], seqno, (unsigned int)pkt_len, (uint8_t *)buf);
  // print_hex_memory((uint8_t *)buf);
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
  // printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
	 // to->u8[0], to->u8[1], retransmissions);
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

  while(1) {
    static struct etimer et;

    etimer_set(&et, 10*CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
