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
#include <mutex>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
extern "C" void __console_printf(const char *, ...);
#else
#error This addon is for ZOS only
#endif

#if ' ' == 0x40
#error Not compiled with -qascii
#endif

static int byte0_next_state[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,
    3,  -1, -1, -1, -1, -1, -1, -1, -1};

int utf8scan(char *errmsg, size_t sz, int fd) {
  unsigned char c;
  unsigned char onebyte;
  char output[80];
  int bytes;
  int state = 0;
  unsigned int value;
  unsigned char d[4];
  size_t offset = 0;
  int linenum = 1;
  c = read(fd, &onebyte, 1);
  while (c == 1) {
    switch (state) {
    case 0:
      state = byte0_next_state[onebyte];
      if (-1 == state) {
        snprintf(errmsg, sz,
                 "Error deleted in: fd %d, "
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, byte 0x%02X malformed, not one of 0xxxxxxx, "
                 "110xxxxx, 1110xxxx, 11110xxx\n",
                 fd, offset, linenum, onebyte);
        return -1;
      }
      if (state == 0) {
        if (onebyte == 0x0a)
          ++linenum;
        break;
      } else {
        d[0] = onebyte;
      }
      break;
    case 1:
      if ((onebyte & 0xc0) == 0x80) {
        d[1] = onebyte;
        value = (0x1c & d[0] << 6) | (((0x03 & d[0]) << 6) | (0x3f & d[1]));
        if (value < 0x80 || value > 0x7ff) {
          snprintf(errmsg, sz,
                   "Error deleted in: fd %d, "
                   "Invalid unicode sequence at file offset %lu around line "
                   "%d, 2-byte sequence 0x%02X%02X value U+%04X invalid, range "
                   "out of U+0080 and U+07FF\n",
                   fd, offset, linenum, d[0], d[1], value);
          return -1;
        }
        state = 0;
      } else {
        snprintf(errmsg, sz,
                 "Error deleted in: fd %d, "
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, 2-byte sequence 0x%02X%02X 2nd byte malformed, not "
                 "110xxxxx-10xxxxxx\n",
                 fd, offset, linenum, d[0], onebyte);
        return -1;
      }
      break;

    case 2:
      if ((onebyte & 0xc0) == 0x80) {
        d[1] = onebyte;
        state = 22;
      } else {
        snprintf(errmsg, sz,
                 "Error deleted in: fd %d, "
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, 3-byte sequence 0x%02X%02Xxx 2nd byte malformed, not "
                 "1110xxxx-10xxxxxx-xxxxxxxx\n",
                 fd, offset, linenum, d[0], onebyte);
        return -1;
      }
      break;

    case 3:
      if ((onebyte & 0xc0) == 0x80) {
        d[1] = onebyte;
        state = 33;
      } else {
        snprintf(errmsg, sz,
                 "Error deleted in: fd %d, "
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, 4-byte sequence 0x%02X%02Xxxxx 2nd byte malformed, not "
                 "11110xxx-10xxxxxx-xxxxxxxx-xxxxxxxx\n",
                 fd, offset, linenum, d[0], onebyte);
        return -1;
      }
      break;

    case 33:
      if ((onebyte & 0xc0) == 0x80) {
        d[2] = onebyte;
        state = 333;
      } else {
        snprintf(errmsg, sz,
                 "Error deleted in: fd %d, "
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, 4-byte sequence 0x%02X%02X%02Xxx 3rd byte malformed, not "
                 "11110xxx-10xxxxxx-10xxxxxxx-xxxxxxxx\n",
                 fd, offset, linenum, d[0], d[1], onebyte);
        return -1;
      }
      break;

    case 22:
      if ((onebyte & 0xc0) == 0x80) {
        d[2] = onebyte;
        value =
            ((0x000f & d[0]) << 12) | ((0x003f & d[1]) << 6) | (0x3f & d[2]);
        if (value < 0x0800 || value > 0x0ffff) {
          snprintf(errmsg, sz,
                   "Error deleted in: fd %d, "
                   "Invalid unicode sequence at file offset %lu around line "
                   "%d, 3-byte sequence 0x%02X%02X%02X value U+%04X "
                   "invalid, range "
                   "out of U+0800 and U+FFFF\n",
                   fd, offset, linenum, d[0], d[1], d[2], value);
          return -1;
        }
        state = 0;
      } else {
        snprintf(errmsg, sz,
                 "Error deleted in: fd %d, "
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, 3-byte sequence 0x%02X%02X%02X 3rd byte malformed, not "
                 "11110xxx-10xxxxxx-10xxxxxxx\n",
                 fd, offset, linenum, d[0], d[1], onebyte);
        return -1;
      }
      break;
    case 333:
      if ((onebyte & 0xc0) == 0x80) {
        d[3] = onebyte;
        value = ((0x0007 & d[0]) << 18) | ((0x003f & d[1]) << 12) |
                ((0x003f & d[2]) << 6) | (0x3f & d[3]);
        if (value < 0x010000 || value > 0x010ffff) {
          snprintf(errmsg, sz,
                   "Error deleted in: fd %d, "
                   "Invalid unicode sequence at file offset %lu around line "
                   "%d, 4-byte sequence 0x%02X%02X%02X%02X value U+%05X "
                   "invalid, range "
                   "out of U+10000 and U+10FFFF\n",
                   fd, offset, linenum, d[0], d[1], d[2], d[3], value);
          return -1;
        }
        state = 0;
      } else {
        snprintf(errmsg, sz,
                 "Error deleted in: fd %d, "
                 "Invalid unicode sequence at file offset %lu around line "
                 "%d, 4-byte sequence 0x%02X%02X%02X%02Xx 4th byte "
                 "malformed, not "
                 "11110xxx-10xxxxxx-10xxxxxxx-10xxxxxx\n",
                 fd, offset, linenum, d[0], d[1], d[2], onebyte);
        return -1;
      }
      break;
    default:
      snprintf(errmsg, sz,
               "Error deleted in: fd %d, "
               "Invalid unicode sequence at file offset %lu around line "
               "%d, parser in unknown state %d, byte read 0x%02X\n",
               fd, offset, linenum, state, onebyte);
      return -1;
    }
    ++offset;
    c = read(fd, &onebyte, 1);
  }
  if (state != 0) {
    snprintf(errmsg, sz,
             "Error deleted in: fd %d, "
             "Excepted End of File detected at file offset %lu around line "
             "%d, parser in state %d, byte read 0x%02X\n",
             fd, offset, linenum, state, onebyte);
    return -1;
  }
  return 0;
}
int cp1047scan(char *errmsg, size_t sz, int fd) {
  static const unsigned char _tab_e[256] __attribute__((aligned(8))) = {
      1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
  };
  char buffer[4096];
  int b = read(fd, buffer, 4096);
  while (b > 0) {
    unsigned long bytes = b;
    char *str = buffer;
    unsigned long code_out;

   __asm volatile(" trte %1,%3,0\n"
                  " jo *-4\n"
                  : "+NR:r3"(bytes), "+NR:r2"(str), "+r"(bytes), "+r"(code_out)
                  : "NR:r1"(_tab_e)
                  :);
    if ((str - buffer) != b) {
      snprintf(errmsg, sz, "Character not belong to CP 1047 found");
      return -1;
    }
    b = read(fd, buffer, 4096);
  }
  return 0;
}
int filescan(char *errmsg, size_t sz, int fd) {
  __convertmode cm(_CVTSTATE_OFF);
  char utf8msg[1024];
  char ebcdicmsg[1024];
  off_t original, off; 
  int result;
  original = lseek(fd, 0, SEEK_CUR);
  if (-1 == original) {
    errstring(errmsg, sz, errno, "lseek error on fd %d", fd);
    return -1;
  }
  int rc = lseek(fd, 0, SEEK_SET);
  if (-1 == original) {
    errstring(errmsg, sz, errno, "lseek error on fd %d", fd);
    return -1;
  }

  result = utf8scan(utf8msg, 1024, fd);
  if (result == 0) {
    result = 819;
    goto quit;
  }

  off = lseek(fd, original, SEEK_SET);
  if (-1 == off) {
    errstring(errmsg, sz, errno, "lseek error on fd %d", fd);
    return -1;
  }
  result = cp1047scan(ebcdicmsg, 1024, fd);
  if (result == 0) {
    result = 1047;
    goto quit;
  }
  snprintf(errmsg, sz, "unicode: %s, ebcidc-1047: %s", utf8msg, ebcdicmsg);
  result = 65535;
quit:
  off = lseek(fd, original, SEEK_SET);
  if (-1 == off) {
    errstring(errmsg, sz, errno, "lseek error on fd %d", fd);
    return -1;
  }
  return result;
}
