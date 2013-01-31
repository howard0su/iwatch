#include "contiki.h"

/*
 * This implement the digit watch
 * Wake up every 1 second and update the watch
 * If in 10 minutes, no key or other things
 * if get system key in non-suspend state, post event to system.
 */

