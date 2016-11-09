#include "lib.h"
#include <string.h>


const uint32 INIT_SIZE = 256;

uint32 calc_capacity(uint32 min_capacity)
{
  uint32 capacity = INIT_SIZE;
  while (capacity < min_capacity)
    capacity *= 2;
  return capacity;
}

////////////////////////////////////////////////////////////////////////////////

void init_new_slots(OBJ *slots, uint32 start, uint32 end)
{
  for (uint32 i=start ; i < end ; i++)
  {
    OBJ *slot = slots + i;
    slot->core_data.int_ = i + 1;
    slot->extra_data.word = 0; // Should not be necessary
    assert(slot->extra_data.std.type == TYPE_BLANK_OBJ);
  }
}

////////////////////////////////////////////////////////////////////////////////

void value_store_init(VALUE_STORE *store)
{
  store->slots = new_obj_array(INIT_SIZE);
  store->capacity = INIT_SIZE;
  store->usage = 0;
  store->first_free = 0;
  init_new_slots(store->slots, 0, INIT_SIZE);
}

void value_store_cleanup(VALUE_STORE *store)
{
  OBJ *slots = store->slots;
  uint32 capacity = store->capacity;
  for (uint32 i=0 ; i < capacity ; i++)
    release(slots[i]);
  delete_obj_array(slots, capacity);
}

////////////////////////////////////////////////////////////////////////////////

void value_store_updates_init(VALUE_STORE *store, VALUE_STORE_UPDATES *updates)
{
  updates->slots = store->slots;
  updates->first_free = store->first_free;
  updates->capacity = store->capacity;
}

void value_store_updates_cleanup(VALUE_STORE_UPDATES *updates)
{

}

////////////////////////////////////////////////////////////////////////////////

uint32 value_store_insert(VALUE_STORE_UPDATES *updates, OBJ value)
{
  uint32 first_free = updates->first_free;
  uint32 capacity = updates->capacity;

  updates->values.push_back(value);
  updates->indexes.push_back(first_free);

  if (first_free < capacity)
    updates->first_free = updates->slots[first_free].core_data.int_;
  else
    updates->first_free = first_free + 1;

  return first_free;
}

////////////////////////////////////////////////////////////////////////////////

void value_store_copy(VALUE_STORE *store, VALUE_STORE_UPDATES *updates)
{

}

void value_store_apply(VALUE_STORE *store, VALUE_STORE_UPDATES *updates)
{
  OBJ *slots = store->slots;

  vector<OBJ> &values = updates->values;
  vector<uint32> &indexes = updates->indexes;

  uint32 count = values.size();
  uint32 usage = store->usage;
  uint32 capacity = store->capacity;
  uint32 new_usage = usage + count;

  if (capacity < new_usage)
  {
    uint32 new_capacity = calc_capacity(new_usage);
    OBJ *new_slots = new_obj_array(new_capacity);
    memcpy(new_slots, slots, capacity * sizeof(OBJ));
    init_new_slots(new_slots, new_usage, new_capacity);
    delete_obj_array(slots, capacity);
    slots = new_slots;
    capacity = new_capacity;
    store->slots = slots;
    store->capacity = capacity;
  }

  for (int i=0 ; i < count ; i++)
  {
    OBJ value = values[i];
    uint32 idx = indexes[i];
    slots[idx] = copy_obj(value);
  }

  store->usage = new_usage;
  store->first_free = updates->first_free;
}

////////////////////////////////////////////////////////////////////////////////

OBJ lookup_surrogate(VALUE_STORE *store, int64 surr)
{
  OBJ value = store->slots[surr];
  add_ref(value);
  return value;
}

int64 lookup_value(VALUE_STORE *store, OBJ value)
{
  uint32 capacity = store->capacity;
  OBJ *slots = store->slots;
  for (uint32 i=0 ; i < capacity ; i++)
  {
    OBJ curr_value = slots[i];
    if (curr_value.extra_data.std.type != TYPE_BLANK_OBJ && comp_objs(value, curr_value) == 0)
      return i;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////

int64 lookup_value_ex(VALUE_STORE *store, VALUE_STORE_UPDATES *updates, OBJ value)
{
  int64 idx = lookup_value(store, value);
  if (idx != -1)
    return idx;

  vector<OBJ> &values = updates->values;
  uint32 count = values.size();
  for (int i=0 ; i < count ; i++)
    if (comp_objs(value, values[i]) == 0)
      return updates->indexes[i];
  return -1;
}
