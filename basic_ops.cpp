#include "lib.h"
#include "generated.h"

#include <cstdlib>


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
  if (set == empty_set)
    return false;
  Set *s = get_set_ptr(set);
  int idx = find_obj(s->elems, s->size, elem);
  return idx != -1;
}

////////////////////////////////////////////////////////////////////////////////

long long get_int_val(Obj obj)
{
  assert(is_int(obj));
  fail_if_not(is_int(obj), "Not an integer");
  return obj >> SHORT_TAG_SIZE;
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

long long mantissa(Obj obj)
{
  long long mantissa;
  int dec_exp;
  mantissa_and_dec_exp(get_float_ptr(obj)->value, mantissa, dec_exp);
  return mantissa;
}

int dec_exp(Obj obj)
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

Obj to_obj(bool b)
{
  // assert((int(b) == 0 && b == false) || (int(b) == 1 && b == true));
  // assert(((32 >> int(b)) | 1) == (b ? generated::True_S : generated::False_S));
  // return (32 >> int(ib)) | 1;
  return b ? generated::True_S : generated::False_S;  // Surprisingly, this seems to be faster...
}

Obj to_obj(int n)
{
  Obj obj = n << SHORT_TAG_SIZE;
  assert(get_int_val(obj) == n);
  return obj;
}

Obj to_obj(long long n)
{
  Obj obj = n << SHORT_TAG_SIZE;
  assert(get_int_val(obj) == n);
  return obj;
}

Obj obj_neg(Obj obj)
{
  assert(obj == generated::True_S || obj == generated::False_S);
  fail_if_not(obj == generated::True_S || obj == generated::False_S, "Not a boolean");
  return obj == generated::True_S ? generated::False_S : generated::True_S;
  // assert((obj ^ 0x30) == (obj == generated::True_S ? generated::False_S : generated::True_S));
  // return obj ^ 0x30;
}

Obj at(Obj seq, int idx)
{
  Seq *s = get_seq_ptr(seq);
  fail_if_not(idx < s->length, "Invalid sequence index");
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

Obj rand_set_elem(Obj set)
{
  Set *set_ptr = get_set_ptr(set);
  int idx = std::rand() % set_ptr->size;
  return set_ptr->elems[idx];
}
