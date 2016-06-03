#include "lib.h"

#include <cstring>


using namespace std;

// Native object types:
//   0001  1    Symbol
//   0011  3    Set
//   0101  5    Seq
//   0111  7    Map
//   1001  9    Tagged Object


int set_obj_mem_size(int size)
{
  return nblocks16(sizeof(SET_OBJ) + (size - 1) * sizeof(OBJ));
}

int full_seq_obj_mem_size(int capacity)
{
  return nblocks16(sizeof(FULL_SEQ_OBJ) + (capacity - 1) * sizeof(OBJ));
}

int ref_seq_obj_mem_size()
{
  return nblocks16(sizeof(REF_SEQ_OBJ));
}

int map_obj_mem_size(int size)
{
  return nblocks16(sizeof(MAP_OBJ) + (2 * size - 1) * sizeof(OBJ));
}

int tag_obj_mem_size()
{
  return nblocks16(sizeof(TAG_OBJ));
}

int float_obj_mem_size()
{
  return nblocks16(sizeof(FLOAT));
}

////////////////////////////////////////////////////////////////////////////////

unsigned int full_seq_capacity(int nblocks16)
{
  return (16 * nblocks16 - sizeof(FULL_SEQ_OBJ)) / sizeof(OBJ) + 1;
}

////////////////////////////////////////////////////////////////////////////////

SET_OBJ *new_set(int size)
{
  assert(size > 0);

  SET_OBJ *set = (SET_OBJ *) new_obj(set_obj_mem_size(size));
  set->ref_count = 1;
  set->size = size;
  return set;
}

FULL_SEQ_OBJ *new_full_seq(int length)
{
  assert(length > 0);
  
  int actual_size_blocks;
  FULL_SEQ_OBJ *seq = (FULL_SEQ_OBJ *) new_obj(full_seq_obj_mem_size(length), actual_size_blocks);
  seq->ref_count = 1;
  seq->length = length;
  seq->elems = seq->buffer;
  seq->capacity = full_seq_capacity(actual_size_blocks);
  seq->used_capacity = length;
  return seq;
}

REF_SEQ_OBJ *new_ref_seq()
{
  REF_SEQ_OBJ *seq = (REF_SEQ_OBJ *) new_obj(ref_seq_obj_mem_size());
  seq->ref_count = 1;
  return seq;
}

REF_SEQ_OBJ *new_ref_seq(FULL_SEQ_OBJ *full_seq, OBJ *elems, int length)
{
  REF_SEQ_OBJ *seq  = new_ref_seq();
  seq->length   = length;
  seq->elems    = elems;
  seq->full_seq = full_seq;
  return seq;
}

MAP_OBJ *new_map(int size)
{
  assert(size > 0);

  MAP_OBJ *map = (MAP_OBJ *) new_obj(map_obj_mem_size(size));
  map->ref_count = 1;
  map->size = size;
  return map;
}

TAG_OBJ *new_tag_obj()
{
  TAG_OBJ *tag_obj = (TAG_OBJ *) new_obj(tag_obj_mem_size());
  tag_obj->ref_count = 1;
  return tag_obj;
}

FLOAT *new_float()
{
  FLOAT *float_obj = (FLOAT *) new_obj(tag_obj_mem_size());
  float_obj->ref_count = 1;
  return float_obj;
}

////////////////////////////////////////////////////////////////////////////////

SET_OBJ *shrink_set(SET_OBJ *set, int new_size)
{
  assert(new_size < set->size);
  assert(set->ref_count == 1);

  int size = set->size;
  int mem_size = set_obj_mem_size(size);
  int new_mem_size = set_obj_mem_size(new_size);

  if (mem_size == new_mem_size)
  {
    // If the memory footprint is exactly the same, I can safely reuse
    // the same object without causing problems in the memory allocator.
    set->size = new_size;
    return set;
  }

  SET_OBJ *new_set = ::new_set(new_size);
  memcpy(new_set->elems, set->elems, new_size * sizeof(OBJ));
  free_obj(set, mem_size);

  return new_set;
}

////////////////////////////////////////////////////////////////////////////////

OBJ *new_obj_array(int size)
{
  return (OBJ *) new_obj(nblocks16(size * sizeof(OBJ)));
}

void delete_obj_array(OBJ *buffer, int size)
{
  free_obj(buffer, nblocks16(size * sizeof(OBJ)));
}

int *new_int_array(int size)
{
  return (int *) new_obj(nblocks16(size * sizeof(int)));
}

void delete_int_array(int *buffer, int size)
{
  free_obj(buffer, nblocks16(size * sizeof(int)));
}

void **new_ptr_array(int size)
{
  return (void **) new_obj(nblocks16(size * sizeof(void *)));
}

void delete_ptr_array(void **buffer, int size)
{
  free_obj(buffer, nblocks16(size * sizeof(void *)));
}

////////////////////////////////////////////////////////////////////////////////

const int MAX_QUEUE_SIZE = 1024;

static void delete_obj(OBJ);

static void release(OBJ *objs, int count, OBJ *queue, int &queue_start, int &queue_size)
{
  for (int i=0 ; i < count ; i++)
  {
    OBJ obj = objs[i];
    if (is_ref_obj(obj))
    {
      REF_OBJ *ptr = get_ref_obj_ptr(obj);
      
      int ref_count = ptr->ref_count;
      assert(ref_count > 0);
      
      if (ref_count == 1)
      {
        assert(queue_size <= MAX_QUEUE_SIZE);
        
        if (queue_size == MAX_QUEUE_SIZE)
        {
          int idx = queue_start % MAX_QUEUE_SIZE;
          OBJ first_obj = queue[idx];
          queue[idx] = obj;
          queue_start++;
          delete_obj(first_obj);
        }
        else
        {
          int idx = (queue_start + queue_size) % MAX_QUEUE_SIZE;
          queue[idx] = obj;
          queue_size++;
        }       
      }
      else
      {
        ptr->ref_count = ref_count - 1;
      }
    }
  }
}

static void delete_obj(OBJ obj, OBJ *queue, int &queue_start, int &queue_size)
{
  assert(is_ref_obj(obj));

  switch (get_full_type_tag(obj))
  {
    case type_tag_set:
    {
      SET_OBJ *set = get_set_ptr(obj);
      int size = set->size;
      release(set->elems, size, queue, queue_start, queue_size);
      free_obj(set, set_obj_mem_size(size));
      break;
    }

    case type_tag_seq:
    {
      SEQ_OBJ *seq = get_seq_ptr(obj);
      if (is_full_seq(seq))
      {
        FULL_SEQ_OBJ *full_seq = (FULL_SEQ_OBJ *) seq;
        release(full_seq->elems, full_seq->used_capacity, queue, queue_start, queue_size);
        free_obj(full_seq, full_seq_obj_mem_size(full_seq->capacity));
      }
      else
      {
        REF_SEQ_OBJ *ref_seq = (REF_SEQ_OBJ *) seq;
        release(make_obj(ref_seq->full_seq));
        free_obj(ref_seq, ref_seq_obj_mem_size());
      }
      break;
    }

    case type_tag_map:
    {
      MAP_OBJ *map = get_map_ptr(obj);
      int size = map->size;
      release(map->buffer, 2*size, queue, queue_start, queue_size);
      free_obj(map, map_obj_mem_size(size));
      break;    
    }

    case type_tag_tag_obj:
    {
      TAG_OBJ *tag_obj = get_tag_obj_ptr(obj);
      release(&tag_obj->obj, 1, queue, queue_start, queue_size);
      free_obj(tag_obj, tag_obj_mem_size());
      break;    
    }

    case type_tag_float:
    {
      FLOAT *float_obj = get_float_ptr(obj);
      free_obj(float_obj, float_obj_mem_size());
      break;
    }

    default:
      internal_fail();
  }
}

static void delete_obj(OBJ obj)
{
  assert(is_ref_obj(obj));

  int queue_start = 0;
  int queue_size = 1;
  OBJ queue[MAX_QUEUE_SIZE];
  queue[0] = obj;

  while (queue_size > 0)
  {
    OBJ next_obj = queue[queue_start % MAX_QUEUE_SIZE];
    queue_size--;
    queue_start++;
    
    delete_obj(next_obj, queue, queue_start, queue_size);
  }  
}

////////////////////////////////////////////////////////////////////////////////

void add_ref(REF_OBJ *ptr)
{
#ifndef NOGC
  assert(ptr->ref_count > 0);
  ptr->ref_count++;
#endif
}

void add_ref(OBJ obj)
{
#ifndef NOGC
  if (is_ref_obj(obj))
    add_ref(get_ref_obj_ptr(obj));
#endif
}

void release(OBJ obj)
{
#ifndef NOGC
  if (is_ref_obj(obj))
  {
    REF_OBJ *ptr = get_ref_obj_ptr(obj);
    int ref_count = ptr->ref_count;
    assert(ref_count > 0);
    if (ref_count == 1)
      delete_obj(obj);
    else
      ptr->ref_count = ref_count - 1;
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////

void mult_add_ref(OBJ obj, int count)
{
  assert(count > 0);
  
  if (is_ref_obj(obj))
  {
    REF_OBJ *ptr = get_ref_obj_ptr(obj);
    assert(ptr->ref_count > 0);
    ptr->ref_count += count;
  }
}

////////////////////////////////////////////////////////////////////////////////

void vec_add_ref(OBJ *objs, int len)
{
  for (int i=0 ; i < len ; i++)
    add_ref(objs[i]);
}

void vec_release(OBJ *objs, int len)
{
  for (int i=0 ; i < len ; i++)
    release(objs[i]);
}
