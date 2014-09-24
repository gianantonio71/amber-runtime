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
  return nblocks16(sizeof(Set) + (size - 1) * sizeof(Obj));
}

int seq_obj_mem_size(int lenght)
{
  return nblocks16(sizeof(Seq) + (lenght - 1) * sizeof(Obj));
}

int map_obj_mem_size(int size)
{
  return nblocks16(sizeof(Map) + (2 * size - 1) * sizeof(Obj));
}

int tag_obj_mem_size()
{
  return nblocks16(sizeof(TagObj));
}

////////////////////////////////////////////////////////////////////////////////

Set *new_set(int size)
{
  assert(size > 0);

  Set *set = (Set *) new_obj(set_obj_mem_size(size));
  set->ref_count = 1;
  set->size = size;
  return set;
}

Seq *new_seq(int length)
{
  assert(length > 0);
  
  Seq *seq = (Seq *) new_obj(seq_obj_mem_size(length));
  seq->ref_count = 1;
  seq->length = length;
  return seq;
}

Map *new_map(int size)
{
  assert(size > 0);

  Map *map = (Map *) new_obj(map_obj_mem_size(size));
  map->ref_count = 1;
  map->size = size;
  return map;
}

TagObj *new_tag_obj()
{
  TagObj *tag_obj = (TagObj *) new_obj(tag_obj_mem_size());
  tag_obj->ref_count = 1;
  return tag_obj;
}

////////////////////////////////////////////////////////////////////////////////

Set *shrink_set(Set *set, int new_size)
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

  Set *new_set = ::new_set(new_size);
  memcpy(new_set->elems, set->elems, new_size * sizeof(Obj));
  free_obj(set, mem_size);

  return new_set;
}

////////////////////////////////////////////////////////////////////////////////

Obj *new_obj_array(int size)
{
  return (Obj *) new_obj(nblocks16(size * sizeof(Obj)));
}

void delete_obj_array(Obj *buffer, int size)
{
  free_obj(buffer, nblocks16(size * sizeof(Obj)));
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

static void delete_obj(Obj);

static void release(Obj *objs, int count, Obj *queue, int &queue_start, int &queue_size)
{
  for (int i=0 ; i < count ; i++)
  {
    Obj obj = objs[i];
    if (is_ref_obj(obj))
    {
      RefObj *ptr = get_ref_obj_ptr(obj);
      
      int ref_count = ptr->ref_count;
      assert(ref_count > 0);
      
      if (ref_count == 1)
      {
        assert(queue_size <= MAX_QUEUE_SIZE);
        
        if (queue_size == MAX_QUEUE_SIZE)
        {
          int idx = queue_start % MAX_QUEUE_SIZE;
          Obj first_obj = queue[idx];
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

static void delete_obj(Obj obj, Obj *queue, int &queue_start, int &queue_size)
{
  assert(is_ref_obj(obj));
  
  switch (get_type_tag(obj))
  {
    case type_tag_set:
    {
      Set *set = get_set_ptr(obj);
      int size = set->size;
      release(set->elems, size, queue, queue_start, queue_size);
      free_obj(set, set_obj_mem_size(size));
      break;
    }
    
    case type_tag_seq:
    {
      Seq *seq = get_seq_ptr(obj);
      int len = seq->length;
      release(seq->elems, len, queue, queue_start, queue_size);
      free_obj(seq, seq_obj_mem_size(len));
      break;    
    }
    
    case type_tag_map:
    {
      Map *map = get_map_ptr(obj);
      int size = map->size;
      release(map->buffer, 2*size, queue, queue_start, queue_size);
      free_obj(map, map_obj_mem_size(size));
      break;    
    }
    
    case type_tag_tag_obj:
    {
      TagObj *tag_obj = get_tag_obj_ptr(obj);
      release(&tag_obj->obj, 1, queue, queue_start, queue_size);
      free_obj(tag_obj, tag_obj_mem_size());
      break;    
    }
    
    default:
      assert(false);
      fail();
      throw;
  }
}

static void delete_obj(Obj obj)
{
  assert(is_ref_obj(obj));

  int queue_start = 0;
  int queue_size = 1;
  Obj queue[MAX_QUEUE_SIZE];
  queue[0] = obj;

  while (queue_size > 0)
  {
    Obj next_obj = queue[queue_start % MAX_QUEUE_SIZE];
    queue_size--;
    queue_start++;
    
    delete_obj(next_obj, queue, queue_start, queue_size);
  }  
}

////////////////////////////////////////////////////////////////////////////////

void add_ref(Obj obj)
{
#ifndef NOGC
  if (is_ref_obj(obj))
  {
    RefObj *ptr = get_ref_obj_ptr(obj);
    assert(ptr->ref_count > 0);
    ptr->ref_count++;
  }
#endif
}

void release(Obj obj)
{
#ifndef NOGC
  if (is_ref_obj(obj))
  {
    RefObj *ptr = get_ref_obj_ptr(obj);
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

void mult_add_ref(Obj obj, int count)
{
  assert(count > 0);
  
  if (is_ref_obj(obj))
  {
    RefObj *ptr = get_ref_obj_ptr(obj);
    assert(ptr->ref_count > 0);
    ptr->ref_count += count;
  }
}

////////////////////////////////////////////////////////////////////////////////

void vec_add_ref(Obj *objs, int len)
{
  for (int i=0 ; i < len ; i++)
    add_ref(objs[i]);
}

void vec_release(Obj *objs, int len)
{
  for (int i=0 ; i < len ; i++)
    release(objs[i]);
}

