#include <stddef.h>

#include "lib.h"


OBJ_TYPE get_type(OBJ obj)
{
  uint8 type = obj.extra_data.std.type;
  return (OBJ_TYPE) type;
}

OBJ_TYPE get_high_level_type(OBJ obj)
{
  OBJ_TYPE type = get_type(obj);
  if (type == TYPE_SLICE)
    return TYPE_SEQUENCE;
  else
    return type;
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

OBJ make_blank_obj()
{
  OBJ obj;
  obj.core_data.int_ = 0;
  obj.extra_data.word = 0;
  obj.extra_data.std.type = TYPE_BLANK_OBJ;
  return obj;
}

OBJ make_null_obj()
{
  OBJ obj;
  obj.core_data.int_ = 0;
  obj.extra_data.word = 0;
  obj.extra_data.std.type = TYPE_NULL_OBJ;
  return obj;
}

OBJ make_symb(uint16 symb_idx)
{
  OBJ obj;
  obj.core_data.int_ = symb_idx;
  obj.extra_data.std.tags[0] = 0;
  obj.extra_data.std.tags[1] = 0;
  obj.extra_data.std.tags[2] = 0;
  obj.extra_data.std.tag_count = 0;
  obj.extra_data.std.type = TYPE_SYMBOL;
  return obj;
}

OBJ make_bool(bool b)
{
  return make_symb(b ? symb_idx_true : symb_idx_false);
}

OBJ make_int(uint64 value)
{
  OBJ obj;
  obj.core_data.int_ = value;
  obj.extra_data.std.tags[0] = 0;
  obj.extra_data.std.tags[1] = 0;
  obj.extra_data.std.tags[2] = 0;
  obj.extra_data.std.tag_count = 0;
  obj.extra_data.std.type = TYPE_INTEGER;
  return obj;
}

OBJ make_float(double value)
{
  OBJ obj;
  obj.core_data.float_ = value;
  obj.extra_data.std.tags[0] = 0;
  obj.extra_data.std.tags[1] = 0;
  obj.extra_data.std.tags[2] = 0;
  obj.extra_data.std.tag_count = 0;
  obj.extra_data.std.type = TYPE_INTEGER;
  return obj;
}

OBJ make_seq(SEQ_OBJ *ptr, uint32 length)
{
  assert(length == 0 || (ptr != NULL & length <= ptr->size));

  OBJ obj;
  obj.core_data.ptr = length > 0 ? ptr->buffer : NULL;
  obj.extra_data.seq.length = length;
  obj.extra_data.seq.tag = 0;
  obj.extra_data.seq.has_tag = 0;
  obj.extra_data.seq.type = TYPE_SEQUENCE;

  if (length > 0 && get_seq_obj_ptr(obj) != ptr)
  {
    SEQ_OBJ *bad_ptr = get_seq_obj_ptr(obj);
    internal_fail();
  }

  return obj;
}

OBJ make_slice(SEQ_OBJ *ptr, uint32 offset, uint32 length)
{
  assert(ptr != NULL & ((uint64) offset) + ((uint64) length) <= ptr->size);

  // If the offset is 0, then we can just create a normal sequence
  // If length is 0, we must create an empty sequence, which is a normal sequence
  if (offset == 0 | length == 0)
    return make_seq(ptr, length);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer + offset;
  obj.extra_data.slice.length = length;
  obj.extra_data.slice.offset = offset;
  obj.extra_data.slice.type = TYPE_SLICE;

  if (length > 0 && get_seq_obj_ptr(obj) != ptr)
  {
    SEQ_OBJ *bad_ptr = get_seq_obj_ptr(obj);
    internal_fail();
  }

  return obj;
}

OBJ make_empty_seq()
{
  return make_seq(NULL, 0);
}

OBJ make_set(SET_OBJ *ptr)
{
  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data.std.tags[0] = 0;
  obj.extra_data.std.tags[1] = 0;
  obj.extra_data.std.tags[2] = 0;
  obj.extra_data.std.tag_count = 0;
  obj.extra_data.std.type = TYPE_SET;
  return obj;
}

OBJ make_empty_set()
{
  return make_set(NULL);
}

OBJ make_map(MAP_OBJ *ptr)
{
  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data.std.tags[0] = 0;
  obj.extra_data.std.tags[1] = 0;
  obj.extra_data.std.tags[2] = 0;
  obj.extra_data.std.tag_count = 0;
  obj.extra_data.std.type = TYPE_MAP;
  return obj;
}

OBJ make_empty_map()
{
  return make_map(NULL);
}

OBJ make_tag_obj(TAG_OBJ *ptr)
{
  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data.std.tags[0] = 0;
  obj.extra_data.std.tags[1] = 0;
  obj.extra_data.std.tags[2] = 0;
  obj.extra_data.std.tag_count = 0;
  obj.extra_data.std.type = TYPE_TAG_OBJ;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

uint16 get_symb_idx(OBJ obj)
{
  assert(is_symb(obj));
  return obj.core_data.int_;
}

bool get_bool(OBJ obj)
{
  assert(is_bool(obj));
  return obj.core_data.int_ == symb_idx_true;
}

int64 get_int(OBJ obj)
{
  assert(is_int(obj));
  return obj.core_data.int_;
}

double get_float(OBJ obj)
{
  assert(is_float(obj));
  return obj.core_data.float_;
}

uint32 get_seq_length(OBJ seq)
{
  assert(is_seq(seq));
  // The length field should overlap for sequences and slices,
  // there should be no need to check the specific type of sequence
  assert(seq.extra_data.seq.length == seq.extra_data.slice.length);
  return seq.extra_data.seq.length;
}

uint32 get_seq_offset(OBJ seq)
{
  assert(is_seq(seq));
  return get_type(seq) == TYPE_SLICE ? seq.extra_data.slice.offset : 0;
}

uint16 get_tag_idx(OBJ obj)
{
  assert(is_tag_obj(obj));
  return get_tag_obj_ptr(obj)->tag_idx;
}

////////////////////////////////////////////////////////////////////////////////

REF_OBJ *get_ref_obj_ptr(OBJ obj)
{
  assert(is_ref_obj(obj)); //## WHY IS THIS ONLY ASSERTED AND NOT CHECKED?

  if (is_seq(obj))
    return (REF_OBJ *) get_seq_obj_ptr(obj);
  else
    return (REF_OBJ *) obj.core_data.ptr;
}

////////////////////////////////////////////////////////////////////////////////

OBJ *get_seq_buffer_ptr(OBJ obj)
{
  fail_if_not(is_ne_seq(obj), "Object is not a non-empty sequence");

  return (OBJ *) obj.core_data.ptr;
}

SEQ_OBJ *get_seq_obj_ptr(OBJ obj)
{
  fail_if_not(is_ne_seq(obj), "Object is not a non-empty sequence");

  static const uint32 buff_offset = offsetof(SEQ_OBJ, buffer);

  char *buffer_ptr = (char *) obj.core_data.ptr;
  uint32 slice_offset = get_type(obj) == TYPE_SLICE ? obj.extra_data.slice.offset : 0;
  return (SEQ_OBJ *) (buffer_ptr - (buff_offset + slice_offset * sizeof(OBJ)));
}

////////////////////////////////////////////////////////////////////////////////

SET_OBJ *get_set_ptr(OBJ obj)
{
  fail_if_not(is_ne_set(obj), "Object is not a non-empty set");

  return (SET_OBJ *) obj.core_data.ptr;
}

MAP_OBJ *get_map_ptr(OBJ obj)
{
  fail_if_not(is_ne_map(obj), "Object is not a non-empty map");
  
  return (MAP_OBJ *) obj.core_data.ptr;
}

TAG_OBJ *get_tag_obj_ptr(OBJ obj)
{
  fail_if_not(is_tag_obj(obj), "Object is not a named object");
  
  return (TAG_OBJ *) obj.core_data.ptr;
}

////////////////////////////////////////////////////////////////////////////////

bool is_blank_obj(OBJ obj)
{
  return get_type(obj) == TYPE_BLANK_OBJ;
}

bool is_null_obj(OBJ obj)
{
  return get_type(obj) == TYPE_NULL_OBJ;
}

bool is_symb(OBJ obj)
{
  return get_type(obj) == TYPE_SYMBOL;
}

bool is_bool(OBJ obj)
{
  return get_type(obj) == TYPE_SYMBOL & (obj.core_data.int_ == symb_idx_false or obj.core_data.int_ == symb_idx_true);
}

bool is_int(OBJ obj)
{
  return get_type(obj) == TYPE_INTEGER;
}

bool is_float(OBJ obj)
{
  return get_type(obj) == TYPE_FLOAT;
}

bool is_seq(OBJ obj)
{
  return get_type(obj) == TYPE_SEQUENCE | get_type(obj) == TYPE_SLICE;
}

bool is_empty_seq(OBJ obj)
{
  return get_type(obj) == TYPE_SEQUENCE & obj.core_data.ptr == NULL;
}

bool is_ne_seq(OBJ obj)
{
  return (get_type(obj) == TYPE_SEQUENCE & obj.core_data.ptr != NULL) | get_type(obj) == TYPE_SLICE;
}

bool is_set(OBJ obj)
{
  return get_type(obj) == TYPE_SET;
}

bool is_empty_set(OBJ obj)
{
  return get_type(obj) == TYPE_SET & obj.core_data.ptr == NULL;
}

bool is_ne_set(OBJ obj)
{
  return get_type(obj) == TYPE_SET & obj.core_data.ptr != NULL;
}

bool is_map(OBJ obj)
{
  return get_type(obj) == TYPE_MAP;
}

bool is_empty_map(OBJ obj)
{
  return get_type(obj) == TYPE_MAP & obj.core_data.ptr == NULL;
}

bool is_ne_map(OBJ obj)
{
  return get_type(obj) == TYPE_MAP & obj.core_data.ptr != NULL;
}

bool is_tag_obj(OBJ obj)
{
  return get_type(obj) == TYPE_TAG_OBJ;
}

////////////////////////////////////////////////////////////////////////////////

bool is_symb(OBJ obj, uint16 symb_idx)
{
  return get_type(obj) == TYPE_SYMBOL & obj.core_data.int_ == symb_idx;
}

bool is_int(OBJ obj, int64 n)
{
  return get_type(obj) == TYPE_INTEGER & obj.core_data.int_ == n;
}


////////////////////////////////////////////////////////////////////////////////

bool is_inline_obj(OBJ obj)
{
  return get_type(obj) <= MAX_INLINE_OBJ_TYPE_VALUE | obj.core_data.ptr == NULL;
}

bool is_ref_obj(OBJ obj)
{
  return !is_inline_obj(obj);
}

////////////////////////////////////////////////////////////////////////////////

bool are_shallow_eq(OBJ obj1, OBJ obj2)
{
  return obj1.core_data.int_ == obj2.core_data.int_ && obj1.extra_data.word == obj2.extra_data.word;
}

int shallow_cmp(OBJ obj1, OBJ obj2)
{
  assert(is_inline_obj(obj1) & is_inline_obj(obj2));

  int64 extra_data_1 = obj1.extra_data.word;
  int64 extra_data_2 = obj2.extra_data.word;

  int64 diff;
  if (extra_data_1 != extra_data_2)
    diff = extra_data_2 - extra_data_1;
  else
    diff = obj2.core_data.int_ - obj1.core_data.int_;

  return (diff > 0) - (diff < 0);
}
