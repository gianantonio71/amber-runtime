#include "lib.h"


Obj make_symb(int idx)
{
  return ((idx + 1) << 4) | 1;
}

int get_symb_idx(Obj obj)
{
  assert(is_symb(obj));
  
  return (obj >> 4) - 1;
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

Obj make_obj(Set *ptr)
{
  return int(ptr) | type_tag_set;
}

Obj make_obj(Seq *ptr)
{
  return int(ptr) | type_tag_seq;
}

Obj make_obj(Map *ptr)
{
  return int(ptr) | type_tag_map;
}

Obj make_obj(TagObj *ptr)
{
  return int(ptr) | type_tag_tag_obj;
}

////////////////////////////////////////////////////////////////////////////////

void *get_ptr(Obj obj)
{
  // assert(((void *) (obj & ~0xF)) == 0 || is_alive((void *) (obj & ~0xF)));

  return (void *) (obj & ~0xF);
}

RefObj *get_ref_obj_ptr(Obj obj)
{
  assert(is_ref_obj(obj));
  
  return (RefObj *) get_ptr(obj);
}

Set *get_set_ptr(Obj obj)
{
  assert(is_ne_set(obj));
  
  return (Set *) get_ptr(obj);
}

Seq *get_seq_ptr(Obj obj)
{
  assert(is_ne_seq(obj));
  
  return (Seq *) get_ptr(obj);
}

Map *get_map_ptr(Obj obj)
{
  assert(is_ne_map(obj));
  
  return (Map *) get_ptr(obj);
}

TagObj *get_tag_obj_ptr(Obj obj)
{
  assert(is_tag_obj(obj));
  
  return (TagObj *) get_ptr(obj);
}

////////////////////////////////////////////////////////////////////////////////

TypeTag get_type_tag(Obj obj)
{
  assert(!is_int(obj));

  return TypeTag(obj & 0xF);
}

////////////////////////////////////////////////////////////////////////////////

bool is_symb(Obj obj)
{
  return (obj & 0xF) == type_tag_symb;
}

bool is_int(Obj obj)
{
  return (obj & 1) == 0;
}

bool is_empty_collection(Obj obj)
{
  return (obj >> 4) == 0;
}

bool is_ne_seq(Obj obj)
{
  return (obj & 0xF) == type_tag_seq;
}

bool is_ne_set(Obj obj)
{
  return (obj & 0xF) == type_tag_set;
}

bool is_ne_map(Obj obj)
{
  return (obj & 0xF) == type_tag_map;
}

bool is_tag_obj(Obj obj)
{
  return (obj & 0xF) == type_tag_tag_obj;
}

bool is_inline_obj(Obj obj)
{
  return is_int(obj) | is_symb(obj) | is_empty_collection(obj);
}

bool is_ref_obj(Obj obj)
{
  return !is_inline_obj(obj);
}