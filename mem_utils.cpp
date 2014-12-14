#include "lib.h"


ShortTypeTag get_short_type_tag(Obj obj)
{
  return ShortTypeTag(obj & SHORT_TAG_MASK);
}

TypeTag get_full_type_tag(Obj obj)
{
  return TypeTag(obj & FULL_TAG_MASK);
}

////////////////////////////////////////////////////////////////////////////////

Obj make_symb(int idx)
{
  return symb(idx);
}

int get_symb_idx(Obj obj)
{
  assert(is_symb(obj));
  
  return obj >> FULL_TAG_SIZE;
}

////////////////////////////////////////////////////////////////////////////////

Obj *get_key_array_ptr(Map *map)
{
  return map->buffer;
}

Obj *get_value_array_ptr(Map *map)
{
  return map->buffer + map->size;
}

////////////////////////////////////////////////////////////////////////////////

long long pack_ptr(void *ptr)
{
  return ((long long) ptr) << POINTER_SHIFT;
}

// void *get_ptr(Obj obj)
// {
//   // assert(((void *) (obj & ~0xF)) == 0 || is_alive((void *) (obj & ~0xF)));
//   return (void *) (obj & ~0xF);
// }

void *get_ptr(Obj obj)
{
  return (void *) (obj >> POINTER_SHIFT);
}

////////////////////////////////////////////////////////////////////////////////

Obj make_obj(Set *ptr)
{
  return pack_ptr(ptr) | type_tag_set;
}

Obj make_obj(Seq *ptr)
{
  return pack_ptr(ptr) | type_tag_seq;
}

Obj make_obj(Map *ptr)
{
  return pack_ptr(ptr) | type_tag_map;
}

Obj make_obj(TagObj *ptr)
{
  return pack_ptr(ptr) | type_tag_tag_obj;
}

Obj make_obj(Float *ptr)
{
  return pack_ptr(ptr) | type_tag_float;
}

////////////////////////////////////////////////////////////////////////////////

RefObj *get_ref_obj_ptr(Obj obj)
{
  assert(is_ref_obj(obj)); //## WHY IS THIS ONLY ASSERTED AND NOT CHECKED?
  
  return (RefObj *) get_ptr(obj);
}

////////////////////////////////////////////////////////////////////////////////

Set *get_set_ptr(Obj obj)
{
  fail_if_not(is_ne_set(obj), "Object is not a non-empty set");

  return (Set *) get_ptr(obj);
}

Seq *get_seq_ptr(Obj obj)
{
  fail_if_not(is_ne_seq(obj), "Object is not a non-empty sequence");
  
  return (Seq *) get_ptr(obj);
}

Map *get_map_ptr(Obj obj)
{
  fail_if_not(is_ne_map(obj), "Object is not a non-empty map");
  
  return (Map *) get_ptr(obj);
}

TagObj *get_tag_obj_ptr(Obj obj)
{
  fail_if_not(is_tag_obj(obj), "Object is not a named object");
  
  return (TagObj *) get_ptr(obj);
}

Float *get_float_ptr(Obj obj)
{
  fail_if_not(is_float(obj), "Object is not a floating point number");

  return (Float *) get_ptr(obj);
}

////////////////////////////////////////////////////////////////////////////////

bool is_symb(Obj obj)
{
  return get_full_type_tag(obj) == type_tag_symb;
}

bool is_int(Obj obj)
{
  return get_short_type_tag(obj) == short_type_tag_integer;
}

bool is_float(Obj obj)
{
  return get_full_type_tag(obj) == type_tag_float;
}

bool is_ne_seq(Obj obj)
{
  return get_full_type_tag(obj) == type_tag_seq;
}

bool is_ne_set(Obj obj)
{
  return get_full_type_tag(obj) == type_tag_set;
}

bool is_ne_map(Obj obj)
{
  return get_full_type_tag(obj) == type_tag_map;
}

bool is_tag_obj(Obj obj)
{
  return get_full_type_tag(obj) == type_tag_tag_obj;
}

bool is_inline_obj(Obj obj)
{
  return !is_ref_obj(obj);
}

bool is_ref_obj(Obj obj)
{
  return get_short_type_tag(obj) == short_type_tag_reference;
}

////////////////////////////////////////////////////////////////////////////////

bool is_seq(Obj obj)
{
  return obj == empty_seq | is_ne_seq(obj);
}

bool is_set(Obj obj)
{
  return obj == empty_set | is_ne_set(obj);
}

bool is_map(Obj obj)
{
  return obj == empty_map | is_ne_map(obj);
}
