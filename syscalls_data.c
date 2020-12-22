#include "rtos_impl.h"

int K_gettid(void) {
    return g_running;
}
