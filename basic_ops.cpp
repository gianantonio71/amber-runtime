#include "lib.h"

#include <cstdlib>


bool inline_eq(OBJ obj1, OBJ obj2)
{
  assert(is_inline_obj(obj2) & !is_float(obj2));
  return are_shallow_eq(obj1, obj2);
}

bool are_eq(OBJ obj1, OBJ obj2)
{
  return comp_objs(obj1, obj2) == 0;
}

bool is_out_of_range(SET_ITER &it)
{
  return it.idx >= it.size;
}

bool is_out_of_range(SEQ_ITER &it)
{
  return it.idx >= it.len;
}

bool is_out_of_range(MAP_ITER &it)
{
  return it.idx >= it.size;
}

bool has_elem(OBJ set, OBJ elem)
{
  if (is_empty_set(set))
    return false;
  SET_OBJ *s = get_set_ptr(set);
  bool found;
  find_obj(s->buffer, s->size, elem, found);
  return found;
}

////////////////////////////////////////////////////////////////////////////////

int64 get_int_val(OBJ obj)
{
  fail_if_not(is_int(obj), "Not an integer");
  return get_int(obj);
}

uint32 get_set_size(OBJ set)
{
  fail_if_not(is_set(set), "Not a set");
  if (is_empty_set(set))
    return 0;
  return get_set_ptr(set)->size;
}

uint32 get_seq_len(OBJ seq)
{
  fail_if_not(is_seq(seq), "Not a sequence");
  return get_seq_length(seq);
}

uint32 get_map_size(OBJ map)
{
  fail_if_not(is_map(map), "Not a map");
  if (is_empty_map(map))
    return 0;
  return get_map_ptr(map)->size;
}

int64 mantissa(OBJ obj)
{
  int64 mantissa;
  int32 dec_exp;
  mantissa_and_dec_exp(get_float(obj), mantissa, dec_exp);
  return mantissa;
}

int64 dec_exp(OBJ obj)
{
  int64 mantissa;
  int32 dec_exp;
  mantissa_and_dec_exp(get_float(obj), mantissa, dec_exp);
  return dec_exp;
}

int64 rand_nat(uint64 max)
{
  return rand() % max; //## BUG: THE FUNCTION rand() ONLY GENERATES A LIMITED RANGE OF INTEGERS
}

int64 unique_nat()
{
  static int64 next_val = 0;
  return next_val++;
}

////////////////////////////////////////////////////////////////////////////////

OBJ obj_neg(OBJ obj)
{
  assert(is_bool(obj));
  return make_bool(!get_bool(obj));
}

OBJ at(OBJ seq, int64 idx)
{
  assert(is_seq(seq));
  hard_fail_if_not(((uint64) idx) < get_seq_length(seq), "Invalid sequence index");
  return get_seq_buffer_ptr(seq)[idx];
}

OBJ get_tag(OBJ obj)
{
  return make_symb(get_tag_idx(obj));
}

OBJ get_curr_obj(SEQ_ITER &it)
{
  assert(it.idx < it.len);
  return it.buffer[it.idx];
}

OBJ get_curr_obj(SET_ITER &it)
{
  assert(it.idx < it.size);
  return it.buffer[it.idx];
}

OBJ get_curr_key(MAP_ITER &it)
{
  assert(it.idx < it.size);
  return it.buffer[it.idx];
}

OBJ get_curr_value(MAP_ITER &it)
{
  assert(it.idx < it.size);
  return it.buffer[it.idx+it.size];
}

OBJ rand_set_elem(OBJ set)
{
  SET_OBJ *set_ptr = get_set_ptr(set);
  uint32 idx = std::rand() % set_ptr->size;
  return set_ptr->buffer[idx];
}
