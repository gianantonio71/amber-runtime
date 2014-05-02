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
  
  Seq *seq = new_seq(length);
  
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
  
  TagObj *tag_obj = new_tag_obj();
  
  tag_obj->tag = tag;
  tag_obj->obj = obj;
  
  return make_obj(tag_obj);
}

Obj make_array(int size, Obj value)
{
  if (size == 0)
    return empty_seq;

  Seq *seq = new_seq(size);
  
  for (int i=0 ; i < size ; i++)
    seq->elems[i] = value;
  
  mult_add_ref(value, size);
  
  return make_obj(seq);
}

Obj get_seq_slice(Obj seq, int idx_first, int len)
{
  assert(idx_first >= 0 && len >= 0);
  assert(seq == empty_seq || is_ne_seq(seq));
  
  if (len == 0)
    return empty_seq;
  
  if (!is_ne_seq(seq))
    fail();
    
  Seq *s = get_seq_ptr(seq);

  int right_bound = idx_first + len;
  
  if (right_bound > s->length)
  {
    print(seq);
    print(idx_first);
    print(len);
    fail();
  }
  
  Obj *elems = s->elems + idx_first;
  
  vec_add_ref(elems, len);
  
  return make_seq(elems, len);
}

Obj join_seqs(Obj left, Obj right)
{
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
  
  int left_len = left_ptr->length;
  int right_len = right_ptr->length;
  
  Obj *left_elems = left_ptr->elems;
  Obj *right_elems = right_ptr->elems;
  
  vec_add_ref(left_elems, left_len);
  vec_add_ref(right_elems, right_len);
  
  int len = left_len + right_len;
  
  Seq *seq = new_seq(len);
  
  for (int i=0 ; i < left_len ; i++)
    seq->elems[i] = left_elems[i];
  for (int i=0 ; i < right_len ; i++)
    seq->elems[i+left_len] = right_elems[i];
  
  return make_obj(seq);
}

Obj rev_seq(Obj seq)
{
  if (seq == empty_seq)
    return empty_seq;
  
  Seq *s = get_seq_ptr(seq);
  
  int len = s->length;
  Obj *elems = s->elems;
  
  vec_add_ref(elems, len);
  
  Seq *rs = new_seq(len);
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
  if (seq == empty_seq)
    fail();

  Seq *s = get_seq_ptr(seq);
  
  if (idx >= s->length)
    fail();
  
  Obj *elems = s->elems;
  release(elems[idx]);
  elems[idx] = value;
}

Obj lookup(Obj map, Obj key)
{
  if (map == empty_map)
    fail();

  if (!is_ne_map(map))
    print(map);

  Map *m = get_map_ptr(map);
  int size = m->size;
  Obj *keys = m->buffer;
  
  int idx = find_obj(keys, size, key);
  if (idx == -1)
    fail();
  
  Obj *values = keys + size;
  Obj value = values[idx];

  //add_ref(value);
    
  return value;
}

Obj lookup(Obj map, Obj key, bool &found)
{
  if (map == empty_map)
  {
    found = false;
    return to_obj(0); //## WHAT SHOULD I RETURN HERE?
  }

  Map *m = get_map_ptr(map);
  int size = m->size;
  Obj *keys = m->buffer;
  
  int idx = find_obj(keys, size, key);
  if (idx == -1)
  {
    found = false;
    return to_obj(0); //## WHAT SHOULD I RETURN HERE?
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
  return lookup(map, key);
}

Obj ext_lookup(Obj map_or_tag_obj, Obj key, bool &found)
{
  Obj map = is_tag_obj(map_or_tag_obj) ? get_inner_obj(map_or_tag_obj) : map_or_tag_obj;
  return lookup(map, key, found);
}

Obj merge_maps(Obj map1, Obj map2)
{
  if (map1 == empty_map)
  {
    add_ref(map2);
    return map2;
  }

  if (map2 == empty_map)
  {
    add_ref(map1);
    return map1;
  }

  Map *m1 = get_map_ptr(map1);
  Map *m2 = get_map_ptr(map2);
  
  int size1 = m1->size;
  int size2 = m2->size;
  
  Obj *ks1 = m1->buffer;
  Obj *ks2 = m2->buffer;
  
  Obj *vs1 = ks1 + size1;
  Obj *vs2 = ks2 + size2;
  
  int size = size1 + size2;
  Map *m = new_map(size);
  Obj *ks = m->buffer;
  Obj *vs = ks + size;
  
  //int i=0, j=0;
  //for ( ; ; )
  //{
  //
  //}


  //## BAD BAD BAD VERY INEFFICIENT
  for (int i=0 ; i < size1 ; i++)
  {
    ks[i] = ks1[i];
    vs[i] = vs1[i];
  }
  for (int i=0 ; i < size2 ; i++)
  {
    ks[size1+i] = ks2[i];
    vs[size1+i] = vs2[i];
  }
  sort_and_check_no_dups(ks, vs, size);
  vec_add_ref(m->buffer, 2 * size);
  
  
  return make_obj(m);
}

Obj seq_to_set(Obj obj)
{
  assert(obj == empty_seq || is_ne_seq(obj));

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
  assert(seq_obj == empty_seq || is_ne_seq(seq_obj));
  
  if (seq_obj == empty_seq)
    return empty_map;

  Seq *seq = get_seq_ptr(seq_obj);
  int len = seq->length;
  Obj *elems = seq->elems;
  
  int *idxs = new_int_array(len);
  Obj *counters = new_obj_array(len);

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
  delete_int_array(counters, len);
  
  return make_obj(res);
}

Obj list_to_seq(Obj list)
{
  if (list == empty_seq)
    return empty_seq;

  assert(is_ne_seq(list));

  int len = 0;
  for (Obj tail=list ; tail != empty_seq ; tail=at(tail, 1))
    len++;
  
  Seq *seq = new_seq(len);
  
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
  
  Seq *seq = new_seq(size);
  Obj *dest = seq->elems;
  for (int i=0 ; i < size ; i++)
    dest[i] = src[i];
  vec_add_ref(dest, size);
  
  return make_obj(seq);
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
  //print_all_live_objs();
  throw;
}
