#include <stdio.h>
#include <limits.h>

// TODO: Change function prototype if necessary.
void logop(int res, int x, int y, int linenum) {
//    printf("[LOG] Computed %x at line %d\n", res, linenum);
  // TODO: Produce warning if the integer overflow is found.
  
  if(x != 0 && y != 0 && (x > res/y || y > res/x)) {
    printf("[WARNING] Integer overflow detected at line %d\n", linenum);
  }
}

