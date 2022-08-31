/*
 * Licensed Materials - Property of IBM
 * (C) Copyright IBM Corp. 2022. All Rights Reserved.
 * US Government Users Restricted Rights - Use, duplication or disclosure
 * restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#if defined(__MVS__)
#include "mvsutils.h"
#include <_Nascii.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <strings.h>
#include <unistd.h>
#else
#error This addon is for ZOS only
#endif

#if ' ' == 0x40
#error Not compiled with -qascii
#endif

extern int logging;

static void __console_multiline(const void *p, unsigned int len) {
  if (logging)
    fprintf(stderr, "In __console_multiline: %p - length: %d\n", p, len);
  const unsigned char *str = (const unsigned char *)p;
  static unsigned int _wto_init[] = {
      0x00088050, 0x55555555, 0x02000068, 0x00800000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000, 0x00200000, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x40404040, 0x40404040, 0x40404040, 0x40404040, 0x40404040,
      0x40404040, 0x00000000, 0x00000000, 0x40404040, 0x40404040, 0x40404040,
      0x40404040, 0x00000000, 0x00000000, 0x00000000, 0x2000000a, 0x00082000,
      0x55555555 };
  struct wto_parm {
    unsigned short text_len;
    unsigned short msgflags;
    void *__ptr32 textaddr;
    unsigned char version;
    unsigned char misc_flags;
    unsigned char reply_len;
    unsigned char len_wpx;
    unsigned short ext_msc_flags;
    unsigned short res1;
    void *__ptr32 reply_buffer_addr;
    void *__ptr32 reply_ebc_addr;
    void *__ptr32 connect_id;
    unsigned short desc_code;
    unsigned short res2;
    unsigned char extended_routing_code[16];
    unsigned short message_type;
    unsigned short message_priority;
    unsigned char job_id[8];
    unsigned char job_name[8];
    unsigned char retrieve_key[8];
    void *__ptr32 token_for_dom;
    void *__ptr32 console_id;
    unsigned char system_name[8];
    unsigned char console_name[8];
    void *__ptr32 reply_console_name_id_addr;
    void *__ptr32 cart_addr;
    void *__ptr32 wsparm_addr;
    unsigned short line_type;
    unsigned char area_id;
    unsigned char total_num_of_lines;
    struct line_parm {
      unsigned short msg_len;
      unsigned short line_type;
      void *__ptr32 text_addr;
    } line[256];
    struct content {
      unsigned short size;
      unsigned char buf[70];
    } firstline, content[255];
  };
#undef WIDTH_1
#undef WIDTH
#define WIDTH_1 65
#define WIDTH 69
  struct wto_parm * parm =
      (struct wto_parm *)__malloc31(sizeof(struct wto_parm));
  memcpy(parm, &_wto_init, sizeof(_wto_init));
  int i;
  unsigned char cnt = 0;
  do {
    parm->textaddr = &parm->firstline;
    if (len > WIDTH_1) {
      parm->firstline.size = WIDTH_1;
      memcpy(parm->firstline.buf, str, WIDTH_1);
      __a2e_l((char *)parm->firstline.buf, WIDTH_1);
      str += WIDTH_1;
      len -= WIDTH_1;
      ++cnt;
    } else {
      parm->firstline.size = len;
      memcpy(parm->firstline.buf, str, len);
      __a2e_l((char *)parm->firstline.buf, len);
      ++cnt;
      break;
    }
    if (logging)
      fprintf(stderr, "In __console_multiline: data[first].text_addr: %p, data[first].size: %d\n", parm->textaddr, parm->firstline.size);

    for (i = 0; i < 256; ++i) {
      if (logging)
        fprintf(stderr, "In __console_multiline: loop i: %d\n", i);
      parm->line[i].text_addr = &parm->content[i];
      parm->line[i].msg_len = 8;
      if (len > WIDTH) {
        parm->line[i].line_type = 0x2000;
        parm->content[i].size = WIDTH;
        memcpy(parm->content[i].buf, str, WIDTH);
        if (logging)
          fprintf(stderr, "In __console_multiline: str: %.*s\n", WIDTH, parm->content[i].buf);
        __a2e_l((char *)parm->content[i].buf, WIDTH);
        str += WIDTH;
        len -= WIDTH;
        ++cnt;
      } else {
        parm->line[i].line_type = 0x3000;
        parm->content[i].size = len;
        memcpy(parm->content[i].buf, str, len);
        if (logging)
          fprintf(stderr, "In __console_multiline: str: %.*s\n", len, parm->content[i].buf);
        __a2e_l((char *)parm->content[i].buf, len);
        str += len;
        len = 0;
        ++cnt;
        break;
      }
      if (logging)
        fprintf(stderr, "In __console_multiline: data[%d].text_addr: %p, data[%d].size: %d\n", i, parm->line[i].text_addr, i, parm->line[i].msg_len);
    }
    if (i == 256) {
      parm->line[255].line_type = 0x3000;
    }
    if (logging)
      fprintf(stderr, "In __console_multiline: data[%d].text_addr: %p, data[%d].size: %d\n", i, parm->line[i].text_addr, i, parm->line[i].msg_len);
  } while (0);
  parm->total_num_of_lines = cnt;

  if (logging)
    fprintf(stderr, "In __console_multiline: Writing via svc 35, # of lines: %d\n", parm->total_num_of_lines);

  __asm(" la  0,0 \n"
        " lr  1,%0 \n"
        " svc 35 \n"
        : "+NR:r1"(parm)
        :
        : "r0", "r15");

  if (logging)
    fprintf(stderr, "In __console_multiline: After svc 35, freeing memory\n");

  free(parm);

  if (logging)
    fprintf(stderr, "Existing __console_multiline\n");
#undef WIDTH_1
#undef WIDTH
}
static void __console(const void *p_in, int len_i) {
  if (logging)
    fprintf(stderr, "In __console: %p - length: %d\n", p_in, len_i);
  const unsigned char *p = (const unsigned char *)p_in;
  int len = len_i;
  typedef struct wtob {
    unsigned short sz;
    unsigned short flags;
    unsigned char msgarea[256];
  } wtob_t;
  wtob_t * m = (wtob_t *)__malloc31(len + 8);
  if (0 == m)
    return;
  m->sz = len + 4;
  m->flags = 0x8000;
  memcpy(m->msgarea, p, len);
  if (logging)
    fprintf(stderr, "In __console: msgarea: %.*s\n", len, m->msgarea);
  __a2e_l((char *)m->msgarea, len);
  memcpy(m->msgarea + len, "\x20\x00\x00\x20", 4);
  if (logging)
    fprintf(stderr, "In __console: before svc 35 call\n");
  __asm(" la  0,0 \n"
        " lr  1,%0 \n"
        " svc 35 \n"
        :
        : "r"(m)
        : "r0", "r1", "r15");
  if (logging)
    fprintf(stderr, "In __console: after svc 35 call, freeing\n");
  free(m);
  if (logging)
    fprintf(stderr, "Exiting __console\n");
}
extern "C" void __con_print(const char *msg) {
  if (logging)
    fprintf(stderr, "In __con_print\n");
  int len = strlen(msg);
  if (len > 126)
    __console_multiline(msg, len);
  else
    __console(msg, len);
  return;
}
