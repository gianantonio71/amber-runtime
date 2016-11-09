#include "lib.h"

#include <cstring>


using namespace std;


uint32 set_obj_mem_size(uint32 size)
{
  return sizeof(SET_OBJ) + (size - 1) * sizeof(OBJ);
}

uint32 seq_obj_mem_size(uint32 capacity)
{
  return sizeof(SEQ_OBJ) + (capacity - 1) * sizeof(OBJ);
}

uint32 map_obj_mem_size(uint32 size)
{
  return sizeof(MAP_OBJ) + (2 * size - 1) * sizeof(OBJ);
}

uint32 tag_obj_mem_size()
{
  return sizeof(TAG_OBJ);
}

////////////////////////////////////////////////////////////////////////////////

uint32 seq_capacity(uint32 byte_size)
{
  return (byte_size - sizeof(SEQ_OBJ)) / sizeof(OBJ) + 1;
}

////////////////////////////////////////////////////////////////////////////////

SEQ_OBJ *new_seq(uint32 length)
{
  assert(length > 0);

  uint32 actual_byte_size;
  SEQ_OBJ *seq = (SEQ_OBJ *) new_obj(seq_obj_mem_size(length), actual_byte_size);
  seq->ref_obj.ref_count = 1;
  seq->capacity = seq_capacity(actual_byte_size);
  seq->size = length;
  return seq;
}

SET_OBJ *new_set(uint32 size)
{
  SET_OBJ *set = (SET_OBJ *) new_obj(set_obj_mem_size(size));
  set->ref_obj.ref_count = 1;
  set->size = size;
  return set;
}

MAP_OBJ *new_map(uint32 size)
{
  assert(size > 0);

  MAP_OBJ *map = (MAP_OBJ *) new_obj(map_obj_mem_size(size));
  map->ref_obj.ref_count = 1;
  map->size = size;
  return map;
}

TAG_OBJ *new_tag_obj()
{
  TAG_OBJ *tag_obj = (TAG_OBJ *) new_obj(tag_obj_mem_size());
  tag_obj->ref_obj.ref_count = 1;
  tag_obj->unused_field = 0;
  return tag_obj;
}

////////////////////////////////////////////////////////////////////////////////

SET_OBJ *shrink_set(SET_OBJ *set, uint32 new_size)
{
  assert(new_size < set->size);
  assert(set->ref_obj.ref_count == 1);

  uint32 size = set->size;
  uint32 mem_size = set_obj_mem_size(size);
  uint32 new_mem_size = set_obj_mem_size(new_size);

  if (mem_size == new_mem_size)
  {
    // If the memory footprint is exactly the same, I can safely reuse
    // the same object without causing problems in the memory allocator.
    set->size = new_size;
    return set;
  }

  SET_OBJ *new_set = ::new_set(new_size);
  memcpy(new_set->buffer, set->buffer, new_size * sizeof(OBJ));
  free_obj(set, mem_size);

  return new_set;
}

////////////////////////////////////////////////////////////////////////////////

OBJ *new_obj_array(uint32 size)
{
  return (OBJ *) new_obj(size * sizeof(OBJ));
}

void delete_obj_array(OBJ *buffer, uint32 size)
{
  free_obj(buffer, size * sizeof(OBJ));
}

uint32 *new_uint32_array(uint32 size)
{
  return (uint32 *) new_obj(size * sizeof(uint32));
}

void delete_uint32_array(uint32 *buffer, uint32 size)
{
  free_obj(buffer, size * sizeof(uint32));
}

void **new_ptr_array(uint32 size)
{
  return (void **) new_obj(size * sizeof(void *));
}

void delete_ptr_array(void **buffer, uint32 size)
{
  free_obj(buffer, size * sizeof(void *));
}

////////////////////////////////////////////////////////////////////////////////

const uint32 MAX_QUEUE_SIZE = 1024;

static void delete_obj(OBJ);

static void release(OBJ *objs, uint32 count, OBJ *queue, uint32 &queue_start, uint32 &queue_size)
{
  for (uint32 i=0 ; i < count ; i++)
  {
    OBJ obj = objs[i];
    if (is_gc_obj(obj))
    {
      REF_OBJ *ptr = get_ref_obj_ptr(obj);

      uint32 ref_count = ptr->ref_count;
      assert(ref_count > 0);

      if (ref_count == 1)
      {
        assert(queue_size <= MAX_QUEUE_SIZE);

        if (queue_size == MAX_QUEUE_SIZE)
        {
          uint32 idx = queue_start % MAX_QUEUE_SIZE;
          OBJ first_obj = queue[idx];
          queue[idx] = obj;
          queue_start++;
          delete_obj(first_obj);
        }
        else
        {
          uint32 idx = (queue_start + queue_size) % MAX_QUEUE_SIZE;
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

static void delete_obj(OBJ obj, OBJ *queue, uint32 &queue_start, uint32 &queue_size)
{
  assert(is_gc_obj(obj));

  REF_OBJ *ref_obj = get_ref_obj_ptr(obj);

  switch (get_ref_obj_type(obj))
  {
    case TYPE_SEQUENCE:
    {
      SEQ_OBJ *seq = (SEQ_OBJ *) ref_obj;
      release(seq->buffer, seq->size, queue, queue_start, queue_size);
      free_obj(seq, seq_obj_mem_size(seq->capacity));
      break;
    }

    case TYPE_SET:
    {
      SET_OBJ *set = (SET_OBJ *) ref_obj;
      uint32 size = set->size;
      release(set->buffer, size, queue, queue_start, queue_size);
      free_obj(set, set_obj_mem_size(size));
      break;
    }

    case TYPE_MAP:
    {
      MAP_OBJ *map = (MAP_OBJ *) ref_obj;
      uint32 size = map->size;
      release(map->buffer, 2*size, queue, queue_start, queue_size);
      free_obj(map, map_obj_mem_size(size));
      break;
    }

    case TYPE_TAG_OBJ:
    {
      TAG_OBJ *tag_obj = (TAG_OBJ *) ref_obj;
      release(&tag_obj->obj, 1, queue, queue_start, queue_size);
      free_obj(tag_obj, tag_obj_mem_size());
      break;
    }

    default:
      internal_fail();
  }
}

static void delete_obj(OBJ obj)
{
  assert(is_gc_obj(obj));

  uint32 queue_start = 0;
  uint32 queue_size = 1;
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
  if (is_gc_obj(obj))
    add_ref(get_ref_obj_ptr(obj));
#endif
}

void release(OBJ obj)
{
#ifndef NOGC
  if (is_gc_obj(obj))
  {
    REF_OBJ *ptr = get_ref_obj_ptr(obj);
    uint32 ref_count = ptr->ref_count;
    assert(ref_count > 0);
    if (ref_count == 1)
      delete_obj(obj);
    else
      ptr->ref_count = ref_count - 1;
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////

void vec_add_ref(OBJ *objs, uint32 len)
{
  for (uint32 i=0 ; i < len ; i++)
    add_ref(objs[i]);
}

void vec_release(OBJ *objs, uint32 len)
{
  for (uint32 i=0 ; i < len ; i++)
    release(objs[i]);
}
