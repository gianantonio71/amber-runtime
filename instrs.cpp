#include <cstdio>
#include <cstring>
#include <cmath>
#include <map>

#include "lib.h"
#include "generated.h"



void init(Stream &s)
{
  s.buffer = 0;
  s.capacity = 0;
  s.count = 0;
}

void append(Stream &s, Obj obj) // obj must be already reference-counted
{
  assert(s.count <= s.capacity);
  
  int count = s.count;
  int capacity = s.capacity;
  Obj *buffer = s.buffer;
  
  if (count == capacity)
  {
    int new_capacity = capacity == 0 ? 32 : 2 * capacity;
    Obj *new_buffer = new_obj_array(new_capacity);
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

Obj make_set(Obj *elems, int size)
{
  if (size == 0)
    return empty_set;

  size = sort_and_release_dups(elems, size);
  
  Set *set = new_set(size);
  Obj *es = set->elems;
  for (int i=0 ; i < size ; i++)
    es[i] = elems[i];
  
  return make_obj(set);
}

Obj make_set(Stream &s)
{
  assert((s.count == 0 && s.capacity == 0 && s.buffer == 0) || (s.count > 0 && s.capacity > 0 && s.buffer != 0));

  int count = s.count;
  if (count == 0)
    return empty_set;

  Obj *buffer = s.buffer;
  Obj set = make_set(buffer, count);
  delete_obj_array(buffer, s.capacity);
  return set;
}

Obj make_seq(Obj *elems, int length) // Objects in elems must be already reference-counted
{
  if (length == 0)
    return empty_seq;
  
  Seq *seq = new_full_seq(length);
  
  for (int i=0 ; i < length ; i++)
    seq->elems[i] = elems[i];
  
  return make_obj(seq);
}

Obj make_seq(Stream &s)
{
  if (s.count == 0)
    return empty_seq;

  //## COULD IT BE OPTIMIZED?
  
  Obj seq = make_seq(s.buffer, s.count);
  
  delete_obj_array(s.buffer, s.capacity);

  return seq;
}

Obj make_map(Obj *keys, Obj *values, int size)
{
  if (size == 0)
    return empty_map;
    
  sort_and_check_no_dups(keys, values, size);
  
  Map *map = new_map(size);
  Obj *ks  = map->buffer;
  Obj *vs  = ks + map->size;
  
  for (int i=0 ; i < size ; i++)
  {
    ks[i] = keys[i];
    vs[i] = values[i];
  }
  
  return make_obj(map);
}

Obj make_map(Stream &key_stream, Stream &value_stream)
{
  assert(key_stream.count == value_stream.count);
  
  if (key_stream.count == 0)
    return empty_map;
    
  Obj map = make_map(key_stream.buffer, value_stream.buffer, key_stream.count);
  
  delete_obj_array(key_stream.buffer, key_stream.capacity);
  delete_obj_array(value_stream.buffer, value_stream.capacity);
  
  return map;
}

Obj make_tagged_obj(Obj tag, Obj obj)
{
  assert(is_symb(tag));
  fail_if_not(is_symb(tag), "Not a symbol");
  
  TagObj *tag_obj = new_tag_obj();
  
  tag_obj->tag = tag;
  tag_obj->obj = obj;
  
  return make_obj(tag_obj);
}

Obj make_float(double val)
{
  Float *float_ptr = new_float();
  float_ptr->value = val;
  return make_obj(float_ptr);
}

Obj neg_float(Obj obj)
{
  Float *ptr = get_float_ptr(obj);
  return make_float(-ptr->value);
}

Obj add_floats(Obj obj1, Obj obj2)
{
  Float *ptr1 = get_float_ptr(obj1);
  Float *ptr2 = get_float_ptr(obj2);
  return make_float(ptr1->value + ptr2->value);
}

Obj sub_floats(Obj obj1, Obj obj2)
{
  Float *ptr1 = get_float_ptr(obj1);
  Float *ptr2 = get_float_ptr(obj2);
  return make_float(ptr1->value - ptr2->value);
}

Obj mult_floats(Obj obj1, Obj obj2)
{
  Float *ptr1 = get_float_ptr(obj1);
  Float *ptr2 = get_float_ptr(obj2);
  return make_float(ptr1->value * ptr2->value);
}

Obj div_floats(Obj obj1, Obj obj2)
{
  Float *ptr1 = get_float_ptr(obj1);
  Float *ptr2 = get_float_ptr(obj2);
  return make_float(ptr1->value / ptr2->value);
}

Obj square_root(Obj obj)
{
  Float *ptr = get_float_ptr(obj);
  return make_float(sqrt(ptr->value));
}

Obj floor(Obj obj)
{
  throw;
}

Obj ceiling(Obj obj)
{
  throw;
}

Obj int_to_float(Obj obj)
{
  return make_float(get_int_val(obj));
}

Obj make_array(int size, Obj value)
{
  if (size <= 0) //## DON'T LIKE THIS
    return empty_seq;

  Seq *seq = new_full_seq(size);
  
  for (int i=0 ; i < size ; i++)
    seq->elems[i] = value;
  
  mult_add_ref(value, size);
  
  return make_obj(seq);
}

Obj get_seq_slice(Obj seq, int idx_first, int len)
{
  assert(idx_first >= 0 && len >= 0);
  assert(seq == empty_seq || is_ne_seq(seq));
  fail_if_not(is_seq(seq), "_slice_: First param is not a sequence");
  hard_fail_if_not(idx_first >= 0, "_slice_: Invalid sequence index"); // Depending on the signature of the function,
  hard_fail_if_not(len >= 0, "_slice_: Invalid sequence length");      // these two checks may be unnecessary

  if (len == 0)
    return empty_seq;

  Seq *s = get_seq_ptr(seq);
  int right_bound = idx_first + len;
  hard_fail_if_not(right_bound <= s->length, "Invalid subsequence start/length combination");  
  Obj *elems = s->elems + idx_first;

  FullSeq *full_seq = is_full_seq(s) ? (FullSeq *) s : ((RefSeq *)s)->full_seq;
  add_ref(full_seq);
  return make_obj(new_ref_seq(full_seq, s->elems+idx_first, len));
}

Obj join_seqs_helper(Seq *left_ptr, Obj *right_elems, int right_len)
{
  int left_len = left_ptr->length;
  Obj *left_elems = left_ptr->elems;

  bool can_be_extended = false;
  FullSeq *full_seq_ptr = NULL;
  if (is_full_seq(left_ptr))
  {
    full_seq_ptr = (FullSeq *) left_ptr;
    if (full_seq_ptr->used_capacity == left_len)
      if (full_seq_ptr->used_capacity + right_len <= full_seq_ptr->capacity)
      {
        assert(full_seq_ptr->buffer + full_seq_ptr->used_capacity == left_elems + left_len);
        can_be_extended = true;
      }
  }
  else
  {
    full_seq_ptr = ((RefSeq *) left_ptr)->full_seq;
    if (left_elems + left_len == full_seq_ptr->buffer + full_seq_ptr->used_capacity)
      if (full_seq_ptr->used_capacity + right_len <= full_seq_ptr->capacity)
        can_be_extended = true;
  }

  vec_add_ref(right_elems, right_len);

  if (can_be_extended)
  {
    memcpy(left_elems+left_len, right_elems, sizeof(Obj) * right_len);
    full_seq_ptr->used_capacity += right_len;
    add_ref(full_seq_ptr);
    return make_obj(new_ref_seq(full_seq_ptr, full_seq_ptr->elems, left_len+right_len));
  }

  vec_add_ref(left_elems, left_len);

  int len = left_len + right_len;

  Seq *seq = new_full_seq(len);

  for (int i=0 ; i < left_len ; i++)
    seq->elems[i] = left_elems[i];
  for (int i=0 ; i < right_len ; i++)
    seq->elems[i+left_len] = right_elems[i];

  return make_obj(seq);
}

Obj append_to_seq(Obj seq, Obj obj)
{
  if (seq == empty_seq)
    return make_seq(&obj, 1);

  Seq *seq_ptr = get_seq_ptr(seq);
  Obj res = join_seqs_helper(seq_ptr, &obj, 1);
  release(seq);
  release(obj);
  return res;
}

Obj join_seqs(Obj left, Obj right)
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

  Seq *left_ptr  = get_seq_ptr(left);
  Seq *right_ptr = get_seq_ptr(right);

  int right_len = right_ptr->length;
  Obj *right_elems = right_ptr->elems;

  return join_seqs_helper(left_ptr, right_elems, right_len);
}

Obj join_mult_seqs(Obj seqs)
{
  if (seqs == empty_seq)
    return empty_seq;

  Seq *seqs_ptr = get_seq_ptr(seqs);
  int seqs_count = seqs_ptr->length;

  int res_len = 0;
  for (int i=0 ; i < seqs_count ; i++)
  {
    Obj seq = seqs_ptr->elems[i];
    if (seq != empty_seq)
      res_len += get_seq_ptr(seq)->length;
  }

  if (res_len == 0)
    return empty_seq;

  Seq *res_seq = new_full_seq(res_len);

  int copied = 0;
  for (int i=0 ; i < seqs_count ; i++)
  {
    Obj seq = seqs_ptr->elems[i];
    if (seq != empty_seq)
    {
      Seq *seq_ptr = get_seq_ptr(seq);
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

Obj rev_seq(Obj seq)
{
  // No need to check the parameters here

  if (seq == empty_seq)
    return empty_seq;
  
  Seq *s = get_seq_ptr(seq);
  
  int len = s->length;
  Obj *elems = s->elems;
  
  vec_add_ref(elems, len);
  
  Seq *rs = new_full_seq(len);
  Obj *rev_elems = rs->elems;
  
  for (int i=0 ; i < len ; i++)
    rev_elems[len-i-1] = elems[i];
  
  return make_obj(rs);  
}

Obj get_at(Obj seq, int idx) // Increases reference count
{
  Obj obj = at(seq, idx);
  add_ref(obj);
  return obj;
}

void set_at(Obj seq, int idx, Obj value) // Value must be already reference counted
{
  // This is not called directly by the user, so asserts should be sufficient
  Seq *s = get_seq_ptr(seq);
  assert(idx < s->length);
  Obj *elems = s->elems;
  release(elems[idx]);
  elems[idx] = value;
}

Obj lookup(Obj map, Obj key)
{
  fail_if_not(is_ne_map(map), "First parameter is not a non-empty map"); // This could also be called by a dot access
  hard_fail_if(map == empty_map, "_lookup_: Key not found"); // Depending on the signature of the builtin operation, this may be unnecessary

  Map *m = get_map_ptr(map);
  int size = m->size;
  Obj *keys = m->buffer;
  
  int idx = find_obj(keys, size, key);
  // hard_fail_if(idx == -1, "Key not found"); // This function could be called by a dot access
  if (idx == -1)
  {
    if (is_symb(key))
    {
      int idx = get_symb_idx(key);
      const char *str = generated::map_symb_to_str[idx];
      char buff[1024];
      strcpy(buff, "Key not found: ");
      strncat(buff, str, 512);
      hard_fail(buff);
    }
    else
      hard_fail("Key not found");
  }
  
  Obj *values = keys + size;
  Obj value = values[idx];

  //add_ref(value);
    
  return value;
}

Obj lookup(Obj map, Obj key, bool &found)
{
  // The first parameter is checked anyway

  if (map == empty_map)
  {
    found = false;
    return to_obj(0LL); //## WHAT SHOULD I RETURN HERE?
  }

  Map *m = get_map_ptr(map);
  int size = m->size;
  Obj *keys = m->buffer;
  
  int idx = find_obj(keys, size, key);
  if (idx == -1)
  {
    found = false;
    return to_obj(0LL); //## WHAT SHOULD I RETURN HERE?
  }
  
  Obj *values = keys + size;
  Obj value = values[idx];
  
  //add_ref(value);
  
  found = true;
  return value;
}

Obj ext_lookup(Obj map_or_tag_obj, Obj key)
{
  Obj map = is_tag_obj(map_or_tag_obj) ? get_inner_obj(map_or_tag_obj) : map_or_tag_obj;
  Map *ptr = get_map_ptr(map);
  int size = ptr->size;
  Obj *keys = ptr->buffer;
  Obj *values = keys + size;
  for (int i=0 ; i < size ; i++)
    if (keys[i] == key)
      return values[i];
  fail();
}

Obj ext_lookup(Obj map_or_tag_obj, Obj key, bool &found)
{
  Obj map = is_tag_obj(map_or_tag_obj) ? get_inner_obj(map_or_tag_obj) : map_or_tag_obj;
  if (map != empty_map)
  {
    Map *ptr = get_map_ptr(map);
    int size = ptr->size;
    Obj *keys = ptr->buffer;
    Obj *values = keys + size;
    for (int i=0 ; i < size ; i++)
      if (keys[i] == key)
      {
        found = true;
        return values[i];
      }
  }
  found = false;
  return to_obj(0LL); //## WHAT SHOULD I RETURN HERE?
}


Obj merge_sets(Obj sets)
{
  Obj merge_sets_impl(Obj);
  return merge_sets_impl(sets);
}

Obj merge_maps(Obj maps)
{
  Obj merge_maps_impl(Obj);
  return merge_maps_impl(maps);
}


Obj seq_to_set(Obj obj)
{
  if (obj == empty_seq)
    return empty_set;
  
  Seq *seq = get_seq_ptr(obj);
  int len = seq->length;
  Obj *seq_elems = seq->elems;
  
  Obj *elems = new_obj_array(len);
  for (int i=0 ; i < len ; i++)
  {
    Obj elem = seq_elems[i];
    add_ref(elem);
    elems[i] = elem;  
  }
  
  Obj res = make_set(elems, len);
  
  delete_obj_array(elems, len);
  
  return res;
}


Obj seq_to_mset(Obj seq_obj)
{
  if (seq_obj == empty_seq)
    return empty_map;

  Seq *seq = get_seq_ptr(seq_obj);
  int len = seq->length;
  Obj *elems = seq->elems;
  
  int *idxs = new_int_array(len);
  Obj *counters = new_obj_array(len); //## WHY OBJECTS AND NOT INTEGERS?

  int n = sort_group_and_count(elems, len, idxs, counters);
  
  Map *res = new_map(n);
  Obj *keys = res->buffer;
  Obj *values = keys + n;
  
  for (int i=0 ; i < n ; i++)
  {
    Obj obj = elems[idxs[i]];
    add_ref(obj);
    keys[i] = obj;
    values[i] = counters[i];
  }
  
  delete_int_array(idxs, len);
  delete_obj_array(counters, len);
  
  return make_obj(res);
}

Obj list_to_seq(Obj list)
{
  if (list == empty_seq)
    return empty_seq;

  int len = 0;
  for (Obj tail=list ; tail != empty_seq ; tail=at(tail, 1))
    len++;
  
  Seq *seq = new_full_seq(len);
  
  Obj *elems = seq->elems;
  Obj tail = list;
  
  for (int i=0 ; i < len ; i++)
  {
    elems[i] = get_at(tail, 0);
    tail = at(tail, 1);
  }
  
  return make_obj(seq);
}

Obj internal_sort(Obj set)
{
  if (set == empty_set)
    return empty_seq;

  Set *s = get_set_ptr(set);
  int size = s->size;
  Obj *src = s->elems;
  
  Seq *seq = new_full_seq(size);
  Obj *dest = seq->elems;
  for (int i=0 ; i < size ; i++)
    dest[i] = src[i];
  vec_add_ref(dest, size);
  
  return make_obj(seq);
}

static std::map<Obj, Obj> attachments_map;

Obj add_attachment(Obj target, Obj data)
{
  std::map<Obj, Obj>::iterator it = attachments_map.find(target);
  if (it == attachments_map.end())
  {
    add_ref(target);
    add_ref(data);
    Obj set = make_set(&data, 1);
    attachments_map[target] = set;
    add_obj_to_cache(target);
    add_obj_to_cache(set);
  }
  else
  {
    Obj curr_data_set = it->second;

    Obj sets[2];
    sets[0] = curr_data_set;
    add_ref(data);
    sets[1] = make_set(&data, 1);
    Obj set_of_sets = make_set(sets, 2);
    Obj new_data_set = merge_sets(set_of_sets);
    release(set_of_sets);

    // The current data set cannot be released because it's still in the list of cached object to release
    // release(curr_data_set);
    attachments_map[target] = new_data_set;
    add_obj_to_cache(new_data_set);
  }
  add_ref(target);
  return target;
}

Obj fetch_attachments(Obj obj)
{
  std::map<Obj, Obj>::iterator it = attachments_map.find(obj);
  if (it == attachments_map.end())
    return empty_set;
  Obj res = it->second;
  add_ref(res);
  return res;
}

void get_set_iter(SetIter &it, Obj set)
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

void get_seq_iter(SeqIter &it, Obj seq)
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

void get_map_iter(MapIter &it, Obj map)
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

void move_forward(SetIter &it)
{
  assert(!is_out_of_range(it));
  it.idx++;
}

void move_forward(SeqIter &it)
{
  assert(!is_out_of_range(it));
  it.idx++;
}

void move_forward(MapIter &it)
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

void runtime_check(Obj cond)
{
  if (cond != generated::S_true)
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
