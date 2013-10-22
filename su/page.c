#include "page.h"
#include "stdlib.h"

int init_pg_pool()
{
  PG_POOL = malloc(SU_PG_POOL_SIZE*sizeof(struct page));
  if(!PG_POOL){
    perror("can't init page pool");
    //return -1;
    exit(EXIT_FAILURE);
  }
  return 0;
}
struct page *build_page(void)
{
  struct page *p = malloc(sizeof(struct page));
  if(!p){
    perror("can't build page");
    exit(EXIT_FAILURE);
  }
  PG_CURERNT = p;
  return p;
}
struct page *get_cp()
{
  if(PG_CURERNT)return PG_CURERNT;
  else PG_CURERNT = build_page();
}
int main(void)
{
  struct page *page;
  page = malloc(sizeof(struct page));
  PG_CURERNT = page;
  if(!page)perror("malloc");
  return 0;
}
