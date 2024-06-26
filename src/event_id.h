#ifndef PROC_ASM_EVENT_ID_H
#define PROC_ASM_EVENT_ID_H
#include "engine/events.h"

namespace EventId {

    constexpr int REGISTER_CHANGED = 0;
    constexpr int TICKS_CHANGED = 1;
    constexpr int IN_PORT_CHANGED = 2;
    constexpr int OUT_PORT_CHANGED = 3;
    constexpr int RUNNING_CHANGED = 4;

}
#endif
