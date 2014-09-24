#include "lib.h"
#include "generated.h"


bool inline_eq(Obj obj1, Obj obj2)
{
  assert(is_inline_obj(obj2));
  return obj1 == obj2;
}

bool are_eq(Obj obj1, Obj obj2)
{
  return comp_objs(obj1, obj2) == 0;
}

bool is_out_of_range(SetIter &it)
{
  return it.idx >= it.size;
}

bool is_out_of_range(SeqIter &it)
{
  return it.idx >= it.len;
}

bool is_out_of_range(MapIter &it)
{
  return it.idx >= it.size;
}

bool has_elem(Obj set, Obj elem)
{
  Set *s = get_set_ptr(set);
  if (s == 0)
    return false;
  int idx = find_obj(s->elems, s->size, elem);
  return idx != -1;
}

////////////////////////////////////////////////////////////////////////////////

int get_int_val(Obj obj)
{
  assert(is_int(obj));
  return obj / 2;
}

int get_set_size(Obj set)
{
  if (set == empty_set)
    return 0;
  return get_set_ptr(set)->size;
}

int get_seq_len(Obj seq)
{
  if (seq == empty_seq)
    return 0;
  return get_seq_ptr(seq)->length;
}

int get_map_size(Obj map)
{
  if (map == empty_map)
    return 0;
  return get_map_ptr(map)->size;
}

int unique_int()
{
  static int next_val = 0;
  return next_val++;
}

////////////////////////////////////////////////////////////////////////////////

Obj to_obj(bool b)
{
  // assert((int(b) == 0 && b == false) || (int(b) == 1 && b == true));
  // assert(((32 >> int(b)) | 1) == (b ? generated::S_true : generated::S_false));
  // return (32 >> int(ib)) | 1;
  return b ? generated::S_true : generated::S_false;  // Surprisingly, this seems to be faster...
}

Obj to_obj(int n)
{
  assert((2 * n) / 2 == n);
  return 2 * n;
}

Obj obj_neg(Obj obj)
{
  assert(obj == generated::S_true || obj == generated::S_false);
  return obj == generated::S_true ? generated::S_false : generated::S_true;
  // assert((obj ^ 0x30) == (obj == generated::S_true ? generated::S_false : generated::S_true));
  // return obj ^ 0x30;
}

Obj at(Obj seq, int idx)
{
  Seq *s = get_seq_ptr(seq);
  
  if (idx >= s->length)
    fail();
  
  return s->elems[idx];
}

Obj get_tag(Obj obj)
{
  return get_tag_obj_ptr(obj)->tag;
}

Obj get_inner_obj(Obj obj)
{
  return get_tag_obj_ptr(obj)->obj;
}

Obj get_curr_obj(SetIter &it)
{
  assert(it.idx < it.set->size);
  return it.set->elems[it.idx];
}

Obj get_curr_obj(SeqIter &it)
{
  assert(it.idx < it.seq->length);
  return it.seq->elems[it.idx];
}

Obj get_curr_key(MapIter &it)
{
  assert(it.idx < it.map->size);
  return it.map->buffer[it.idx];
}

Obj get_curr_value(MapIter &it)
{
  assert(it.idx < it.map->size);
  Map *map = it.map;
  return map->buffer[it.idx+map->size];
}