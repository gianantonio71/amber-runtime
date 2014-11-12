#include "lib.h"

#include <algorithm>


int find_obj(Obj *sorted_array, int len, Obj obj) // The array mustn't contain duplicates
{
  int low_idx = 0;
  int high_idx = len - 1;
  
  while (low_idx <= high_idx)
  {
    int middle_idx = (low_idx + high_idx) / 2;
    Obj middle_obj = sorted_array[middle_idx];
    
    int cr = comp_objs(obj, middle_obj);
    
    if (cr == 0)
      return middle_idx;
    
    if (cr > 0)
      high_idx = middle_idx - 1;
    else
      low_idx = middle_idx + 1;
  }
  
  return -1;
}

struct obj_less {
  bool operator () (Obj obj1, Obj obj2) {
    return comp_objs(obj1, obj2) > 0;
  }
};


int sort_and_release_dups(Obj *objs, int size)
{
  if (size < 2)
    return size;

  int low_idx = 0;
  int high_idx = size - 1;
  for ( ; ; )
  {
    // Advancing the lower cursor to the next non-inline object
    while (low_idx < high_idx & is_inline_obj(objs[low_idx]))
      low_idx++;

    // Advancing the upper cursor to the next inline object
    while (high_idx > low_idx & not is_inline_obj(objs[high_idx]))
      high_idx--;

    if (low_idx == high_idx)
      break;

    Obj tmp = objs[low_idx];
    objs[low_idx] = objs[high_idx];
    objs[high_idx] = tmp;
  }

  int inline_count = is_inline_obj(objs[low_idx]) ? low_idx + 1 : low_idx;

  int idx = 0;
  if (inline_count > 0)
  {
    std::sort(objs, objs+inline_count);

    Obj last_obj = objs[0];
    for (int i=1 ; i < inline_count ; i++)
    {
      Obj next_obj = objs[i];
      if (!inline_eq(last_obj, next_obj))
      {
        idx++;
        last_obj = next_obj;
        assert(idx <= i);
        if (idx != i)
          objs[idx] = next_obj;
      }
    }

    idx++;
    if (inline_count == size)
      return idx;
  }

  std::sort(objs+inline_count, objs+size, obj_less());

  if (idx != inline_count)
    objs[idx] = objs[inline_count];

  for (int i=inline_count+1 ; i < size ; i++)
    // if (are_eq(objs[idx], objs[i]))
    if (comp_objs(objs[idx], objs[i]) == 0)
      release(objs[i]);
    else
    {
      idx++;
      assert(idx <= i);
      if (idx != i)
        objs[idx] = objs[i];
    }

  return idx + 1;
}


struct obj_idx_less
{
  Obj *objs;

  obj_idx_less(Obj *objs) : objs(objs) {}

  bool operator () (int idx1, int idx2)
  {
    return comp_objs(objs[idx1], objs[idx2]) > 0;
  }
};

int sort_group_and_count(Obj *objs, int len, int *idxs, Obj *counters)
{
  assert(len > 0);
  
  for (int i=0 ; i < len ; i++)
    idxs[i] = i;
  
  std::sort(idxs, idxs+len, obj_idx_less(objs));
  
  int n = 0;
  
  for (int i=0 ; i < len ; )
  {
    assert(i >= n);
    
    int j = i + 1;
    while (j < len && comp_objs(objs[idxs[i]], objs[idxs[j]]) == 0)
      j++;
    
    int count = j - i;
    
    idxs[n] = idxs[i];
    counters[n] = to_obj(count);
    n++;
    
    i = j;
  }
  
  return n;
}


void sort_and_check_no_dups(Obj *keys, Obj *values, int size)
{
  if (size < 2)
    return;
  
  int *idxs = new_int_array(size);
  for (int i=0 ; i < size ; i++)
    idxs[i] = i;
  
  std::sort(idxs, idxs+size, obj_idx_less(keys));

  for (int i=0 ; i < size ; i++)
    if (idxs[i] != i)
    {
      Obj key = keys[i];
      Obj value = values[i];
      
      for (int j = i ; ; )
      {
        int k = idxs[j];
        idxs[j] = j;
        
        if (k == i)
        {
          keys[j]   = key;
          values[j] = value;
          break;
        }
        else
        {
          keys[j]   = keys[k];
          values[j] = values[k];
          j = k;
        }
      }
    }
  
  delete_int_array(idxs, size);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Returns:   > 0     if obj1 < obj2
//              0     if obj1 = obj2
//            < 0     if obj1 > obj2

int comp_objs(Obj obj1, Obj obj2)
{
  if (obj1 == obj2)
    return 0;

  bool is_inline_1 = is_inline_obj(obj1);
  bool is_inline_2 = is_inline_obj(obj2);

  if (is_inline_1)
    if (is_inline_2)
      return obj2 - obj1;
    else
      return 1;
  else if (is_inline_2)
    return -1;

  int tag1 = get_full_type_tag(obj1);
  int tag2 = get_full_type_tag(obj2);

  if (tag1 != tag2)
    return tag2 - tag1;

  int count = -1;
  Obj *elems1 = 0;
  Obj *elems2 = 0;

  switch (tag1)
  {
    case type_tag_set:
    {
      Set *set1 = get_set_ptr(obj1);
      Set *set2 = get_set_ptr(obj2);
      int size1 = set1->size;
      int size2 = set2->size;
      if (size1 != size2)
        return size2 - size1;
      count = size1;
      elems1 = set1->elems;
      elems2 = set2->elems;
      break;
    }

    case type_tag_seq:
    {
      Seq *seq1 = get_seq_ptr(obj1);
      Seq *seq2 = get_seq_ptr(obj2);
      int len1 = seq1->length;
      int len2 = seq2->length;
      if (len1 != len2)
        return len2 - len1;
      count = len1;
      elems1 = seq1->elems;
      elems2 = seq2->elems;
      break;
    }

    case type_tag_map:
    {
      Map *map1 = get_map_ptr(obj1);
      Map *map2 = get_map_ptr(obj2);
      int size1 = map1->size;
      int size2 = map2->size;
      if (size1 != size2)
        return size2 - size1;
      count = 2 * size1;
      elems1 = map1->buffer;
      elems2 = map2->buffer;
      break;
    }

    case type_tag_tag_obj:
    {
      TagObj *tag_obj1 = get_tag_obj_ptr(obj1);
      TagObj *tag_obj2 = get_tag_obj_ptr(obj2);
      Obj tag1 = tag_obj1->tag;
      Obj tag2 = tag_obj2->tag;
      if (tag1 != tag2)
        return tag2 - tag1;
      return comp_objs(tag_obj1->obj, tag_obj2->obj);
    }

    default:
      internal_fail();
  }

  for (int i=0 ; i < count ; i++)
  {
    int cr = comp_objs(elems1[i], elems2[i]);
    if (cr != 0)
      return cr;
  }

  return 0;
}
