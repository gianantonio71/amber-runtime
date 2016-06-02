#include "lib.h"

#include <algorithm>


uint32 find_obj(OBJ *sorted_array, uint32 len, OBJ obj, bool &found) // The array mustn't contain duplicates
{
  if (len > 0)
  {
    int64 low_idx = 0;
    int64 high_idx = len - 1;
    
    while (low_idx <= high_idx)
    {
      int64 middle_idx = (low_idx + high_idx) / 2;
      OBJ middle_obj = sorted_array[middle_idx];

      int cr = comp_objs(obj, middle_obj);

      if (cr == 0)
      {
        found = true;
        return middle_idx;
      }

      if (cr > 0)
        high_idx = middle_idx - 1;
      else
        low_idx = middle_idx + 1;
    }
  }

  found = false;
  return -1;
}

struct obj_less {
  bool operator () (OBJ obj1, OBJ obj2) {
    return comp_objs(obj1, obj2) > 0;
  }
};

struct obj_inline_less {
  bool operator () (OBJ obj1, OBJ obj2) {
    return shallow_cmp(obj1, obj2) > 0;
  }
};


uint32 sort_and_release_dups(OBJ *objs, uint32 size)
{
  if (size < 2)
    return size;

  uint32 low_idx = 0;
  uint32 high_idx = size - 1; // size is greater than 0 (actually 1) here, so this is always non-negative (actually positive)
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

    OBJ tmp = objs[low_idx];
    objs[low_idx] = objs[high_idx];
    objs[high_idx] = tmp;
  }

  uint32 inline_count = is_inline_obj(objs[low_idx]) ? low_idx + 1 : low_idx;

  uint32 idx = 0;
  if (inline_count > 0)
  {
    std::sort(objs, objs+inline_count, obj_inline_less());

    OBJ last_obj = objs[0];
    for (uint32 i=1 ; i < inline_count ; i++)
    {
      OBJ next_obj = objs[i];
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

  for (uint32 i=inline_count+1 ; i < size ; i++)
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
  OBJ *objs;

  obj_idx_less(OBJ *objs) : objs(objs) {}

  bool operator () (uint32 idx1, uint32 idx2)
  {
    return comp_objs(objs[idx1], objs[idx2]) > 0;
  }
};

uint32 sort_group_and_count(OBJ *objs, uint32 len, uint32 *idxs, OBJ *counters)
{
  assert(len > 0);
  
  for (uint32 i=0 ; i < len ; i++)
    idxs[i] = i;
  
  std::sort(idxs, idxs+len, obj_idx_less(objs));
  
  uint32 n = 0;
  
  for (uint32 i=0 ; i < len ; )
  {
    assert(i >= n);
    
    uint32 j = i + 1;
    while (j < len && comp_objs(objs[idxs[i]], objs[idxs[j]]) == 0)
      j++;
    
    uint32 count = j - i;
    
    idxs[n] = idxs[i];
    counters[n] = make_int(count);
    n++;
    
    i = j;
  }
  
  return n;
}


void sort_and_check_no_dups(OBJ *keys, OBJ *values, uint32 size)
{
  if (size < 2)
    return;
  
  uint32 *idxs = new_uint32_array(size);
  for (uint32 i=0 ; i < size ; i++)
    idxs[i] = i;
  
  std::sort(idxs, idxs+size, obj_idx_less(keys));

  for (uint32 i=0 ; i < size ; i++)
    if (idxs[i] != i)
    {
      OBJ key = keys[i];
      OBJ value = values[i];
      
      for (uint32 j = i ; ; )
      {
        uint32 k = idxs[j];
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
  
  delete_uint32_array(idxs, size);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Returns:   > 0     if obj1 < obj2
//              0     if obj1 = obj2
//            < 0     if obj1 > obj2

int comp_objs(OBJ obj1, OBJ obj2)
{
  if (are_shallow_eq(obj1, obj2))
    return 0;

  bool is_inline_1 = is_inline_obj(obj1);
  bool is_inline_2 = is_inline_obj(obj2);

  if (is_inline_1)
    if (is_inline_2)
      return shallow_cmp(obj1, obj2);
    else
      return 1;
  else if (is_inline_2)
    return -1;

  OBJ_TYPE type1 = get_logical_type(obj1);
  OBJ_TYPE type2 = get_logical_type(obj2);

  if (type1 != type2)
    return type2 - type1;

  uint32 count = 0;
  OBJ *elems1 = 0;
  OBJ *elems2 = 0;

  switch (type1)
  {
    case TYPE_SEQUENCE:
    {
      uint32 len1 = get_seq_length(obj1);
      uint32 len2 = get_seq_length(obj2);
      if (len1 != len2)
        return len2 - len1;
      count = len1;
      elems1 = get_seq_buffer_ptr(obj1);
      elems2 = get_seq_buffer_ptr(obj2);
      break;
    }

    case TYPE_SET:
    {
      SET_OBJ *set1 = get_set_ptr(obj1);
      SET_OBJ *set2 = get_set_ptr(obj2);
      uint32 size1 = set1->size;
      uint32 size2 = set2->size;
      if (size1 != size2)
        return size2 - size1;
      count = size1;
      elems1 = set1->buffer;
      elems2 = set2->buffer;
      break;
    }

    case TYPE_MAP:
    {
      MAP_OBJ *map1 = get_map_ptr(obj1);
      MAP_OBJ *map2 = get_map_ptr(obj2);
      uint32 size1 = map1->size;
      uint32 size2 = map2->size;
      if (size1 != size2)
        return size2 - size1;
      count = 2 * size1;
      elems1 = map1->buffer;
      elems2 = map2->buffer;
      break;
    }

    case TYPE_TAG_OBJ:
    {
      uint16 tag_idx_1 = get_tag_idx(obj1);
      uint16 tag_idx_2 = get_tag_idx(obj2);
      if (tag_idx_1 != tag_idx_2)
        return tag_idx_2 - tag_idx_1;
      return comp_objs(get_inner_obj(obj1), get_inner_obj(obj2));
    }

    default:
      internal_fail();
  }

  for (uint32 i=0 ; i < count ; i++)
  {
    int cr = comp_objs(elems1[i], elems2[i]);
    if (cr != 0)
      return cr;
  }

  return 0;
}
