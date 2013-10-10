#ifndef SHAPING_HPP
#define SHAPING_HPP

#include <node.h>

void InitShaping(v8::Handle<v8::Object> target);
v8::Handle<v8::Value> Shaping(const v8::Arguments& args);

#endif
