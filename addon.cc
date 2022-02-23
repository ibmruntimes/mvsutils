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
#include <fcntl.h>
#include <mutex>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#else
#error This addon is for ZOS only
#endif

#if ' ' == 0x40
#error Not compiled with -qascii
#endif

class ErrorAsyncWorker : public Napi::AsyncWorker {
public:
  ErrorAsyncWorker(const Napi::Function &callback, Napi::Error error)
      : Napi::AsyncWorker(callback), error(error) {}

protected:
  void Execute() override {}
  void OnOK() override {
    Napi::Env env = Env();

    Callback().MakeCallback(Receiver().Value(),
                            {error.Value(), env.Undefined()});
  }

  void OnError(const Napi::Error &e) override {
    Napi::Env env = Env();

    Callback().MakeCallback(Receiver().Value(), {e.Value(), env.Undefined()});
  }

private:
  Napi::Error error;
};

class MvsutilsGuessFileWorker : public Napi::AsyncWorker {

public:
  MvsutilsGuessFileWorker(const Napi::Function &callback, int filedesc,
                          // const char *filename, Napi::Object &object)
                          const char *filename)
      : Napi::AsyncWorker(callback), fd(filedesc), result(-1) {
    Napi::Env env = Env();
    strncpy(file, filename, 1024);
  }
  ~MvsutilsGuessFileWorker() {}

protected:
  void Execute() override {
    Napi::Env env = Env();
    result = filescan(message, 1024, fd);
  }
  void OnOK() override {
    Napi::Env env = Env();
    Napi::Object obj = Napi::Object::New(env);
    if (-1 == result) {
      obj.Set("error", message);
    } else {
      obj.Set("ccsid", result);
    }
    if (file[0] != 0) {
      obj.Set("file", file);
      close(fd);
    } else {
      obj.Set("fd", fd);
    }
    Callback().MakeCallback(Receiver().Value(), {obj});
  }
  void OnError(const Napi::Error &e) override {
    Napi::Env env = Env();
    Callback().MakeCallback(Receiver().Value(), {e.Value()});
  }

private:
  char file[1024];
  char message[1024];
  int fd;
  int result;
};

Napi::Number ConsoleSync(const Napi::CallbackInfo &info) {
  __ae_runmode ae(__AE_ASCII_MODE);
  Napi::Env env = info.Env();
  if (info.Length() < 1) {
    Napi::Error::New(env, "Need at least one string as argument")
        .ThrowAsJavaScriptException();
    return Napi::Number::New(env, -1);
  }

  std::stringstream message;
  for (int i = 0; i < info.Length(); ++i) {
    message << info[i].ToString().Utf8Value();
    if (i < (info.Length() - 1))
      message << " ";
  }
  __con_print(message.str().c_str());
  return Napi::Number::New(env, 0);
}
Napi::Value GetFileCcsid(const Napi::CallbackInfo &info) {
  __ae_runmode ae(__AE_ASCII_MODE);
  char message[1024];
  char filename[1024];
  Napi::Env env = info.Env();
  if (info.Length() < 1) {
    Napi::Error::New(env, "Need file name or file descriptor as argument")
        .ThrowAsJavaScriptException();
    return Napi::Number::New(env, -1);
  }
  Napi::Object res = Napi::Object::New(env);
  if (info[0].IsNumber()) {
    int fd = info[0].As<Napi::Number>();
    struct stat st;
    int rc, err;
    rc = fstat(fd, &st);
    if (rc != 0) {
      res.Set("error",
              errstring(message, 1024, errno, "fstat error on fd %d ", fd));
    } else {
      res.Set("text", st.st_tag.ft_txtflag);
      res.Set("ccsid", st.st_tag.ft_ccsid);
    }
  } else {
    const char *tmp = info[0].ToString().Utf8Value().c_str();
    strncpy(filename, tmp, 1024);
    struct stat st;
    int rc, err;
    rc = stat(filename, &st);
    if (rc != 0) {
      res.Set("error", errstring(message, 1024, errno, "stat error on file %s ",
                                 filename));
    } else {
      res.Set("text", st.st_tag.ft_txtflag);
      res.Set("ccsid", st.st_tag.ft_ccsid);
    }
  }
  return res;
}
Napi::Value SetFileCcsid(const Napi::CallbackInfo &info) {
  __ae_runmode ae(__AE_ASCII_MODE);
  char message[1024];
  char filename[1024];
  Napi::Env env = info.Env();
  if (info.Length() < 3) {
    Napi::Error::New(env, "First argument should be file descriptor, second "
                          "argument should is 0 or 1 for text bit, third "
                          "argument should be CCSID")
        .ThrowAsJavaScriptException();
    return Napi::Number::New(env, -1);
  }
  int fd = -1;
  if (info[0].IsNumber()) {
    fd = info[0].As<Napi::Number>();
  } else {
    const char *tmp = info[0].ToString().Utf8Value().c_str();
    strncpy(filename, tmp, 1024);
  }
  int text;
  int ccsid;
  if (!info[1].IsNumber()) {
    Napi::TypeError::New(env, "text bit should 0 or 1")
        .ThrowAsJavaScriptException();
    return Napi::Number::New(env, -1);
  } else {
    text = info[1].As<Napi::Number>();
    if (text != 0 && text != 1) {
      Napi::TypeError::New(env, "text bit should 0 or 1")
          .ThrowAsJavaScriptException();
      return Napi::Number::New(env, -1);
    }
  }
  if (!info[2].IsNumber()) {
    Napi::TypeError::New(env, "CCSID should be a number")
        .ThrowAsJavaScriptException();
    return Napi::Number::New(env, -1);
  } else {
    ccsid = info[2].As<Napi::Number>();
    if (ccsid > 65535 || ccsid < 0) {
      Napi::TypeError::New(env, "CCSID not valid").ThrowAsJavaScriptException();
      return Napi::Number::New(env, -1);
    }
  }
  attrib_t attr;
  memset(&attr, 0, sizeof(attr));
  attr.att_filetagchg = 1;
  attr.att_filetag.ft_ccsid = ccsid;
  if (text)
    attr.att_filetag.ft_txtflag = 1;
  Napi::Object res = Napi::Object::New(env);
  int rc;
  if (fd != -1) {
    rc = __fchattr(fd, &attr, sizeof(attr));

    if (-1 == rc) {
      res.Set("error", errstring(message, 1024, errno,
                                 "__fchattr error on fd %d, ", fd));
    } else {
      res.Set("rc", rc);
    }
  } else {
    rc = __lchattr(filename, &attr, sizeof(attr));

    if (-1 == rc) {
      res.Set("error", errstring(message, 1024, errno,
                                 "__lchattr error on file %s ", filename));
    } else {
      res.Set("rc", rc);
    }
  }
  return res;
}

Napi::Value GuessFileCcsid(const Napi::CallbackInfo &info) {
  __ae_runmode ae(__AE_ASCII_MODE);
  Napi::Env env = info.Env();
  Napi::Object res = Napi::Object::New(env);
  char message[1024];
  char filename[1024];
  int ccsid = 0;
  int fd = -1;
  if (info.Length() < 1) {
    Napi::Error::New(env, "Need file name or file descriptor as argument")
        .ThrowAsJavaScriptException();
    return Napi::Number::New(env, -1);
  }
  if (info[0].IsNumber()) {
    filename[0] = 0;
    fd = info[0].As<Napi::Number>();
    res.Set("fd", fd);
  } else {
    const char *tmp = info[0].ToString().Utf8Value().c_str();
    strncpy(filename, tmp, 1024);
    fd = open(filename, O_RDONLY);
    if (-1 == fd) {
      res.Set("error",
              errstring(message, 1024, errno, "open error on %s ", filename));
      return res;
    }
    res.Set("file", filename);
  }
  if (info.Length() > 1 && info[1].IsFunction()) {
    Napi::Function cb = info[1].As<Napi::Function>();
    (new MvsutilsGuessFileWorker(cb, fd, filename))->Queue();
  } else {
    int rc = filescan(message, 1024, fd);
    if (rc == -1) {
      res.Set("error", message);
    } else {
      res.Set("ccsid", rc);
    }
    if (filename[0] != 0)
      close(fd);
  }
  return res;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("SimpleConsoleMessage", Napi::Function::New(env, ConsoleSync));
  exports.Set("GetFileCcsid", Napi::Function::New(env, GetFileCcsid));
  exports.Set("SetFileCcsid", Napi::Function::New(env, SetFileCcsid));
  exports.Set("GuessFileCcsid", Napi::Function::New(env, GuessFileCcsid));
  return exports;
}
NODE_API_MODULE(mvsutils, Init)
