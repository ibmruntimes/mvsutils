/*
 * Licensed Materials - Property of IBM
 * (C) Copyright IBM Corp. 2019. All Rights Reserved.
 * US Government Users Restricted Rights - Use, duplication or disclosure
 * restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#if defined(__MVS__)
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

extern "C" char *errstring(char *buffer, size_t size, int err,
                           const char *format, ...) {
  char *org = buffer;
  if (format) {
    va_list ap;
    va_start(ap, format);
    int bytes = vsnprintf(buffer, size, format, ap);
    if (bytes > 0) {
      buffer += bytes;
      size -= bytes;
    }
    va_end(ap);
  }
  char message[1024];
  int rc = strerror_r(err, message, 1024);
  if (-1 == rc) {
    snprintf(buffer, size, "original errno: %d, strerror errno:%d", err, errno);
  } else {
    snprintf(buffer, size, "errno: %d, %s", err, message);
  }
  return org;
}
