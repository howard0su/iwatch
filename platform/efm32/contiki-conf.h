/* -*- C -*- */
/* @(#)$Id: contiki-conf.h,v 1.91 2011/01/09 21:04:14 adamdunkels Exp $ */
#ifndef ECONTIKI_CONF_H
#define ECONTIKI_CONF_H
//#ifndef CONTIKI_CONF_H
//#define CONTIKI_CONF_H

#ifdef PLATFORM_CONF_H
#include PLATFORM_CONF_H
#else
#include "platform-conf.h"
#endif /* PLATFORM_CONF_H */

#include <inttypes.h>
#include <string.h>  // For memcpm().

#define CLOCK_CONF_SECOND 	8
#define TICK_PER_SECOND		(CLOCK_CONF_SECOND)

#define CCIF
#define CLIF

#define BIT0 1
#define BIT1 2
#define BIT2 4
#define BIT3 8
#define BIT4 16
#define BIT5 32
#define BIT6 64
#define BIT7 128

#ifdef EM_DEBUG
#define EM_ASSERT(EX)                                                         \
if (!(EX))                                                                    \
{                                                                             \
    volatile char dummy = 0;                                                  \
    rt_kprintf("(%s) ASSERT failed at %s:%d \n", #EX, __FUNCTION__, __LINE__);\
    while (dummy == 0);                                                       \
}
#else
	#define EM_ASSERT(EX)
#endif


#if 0
	#ifndef NETSTACK_CONF_MAC
	#define NETSTACK_CONF_MAC     csma_driver
	#endif /* NETSTACK_CONF_MAC */
	
	#ifndef NETSTACK_CONF_RDC
	#define NETSTACK_CONF_RDC     contikimac_driver
	#endif /* NETSTACK_CONF_RDC */
	
	#ifndef NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
	#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8
	#endif /* NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE */
	
	#ifndef NETSTACK_CONF_RADIO
	#define NETSTACK_CONF_RADIO   cc2420_driver
	#endif /* NETSTACK_CONF_RADIO */
	
	#ifndef NETSTACK_CONF_FRAMER
	#define NETSTACK_CONF_FRAMER  framer_802154
	#endif /* NETSTACK_CONF_FRAMER */
	
	#ifndef CC2420_CONF_AUTOACK
	#define CC2420_CONF_AUTOACK              1
	#endif /* CC2420_CONF_AUTOACK */

	#define NULLRDC_CONF_802154_AUTOACK 1

	#if WITH_UIP6
		/* Network setup for IPv6 */
		#define NETSTACK_CONF_NETWORK sicslowpan_driver
		
		/* Specify a minimum packet size for 6lowpan compression to be
		   enabled. This is needed for ContikiMAC, which needs packets to be
		   larger than a specified size, if no ContikiMAC header should be
		   used. */
		#define SICSLOWPAN_CONF_COMPRESSION_THRESHOLD 63
		#define CONTIKIMAC_CONF_WITH_CONTIKIMAC_HEADER 0
		
		#define CXMAC_CONF_ANNOUNCEMENTS         0
		#define XMAC_CONF_ANNOUNCEMENTS          0
		
		#ifndef QUEUEBUF_CONF_NUM
		#define QUEUEBUF_CONF_NUM                8
		#endif
		
	#else /* WITH_UIP6 */

/* Network setup for non-IPv6 (rime). */

		#define NETSTACK_CONF_NETWORK rime_driver
		
		#define COLLECT_CONF_ANNOUNCEMENTS       1
		#define CXMAC_CONF_ANNOUNCEMENTS         0
		#define XMAC_CONF_ANNOUNCEMENTS          0
		#define CONTIKIMAC_CONF_ANNOUNCEMENTS    0
		
		#define CONTIKIMAC_CONF_COMPOWER         1
		#define XMAC_CONF_COMPOWER               1
		#define CXMAC_CONF_COMPOWER              1
		
		#ifndef COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS
		#define COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS     32
		#endif /* COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS */
		
		#ifndef QUEUEBUF_CONF_NUM
		#define QUEUEBUF_CONF_NUM                16
		#endif /* QUEUEBUF_CONF_NUM */
		
		#ifndef TIMESYNCH_CONF_ENABLED
		#define TIMESYNCH_CONF_ENABLED           0
		#endif /* TIMESYNCH_CONF_ENABLED */
		
		#if TIMESYNCH_CONF_ENABLED
		/* CC2420 SDF timestamps must be on if timesynch is enabled. */
		#undef CC2420_CONF_SFD_TIMESTAMPS
		#define CC2420_CONF_SFD_TIMESTAMPS       1
		#endif /* TIMESYNCH_CONF_ENABLED */
	
	#endif /* WITH_UIP6 */

	#define PACKETBUF_CONF_ATTRS_INLINE 1
	
	#ifndef RF_CHANNEL
	#define RF_CHANNEL              26
	#endif /* RF_CHANNEL */
	
	#define CONTIKIMAC_CONF_BROADCAST_RATE_LIMIT 0
	
	#define IEEE802154_CONF_PANID       0xABCD
	
#endif	//#if 0



#define SHELL_VARS_CONF_RAM_BEGIN 0x1100
#define SHELL_VARS_CONF_RAM_END 0x2000

#define PROFILE_CONF_ON 0
#ifndef ENERGEST_CONF_ON
#define ENERGEST_CONF_ON 1
#endif /* ENERGEST_CONF_ON */

#define ELFLOADER_CONF_TEXT_IN_ROM 0
#ifndef ELFLOADER_CONF_DATAMEMORY_SIZE
#define ELFLOADER_CONF_DATAMEMORY_SIZE 0x400
#endif /* ELFLOADER_CONF_DATAMEMORY_SIZE */
#ifndef ELFLOADER_CONF_TEXTMEMORY_SIZE
#define ELFLOADER_CONF_TEXTMEMORY_SIZE 0x800
#endif /* ELFLOADER_CONF_TEXTMEMORY_SIZE */


#define AODV_COMPLIANCE
#define AODV_NUM_RT_ENTRIES 32

#define WITH_ASCII 1

#define PROCESS_CONF_NUMEVENTS 8
#define PROCESS_CONF_STATS 1
/*#define PROCESS_CONF_FASTPOLL    4*/

#ifdef WITH_UIP6

	#define RIMEADDR_CONF_SIZE              8
	
	#define UIP_CONF_LL_802154              1
	#define UIP_CONF_LLH_LEN                0
	
	#define UIP_CONF_ROUTER                 1
	#ifndef UIP_CONF_IPV6_RPL
	#define UIP_CONF_IPV6_RPL               1
	#endif /* UIP_CONF_IPV6_RPL */
	
	/* configure number of neighbors and routes */
	#ifndef UIP_CONF_DS6_NBR_NBU
	#define UIP_CONF_DS6_NBR_NBU     30
	#endif /* UIP_CONF_DS6_NBR_NBU */
	#ifndef UIP_CONF_DS6_ROUTE_NBU
	#define UIP_CONF_DS6_ROUTE_NBU   30
	#endif /* UIP_CONF_DS6_ROUTE_NBU */
	
	#define UIP_CONF_ND6_SEND_RA		0
	#define UIP_CONF_ND6_REACHABLE_TIME     600000
	#define UIP_CONF_ND6_RETRANS_TIMER      10000
	
	#define UIP_CONF_IPV6                   1
	#ifndef UIP_CONF_IPV6_QUEUE_PKT
	#define UIP_CONF_IPV6_QUEUE_PKT         0
	#endif /* UIP_CONF_IPV6_QUEUE_PKT */
	#define UIP_CONF_IPV6_CHECKS            1
	#define UIP_CONF_IPV6_REASSEMBLY        0
	#define UIP_CONF_NETIF_MAX_ADDRESSES    3
	#define UIP_CONF_ND6_MAX_PREFIXES       3
	#define UIP_CONF_ND6_MAX_NEIGHBORS      4
	#define UIP_CONF_ND6_MAX_DEFROUTERS     2
	#define UIP_CONF_IP_FORWARD             0
	#ifndef UIP_CONF_BUFFER_SIZE
	#define UIP_CONF_BUFFER_SIZE		240
	#endif
	
	#define SICSLOWPAN_CONF_COMPRESSION_IPV6        0
	#define SICSLOWPAN_CONF_COMPRESSION_HC1         1
	#define SICSLOWPAN_CONF_COMPRESSION_HC01        2
	#define SICSLOWPAN_CONF_COMPRESSION             SICSLOWPAN_COMPRESSION_HC06
	#ifndef SICSLOWPAN_CONF_FRAG
	#define SICSLOWPAN_CONF_FRAG                    1
	#define SICSLOWPAN_CONF_MAXAGE                  8
	#endif /* SICSLOWPAN_CONF_FRAG */
	#define SICSLOWPAN_CONF_CONVENTIONAL_MAC	1
	#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS       2
	#ifndef SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS
	#define SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS   5
	#endif /* SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS */
#else /* WITH_UIP6 */
	#define UIP_CONF_IP_FORWARD      1
	#define UIP_CONF_BUFFER_SIZE     108
#endif /* WITH_UIP6 */

#define UIP_CONF_ICMP_DEST_UNREACH 1

#define UIP_CONF_DHCP_LIGHT
#define UIP_CONF_LLH_LEN         0
#ifndef  UIP_CONF_RECEIVE_WINDOW
#define UIP_CONF_RECEIVE_WINDOW  48
#endif
#ifndef  UIP_CONF_TCP_MSS
#define UIP_CONF_TCP_MSS         48
#endif
#define UIP_CONF_MAX_CONNECTIONS 4
#define UIP_CONF_MAX_LISTENPORTS 8
#define UIP_CONF_UDP_CONNS       12
#define UIP_CONF_FWCACHE_SIZE    30
#define UIP_CONF_BROADCAST       1
#define UIP_ARCH_IPCHKSUM        0
#define UIP_CONF_UDP             1
#define UIP_CONF_UDP_CHECKSUMS   1
#define UIP_CONF_PINGADDRCONF    0
#define UIP_CONF_LOGGING         0

#define UIP_CONF_TCP_SPLIT       0



/* include the project config */
/* PROJECT_CONF_H might be defined in the project Makefile */
#ifdef PROJECT_CONF_H
#include PROJECT_CONF_H
#endif /* PROJECT_CONF_H */


#endif /* CONTIKI_CONF_H */
