#ifndef _HFP_H_
#define _HFP_H_
void hfp_init(int channel);
void hfp_handler(int rfcomm_channel_id, int type, uint8_t *packet, uint16_t len);
#endif