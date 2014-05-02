#include "os_interface.h"

#include "windows.h"

int get_tick_count()
{
  return GetTickCount();
}
