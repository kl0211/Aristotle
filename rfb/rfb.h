/*
 * rfb.h
 *
 *
 */

#ifndef _RFB_H
#define _RFB_H

#include <endian.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define CLIENTMAX 20

/* Service defaults */
#define BKLG_DFT 20
#define NAME_DFT "Aristotle"
#define COLORS {0x00005577, 0x00007755, 0x00550077, \
                0x00557700, 0x00770055, 0x00775500}

/* Packing/unpacking for size and endianness */
#define PK8(i) ((uint8_t) i)
#define UPK8(i) ((uint8_t) i)
#define PK16(i) (htons((uint16_t) i))
#define UPK16(i) (ntohs((uint16_t) i))
#define PK32(i) (htonl((uint32_t) i))
#define UPK32(i) (ntohl((uint32_t) i))

/* RFB version string */
#define RFBVER_SZ (12) /* Version string size */
#define RFBVER(maj,min) "RFB 00" #maj ".00" #min "\n"
#define MAJ(ver) (atoi(ver + 4))
#define MIN(ver) (atoi(ver + 8))

/* RFB security types */
#define RFBSEC_SZ (2) /* Max size of sec type list */
#define RFBSEC_ERR (0)
#define RFBSEC_NONE (1)
#define RFBSEC_VNC (2)

/* RFB pixel format */
typedef struct rfb_pxlinfo
{
  uint8_t bits; /* Bits per pixel */
  uint8_t depth; /* Significant bits per pixel */
  uint8_t bigend; /* Big endian flag */
  uint8_t trucol; /* True color flag */
  uint16_t rmax; /* Max red value */
  uint16_t gmax; /* Max green value */
  uint16_t bmax; /* Max blue value */
  uint8_t rshift; /* Red right shift */
  uint8_t gshift; /* Green right shift */
  uint8_t bshift; /* Blue right shift */
} rfb_pxlfmt_t;
#define RFBPXL_DFT \
((struct rfb_pxlinfo) \
{ \
  32, 24, \
  1, \
  1, \
  0xff, 0xff, 0xff, \
  16, 8, 0 \
})

/* RFB protocol information struct */
typedef struct rfb_protinfo
{
  unsigned char ver[RFBVER_SZ]; /* RFB version string */
  uint8_t seclen; /* Number of security types */
  uint8_t sec[RFBSEC_SZ]; /* List of security types */
  uint16_t fbwidth; /* Framebuffer width */
  uint16_t fbheight; /* Framebuffer height */
  struct rfb_pxlinfo pxl; /* Pixel format */
  uint32_t namelen; /* Name length */
  char *name; /* Pointer to name string */
} rfb_protinfo_t;
#define RFBPROTO_DFT \
((struct rfb_protinfo) \
{ \
  RFBVER(3,8), \
  1, {RFBSEC_NONE}, \
  720, 480, \
  RFBPXL_DFT, \
  (sizeof NAME_DFT) - 1, NAME_DFT \
})

/* Struct to describe an RFB user-input event */
typedef struct rfb_event
{
  uint32_t color;
  uint16_t x;
  uint16_t y;
  struct rfb_event *next;
  uint8_t buttmask;
} rfb_event_t;

/* Struct to describe a client to the server */
typedef struct rfb_client
{
  uint16_t startx;
  uint16_t starty;
  uint16_t endx;
  uint16_t endy;
  uint32_t color;
  struct rfb_client *next;
  uint8_t mod;
} rfb_client_t;

/* Info to pass back to the calling routine. */
typedef struct rfb_servinfo
{
  int sockfd;
  uint32_t *bitmap;
  struct rfb_event *event_list;
  pthread_mutex_t event_mutex;
  struct rfb_client *client_list;
  pthread_mutex_t client_mutex;
  pthread_t thr;
  struct rfb_protinfo pinfo;
} rfb_servinfo_t;

/* Information necessary for a client thread. */
typedef struct rfb_threadinfo
{
  int sockfd;
  struct rfb_servinfo *sinfop;
  struct rfb_threadinfo *next;
  pthread_t thr;
  struct rfb_client client;
} rfb_threadinfo_t;

/* External interface */
rfb_servinfo_t *rfb_run(int w, int h, char *name, char *serv);
void rfb_kill(rfb_servinfo_t *sinfop);
rfb_event_t *rfb_event_next(rfb_event_t *event_list);

#endif /* _RFB_H */
