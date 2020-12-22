#include "rtos_impl.h"

int K_gettid(void)
{
    return _gettask()->tid;
}
