#pragma once
#include <iostream>
#include <stdio.h>

static inline void Warning(const char* str, ...) {
    std::cout << "warning " << str << std::endl;
};
static inline void Error(const char* str, ... ) {
    std::cout <<  "Error " <<  str << std::endl;
};