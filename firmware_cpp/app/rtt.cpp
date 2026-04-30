#include "rtt.hpp"
#include "SEGGER_RTT.h"
#include <cstdarg>
#include <cstdio>

namespace RTT {
    void init() {
        SEGGER_RTT_Init();
    }

    void printf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char buf[128];
        vsnprintf(buf, sizeof(buf), fmt, args);
        SEGGER_RTT_WriteString(0, buf);
        va_end(args);
    }
}
