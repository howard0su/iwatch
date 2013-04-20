/*
 * Copyright (C) 2010 by Matthias Ringwald
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MATTHIAS RINGWALD AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <string.h>
#include <stdlib.h>

#include "remote_device_db.h"
#include "btstack_memory.h"
#include "debug.h"

#include <btstack/utils.h>
#include <btstack/linked_list.h>

#include "dev/flash.h"

#pragma constseg = INFOB
const struct _configdata config_data;
#pragma constseg = default

// Device info
static void db_open(void){
}

static void db_close(void){
}

static int get_name(bd_addr_t *bd_addr, device_name_t *device_name) {

  if (memcmp(bd_addr, config_data.bd_addr, BD_ADDR_LEN) != 0)
  {
    return 0;
  }

  memcpy(device_name, config_data.device_name, DEVICE_NAME_LEN);

  return 1;
}

static int get_link_key(bd_addr_t *bd_addr, link_key_t *link_key) {
  log_info("get link key for %s\n", bd_addr_to_str(*bd_addr));
  if (memcmp(*bd_addr, config_data.bd_addr, BD_ADDR_LEN) != 0)
  {
    return 0;
  }

  memcpy(*link_key, config_data.link_key, LINK_KEY_LEN);

  return 1;
}

static void delete_link_key(bd_addr_t *bd_addr){
  log_info("delete link key for %s\n", bd_addr_to_str(*bd_addr));
  if (memcmp(*bd_addr, config_data.bd_addr, BD_ADDR_LEN) != 0)
  {
    return;
  }
  struct _configdata newdata;
  memcpy(&newdata, &config_data, sizeof(config_data));
  memset(newdata.bd_addr, 0xFF, BD_ADDR_LEN);
  memset(newdata.link_key, 0xFF, LINK_KEY_LEN);

  // write to flash
  flash_setup();
  flash_clear((uint16_t*)&config_data);
  flash_writepage((uint16_t*)&config_data, (uint16_t*)&newdata, sizeof(newdata));
  flash_done();
}


static void put_link_key(bd_addr_t *bd_addr, link_key_t *link_key){
  log_info("put link key for %s\n", bd_addr_to_str(*bd_addr));
  struct _configdata newdata;
  memcpy(&newdata, &config_data, sizeof(config_data));

  memcpy(newdata.bd_addr, *bd_addr, BD_ADDR_LEN);
  memcpy(newdata.link_key, *link_key, LINK_KEY_LEN);

  // write to flash
  flash_setup();
  flash_clear((uint16_t*)&config_data);
  flash_writepage((uint16_t*)&config_data, (uint16_t*)&newdata, sizeof(newdata));
  flash_done();
}

static void delete_name(bd_addr_t *bd_addr){
  if (memcmp(bd_addr, config_data.bd_addr, BD_ADDR_LEN) != 0)
  {
    return;
  }
  struct _configdata newdata;
  memcpy(&newdata, &config_data, sizeof(config_data));
  memset(newdata.device_name, 0, DEVICE_NAME_LEN);

  // write to flash
  flash_setup();
  flash_clear((uint16_t*)&config_data);
  flash_writepage((uint16_t*)&config_data, (uint16_t*)&newdata, sizeof(newdata));
  flash_done();
}

static void put_name(bd_addr_t *bd_addr, device_name_t *device_name){
  struct _configdata newdata;
  memcpy(&newdata, &config_data, sizeof(config_data));

  memcpy(newdata.bd_addr, bd_addr, BD_ADDR_LEN);
  memcpy(newdata.device_name, device_name, MAX_NAME_LEN);

  // write to flash
  flash_setup();
  flash_clear((uint16_t*)&config_data);
  flash_writepage((uint16_t*)&config_data, (uint16_t*)&newdata, sizeof(newdata));
  flash_done();
}


// MARK: PERSISTENT RFCOMM CHANNEL ALLOCATION

static uint8_t persistent_rfcomm_channel(char *serviceName){

  return 1;
}


const remote_device_db_t remote_device_db_memory = {
    db_open,
    db_close,
    get_link_key,
    put_link_key,
    delete_link_key,
    get_name,
    put_name,
    delete_name,
    persistent_rfcomm_channel
};
