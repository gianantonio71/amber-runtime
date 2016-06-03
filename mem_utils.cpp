#include "lib.h"


SHORT_TYPE_TAG get_short_type_tag(OBJ obj)
{
  return SHORT_TYPE_TAG(obj & SHORT_TAG_MASK);
}

TYPE_TAG get_full_type_tag(OBJ obj)
{
  return TYPE_TAG(obj & FULL_TAG_MASK);
}

////////////////////////////////////////////////////////////////////////////////

OBJ make_symb(int idx)
{
  return symb(idx);
}

int get_symb_idx(OBJ obj)
{
  assert(is_symb(obj));
  
  return obj >> FULL_TAG_SIZE;
}

////////////////////////////////////////////////////////////////////////////////

OBJ *get_key_array_ptr(MAP_OBJ *map)
{
  return map->buffer;
}

OBJ *get_value_array_ptr(MAP_OBJ *map)
{
  return map->buffer + map->size;
}

////////////////////////////////////////////////////////////////////////////////

long long pack_ptr(void *ptr)
{
  return ((long long) ptr) << POINTER_SHIFT;
}

// void *get_ptr(OBJ obj)
// {
//   // assert(((void *) (obj & ~0xF)) == 0 || is_alive((void *) (obj & ~0xF)));
//   return (void *) (obj & ~0xF);
// }

void *get_ptr(OBJ obj)
{
  return (void *) (obj >> POINTER_SHIFT);
}

////////////////////////////////////////////////////////////////////////////////

OBJ make_obj(SET_OBJ *ptr)
{
  return pack_ptr(ptr) | type_tag_set;
}

OBJ make_obj(SEQ_OBJ *ptr)
{
  return pack_ptr(ptr) | type_tag_seq;
}

OBJ make_obj(MAP_OBJ *ptr)
{
  return pack_ptr(ptr) | type_tag_map;
}

OBJ make_obj(TAG_OBJ *ptr)
{
  return pack_ptr(ptr) | type_tag_tag_obj;
}

OBJ make_obj(FLOAT *ptr)
{
  return pack_ptr(ptr) | type_tag_float;
}

////////////////////////////////////////////////////////////////////////////////

REF_OBJ *get_ref_obj_ptr(OBJ obj)
{
  assert(is_ref_obj(obj)); //## WHY IS THIS ONLY ASSERTED AND NOT CHECKED?
  
  return (REF_OBJ *) get_ptr(obj);
}

////////////////////////////////////////////////////////////////////////////////

SET_OBJ *get_set_ptr(OBJ obj)
{
  fail_if_not(is_ne_set(obj), "Object is not a non-empty set");

  return (SET_OBJ *) get_ptr(obj);
}

SEQ_OBJ *get_seq_ptr(OBJ obj)
{
  fail_if_not(is_ne_seq(obj), "Object is not a non-empty sequence");
  
  return (SEQ_OBJ *) get_ptr(obj);
}

FULL_SEQ_OBJ *get_full_seq_ptr(OBJ obj)
{
  //## BUG: HERE CHECK THAT THE SEQUENCE IS ACTUALLY A FULL SEQUENCE
  return (FULL_SEQ_OBJ *) get_seq_ptr(obj);
}

REF_SEQ_OBJ *get_ref_seq_ptr(OBJ obj)
{
  //## BUG: HERE CHECK THAT THE SEQUENCE IS ACTUALLY A REFERENCE SEQUENCE
  return (REF_SEQ_OBJ *) get_seq_ptr(obj);
}

MAP_OBJ *get_map_ptr(OBJ obj)
{
  fail_if_not(is_ne_map(obj), "Object is not a non-empty map");
  
  return (MAP_OBJ *) get_ptr(obj);
}

TAG_OBJ *get_tag_obj_ptr(OBJ obj)
{
  fail_if_not(is_tag_obj(obj), "Object is not a named object");
  
  return (TAG_OBJ *) get_ptr(obj);
}

FLOAT *get_float_ptr(OBJ obj)
{
  fail_if_not(is_float(obj), "Object is not a floating point number");

  return (FLOAT *) get_ptr(obj);
}

////////////////////////////////////////////////////////////////////////////////

bool is_full_seq(SEQ_OBJ *seq)
{
  FULL_SEQ_OBJ *full_seq = (FULL_SEQ_OBJ *) seq;
  return full_seq->elems == full_seq->buffer;
}

bool is_ref_seq(SEQ_OBJ *seq)
{
  return !is_full_seq(seq);
}

////////////////////////////////////////////////////////////////////////////////

bool is_symb(OBJ obj)
{
  return get_full_type_tag(obj) == type_tag_symb;
}

bool is_int(OBJ obj)
{
  return get_short_type_tag(obj) == short_type_tag_integer;
}

bool is_float(OBJ obj)
{
  return get_full_type_tag(obj) == type_tag_float;
}

bool is_ne_seq(OBJ obj)
{
  return get_full_type_tag(obj) == type_tag_seq;
}

bool is_ne_set(OBJ obj)
{
  return get_full_type_tag(obj) == type_tag_set;
}

bool is_ne_map(OBJ obj)
{
  return get_full_type_tag(obj) == type_tag_map;
}

bool is_tag_obj(OBJ obj)
{
  return get_full_type_tag(obj) == type_tag_tag_obj;
}

bool is_inline_obj(OBJ obj)
{
  return !is_ref_obj(obj);
}

bool is_ref_obj(OBJ obj)
{
  return get_short_type_tag(obj) == short_type_tag_reference;
}

////////////////////////////////////////////////////////////////////////////////

bool is_seq(OBJ obj)
{
  return obj == empty_seq | is_ne_seq(obj);
}

bool is_set(OBJ obj)
{
  return obj == empty_set | is_ne_set(obj);
}

bool is_map(OBJ obj)
{
  return obj == empty_map | is_ne_map(obj);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ make_blank_obj()
{
  return blank_obj;
}

OBJ make_null_obj()
{
  return null_obj;
}

OBJ make_empty_seq()
{
  return empty_seq;
}

OBJ make_empty_set()
{
  return empty_set;
}

OBJ make_empty_map()
{
  return empty_map;
}

bool is_symb(OBJ obj, uint16 symb_idx)
{
  return obj == make_symb(symb_idx);
}

bool is_int(OBJ obj, int64 value)
{
  return obj == make_int(value);
}

bool is_empty_seq(OBJ obj)
{
  return obj == empty_seq;
}

bool is_empty_set(OBJ obj)
{
  return obj == empty_set;
}

bool is_empty_map(OBJ obj)
{
  return obj == empty_map;
}

bool is_blank_obj(OBJ obj)
{
  return obj == blank_obj;
}

bool is_null_obj(OBJ obj)
{
  return obj == null_obj;
}

bool get_bool(OBJ obj)
{
  assert(obj == symb(symb_idx_false) | obj == symb(symb_idx_true));
  return obj == symb(symb_idx_true);
}

uint32 get_seq_length(OBJ obj)
{
  if (obj == empty_seq)
    return 0;
  return get_seq_ptr(obj)->length;
}

SEQ_OBJ *new_seq(uint32 len)
{
  return new_full_seq(len);
}

OBJ *get_seq_buffer_ptr(OBJ obj)
{
  return get_seq_ptr(obj)->elems;
}

OBJ make_seq(SEQ_OBJ *seq, uint32 len)
{
  assert(seq->length == len);
  return make_obj(seq);
}