#include "lib.h"
#include <cstdlib>

using std::malloc;


void init_new_slots(OBJ *slots, uint32 first, uint32 last)
{
  for (uint32 i=first ; i < last ; i++)
  {
    OBJ *slot = slots + i;
    slot->core_data.int_ = i + 1;
    slot->extra_data.word = 0; // Should not be necessary
    assert(slot->extra_data.std.type == TYPE_BLANK_OBJ);
  }
  OBJ *last_slot = slots + last;
  last_slot->core_data.int_ = -1;
  last_slot->extra_data.word = 0; // Should not be necessary
  assert(last_slot->extra_data.std.type == TYPE_BLANK_OBJ);
}

////////////////////////////////////////////////////////////////////////////////

void value_store_init(VALUE_STORE *store)
{
  const uint32 INIT_SIZE = 256;
  store->slots = (OBJ *) malloc(INIT_SIZE * sizeof(OBJ));
  store->size = INIT_SIZE;
  store->used = 0;
  store->first_free = 0;
  init_new_slots(store->slots, 0, INIT_SIZE-1);
}

void value_store_cleanup(VALUE_STORE *store)
{
  OBJ *slots = store->slots;
  uint32 size = store->size;
  for (uint32 i = 0 ; i < size ; i++)
  {
    OBJ slot = slots[i];
    if (slot.extra_data.std.type != TYPE_BLANK_OBJ)
      release(slot);
  }
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
  uint32 size = store->size;
  OBJ *slots = store->slots;
  for (uint32 i=0 ; i < size ; i++)
  {
    OBJ curr_value = slots[i];
    if (curr_value.extra_data.std.type != TYPE_BLANK_OBJ && comp_objs(value, curr_value) == 0)
      return i;
  }
  return -1;
}

int64 insert_value(VALUE_STORE *store, OBJ value)
{
  OBJ *slots = store->slots;
  uint32 first_free = store->first_free;

  if (first_free == -1)
  {
    // Reallocating the object table
    assert(store->used == store->size);

    uint32 curr_size = store->size;
    uint32 new_size = 2 * curr_size;
    OBJ *slots = (OBJ *) realloc(slots, new_size * sizeof(OBJ));

    init_new_slots(slots, curr_size, new_size-1);

    first_free = curr_size;

    store->slots = slots;
    store->size = new_size;
    store->first_free = first_free;
  }

  uint32 next_free = slots[first_free].core_data.int_;
  slots[first_free] = value;
  store->first_free = next_free;
  store->used++;
  return first_free;
}
