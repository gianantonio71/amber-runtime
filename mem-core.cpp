#include "lib.h"

#include <string.h>
#include <set>


char *align_16(char *ptr)
{
  char *al_ptr = (char *) (((long long)(ptr+15)) & ~15L);
  return al_ptr;
}

const uint32 HEAP_SIZE = 1536 * 1024 * 1024;

static char fake_heap_buff[HEAP_SIZE+16];

static char *fake_heap_top = align_16(fake_heap_buff);
static uint32 fake_heap_used = 0;


void *new_mem_block(uint32 nblocks16)
{
  uint32 block_size = 16 * nblocks16;
  char *mem_block = fake_heap_top;
  fake_heap_used += block_size;
  if (fake_heap_used > HEAP_SIZE)
    halt;
  fake_heap_top += block_size;
  return mem_block;
}

void free_mem_block(void *ptr, uint32 nblocks16)
{
  throw;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const int OBJ_POOL_SIZE = 32; //256 * 1024;

static void *obj_pool[OBJ_POOL_SIZE];

// 16 32 (48) 64 128 256 512 1024 2048 4096
//  1  2 ( 3)  4   8  16  32   64  128  256
//  0  1       2   3   4   5    6    7    8

uint32 log2_ceiling(uint32 _n)
{
  assert(_n > 0);

  uint32 idx = 0;
  uint32 n = _n;

  uint32 m = n >> 16;
  if (m != 0)
  {
    // At least one of the 16 most significant bits is nonzero
    idx += 16;
    n = m;
  }

  m = n >> 8;
  if (m != 0)
  {
    idx += 8;
    n = m;
  }

  m = n >> 4;
  if (m != 0)
  {
    idx += 4;
    n = m;
  }

  m = n >> 2;
  if (m != 0)
  {
    idx += 2;
    n = m;
  }

  m = n >> 1;
  if (m != 0)
  {
    idx += 1;
    n = m;
  }

  assert(n == 1);
  assert(_n >= (1 << idx) && _n < (1 << (idx+1)));

  return _n == (1 << idx) ? idx : idx + 1;
}

void *new_pooled_mem_block(uint32 nblocks16)
{
  uint32 l2c = log2_ceiling(nblocks16);
  void *head = obj_pool[l2c];

  if (head == NULL)
    return new_mem_block(1 << l2c);

  void *next = * (void **) head;
  obj_pool[l2c] = next;

  return head;
}

void *new_pooled_mem_block(uint32 nblocks16_requested, uint32 &nblocks16_returned)
{
  uint32 l2c = log2_ceiling(nblocks16_requested);
  void *head = obj_pool[l2c];

  uint32 actual_size = 1 << l2c;
  nblocks16_returned = actual_size;

  if (head == NULL)
    return new_mem_block(actual_size);

  void *next = * (void **) head;
  obj_pool[l2c] = next;

  return head;
}

void free_pooled_mem_block(void *ptr, uint32 nblocks16)
{
  memset(ptr, 0xFF, 16*nblocks16); //## IS THIS NECESSARY IN RELEASE MODE?

  uint32 l2c = log2_ceiling(nblocks16);
  void *tail = obj_pool[l2c];
  * (void **) ptr = tail;
  obj_pool[l2c] = ptr;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG

uint32 num_of_live_objs;
uint32 max_num_of_live_objs;
uint32 total_num_of_objs;

uint32 live_mem_usage;
uint32 max_live_mem_usage;
uint32 total_mem_requested;

std::set<void *> live_objs;

void inc_live_obj_count(uint32 nblocks16)
{
  num_of_live_objs++;
  total_num_of_objs++;
  if (num_of_live_objs > max_num_of_live_objs)
    max_num_of_live_objs = num_of_live_objs;

  live_mem_usage += nblocks16;
  total_mem_requested += nblocks16;
  if (live_mem_usage > max_live_mem_usage)
    max_live_mem_usage = live_mem_usage;
}

void dec_live_obj_count(uint32 nblocks16)
{
  num_of_live_objs--;
  live_mem_usage -= nblocks16;
}

uint32 get_live_objs_count()
{
  return num_of_live_objs;
}

uint32 get_max_live_objs_count()
{
  return max_num_of_live_objs;
}

uint32 get_total_objs_count()
{
  return total_num_of_objs;
}

uint32 get_live_mem_usage()
{
  return live_mem_usage;
}

uint32 get_max_live_mem_usage()
{
  return max_live_mem_usage;
}

uint32 get_total_mem_requested()
{
  return total_mem_requested;
}

#include <cstdio>

void print_all_live_objs()
{
  if (!live_objs.empty())
  {
    std::fprintf(stderr, "Live objects:\n");
    for (std::set<void*>::iterator it = live_objs.begin() ; it != live_objs.end() ; it++)
    {
      void *ptr = *it;
      std::printf("  %8llx\n", (unsigned long long)ptr);
    }
    std::fflush(stdout);
  }
}

bool is_alive(void *obj)
{
  return live_objs.find(obj) != live_objs.end();
}

#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void *new_obj(uint32 nblocks16)
{
  void *mem_block = new_pooled_mem_block(nblocks16);

#ifndef NDEBUG
  inc_live_obj_count(nblocks16); //## THE SIZE IF THE WRONG ONE, BUT IT IS THE SAME THAT IS REPORTED BACK TO free_obj
  live_objs.insert(mem_block);
#endif

  return mem_block;
}

void *new_obj(uint32 nblocks16_requested, uint32 &nblocks16_returned)
{
  void *mem_block = new_pooled_mem_block(nblocks16_requested, nblocks16_returned);

#ifndef NDEBUG
  inc_live_obj_count(nblocks16_returned);
  live_objs.insert(mem_block);
#endif

  return mem_block;
}

void free_obj(void *ptr, uint32 nblocks16)
{
#ifndef NDEBUG
  assert(num_of_live_objs > 0);
  assert(is_alive(ptr));

  dec_live_obj_count(nblocks16);
  live_objs.erase(live_objs.find(ptr));
#endif

  free_pooled_mem_block(ptr, nblocks16);
}
