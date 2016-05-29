#include <stdio.h>
#include <string.h>

#include <vector>
#include <map>

#include "lib.h"
#include "generated.h"


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

  TAG_OBJ *str = new_tag_obj();
  str->tag_idx = symb_idx_string;
  str->obj = raw_str_obj;

  return make_tag_obj(str);
}

void obj_to_str(OBJ str_obj, char *buffer, uint32 size)
{
  OBJ raw_str_obj = get_tag_obj_ptr(str_obj)->obj;

  if (!is_empty_seq(raw_str_obj))
  {
    OBJ *seq_buffer = get_seq_buffer_ptr(raw_str_obj);
    uint32 len = get_seq_length(raw_str_obj);
    internal_fail_if(len >= size);
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
  char *buffer = new char[len];
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
  uint32 size = get_seq_length(get_tag_obj_ptr(str_obj)->obj) + 1;
  char *buffer = new char[size];
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

struct str_obj_cmp
{
  bool operator() (const OBJ &str1, const OBJ &str2) const
  {
    OBJ raw_str_1 = get_tag_obj_ptr(str1)->obj;
    OBJ raw_str_2 = get_tag_obj_ptr(str2)->obj;

    uint32 len1 = get_seq_length(raw_str_1);
    uint32 len2 = get_seq_length(raw_str_2);

    if (len1 != len2)
      return len1 < len2;

    if (len1 == 0)
      return false;

    OBJ *elems1 = get_seq_buffer_ptr(raw_str_1);
    OBJ *elems2 = get_seq_buffer_ptr(raw_str_2);

    for (uint32 i=0 ; i < len1 ; i++)
    {
      int64 ch1 = get_int(elems1[i]);
      int64 ch2 = get_int(elems2[i]);
      if (ch1 != ch2)
        return ch1 < ch2;
    }

    return false;
  }
};


std::map<OBJ, uint32, str_obj_cmp> str_to_symb_map;
std::vector<OBJ> symb_strs;


static void initialize_symb_strs()
{
  static bool initialized = false;

  if (initialized)
    return;

  initialized = true;

  for (uint32 i=0 ; i < generated::EMB_SYMB_COUNT ; i++)
  {
    OBJ str_obj = str_to_obj(generated::map_symb_to_str[i]);
    add_obj_to_cache(str_obj);
    str_to_symb_map[str_obj] = i;
    symb_strs.push_back(str_obj);
  }
}


OBJ to_str(OBJ obj)
{
  initialize_symb_strs();
  assert(is_symb(obj));
  uint16 idx = get_symb_idx(obj);
  assert(idx < symb_strs.size());
  OBJ str = symb_strs[idx];
  add_ref(str);
  return str;
}


OBJ to_symb(OBJ obj)
{
  initialize_symb_strs();

  std::map<OBJ, uint32, str_obj_cmp>::iterator it = str_to_symb_map.find(obj);
  if (it != str_to_symb_map.end())
    return make_symb(it->second);

  add_ref(obj);
  add_obj_to_cache(obj);

  uint32 next_symb_id = symb_strs.size();
  symb_strs.push_back(obj);
  str_to_symb_map[obj] = next_symb_id;
  return make_symb(next_symb_id);
}
