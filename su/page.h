#ifndef __PAGE_H__
#define __PAGE_H__
#include "gingko.h"
/*定义页头*/
struct pageheader{
  /*定义页面压缩方式*/
#define SU_PH_COMP_NONE   0
#define SU_PH_COMP_LZO    1
#define SU_PH_COMP_ZLIB   2
#define SU_PH_COMP_SNAPPY 3
  u32 flag:1;    //页头定义
  u32 other;     //保留位
  u32 orig_len;  //原始长度
  u32 zip_len;   //压缩后的长度
  u32 crc32;     //校验和
};
struct pageindex{
  //暂时不做定义，需要索引部分进行补充
};
/*页面定义*/
typedef struct page{
  struct pageheader ph;
  struct pageindex *pi;
  /*void 类型的数据区，交给上层进行解析*/
  void *data;
} PAGE;

/*API*/
PAGE * create_default_page();
PAGE * create_page(struct pageheader *ph,struct pageindex*pi);
/**/
int add_line(void *line,PAGE *p);
int delete_line(int lineno,PAGE *p);
int get_line(int lineno,PAGE *p);
int pfree(PAGE *p);
#endif
