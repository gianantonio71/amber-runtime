#include <stddef.h>

#include "lib.h"


const uint32 SEQ_BUFFER_FIELD_OFFSET = offsetof(SEQ_OBJ, buffer);

////////////////////////////////////////////////////////////////////////////////

OBJ_TYPE get_physical_type(OBJ obj)
{
  uint8 type = obj.extra_data.std.type;
  return (OBJ_TYPE) type;
}

OBJ_TYPE get_logical_type(OBJ obj)
{
  OBJ_TYPE type = get_physical_type(obj);

  if (type == TYPE_SLICE)
    return TYPE_SEQUENCE;

  if (obj.extra_data.std.num_tags > 0)
    return TYPE_TAG_OBJ;

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

const uint64 SYMBOL_BASE_MASK   = ((uint64) (TYPE_SYMBOL              )) << 56;
const uint64 INTEGER_MASK       = ((uint64) (TYPE_INTEGER             )) << 56;
const uint64 FLOAT_MASK         = ((uint64) (TYPE_FLOAT               )) << 56;
const uint64 EMPTY_SEQ_MASK     = ((uint64) (TYPE_SEQUENCE            )) << 56;
const uint64 NE_SEQ_BASE_MASK   = ((uint64) (TYPE_SEQUENCE  | (1 << 4))) << 56;
const uint64 EMPTY_SET_MASK     = ((uint64) (TYPE_SET                 )) << 56;
const uint64 NE_SET_MASK        = ((uint64) (TYPE_SET       | (1 << 4))) << 56;
const uint64 EMPTY_MAP_MASK     = ((uint64) (TYPE_MAP                 )) << 56;
const uint64 NE_MAP_MASK        = ((uint64) (TYPE_MAP       | (1 << 4))) << 56;
const uint64 TAG_OBJ_MASK       = ((uint64) (TYPE_TAG_OBJ   | (1 << 4))) << 56;

const int HAS_REF_BIT_SHIFT = 60;

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
  obj.core_data.int_ = 0;
  obj.extra_data.word = symb_idx | SYMBOL_BASE_MASK;

  assert(obj.extra_data.std.symb_idx    == symb_idx);
  assert(obj.extra_data.std.inner_tag   == 0);
  assert(obj.extra_data.std.tag         == 0);
  assert(obj.extra_data.std.unused_byte == 0);
  assert(obj.extra_data.std.type        == TYPE_SYMBOL);
  assert(obj.extra_data.std.has_ref     == 0);
  assert(obj.extra_data.std.num_tags    == 0);
  assert(obj.extra_data.std.unused_flag == 0);
  assert(is_symb(obj, symb_idx));

  return obj;
}

OBJ make_bool(bool b)
{
  OBJ obj;
  obj.core_data.int_ = 0;
  obj.extra_data.word = (b ? symb_idx_true : symb_idx_false) | SYMBOL_BASE_MASK;

  assert(obj.extra_data.std.symb_idx    == (b ? symb_idx_true : symb_idx_false));
  assert(obj.extra_data.std.inner_tag   == 0);
  assert(obj.extra_data.std.tag         == 0);
  assert(obj.extra_data.std.unused_byte == 0);
  assert(obj.extra_data.std.type        == TYPE_SYMBOL);
  assert(obj.extra_data.std.has_ref     == 0);
  assert(obj.extra_data.std.num_tags    == 0);
  assert(obj.extra_data.std.unused_flag == 0);
  assert(is_symb(obj, (b ? symb_idx_true : symb_idx_false)));

  return obj;
}

OBJ make_int(uint64 value)
{
  OBJ obj;
  obj.core_data.int_ = value;
  obj.extra_data.word = INTEGER_MASK;

  assert(obj.extra_data.std.symb_idx    == 0);
  assert(obj.extra_data.std.inner_tag   == 0);
  assert(obj.extra_data.std.tag         == 0);
  assert(obj.extra_data.std.unused_byte == 0);
  assert(obj.extra_data.std.type        == TYPE_INTEGER);
  assert(obj.extra_data.std.has_ref     == 0);
  assert(obj.extra_data.std.num_tags    == 0);
  assert(obj.extra_data.std.unused_flag == 0);
  assert(is_int(obj, value));

  return obj;
}

OBJ make_float(double value)
{
  OBJ obj;
  obj.core_data.float_ = value;
  obj.extra_data.word = FLOAT_MASK;

  assert(obj.extra_data.std.symb_idx    == 0);
  assert(obj.extra_data.std.inner_tag   == 0);
  assert(obj.extra_data.std.tag         == 0);
  assert(obj.extra_data.std.unused_byte == 0);
  assert(obj.extra_data.std.type        == TYPE_FLOAT);
  assert(obj.extra_data.std.has_ref     == 0);
  assert(obj.extra_data.std.num_tags    == 0);
  assert(obj.extra_data.std.unused_flag == 0);

  return obj;
}

OBJ make_seq(SEQ_OBJ *ptr, uint32 length)
{
  assert(length == 0 || (ptr != NULL & length <= ptr->size));

  OBJ obj;
  if (length == 0)
  {
    obj.core_data.ptr = NULL;
    obj.extra_data.word = EMPTY_SEQ_MASK;
  }
  else
  {
    obj.core_data.ptr = ptr->buffer;
    obj.extra_data.word = length | NE_SEQ_BASE_MASK;
  }

  assert(obj.extra_data.seq.length      == length);
  assert(obj.extra_data.seq.tag         == 0);
  assert(obj.extra_data.seq.unused_byte == 0);
  assert(obj.extra_data.seq.type        == TYPE_SEQUENCE);
  assert(obj.extra_data.seq.has_ref     == length > 0);
  assert(obj.extra_data.seq.num_tags    == 0);
  assert(obj.extra_data.seq.unused_flag == 0);

  assert(length == 0 || get_seq_obj_ptr(obj) == ptr);

  return obj;
}

OBJ make_slice(SEQ_OBJ *ptr, uint32 offset, uint32 length)
{
  assert(ptr != NULL & ((uint64) offset) + ((uint64) length) <= ptr->size);

  // If the offset is 0, then we can just create a normal sequence
  // If the length is 0, we must create an empty sequence, which is again a normal sequence
  if (offset == 0 | length == 0)
    return make_seq(ptr, length);

  //## BUG BUG BUG: FIX FIX FIX
  if (offset > 0xFFFFFF)
    hard_fail("Currently subsequences cannot start at offsets greater that 2^24-1");

  OBJ obj;
  obj.core_data.ptr = ptr->buffer + offset;
  obj.extra_data.slice.length = length;
  obj.extra_data.slice.offset = offset;
  obj.extra_data.slice.type = TYPE_SLICE;
  obj.extra_data.slice.has_ref = 1;
  obj.extra_data.slice.num_tags = 0;
  obj.extra_data.slice.unused_flag = 0;

  assert(length == 0 || get_seq_obj_ptr(obj) == ptr);

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
  obj.extra_data.word = ptr == NULL ? EMPTY_SET_MASK : NE_SET_MASK;

  assert(obj.extra_data.std.symb_idx    == 0);
  assert(obj.extra_data.std.inner_tag   == 0);
  assert(obj.extra_data.std.tag         == 0);
  assert(obj.extra_data.std.unused_byte == 0);
  assert(obj.extra_data.std.type        == TYPE_SET);
  assert(obj.extra_data.std.has_ref     == (ptr != NULL));
  assert(obj.extra_data.std.num_tags    == 0);
  assert(obj.extra_data.std.unused_flag == 0);

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
  obj.extra_data.word = ptr == NULL ? EMPTY_MAP_MASK : NE_MAP_MASK;

  assert(obj.extra_data.std.symb_idx    == 0);
  assert(obj.extra_data.std.inner_tag   == 0);
  assert(obj.extra_data.std.tag         == 0);
  assert(obj.extra_data.std.unused_byte == 0);
  assert(obj.extra_data.std.type        == TYPE_MAP);
  assert(obj.extra_data.std.has_ref     == (ptr != NULL));
  assert(obj.extra_data.std.num_tags    == 0);
  assert(obj.extra_data.std.unused_flag == 0);

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
  obj.extra_data.word = TAG_OBJ_MASK;

  assert(obj.extra_data.std.symb_idx    == 0);
  assert(obj.extra_data.std.inner_tag   == 0);
  assert(obj.extra_data.std.tag         == 0);
  assert(obj.extra_data.std.unused_byte == 0);
  assert(obj.extra_data.std.type        == TYPE_TAG_OBJ);
  assert(obj.extra_data.std.has_ref     == 1);
  assert(obj.extra_data.std.num_tags    == 0);
  assert(obj.extra_data.std.unused_flag == 0);

  return obj;
}

OBJ make_ref_tag_obj(uint16 tag_idx, OBJ obj)
{
  TAG_OBJ *tag_obj = new_tag_obj();
  tag_obj->tag_idx = tag_idx;
  tag_obj->obj = obj;
  return make_tag_obj(tag_obj);
}

OBJ make_tag_obj(uint16 tag_idx, OBJ obj)
{
  OBJ_TYPE type = get_physical_type(obj);

  if (type == TYPE_SEQUENCE)
  {
    if (obj.extra_data.seq.num_tags == 0)
    {
      obj.extra_data.seq.tag = tag_idx;
      obj.extra_data.seq.num_tags = 1;
      return obj;
    }
  }
  else if (type != TYPE_SLICE)
  {
    uint8 tags_count = obj.extra_data.std.num_tags;
    if (tags_count < 2)
    {
      obj.extra_data.std.inner_tag = obj.extra_data.std.tag;
      obj.extra_data.std.tag = tag_idx;
      obj.extra_data.std.num_tags = tags_count + 1;
      return obj;
    }
  }

  return make_ref_tag_obj(tag_idx, obj);
}

////////////////////////////////////////////////////////////////////////////////

uint16 get_symb_idx(OBJ obj)
{
  assert(is_symb(obj));
  return obj.extra_data.std.symb_idx;
}

bool get_bool(OBJ obj)
{
  assert(is_bool(obj));
  return obj.extra_data.std.symb_idx == symb_idx_true;
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
  return get_physical_type(seq) == TYPE_SLICE ? seq.extra_data.slice.offset : 0;
}

uint16 get_tag_idx(OBJ obj)
{
  assert(is_tag_obj(obj));
  assert(get_physical_type(obj) != TYPE_SLICE);

  if (obj.extra_data.std.num_tags != 0)
    return obj.extra_data.std.tag;
  else
    return ((TAG_OBJ *) obj.core_data.ptr)->tag_idx;
}

OBJ get_inner_obj(OBJ obj)
{
  assert(is_tag_obj(obj));

  OBJ_TYPE type = get_physical_type(obj);
  assert(type != TYPE_SLICE);

  if (type == TYPE_SEQUENCE)
  {
    assert(obj.extra_data.seq.num_tags == 1);
    obj.extra_data.seq.tag = 0;
    obj.extra_data.seq.num_tags = 0;
    return obj;
  }

  uint8 tags_count = obj.extra_data.std.num_tags;
  if (tags_count > 0)
  {
    obj.extra_data.std.tag = obj.extra_data.std.inner_tag;
    obj.extra_data.std.inner_tag = 0;
    obj.extra_data.std.num_tags = tags_count - 1;
    return obj;
  }

  return ((TAG_OBJ *) obj.core_data.ptr)->obj;
}

////////////////////////////////////////////////////////////////////////////////

OBJ *get_seq_buffer_ptr(OBJ obj)
{
  assert(is_ne_seq(obj));

  return (OBJ *) obj.core_data.ptr;
}

SEQ_OBJ *get_seq_obj_ptr(OBJ obj)
{
  assert(is_ne_seq(obj));

  char *buffer_ptr = (char *) obj.core_data.ptr;
  uint32 slice_offset = get_physical_type(obj) == TYPE_SLICE ? obj.extra_data.slice.offset : 0;
  return (SEQ_OBJ *) (buffer_ptr - (SEQ_BUFFER_FIELD_OFFSET + slice_offset * sizeof(OBJ)));
}

////////////////////////////////////////////////////////////////////////////////

SET_OBJ *get_set_ptr(OBJ obj)
{
  assert(is_ne_set(obj));

  return (SET_OBJ *) obj.core_data.ptr;
}

MAP_OBJ *get_map_ptr(OBJ obj)
{
  assert(is_ne_map(obj));
  
  return (MAP_OBJ *) obj.core_data.ptr;
}

////////////////////////////////////////////////////////////////////////////////

bool is_blank_obj(OBJ obj)
{
  return get_physical_type(obj) == TYPE_BLANK_OBJ;
}

bool is_null_obj(OBJ obj)
{
  return get_physical_type(obj) == TYPE_NULL_OBJ;
}

bool is_symb(OBJ obj)
{
  return (obj.extra_data.word >> 16) == (SYMBOL_BASE_MASK >> 16);
}

bool is_bool(OBJ obj)
{
  assert(symb_idx_false == 0 & symb_idx_true == 1); // The correctness of the body depends on this assumption
  return (obj.extra_data.word >> 1) == (SYMBOL_BASE_MASK >> 1);
}

bool is_int(OBJ obj)
{
  return obj.extra_data.word == INTEGER_MASK;
}

bool is_float(OBJ obj)
{
  return obj.extra_data.word == FLOAT_MASK;
}

bool is_seq(OBJ obj)
{
  OBJ_TYPE physical_type = get_physical_type(obj);
  return (physical_type == TYPE_SEQUENCE & obj.extra_data.seq.num_tags == 0) | physical_type == TYPE_SLICE;
}

bool is_empty_seq(OBJ obj)
{
  return obj.extra_data.word == EMPTY_SEQ_MASK;
}

bool is_ne_seq(OBJ obj)
{
  return ((obj.extra_data.word >> 32) == (NE_SEQ_BASE_MASK >> 32)) | (get_physical_type(obj) == TYPE_SLICE);
}

bool is_set(OBJ obj)
{
  return (obj.extra_data.word & ~(1ULL << HAS_REF_BIT_SHIFT)) == EMPTY_SET_MASK;
}

bool is_empty_set(OBJ obj)
{
  return obj.extra_data.word == EMPTY_SET_MASK;
}

bool is_ne_set(OBJ obj)
{
  return obj.extra_data.word == NE_SET_MASK;
}

bool is_map(OBJ obj)
{
  return (obj.extra_data.word & ~(1ULL << HAS_REF_BIT_SHIFT)) == EMPTY_MAP_MASK;
}

bool is_empty_map(OBJ obj)
{
  return obj.extra_data.word == EMPTY_MAP_MASK;
}

bool is_ne_map(OBJ obj)
{
  return obj.extra_data.word == NE_MAP_MASK;
}

bool is_tag_obj(OBJ obj)
{
  return obj.extra_data.std.num_tags != 0 | get_physical_type(obj) == TYPE_TAG_OBJ;
}

////////////////////////////////////////////////////////////////////////////////

bool is_symb(OBJ obj, uint16 symb_idx)
{
  return obj.extra_data.word == (symb_idx | SYMBOL_BASE_MASK);
}

bool is_int(OBJ obj, int64 n)
{
  return obj.core_data.int_ == n & obj.extra_data.word == INTEGER_MASK;
}


////////////////////////////////////////////////////////////////////////////////

bool is_inline_obj(OBJ obj)
{
  assert(!obj.extra_data.std.has_ref == (get_physical_type(obj) <= MAX_INLINE_OBJ_TYPE_VALUE | obj.core_data.ptr == NULL));
  return !obj.extra_data.std.has_ref;
}

bool is_ref_obj(OBJ obj)
{
  return obj.extra_data.std.has_ref;
}

////////////////////////////////////////////////////////////////////////////////

OBJ_TYPE get_ref_obj_type(OBJ obj)
{
  assert(is_ref_obj(obj));

  OBJ_TYPE type = get_physical_type(obj);
  assert(type == TYPE_SEQUENCE | type == TYPE_SLICE | type == TYPE_SET | type == TYPE_MAP | type == TYPE_TAG_OBJ);

  if (type == TYPE_SLICE)
    return TYPE_SEQUENCE;
  else
    return type;
}

REF_OBJ *get_ref_obj_ptr(OBJ obj)
{
  assert(is_ref_obj(obj));

  OBJ_TYPE type = get_physical_type(obj);
  assert(type == TYPE_SEQUENCE | type == TYPE_SLICE | type == TYPE_SET | type == TYPE_MAP | type == TYPE_TAG_OBJ);

  if (type == TYPE_SLICE)
  {
    char *buffer_ptr = (char *) obj.core_data.ptr;
    return (REF_OBJ *) (buffer_ptr - (SEQ_BUFFER_FIELD_OFFSET + obj.extra_data.slice.offset * sizeof(OBJ)));
  }

  if (type == TYPE_SEQUENCE)
    return (REF_OBJ *) (((char *) obj.core_data.ptr) - SEQ_BUFFER_FIELD_OFFSET);

  return (REF_OBJ *) obj.core_data.ptr;
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
