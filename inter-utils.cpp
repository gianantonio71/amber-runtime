#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <vector>
#include <map>

#include "lib.h"


OBJ str_to_obj(const char *c_str)
{
  uint32 n = strlen(c_str);

  OBJ raw_str_obj;

  if (n == 0)
  {
    raw_str_obj = make_empty_seq();
  }
  else
  {
    SEQ_OBJ *raw_str = new_seq(n);
    for (uint32 i=0 ; i < n ; i++)
      raw_str->buffer[i] = make_int((uint8) c_str[i]);
    raw_str_obj = make_seq(raw_str, n);
  }

  return make_tag_obj(symb_idx_string, raw_str_obj);
}

void obj_to_str(OBJ str_obj, char *buffer, uint32 size)
{
  OBJ raw_str_obj = get_inner_obj(str_obj);

  if (!is_empty_seq(raw_str_obj))
  {
    OBJ *seq_buffer = get_seq_buffer_ptr(raw_str_obj);
    uint32 len = get_seq_length(raw_str_obj);
    assert(len < size);
    for (uint32 i=0 ; i < len ; i++)
      buffer[i] = get_int_val(seq_buffer[i]);
    buffer[len] = '\0';
  }
  else
  {
    buffer[0] = '\0';
  }
}

char *obj_to_byte_array(OBJ byte_seq_obj, uint32 &size)
{
  if (is_empty_seq(byte_seq_obj))
  {
    size = 0;
    return NULL;
  }

  uint32 len = get_seq_length(byte_seq_obj);
  OBJ *elems = get_seq_buffer_ptr(byte_seq_obj);
  char *buffer = (char *) malloc(len);
  for (uint32 i=0 ; i < len ; i++)
  {
    long long val = get_int_val(elems[i]);
    assert(val >= 0 && val <= 255);
    buffer[i] = (char) val;
  }
  size = len;
  return buffer;
}

char *obj_to_str(OBJ str_obj)
{
  uint32 size = get_seq_length(get_inner_obj(str_obj)) + 1;
  char *buffer = (char *) malloc(size);
  obj_to_str(str_obj, buffer, size);
  return buffer;
}

////////////////////////////////////////////////////////////////////////////////

static std::vector<OBJ> cached_objs;

void add_obj_to_cache(OBJ obj)
{
  if (is_ref_obj(obj))
    cached_objs.push_back(obj);
}

void release_all_cached_objs()
{
  uint32 count = cached_objs.size();
  for (uint32 i=0 ; i < count ; i++)
    release(cached_objs[i]);
  cached_objs.clear();
}

////////////////////////////////////////////////////////////////////////////////

bool str_ord(const char *str1, const char *str2)
{
  return strcmp(str1, str2) > 0;
}

typedef std::map<const char *, uint32, bool(*)(const char *, const char *)> str_idx_map_type;

str_idx_map_type str_to_symb_map(str_ord);
//## THESE STRINGS ARE NEVER CLEANED UP. NOT MUCH OF A PROBLEM IN PRACTICE, BUT STILL A BUG...
std::vector<const char *> dynamic_symbs_strs;

const char *symb_repr(uint16);
uint32 embedded_symbs_count();

const char *symb_to_raw_str(OBJ obj)
{
  assert(is_symb(obj));
  uint16 idx = get_symb_idx(obj);
  uint32 count = embedded_symbs_count();
  if (idx < count)
    return symb_repr(idx);
  else
    return dynamic_symbs_strs[idx - count];
}

OBJ to_str(OBJ obj)
{
  return str_to_obj(symb_to_raw_str(obj));
}

uint16 lookup_symb_idx(const char *str_, uint32 len)
{
  uint32 count = embedded_symbs_count();

  if (str_to_symb_map.size() == 0)
    for (uint32 i=0 ; i < count ; i++)
      str_to_symb_map[symb_repr(i)] = i;

  char *str = strndup(str_, len);

  str_idx_map_type::iterator it = str_to_symb_map.find(str);
  if (it != str_to_symb_map.end()) {
    free(str);
    return it->second;
  }

  uint32 next_symb_id = count + dynamic_symbs_strs.size();
  if (next_symb_id > 0xFFFF)
    impl_fail("Exceeded maximum permitted number of symbols (= 2^16)");
  dynamic_symbs_strs.push_back(str);
  str_to_symb_map[str] = next_symb_id;
  return next_symb_id;
}

OBJ to_symb(OBJ obj)
{
  char *str = obj_to_str(obj);
  uint16 symb_idx = lookup_symb_idx(str, strlen(str));
  free(str);
  return make_symb(symb_idx);
}

OBJ extern_str_to_symb(const char *str)
{
  //## CHECK THAT IT'S A VALID SYMBOL, AND THAT IT'S AMONG THE "STATIC" ONES
  return make_symb(lookup_symb_idx(str, strlen(str)));
}
