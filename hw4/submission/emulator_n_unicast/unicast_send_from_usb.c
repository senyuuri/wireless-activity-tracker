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
 *         Best-effort single-hop unicast example
 * \author
 *         Adam Dunkels <adam@sics.se>
 */
#include "contiki.h"
#include "net/rime/rime.h"
#include <stdio.h>

#include "dev/serial-line.h"
#include "dev/cc26xx-uart.h"
#include "ti-lib.h"

/*---------------------------------------------------------------------------*/
PROCESS(unicast_send_from_usb_process, "UNICAST SEND FROM USB");
AUTOSTART_PROCESSES(&unicast_send_from_usb_process);
/*---------------------------------------------------------------------------*/

static int losses = 0;

static void sent_uc(struct unicast_conn *c, int status, int num_tx){
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("unicast message sent to %d.%d: status %d num_tx %d\n",
    dest->u8[0], dest->u8[1], status, num_tx);
  if(status != 0){
    losses++;
  }
}

static const struct unicast_callbacks unicast_callbacks = {sent_uc};
static struct unicast_conn uc;

PROCESS_THREAD(unicast_send_from_usb_process, ev, data){

  PROCESS_EXITHANDLER(unicast_close(&uc);)  
  PROCESS_BEGIN();
  cc26xx_uart_set_input(serial_line_input_byte);
  unicast_open(&uc, 146, &unicast_callbacks);

  static int count =0;
  static int fail = 0;
  
  printf("next\n");
  while(1) {
    static struct etimer et;
    etimer_set(&et, CLOCK_SECOND);
    
    PROCESS_YIELD();

    if(etimer_expired(&et)){
      printf("LOSSES: %d\n", losses);
      printf("RECEIVED FROM USB TO SEND: %d\n", count);
      etimer_set(&et, CLOCK_SECOND);
    }

    if(ev == serial_line_event_message){
      count++;        
      printf("%d:received line: %s\n", count,(char *)data);  

      linkaddr_t recv;
      recv.u8[0] = 0;
      recv.u8[1] = 1;

      packetbuf_copyfrom((char *)data, strlen((char *)data) +1);

      printf("sending (%d): %s\n",strlen((char *)data), (char *)data);
      unicast_send(&uc, &recv);
      printf("next\n");
    }
  }
  PROCESS_END();
}
