/**********************************************************\
 *                                                        *
 *                     Base Type                          *
 *                                                        *
\**********************************************************/
typedef unsigned       char      u8;
typedef unsigned       short     u16;
typedef unsigned       int       u32;
typedef unsigned       long      u64;

//double,float
#define  SU_TYPE_STR_L1B  0
#define  SU_TYPE_STR_L2B  1
#define  SU_TYPE_STR_L3B  2
#define  SU_TYPE_STR_L4B  3
struct l1b
{
  u8 flag:2;
  u8 len:6;
  u8 data[0];
};
struct l2b
{
  u16 flag:2;
  u16 len:14;
  u8  data[0];
};
struct l3b
{
  u16 flag:2;
  u16 len:14;
  u8  lenhb;
  u8  data[0];
};
struct l4b
{
  u32 flag:2;
  u32 len:30;
  u8  data[0];
};
union string_t
{
  struct l1b l1;
  struct l2b l2;
  struct l3b l3;
  struct l4b l4;
};

/**********************************************************\
 *                                                        *
 *                     Complex Type                       *
 *                                                        *
\**********************************************************/
union array_t
{
  struct l1b l1;
  struct l2b l2;
  struct l3b l3;
  struct l4b l4;
};

union map_t
{
  struct l1b l1;
  struct l2b l2;
  struct l3b l3;
  struct l4b l4;
};

struct struct_t
{
  u8 nr;
  u8 data[0];
};
