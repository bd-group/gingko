#include <stdlib.h>
#include "type.h"
//define the dataline
struct lineheader0
{
  u16 l1fld;   //L 1(num) F L D 
  u16 len;
  u32 llen;    // LLen
};
struct lineheader
{
  u16 l1fld;
  u16 pad;
  u32 offset;  //offset in current page
};
struct line
{
  struct lineheader *lh;  //line header
  void *ld;               //line data
};
struct lineheader lharray[]; //total 1 + lharray[0].len

