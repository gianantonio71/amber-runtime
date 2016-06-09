#include <cstdio>
#include <cstring>
#include <cmath>
#include <map>

#include "lib.h"
#include "generated.h"



void init(STREAM &s)
{
  s.buffer = 0;
  s.capacity = 0;
  s.count = 0;
}

void append(STREAM &s, OBJ obj) // obj must be already reference-counted
{
  assert(s.count <= s.capacity);
  
  int count = s.count;
  int capacity = s.capacity;
  OBJ *buffer = s.buffer;
  
  if (count == capacity)
  {
    int new_capacity = capacity == 0 ? 32 : 2 * capacity;
    OBJ *new_buffer = new_obj_array(new_capacity);
    for (int i=0 ; i < count ; i++)
      new_buffer[i] = buffer[i];
    if (capacity != 0)
      delete_obj_array(buffer, capacity);
    s.buffer = new_buffer;
    s.capacity = new_capacity;  
  }
  
  s.buffer[count] = obj;
  s.count++;
}

OBJ build_set(OBJ *elems, int size)
{
  if (size == 0)
    return empty_set;

  size = sort_and_release_dups(elems, size);
  
  SET_OBJ *set = new_set(size);
  OBJ *es = set->elems;
  for (int i=0 ; i < size ; i++)
    es[i] = elems[i];
  
  return make_obj(set);
}

OBJ build_set(STREAM &s)
{
  assert((s.count == 0 && s.capacity == 0 && s.buffer == 0) || (s.count > 0 && s.capacity > 0 && s.buffer != 0));

  int count = s.count;
  if (count == 0)
    return empty_set;

  OBJ *buffer = s.buffer;
  OBJ set = build_set(buffer, count);
  delete_obj_array(buffer, s.capacity);
  return set;
}

OBJ build_seq(OBJ *elems, int length) // Objects in elems must be already reference-counted
{
  if (length == 0)
    return empty_seq;
  
  SEQ_OBJ *seq = new_full_seq(length);
  
  for (int i=0 ; i < length ; i++)
    seq->elems[i] = elems[i];
  
  return make_obj(seq);
}

OBJ build_seq(STREAM &s)
{
  if (s.count == 0)
    return empty_seq;

  //## COULD IT BE OPTIMIZED?
  
  OBJ seq = build_seq(s.buffer, s.count);
  
  delete_obj_array(s.buffer, s.capacity);

  return seq;
}

OBJ build_map(OBJ *keys, OBJ *values, int size)
{
  if (size == 0)
    return empty_map;
    
  sort_and_check_no_dups(keys, values, size);
  
  MAP_OBJ *map = new_map(size);
  OBJ *ks  = map->buffer;
  OBJ *vs  = ks + map->size;
  
  for (int i=0 ; i < size ; i++)
  {
    ks[i] = keys[i];
    vs[i] = values[i];
  }
  
  return make_obj(map);
}

OBJ build_map(STREAM &key_stream, STREAM &value_stream)
{
  assert(key_stream.count == value_stream.count);
  
  if (key_stream.count == 0)
    return empty_map;
    
  OBJ map = build_map(key_stream.buffer, value_stream.buffer, key_stream.count);
  
  delete_obj_array(key_stream.buffer, key_stream.capacity);
  delete_obj_array(value_stream.buffer, value_stream.capacity);
  
  return map;
}

OBJ build_tagged_obj(OBJ tag, OBJ obj)
{
  assert(is_symb(tag));
  fail_if_not(is_symb(tag), "Not a symbol");
  
  TAG_OBJ *tag_obj = new_tag_obj();
  
  tag_obj->tag = tag;
  tag_obj->obj = obj;
  
  return make_obj(tag_obj);
}

OBJ make_float(double val)
{
  FLOAT *float_ptr = new_float();
  float_ptr->value = val;
  return make_obj(float_ptr);
}

OBJ neg_float(OBJ obj)
{
  FLOAT *ptr = get_float_ptr(obj);
  return make_float(-ptr->value);
}

OBJ add_floats(OBJ obj1, OBJ obj2)
{
  FLOAT *ptr1 = get_float_ptr(obj1);
  FLOAT *ptr2 = get_float_ptr(obj2);
  return make_float(ptr1->value + ptr2->value);
}

OBJ sub_floats(OBJ obj1, OBJ obj2)
{
  FLOAT *ptr1 = get_float_ptr(obj1);
  FLOAT *ptr2 = get_float_ptr(obj2);
  return make_float(ptr1->value - ptr2->value);
}

OBJ mult_floats(OBJ obj1, OBJ obj2)
{
  FLOAT *ptr1 = get_float_ptr(obj1);
  FLOAT *ptr2 = get_float_ptr(obj2);
  return make_float(ptr1->value * ptr2->value);
}

OBJ div_floats(OBJ obj1, OBJ obj2)
{
  FLOAT *ptr1 = get_float_ptr(obj1);
  FLOAT *ptr2 = get_float_ptr(obj2);
  return make_float(ptr1->value / ptr2->value);
}

OBJ square_root(OBJ obj)
{
  FLOAT *ptr = get_float_ptr(obj);
  return make_float(sqrt(ptr->value));
}

OBJ floor(OBJ obj)
{
  throw;
}

OBJ ceiling(OBJ obj)
{
  throw;
}

OBJ int_to_float(OBJ obj)
{
  return make_float(get_int_val(obj));
}

OBJ blank_array(int size)
{
  if (size <= 0) //## DON'T LIKE THIS
    return empty_seq;

  SEQ_OBJ *seq = new_full_seq(size);
  
  for (int i=0 ; i < size ; i++)
    seq->elems[i] = blank_obj;
  
  return make_obj(seq);
}

OBJ get_seq_slice(OBJ seq, int idx_first, int len)
{
  assert(idx_first >= 0 && len >= 0);
  assert(seq == empty_seq || is_ne_seq(seq));
  fail_if_not(is_seq(seq), "_slice_: First param is not a sequence");
  hard_fail_if_not(idx_first >= 0, "_slice_: Invalid sequence index"); // Depending on the signature of the function,
  hard_fail_if_not(len >= 0, "_slice_: Invalid sequence length");      // these two checks may be unnecessary

  if (len == 0)
    return empty_seq;

  SEQ_OBJ *s = get_seq_ptr(seq);
  int right_bound = idx_first + len;
  hard_fail_if_not(right_bound <= s->length, "Invalid subsequence start/length combination");  
  OBJ *elems = s->elems + idx_first;

  FULL_SEQ_OBJ *full_seq = is_full_seq(s) ? (FULL_SEQ_OBJ *) s : ((REF_SEQ_OBJ *)s)->full_seq;
  add_ref(full_seq);
  return make_obj(new_ref_seq(full_seq, s->elems+idx_first, len));
}

OBJ join_seqs_helper(SEQ_OBJ *left_ptr, OBJ *right_elems, int right_len)
{
  int left_len = left_ptr->length;
  OBJ *left_elems = left_ptr->elems;

  bool can_be_extended = false;
  FULL_SEQ_OBJ *full_seq_ptr = NULL;
  if (is_full_seq(left_ptr))
  {
    full_seq_ptr = (FULL_SEQ_OBJ *) left_ptr;
    if (full_seq_ptr->used_capacity == left_len)
      if (full_seq_ptr->used_capacity + right_len <= full_seq_ptr->capacity)
      {
        assert(full_seq_ptr->buffer + full_seq_ptr->used_capacity == left_elems + left_len);
        can_be_extended = true;
      }
  }
  else
  {
    full_seq_ptr = ((REF_SEQ_OBJ *) left_ptr)->full_seq;
    if (left_elems + left_len == full_seq_ptr->buffer + full_seq_ptr->used_capacity)
      if (full_seq_ptr->used_capacity + right_len <= full_seq_ptr->capacity)
        can_be_extended = true;
  }

  vec_add_ref(right_elems, right_len);

  if (can_be_extended)
  {
    memcpy(left_elems+left_len, right_elems, sizeof(OBJ) * right_len);
    full_seq_ptr->used_capacity += right_len;
    add_ref(full_seq_ptr);
    return make_obj(new_ref_seq(full_seq_ptr, left_ptr->elems, left_len+right_len));
  }

  vec_add_ref(left_elems, left_len);

  int len = left_len + right_len;

  SEQ_OBJ *seq = new_full_seq(len);

  for (int i=0 ; i < left_len ; i++)
    seq->elems[i] = left_elems[i];
  for (int i=0 ; i < right_len ; i++)
    seq->elems[i+left_len] = right_elems[i];

  return make_obj(seq);
}

OBJ append_to_seq(OBJ seq, OBJ obj)
{
  if (seq == empty_seq)
    return build_seq(&obj, 1);

  SEQ_OBJ *seq_ptr = get_seq_ptr(seq);
  OBJ res = join_seqs_helper(seq_ptr, &obj, 1);
  release(seq);
  release(obj);
  return res;
}

OBJ join_seqs(OBJ left, OBJ right)
{
  // No need to check the parameters here

  if (left == empty_seq)
  {
    add_ref(right);
    return right;
  }

  if (right == empty_seq)
  {
    add_ref(left);
    return left;
  }

  SEQ_OBJ *left_ptr  = get_seq_ptr(left);
  SEQ_OBJ *right_ptr = get_seq_ptr(right);

  int right_len = right_ptr->length;
  OBJ *right_elems = right_ptr->elems;

  return join_seqs_helper(left_ptr, right_elems, right_len);
}

OBJ join_mult_seqs(OBJ seqs)
{
  if (seqs == empty_seq)
    return empty_seq;

  SEQ_OBJ *seqs_ptr = get_seq_ptr(seqs);
  int seqs_count = seqs_ptr->length;

  int res_len = 0;
  for (int i=0 ; i < seqs_count ; i++)
  {
    OBJ seq = seqs_ptr->elems[i];
    if (seq != empty_seq)
      res_len += get_seq_ptr(seq)->length;
  }

  if (res_len == 0)
    return empty_seq;

  SEQ_OBJ *res_seq = new_full_seq(res_len);

  int copied = 0;
  for (int i=0 ; i < seqs_count ; i++)
  {
    OBJ seq = seqs_ptr->elems[i];
    if (seq != empty_seq)
    {
      SEQ_OBJ *seq_ptr = get_seq_ptr(seq);
      int len = seq_ptr->length;
      for (int j=0 ; j < len ; j++)
        res_seq->elems[copied+j] = seq_ptr->elems[j];
      copied += len;
    }
  }
  assert(copied == res_len);

  vec_add_ref(res_seq->elems, res_seq->length);

  return make_obj(res_seq);
}

OBJ rev_seq(OBJ seq)
{
  // No need to check the parameters here

  if (seq == empty_seq)
    return empty_seq;
  
  SEQ_OBJ *s = get_seq_ptr(seq);
  
  int len = s->length;
  OBJ *elems = s->elems;
  
  vec_add_ref(elems, len);
  
  SEQ_OBJ *rs = new_full_seq(len);
  OBJ *rev_elems = rs->elems;
  
  for (int i=0 ; i < len ; i++)
    rev_elems[len-i-1] = elems[i];
  
  return make_obj(rs);  
}

OBJ get_at(OBJ seq, int idx) // Increases reference count
{
  OBJ obj = at(seq, idx);
  add_ref(obj);
  return obj;
}

void set_at(OBJ seq, int idx, OBJ value) // Value must be already reference counted
{
  // This is not called directly by the user, so asserts should be sufficient
  SEQ_OBJ *s = get_seq_ptr(seq);
  assert(idx < s->length);
  OBJ *elems = s->elems;
  release(elems[idx]);
  elems[idx] = value;
}

OBJ lookup(OBJ map, OBJ key)
{
  hard_fail_if(map == empty_map, "_lookup_(...): Map is empty"); // Depending on the signature of the builtin operation, this may be unnecessary
  fail_if_not(is_ne_map(map), "_lookup_(...): First parameter is not a map"); // This could also be called by a dot access

  MAP_OBJ *m = get_map_ptr(map);
  int size = m->size;
  OBJ *keys = m->buffer;
  
  int idx = find_obj(keys, size, key);
  // hard_fail_if(idx == -1, "Key not found"); // This function could be called by a dot access
  if (idx == -1)
  {
    if (is_symb(key))
    {
      char buff[1024];
      strcpy(buff, "_lookup_(...): Key not found: ");
      int len = strlen(buff);
      printed_obj(key, buff+len, sizeof(buff)-len-1);
      hard_fail(buff);
    }
    else
      hard_fail("_lookup_(...): Key not found");
  }
  
  OBJ *values = keys + size;
  OBJ value = values[idx];

  //add_ref(value);
    
  return value;
}

OBJ lookup(OBJ map, OBJ key, bool &found)
{
  // The first parameter is checked anyway

  if (map == empty_map)
  {
    found = false;
    return make_int(0LL); //## WHAT SHOULD I RETURN HERE?
  }

  MAP_OBJ *m = get_map_ptr(map);
  int size = m->size;
  OBJ *keys = m->buffer;
  
  int idx = find_obj(keys, size, key);
  if (idx == -1)
  {
    found = false;
    return make_int(0LL); //## WHAT SHOULD I RETURN HERE?
  }
  
  OBJ *values = keys + size;
  OBJ value = values[idx];
  
  //add_ref(value);
  
  found = true;
  return value;
}

OBJ ext_lookup(OBJ map_or_tag_obj, OBJ key)
{
  OBJ map = is_tag_obj(map_or_tag_obj) ? get_inner_obj(map_or_tag_obj) : map_or_tag_obj;
  MAP_OBJ *ptr = get_map_ptr(map);
  int size = ptr->size;
  OBJ *keys = ptr->buffer;
  OBJ *values = keys + size;
  for (int i=0 ; i < size ; i++)
    if (keys[i] == key)
      return values[i];
  fail();
}

OBJ ext_lookup(OBJ map_or_tag_obj, OBJ key, bool &found)
{
  OBJ map = is_tag_obj(map_or_tag_obj) ? get_inner_obj(map_or_tag_obj) : map_or_tag_obj;
  if (map != empty_map)
  {
    MAP_OBJ *ptr = get_map_ptr(map);
    int size = ptr->size;
    OBJ *keys = ptr->buffer;
    OBJ *values = keys + size;
    for (int i=0 ; i < size ; i++)
      if (keys[i] == key)
      {
        found = true;
        return values[i];
      }
  }
  found = false;
  return make_int(0LL); //## WHAT SHOULD I RETURN HERE?
}


OBJ merge_sets(OBJ sets)
{
  OBJ merge_sets_impl(OBJ);
  return merge_sets_impl(sets);
}

OBJ merge_maps(OBJ maps)
{
  OBJ merge_maps_impl(OBJ);
  return merge_maps_impl(maps);
}


OBJ seq_to_set(OBJ obj)
{
  if (obj == empty_seq)
    return empty_set;
  
  SEQ_OBJ *seq = get_seq_ptr(obj);
  int len = seq->length;
  OBJ *seq_elems = seq->elems;
  
  OBJ *elems = new_obj_array(len);
  for (int i=0 ; i < len ; i++)
  {
    OBJ elem = seq_elems[i];
    add_ref(elem);
    elems[i] = elem;  
  }
  
  OBJ res = build_set(elems, len);
  
  delete_obj_array(elems, len);
  
  return res;
}


OBJ seq_to_mset(OBJ seq_obj)
{
  if (seq_obj == empty_seq)
    return empty_map;

  SEQ_OBJ *seq = get_seq_ptr(seq_obj);
  int len = seq->length;
  OBJ *elems = seq->elems;
  
  int *idxs = new_int_array(len);
  OBJ *counters = new_obj_array(len); //## WHY OBJECTS AND NOT INTEGERS?

  int n = sort_group_and_count(elems, len, idxs, counters);
  
  MAP_OBJ *res = new_map(n);
  OBJ *keys = res->buffer;
  OBJ *values = keys + n;
  
  for (int i=0 ; i < n ; i++)
  {
    OBJ obj = elems[idxs[i]];
    add_ref(obj);
    keys[i] = obj;
    values[i] = counters[i];
  }
  
  delete_int_array(idxs, len);
  delete_obj_array(counters, len);
  
  return make_obj(res);
}

OBJ list_to_seq(OBJ list)
{
  if (list == empty_seq)
    return empty_seq;

  int len = 0;
  for (OBJ tail=list ; tail != empty_seq ; tail=at(tail, 1))
    len++;
  
  SEQ_OBJ *seq = new_full_seq(len);
  
  OBJ *elems = seq->elems;
  OBJ tail = list;
  
  for (int i=0 ; i < len ; i++)
  {
    elems[i] = get_at(tail, 0);
    tail = at(tail, 1);
  }
  
  return make_obj(seq);
}

OBJ internal_sort(OBJ set)
{
  if (set == empty_set)
    return empty_seq;

  SET_OBJ *s = get_set_ptr(set);
  int size = s->size;
  OBJ *src = s->elems;
  
  SEQ_OBJ *seq = new_full_seq(size);
  OBJ *dest = seq->elems;
  for (int i=0 ; i < size ; i++)
    dest[i] = src[i];
  vec_add_ref(dest, size);
  
  return make_obj(seq);
}

static std::map<OBJ, OBJ> attachments_map;

OBJ add_attachment(OBJ target, OBJ data)
{
  std::map<OBJ, OBJ>::iterator it = attachments_map.find(target);
  if (it == attachments_map.end())
  {
    add_ref(target);
    add_ref(data);
    OBJ set = build_set(&data, 1);
    attachments_map[target] = set;
    add_obj_to_cache(target);
    add_obj_to_cache(set);
  }
  else
  {
    OBJ curr_data_set = it->second;

    OBJ sets[2];
    sets[0] = curr_data_set;
    add_ref(data);
    sets[1] = build_set(&data, 1);
    OBJ set_of_sets = build_set(sets, 2);
    OBJ new_data_set = merge_sets(set_of_sets);
    release(set_of_sets);

    // The current data set cannot be released because it's still in the list of cached object to release
    // release(curr_data_set);
    attachments_map[target] = new_data_set;
    add_obj_to_cache(new_data_set);
  }
  add_ref(target);
  return target;
}

OBJ fetch_attachments(OBJ obj)
{
  std::map<OBJ, OBJ>::iterator it = attachments_map.find(obj);
  if (it == attachments_map.end())
    return empty_set;
  OBJ res = it->second;
  add_ref(res);
  return res;
}

void get_set_iter(SET_ITER &it, OBJ set)
{
  it.idx = 0;
  if (set != empty_set)
  {
    it.set = get_set_ptr(set);
    it.size = it.set->size;
  }
  else
  {
    it.set = 0;  //## NOT STRICTLY NECESSARY
    it.size = 0;
  }
}

void get_seq_iter(SEQ_ITER &it, OBJ seq)
{
  it.idx = 0;
  if (seq != empty_seq)
  {
    it.seq = get_seq_ptr(seq);
    it.len = it.seq->length;
  }
  else
  {
    it.seq = 0; //## NOT STRICTLY NECESSARY
    it.len = 0;
  }
}

void get_map_iter(MAP_ITER &it, OBJ map)
{
  it.idx = 0;
  if (map != empty_map)
  {
    it.map = get_map_ptr(map);
    it.size = it.map->size;
  }
  else
  {
    it.map = 0;  //## NOT STRICTLY NECESSARY
    it.size = 0;
  }
}

void move_forward(SET_ITER &it)
{
  assert(!is_out_of_range(it));
  it.idx++;
}

void move_forward(SEQ_ITER &it)
{
  assert(!is_out_of_range(it));
  it.idx++;
}

void move_forward(MAP_ITER &it)
{
  assert(!is_out_of_range(it));
  it.idx++;
}

void fail()
{
#ifndef NDEBUG
  std::fputs("\nFail statement reached. Call stack:\n\n", stderr);
#else
  std::fputs("\nFail statement reached\n", stderr);
#endif
  std::fflush(stderr);
  print_call_stack();
  *(char *)0 = 0; // Causing a runtime crash, useful for debugging
}

void runtime_check(OBJ cond)
{
  if (cond != make_symb(1))
  {
#ifndef NDEBUG
    std::fputs("\nAssertion failed. Call stack:\n\n", stderr);
#else
    std::fputs("\nAssertion failed\n", stderr);
#endif
    std::fflush(stderr);
    print_call_stack();
    *(char *)0 = 0; // Causing a runtime crash, useful for debugging
  }
}



OBJ build_const_uint8_seq(const uint8* buffer, uint32 len)
{
  if (len == 0)
    return empty_seq;

  SEQ_OBJ *seq = new_full_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->elems[i] = make_int(buffer[i]);

  return make_obj(seq);
}


OBJ build_const_uint16_seq(const uint16* buffer, uint32 len)
{
  if (len == 0)
    return empty_seq;

  SEQ_OBJ *seq = new_full_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->elems[i] = make_int(buffer[i]);

  return make_obj(seq);
}


OBJ build_const_uint32_seq(const uint32* buffer, uint32 len)
{
  if (len == 0)
    return empty_seq;

  SEQ_OBJ *seq = new_full_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->elems[i] = make_int(buffer[i]);

  return make_obj(seq);
}


OBJ build_const_int8_seq(const int8* buffer, uint32 len)
{
  if (len == 0)
    return empty_seq;

  SEQ_OBJ *seq = new_full_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->elems[i] = make_int(buffer[i]);

  return make_obj(seq);
}


OBJ build_const_int16_seq(const int16* buffer, uint32 len)
{
  if (len == 0)
    return empty_seq;

  SEQ_OBJ *seq = new_full_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->elems[i] = make_int(buffer[i]);

  return make_obj(seq);
}


OBJ build_const_int32_seq(const int32* buffer, uint32 len)
{
  if (len == 0)
    return empty_seq;

  SEQ_OBJ *seq = new_full_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->elems[i] = make_int(buffer[i]);

  return make_obj(seq);
}


OBJ build_const_int64_seq(const int64* buffer, uint32 len)
{
  if (len == 0)
    return empty_seq;

  SEQ_OBJ *seq = new_full_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->elems[i] = make_int(buffer[i]);

  return make_obj(seq);
}


int get_tag_idx(OBJ obj)
{
  return get_symb_idx(get_tag(obj));
}