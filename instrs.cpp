#include <cstdio>
#include <cstring>
#include <cmath>
#include <map>

#include "lib.h"



void init(STREAM &s)
{
  s.buffer = 0;
  s.capacity = 0;
  s.count = 0;
}

void append(STREAM &s, OBJ obj) // obj must be already reference-counted
{
  assert(s.count <= s.capacity);

  uint32 count = s.count;
  uint32 capacity = s.capacity;
  OBJ *buffer = s.buffer;

  if (count == capacity)
  {
    uint32 new_capacity = capacity == 0 ? 32 : 2 * capacity;
    OBJ *new_buffer = new_obj_array(new_capacity);
    for (uint32 i=0 ; i < count ; i++)
      new_buffer[i] = buffer[i];
    if (capacity != 0)
      delete_obj_array(buffer, capacity);
    s.buffer = new_buffer;
    s.capacity = new_capacity;
  }

  s.buffer[count] = obj;
  s.count++;
}

OBJ build_seq(OBJ *elems, uint32 length) // Objects in elems must be already reference-counted
{
  if (length == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(length);

  for (uint32 i=0 ; i < length ; i++)
    seq->buffer[i] = elems[i];

  return make_seq(seq, length);
}

OBJ build_seq(STREAM &s)
{
  if (s.count == 0)
    return make_empty_seq();

  //## COULD IT BE OPTIMIZED?

  OBJ seq = build_seq(s.buffer, s.count);

  delete_obj_array(s.buffer, s.capacity);

  return seq;
}

OBJ build_set(OBJ *elems, uint32 size)
{
  if (size == 0)
    return make_empty_set();

  size = sort_and_release_dups(elems, size);

  SET_OBJ *set = new_set(size);
  OBJ *es = set->buffer;
  for (uint32 i=0 ; i < size ; i++)
    es[i] = elems[i];

  return make_set(set);
}

OBJ build_set(STREAM &s)
{
  assert((s.count == 0 && s.capacity == 0 && s.buffer == NULL) || (s.count > 0 && s.capacity > 0 && s.buffer != NULL));

  uint32 count = s.count;
  if (count == 0)
    return make_empty_set();

  OBJ *buffer = s.buffer;
  OBJ set = build_set(buffer, count);
  delete_obj_array(buffer, s.capacity);
  return set;
}

OBJ build_tagged_obj(OBJ tag, OBJ obj)
{
  assert(is_symb(tag));
  return make_tag_obj(get_symb_idx(tag), obj);
}

OBJ neg_float(OBJ obj)
{
  return make_float(-get_float(obj));
}

OBJ add_floats(OBJ obj1, OBJ obj2)
{
  return make_float(get_float(obj1) + get_float(obj2));
}

OBJ sub_floats(OBJ obj1, OBJ obj2)
{
  return make_float(get_float(obj1) - get_float(obj2));
}

OBJ mult_floats(OBJ obj1, OBJ obj2)
{
  return make_float(get_float(obj1) * get_float(obj2));
}

OBJ div_floats(OBJ obj1, OBJ obj2)
{
  return make_float(get_float(obj1) / get_float(obj2));
}

OBJ square_root(OBJ obj)
{
  return make_float(sqrt(get_float(obj)));
}

OBJ floor(OBJ obj)
{
  impl_fail("_floor_() not implemented");
}

OBJ ceiling(OBJ obj)
{
  impl_fail("_ceiling_() not implemented");
}

OBJ int_to_float(OBJ obj)
{
  return make_float(get_int_val(obj));
}

OBJ blank_array(int64 size)
{
  if (size > 0xFFFFFFFF)
    impl_fail("Maximum permitted array size exceeded");

  if (size <= 0) //## I DON'T LIKE THIS
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(size);
  OBJ *buffer = seq->buffer;
  OBJ blank_obj = make_blank_obj();

  for (uint32 i=0 ; i < size ; i++)
    buffer[i] = blank_obj;

  return make_seq(seq, size);
}

OBJ get_seq_slice(OBJ seq, int64 idx_first, int64 len)
{
  assert(is_seq(seq));

  if (idx_first < 0 | len < 0 | idx_first + len > get_seq_length(seq))
    soft_fail("_slice_(): Invalid start index and/or subsequence length");

  if (len == 0)
    return make_empty_seq();

  add_ref(seq);

  SEQ_OBJ *ptr = get_seq_ptr(seq);
  uint32 offset = get_seq_offset(seq);
  return make_slice(ptr, get_mem_layout(seq), offset+idx_first, len);
}

OBJ extend_sequence(OBJ seq, OBJ *new_elems, uint32 count)
{
  assert(!is_empty_seq(seq));
  assert(((uint64) get_seq_length(seq) + count <= 0xFFFFFFFF));

  SEQ_OBJ *seq_ptr = get_seq_ptr(seq);
  uint32 offset = get_seq_offset(seq);
  uint32 length = get_seq_length(seq);

  uint32 new_length = length + count;

  uint32 size = seq_ptr->size;
  uint32 capacity = seq_ptr->capacity;

  bool ends_at_last_elem = offset + length == size;
  bool has_needed_spare_capacity = size + count <= capacity;
  bool can_be_extended = ends_at_last_elem & has_needed_spare_capacity;

  if (can_be_extended)
  {
    memcpy(seq_ptr->buffer+size, new_elems, sizeof(OBJ) * count);
    seq_ptr->size = size + count;
    vec_add_ref(new_elems, count);
    add_ref(seq);
    return make_slice(seq_ptr, get_mem_layout(seq), offset, new_length);
  }
  else
  {
    OBJ *buffer = get_seq_buffer_ptr(seq);

    SEQ_OBJ *new_seq_ptr = new_seq(new_length);
    OBJ *new_buffer = new_seq_ptr->buffer;

    memcpy(new_buffer, buffer, sizeof(OBJ) * length);
    memcpy(new_buffer+length, new_elems, sizeof(OBJ) * count);

    vec_add_ref(new_buffer, new_length);

    return make_seq(new_seq_ptr, new_length);
  }
}

OBJ append_to_seq(OBJ seq, OBJ obj)
{
  if (is_empty_seq(seq))
    return build_seq(&obj, 1);

  // Checking that the new sequence doesn't overflow
  if (!(get_seq_length(seq) < 0xFFFFFFFF))
    impl_fail("Resulting sequence is too large");

  OBJ res = extend_sequence(seq, &obj, 1);
  release(seq);
  release(obj);
  return res;
}

OBJ join_seqs(OBJ left, OBJ right)
{
  // No need to check the parameters here

  uint64 right_len = get_seq_length(right);
  if (right_len == 0)
  {
    add_ref(left);
    return left;
  }

  uint64 left_len = get_seq_length(left);
  if (left_len == 0)
  {
    add_ref(right);
    return right;
  }

  if (left_len + right_len > 0xFFFFFFFF)
    impl_fail("_cat_(): Resulting sequence is too large");

  return extend_sequence(left, get_seq_buffer_ptr(right), right_len);
}

// OBJ join_mult_seqs(OBJ seqs)
// {
//   if (is_empty_seq(seq))
//     return make_empty_seq();

//   OBJ *seqs = get_seq_buffer_ptr(seqs);
//   uint32 count = seqs_ptr->length;

//   uint64 res_len = 0;
//   for (uint32 i=0 ; i < count ; i++)
//   {
//     OBJ seq = seqs_ptr->elems[i];
//     if (seq != empty_seq)
//       res_len += get_seq_ptr(seq)->length;
//   }

//   if (res_len == 0)
//     return empty_seq;

//   SEQ_OBJ *res_seq = new_full_seq(res_len);

//   uint32 copied = 0;
//   for (uint32 i=0 ; i < seqs_count ; i++)
//   {
//     OBJ seq = seqs_ptr->elems[i];
//     if (seq != empty_seq)
//     {
//       SEQ_OBJ *seq_ptr = get_seq_ptr(seq);
//       uint32 len = seq_ptr->length;
//       for (uint32 j=0 ; j < len ; j++)
//         res_seq->elems[copied+j] = seq_ptr->elems[j];
//       copied += len;
//     }
//   }
//   assert(copied == res_len);

//   vec_add_ref(res_seq->elems, res_seq->length);

//   return make_obj(res_seq);
// }

OBJ rev_seq(OBJ seq)
{
  // No need to check the parameters here

  uint32 len = get_seq_length(seq);
  if (len <= 1)
  {
    if (len == 1)
      add_ref(seq);
    return seq;
  }

  OBJ *elems = get_seq_buffer_ptr(seq);
  vec_add_ref(elems, len);

  SEQ_OBJ *rs = new_seq(len);
  OBJ *rev_elems = rs->buffer;
  for (uint32 i=0 ; i < len ; i++)
    rev_elems[len-i-1] = elems[i];

  return make_seq(rs, len);
}

OBJ get_at(OBJ seq, uint32 idx) // Increases reference count
{
  OBJ obj = at(seq, idx);
  add_ref(obj);
  return obj;
}

void set_at(OBJ seq, uint32 idx, OBJ value) // Value must be already reference counted
{
  // This is not called directly by the user, so asserts should be sufficient
  assert(idx < get_seq_length(seq));

  OBJ *target = get_seq_buffer_ptr(seq) + idx;
  release(*target);
  *target = value;
}

OBJ lookup(OBJ map, OBJ key)
{
  // No need to check the parameters

  bool found;
  OBJ res = lookup(map, key, found);
  if (found)
    return res;

  if (is_empty_bin_rel(map))
  {
    soft_fail("_lookup_(): Key not found. Map is empty");
  }
  else if (is_symb(key))
  {
    char buff[1024];
    strcpy(buff, "_lookup_(): Key not found: ");
    uint32 len = strlen(buff);
    printed_obj(key, buff+len, sizeof(buff)-len-1);
    soft_fail(buff);
  }
  else
    soft_fail("_lookup_(): Key not found");
}

OBJ lookup(OBJ map, OBJ key, bool &found)
{
  // No need to check the parameters

  if (is_empty_bin_rel(map))
  {
    found = false;
    return make_blank_obj();
  }

  assert(get_physical_type(map) == TYPE_MAP | get_physical_type(map) == TYPE_LOG_MAP);

  BIN_REL_OBJ *m = get_bin_rel_ptr(map);
  uint32 size = m->size;
  OBJ *keys = m->buffer;

  uint32 idx = find_obj(keys, size, key, found);
  if (!found)
  {
    found = false;
    return make_blank_obj();
  }

  OBJ *values = keys + size;
  return values[idx];
}

OBJ ext_lookup(OBJ map_or_tag_obj, OBJ key)
{
  uint16 key_idx = get_symb_idx(key);
  OBJ map = is_tag_obj(map_or_tag_obj) ? get_inner_obj(map_or_tag_obj) : map_or_tag_obj;
  BIN_REL_OBJ *ptr = get_bin_rel_ptr(map);
  uint32 size = ptr->size;
  OBJ *keys = ptr->buffer;
  OBJ *values = keys + size;
  for (uint32 i=0 ; i < size ; i++)
    if (get_symb_idx(keys[i]) == key_idx)
      return values[i];
  internal_fail();
}

OBJ ext_lookup(OBJ map_or_tag_obj, OBJ key, bool &found)
{
  uint16 key_idx = get_symb_idx(key);
  OBJ map = is_tag_obj(map_or_tag_obj) ? get_inner_obj(map_or_tag_obj) : map_or_tag_obj;
  if (!is_empty_bin_rel(map))
  {
    BIN_REL_OBJ *ptr = get_bin_rel_ptr(map);
    uint32 size = ptr->size;
    OBJ *keys = ptr->buffer;
    OBJ *values = keys + size;
    for (uint32 i=0 ; i < size ; i++)
      if (get_symb_idx(keys[i]) == key_idx)
      {
        found = true;
        return values[i];
      }
  }
  found = false;
  return make_blank_obj(); //## WHAT SHOULD I RETURN HERE?
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


OBJ seq_to_set(OBJ seq)
{
  if (is_empty_seq(seq))
    return make_empty_set();

  OBJ *elems = get_seq_buffer_ptr(seq);
  uint32 len = get_seq_length(seq);

  OBJ *elems_copy = new_obj_array(len);
  for (uint32 i=0 ; i < len ; i++)
  {
    OBJ elem = elems[i];
    add_ref(elem);
    elems_copy[i] = elem;
  }
  OBJ res = build_set(elems_copy, len);
  delete_obj_array(elems_copy, len);

  return res;
}


OBJ seq_to_mset(OBJ seq)
{
  if (is_empty_seq(seq))
    return make_empty_bin_rel();

  OBJ *elems = get_seq_buffer_ptr(seq);
  uint32 len = get_seq_length(seq);

  uint32 *idxs = new_uint32_array(len);
  OBJ *counters = new_obj_array(len); //## WHY OBJECTS AND NOT INTEGERS?

  uint32 n = sort_group_and_count(elems, len, idxs, counters);

  BIN_REL_OBJ *res = new_map(n);
  OBJ *keys = res->buffer;
  OBJ *values = keys + n;

  for (uint32 i=0 ; i < n ; i++)
  {
    OBJ obj = elems[idxs[i]];
    add_ref(obj);
    keys[i] = obj;
    values[i] = counters[i];
  }

  delete_uint32_array(idxs, len);
  delete_obj_array(counters, len);

  return make_map(res);
}

OBJ internal_sort(OBJ set)
{
  if (is_empty_set(set))
    return make_empty_seq();

  SET_OBJ *s = get_set_ptr(set);
  uint32 size = s->size;
  OBJ *src = s->buffer;

  SEQ_OBJ *seq = new_seq(size);
  OBJ *dest = seq->buffer;
  for (uint32 i=0 ; i < size ; i++)
    dest[i] = src[i];
  vec_add_ref(dest, size);

  return make_seq(seq, size);
}

static std::map<REF_OBJ*, OBJ> attachments_map;

OBJ add_attachment(OBJ target_obj, OBJ data)
{
  if (is_ref_obj(target_obj))
  {
    REF_OBJ *target_ptr = get_ref_obj_ptr(target_obj);

    std::map<REF_OBJ*, OBJ>::iterator it = attachments_map.find(target_ptr);

    if (it == attachments_map.end())
    {
      add_ref(target_obj);
      add_ref(data);
      OBJ set = build_set(&data, 1);
      attachments_map[target_ptr] = set;
      add_obj_to_cache(target_obj);
      add_obj_to_cache(set);
    }
    else
    {
      // SET_OBJ *curr_data_set = get_set_ptr(it->second);
      // uint32 size = curr_data_set->size;
      // OBJ *buffer = curr_data_set->buffer;
      // OBJ *new_values = new_obj_array(size+1, make_blank_obj());
      // for (uint32 i=0 ; i < size ; i++)
      //   new_values[i] = buffer[i];
      // new_values[size] = data;
      // add_ref(data);
      // OBJ new_data_set = make_set(new_values, size+1);

      OBJ curr_data_set = it->second;
      OBJ sets[2];
      // The current data set has to be add-referenced because it's still in the list of cached object to release
      add_ref(curr_data_set);
      sets[0] = curr_data_set;
      add_ref(data);
      sets[1] = build_set(&data, 1);
      OBJ set_of_sets = build_set(sets, 2);
      OBJ new_data_set = merge_sets(set_of_sets);
      release(set_of_sets);
      attachments_map[target_ptr] = new_data_set;
      add_obj_to_cache(new_data_set);
    }

    add_ref(target_obj);
  }

  return target_obj;
}

OBJ fetch_attachments(OBJ obj)
{
  if (is_ref_obj(obj))
  {
    REF_OBJ *ptr = get_ref_obj_ptr(obj);
    std::map<REF_OBJ*, OBJ>::iterator it = attachments_map.find(ptr);
    if (it != attachments_map.end())
    {
      OBJ res = it->second;
      add_ref(res);
      return res;
    }
  }
  return make_empty_set();
}

void get_set_iter(SET_ITER &it, OBJ set)
{
  it.idx = 0;
  if (!is_empty_set(set))
  {
    SET_OBJ *ptr = get_set_ptr(set);
    it.buffer = ptr->buffer;
    it.size = ptr->size;
  }
  else
  {
    it.buffer = 0;  //## NOT STRICTLY NECESSARY
    it.size = 0;
  }
}

void get_seq_iter(SEQ_ITER &it, OBJ seq)
{
  it.idx = 0;
  if (!is_empty_seq(seq))
  {
    it.buffer = get_seq_buffer_ptr(seq);
    it.len = get_seq_length(seq);
  }
  else
  {
    it.buffer = 0; //## NOT STRICTLY NECESSARY
    it.len = 0;
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

void move_forward(BIN_REL_ITER &it)
{
  assert(!is_out_of_range(it));
  it.idx++;
}

void move_forward(TERN_REL_ITER &it)
{
  assert(!is_out_of_range(it));
  it.idx++;
}

void fail()
{
#ifndef NDEBUG
  const char *MSG = "\nFail statement reached. Call stack:\n\n";
#else
  const char *MSG = "\nFail statement reached\n";
#endif

  soft_fail(MSG);
}

void runtime_check(OBJ cond)
{
  assert(is_bool(cond));

  if (!get_bool(cond))
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

////////////////////////////////////////////////////////////////////////////////

OBJ build_const_uint8_seq(const uint8* buffer, uint32 len)
{
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_uint16_seq(const uint16* buffer, uint32 len)
{
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_uint32_seq(const uint32* buffer, uint32 len)
{
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int8_seq(const int8* buffer, uint32 len)
{
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int16_seq(const int16* buffer, uint32 len)
{
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int32_seq(const int32* buffer, uint32 len)
{
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int64_seq(const int64* buffer, uint32 len)
{
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}
