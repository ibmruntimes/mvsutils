/*
 * Licensed Materials - Property of IBM
 * (C) Copyright IBM Corp. 2019. All Rights Reserved.
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

static void __console(const void *p_in, int len_i) {
  const unsigned char *p = (const unsigned char *)p_in;
  int len = len_i;
  typedef struct wtob {
    unsigned short sz;
    unsigned short flags;
    unsigned char msgarea[256];
  } wtob_t;
  wtob_t *m = (wtob_t *)__malloc31(len + 8);
  if (0 == m)
    return;
  m->sz = len + 4;
  m->flags = 0x8000;
  memcpy(m->msgarea, p, len);
  memcpy(m->msgarea + len, "\x20\x00\x00\x20", 4);
  __asm(" la  0,0 \n"
        " lr  1,%0 \n"
        " svc 35 \n"
        :
        : "r"(m)
        : "r0", "r1", "r15");
  free(m);
}
extern "C" void __con_print(const char *msg) {
  int len = strlen(msg);
  char *m = (char *)alloca(len + 1);
  memcpy(m, msg, len + 1);
  __a2e_l(m, len);
  int mode;
  __console(m, len);
  return;
}
