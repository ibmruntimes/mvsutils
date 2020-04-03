/*
 * Licensed Materials - Property of IBM
 * (C) Copyright IBM Corp. 2019. All Rights Reserved.
 * US Government Users Restricted Rights - Use, duplication or disclosure
 * restricted by GSA ADP Schedule Contract with IBM Corp.
 */
#ifndef __MVSUTILS_H
#define __MVSUTILS_H
#include <_Nascii.h>
#include <napi.h>

#ifdef __cplusplus
extern "C" {
#endif
void __con_print(const char *);
char *errstring(char *, unsigned long, int, const char *, ...);
int filescan(char *message, size_t sz, int fd);

#ifdef __cplusplus
}
class __ae_runmode {
  int mode;

public:
  __ae_runmode(int new_mode) { mode = __ae_thread_swapmode(new_mode); }
  ~__ae_runmode() { __ae_thread_swapmode(mode); }
};

class __convertmode {
  int convert_state;
  bool restore;

public:
  __convertmode(int new_value) : restore(false) {
    convert_state = __ae_autoconvert_state(_CVTSTATE_QUERY);
    if (new_value != convert_state) {
      __ae_autoconvert_state(new_value);
      restore = true;
    }
  }
  ~__convertmode() {
    if (restore) {
      __ae_autoconvert_state(convert_state);
    }
  }
};

#endif
#endif
