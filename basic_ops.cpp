#include "lib.h"
#include "generated.h"

#include <cstdlib>


bool inline_eq(OBJ obj1, OBJ obj2)
{
  assert(is_inline_obj(obj2));
  return obj1 == obj2;
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
  if (set == empty_set)
    return false;
  SET_OBJ *s = get_set_ptr(set);
  int idx = find_obj(s->elems, s->size, elem);
  return idx != -1;
}

////////////////////////////////////////////////////////////////////////////////

long long get_int_val(OBJ obj)
{
  assert(is_int(obj));
  fail_if_not(is_int(obj), "Not an integer");
  return obj >> SHORT_TAG_SIZE;
}

int get_set_size(OBJ set)
{
  if (set == empty_set)
    return 0;
  return get_set_ptr(set)->size;
}

int get_seq_len(OBJ seq)
{
  if (seq == empty_seq)
    return 0;
  return get_seq_ptr(seq)->length;
}

int get_map_size(OBJ map)
{
  if (map == empty_map)
    return 0;
  return get_map_ptr(map)->size;
}

long long mantissa(OBJ obj)
{
  long long mantissa;
  int dec_exp;
  mantissa_and_dec_exp(get_float_ptr(obj)->value, mantissa, dec_exp);
  return mantissa;
}

int dec_exp(OBJ obj)
{
  long long mantissa;
  int dec_exp;
  mantissa_and_dec_exp(get_float_ptr(obj)->value, mantissa, dec_exp);
  return dec_exp;
}

int rand_nat(int max)
{
  return rand() % max; //## BUG: THE FUNCTION rand() ONLY GENERATES A LIMITED RANGE OF INTEGERS
}

long long unique_nat()
{
  static long long next_val = 0;
  return next_val++;
}

////////////////////////////////////////////////////////////////////////////////

OBJ make_bool(bool b)
{
  return b ? make_symb(1) : make_symb(0);
}

OBJ make_int(long long n)
{
  OBJ obj = n << SHORT_TAG_SIZE;
  assert(get_int_val(obj) == n);
  return obj;
}

OBJ obj_neg(OBJ obj)
{
  assert(obj == make_symb(1) || obj == make_symb(0));
  fail_if_not(obj == make_symb(1) || obj == make_symb(0), "Not a boolean");
  return obj == make_symb(1) ? make_symb(0) : make_symb(1);
  // assert((obj ^ 0x30) == (obj == make_symb(1) ? make_symb(0) : make_symb(1)));
  // return obj ^ 0x30;
}

OBJ at(OBJ seq, int idx)
{
  SEQ_OBJ *s = get_seq_ptr(seq);
  fail_if_not(idx < s->length, "Invalid sequence index");
  return s->elems[idx];
}

OBJ search_or_lookup(OBJ coll, OBJ value)
{
  if (is_seq(coll))
    return at(coll, get_int_val(value));

  if (is_set(coll))
    return make_bool(has_elem(coll, value));

  if (is_map(coll))
    return lookup(coll, value);

  hard_fail("Object being searched is not a collection");
  throw;
}

OBJ get_tag(OBJ obj)
{
  return get_tag_obj_ptr(obj)->tag;
}

OBJ get_inner_obj(OBJ obj)
{
  return get_tag_obj_ptr(obj)->obj;
}

OBJ get_curr_obj(SET_ITER &it)
{
  assert(it.idx < it.set->size);
  return it.set->elems[it.idx];
}

OBJ get_curr_obj(SEQ_ITER &it)
{
  assert(it.idx < it.seq->length);
  return it.seq->elems[it.idx];
}

OBJ get_curr_key(MAP_ITER &it)
{
  assert(it.idx < it.map->size);
  return it.map->buffer[it.idx];
}

OBJ get_curr_value(MAP_ITER &it)
{
  assert(it.idx < it.map->size);
  MAP_OBJ *map = it.map;
  return map->buffer[it.idx+map->size];
}

OBJ rand_set_elem(OBJ set)
{
  SET_OBJ *set_ptr = get_set_ptr(set);
  int idx = std::rand() % set_ptr->size;
  return set_ptr->elems[idx];
}
