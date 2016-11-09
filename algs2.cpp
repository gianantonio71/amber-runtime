#include "lib.h"

#include <queue>


class obj_tag
{
public:
  obj_tag(OBJ obj, uint32 tag) : obj(obj), tag(tag)
  {

  }

  OBJ obj;
  uint32 tag;
};


bool operator < (const obj_tag &lhs, const obj_tag &rhs)
{
  return comp_objs(lhs.obj, rhs.obj) < 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ merge_sets_impl(OBJ set_of_sets)
{
  if (is_empty_set(set_of_sets))
    return make_empty_set();

  SET_OBJ *set_of_sets_ptr = get_set_ptr(set_of_sets);
  uint32 size = set_of_sets_ptr->size;

  assert(size > 0);

  OBJ *sets = set_of_sets_ptr->buffer;

  for (uint32 i=1 ; i < size ; i++)
    assert(!is_empty_set(sets[i]));

  // Skipping the empty set (if any), which must be the first one in the array
  if (is_empty_set(sets[0]))
  {
    sets++;
    size--;
  }

  //## HERE I SHOULD CHECK THAT ALL THE SETS THAT ARE LEFT ARE NOT EMPTY

  if (size == 0)
    return make_empty_set();

  if (size == 1)
  {
    OBJ res = sets[0];
    add_ref(res);
    return res;
  }

  // If we are here, it means that there are at least two non-empty sets to merge

  uint64 elem_count = 0;                            // Total number of elements in all the sets
  uint32 *sizes = new_uint32_array(size);           // Sizes of sets
  OBJ **elem_arrays = (OBJ **) new_ptr_array(size); // Pointers to the elements in each of the sets
  for (uint32 i=0 ; i < size ; i++)
  {
    SET_OBJ *set_ptr = get_set_ptr(sets[i]);
    assert(set_ptr != 0);
    uint32 array_size = set_ptr->size;
    sizes[i] = array_size;
    elem_arrays[i] = set_ptr->buffer;
    elem_count += array_size;
  }

  // Checking that we are not exceeding the maximum set size
  //## THE ACTUAL SIZE OF THE RESULTING SET COULD BE LOWER, IF SOME ELEMENTS ARE EQUAL
  if (elem_count > 0xFFFFFFFF)
    impl_fail("Maximum set size exceeded");

  // Creating and initializing the priority queue
  // by inserting the first element of each set/array
  std::priority_queue<obj_tag> pq;
  for (uint32 i=0 ; i < size ; i++)
  {
    assert(sizes[i] > 0);
    obj_tag ot(elem_arrays[i][0], i);
    pq.push(ot);
  }

  // Cursors for each of the <elem_arrays> defined above.
  // We immediately initialize them to 1, as we've already
  // taken the first object from each of the arrays
  uint32 *idxs = new_uint32_array(size);
  for (uint32 i=0 ; i < size ; i++)
    idxs[i] = 1;

  // Tentatively allocating the output object (it may have
  // to be reallocated if there are duplicates elements
  // in the input, so that the size of the union is lower
  // then the sums of the sizes of the inputs) and
  // initializing its cursor
  SET_OBJ *res_set = new_set(elem_count);
  OBJ *dest_array = res_set->buffer;
  uint32 dest_idx = 0;

  // Main loop: popping elements from the priority queue,
  // storing them in the output array, and inserting the
  // next element (if there is one) from the same array
  // the popped value was from
  for (uint32 done=0 ; done < elem_count ; done++)
  {
    // Popping the value (obj) and the index of its array (array_idx)
    obj_tag ot = pq.top();
    pq.pop();
    uint32 array_idx = ot.tag;
    // If the current element is different from the previous one
    // we store it in the output array and update its counter
    if (dest_idx == 0 or not are_eq(ot.obj, dest_array[dest_idx-1]))
      dest_array[dest_idx++] = ot.obj;

    uint32 src_idx = idxs[array_idx];
    uint32 src_size = sizes[array_idx];
    OBJ *src_array = elem_arrays[array_idx];
    // If the array that contained the value just popped
    // has more elements, we take the next one and store
    // it in the priority queue
    if (src_idx < src_size)
    {
      OBJ new_obj = src_array[src_idx];
      idxs[array_idx] = src_idx + 1;
      obj_tag ot(new_obj, array_idx);
      pq.push(ot);
    }
  }

  // Reallocating the array if it turns out that its
  // final size is lower than the maximum one
  if (elem_count != dest_idx)
    res_set = shrink_set(res_set, dest_idx);

  // Releasing the temporary arrays
  delete_uint32_array(sizes, size);
  delete_uint32_array(idxs, size);
  delete_ptr_array((void **) elem_arrays, size);

  // Calling add_ref() for all the elements of the new array
  vec_add_ref(res_set->buffer, dest_idx);

  return make_set(res_set);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ merge_maps_impl(OBJ set_of_maps)
{
  if (is_empty_set(set_of_maps))
    return make_empty_map();

  SET_OBJ *set_of_maps_ptr = get_set_ptr(set_of_maps);
  uint32 size = set_of_maps_ptr->size;

  assert(size > 0);

  OBJ *maps = set_of_maps_ptr->buffer;

  for (uint32 i=1 ; i < size ; i++)
    assert(!is_empty_map(maps[i]));

  // Skipping the empty map (if any), which can only be the first item in the set
  if (is_empty_map(maps[0]))
  {
    maps++;
    size--;
  }

  //## HERE I SHOULD CHECK THAT ALL REMAINING MAPS ARE NOT EMPTY

  if (size == 0)
    return make_empty_map();

  if (size == 1)
  {
    OBJ res = maps[0];
    add_ref(res);
    return res;
  }

  // If we are here, it means that there are at least two non-empty maps to merge

  uint64 pair_count = 0;                              // Total number of key/value pairs in all the maps
  uint32 *sizes = new_uint32_array(size);             // Sizes of all maps
  OBJ **key_arrays = (OBJ **) new_ptr_array(size);    // Pointers to the key arrays in each of the maps
  OBJ **value_arrays = (OBJ **) new_ptr_array(size);  // Pointers to the value arrays in each of the maps
  for (uint32 i=0 ; i < size ; i++)
  {
    MAP_OBJ *map_ptr = get_map_ptr(maps[i]);
    assert(map_ptr != 0);
    uint32 map_size = map_ptr->size;
    assert(map_size > 0);
    sizes[i] = map_size;
    key_arrays[i] = get_key_array_ptr(map_ptr);
    value_arrays[i] = get_value_array_ptr(map_ptr);
    pair_count += map_size;
  }

  // Checking that we are not exceeding the maximum map size
  //## THE ACTUAL SIZE OF THE RESULTING MAP COULD BE LOWER, IF SOME KEY/VALUE PAIRS ARE EQUAL
  if (pair_count > 0xFFFFFFFF)
    impl_fail("Maximum map size exceeded");

  // Creating and initializing the priority queue
  // by inserting the first key of each map
  std::priority_queue<obj_tag> pq;
  for (uint32 i=0 ; i < size ; i++)
  {
    assert(sizes[i] > 0);
    obj_tag ot(key_arrays[i][0], i);
    pq.push(ot);
  }

  // Cursors for each of the <key/value_arrays> defined above.
  // We immediately initialize them to 1, as we've already
  // taken the first object from each of the arrays
  uint32 *idxs = new_uint32_array(size);
  for (uint32 i=0 ; i < size ; i++)
    idxs[i] = 1;

  // Allocating the output object and initializing its cursor
  MAP_OBJ *res_map = new_map(pair_count);
  OBJ *dest_key_array = get_key_array_ptr(res_map);
  OBJ *dest_value_array = get_value_array_ptr(res_map);
  uint32 dest_idx = 0;

  // Main loop: popping elements from the priority queue,
  // storing them in the output array, and inserting the
  // next element (if there is one) from the same array
  // the popped value was from
  for (uint32 done=0 ; done < pair_count ; done++)
  {
    // Popping the key (obj) and the index of its array (array_idx)
    obj_tag ot = pq.top();
    pq.pop();
    uint32 array_idx = ot.tag;
    uint32 src_idx = idxs[array_idx];
    OBJ key = ot.obj;
    OBJ value = value_arrays[array_idx][src_idx-1];

    // Checking that this key is not a duplicate
    if (dest_idx > 0 && are_eq(key, dest_key_array[dest_idx-1]))
      soft_fail("_merge_(): Maps have common keys");

    // Storing key and value in the target arrays and updating the cursor
    dest_key_array[dest_idx] = key;
    dest_value_array[dest_idx] = value;
    dest_idx++;

    uint32 src_size = sizes[array_idx];
    OBJ *src_key_array = key_arrays[array_idx];
    // If the map that contained the key just popped has more elements,
    // we take the key of the next one and store it in the priority queue
    if (src_idx < src_size)
    {
      OBJ new_key = src_key_array[src_idx];
      idxs[array_idx] = src_idx + 1;
      obj_tag ot(new_key, array_idx);
      pq.push(ot);
    }
  }

  // Releasing the temporary arrays
  delete_uint32_array(sizes, size);
  delete_uint32_array(idxs, size);
  delete_ptr_array((void **) key_arrays, size);
  delete_ptr_array((void **) value_arrays, size);

  // Calling add_ref() for all the elements of the new array
  vec_add_ref(dest_key_array, pair_count);
  vec_add_ref(dest_value_array, pair_count);

  return make_map(res_map);
}
