#include "gingko.h"
struct column
{
  u16  fld;      //field ID
  u16  flag;     //which ligth-weight codecs
  void *codec;   //refer to ligth-weight codecs
  void *fldstat; //refer to fild statistic area
};
