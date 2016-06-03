#include <stdio.h>
#include <string.h>

#include <vector>
#include <map>

#include "lib.h"
#include "generated.h"



OBJ str_to_obj(const char *c_str)
{
  int n = strlen(c_str);

  OBJ raw_str_obj = null_obj;

  if (n == 0)
  {
    raw_str_obj = empty_seq;
  }
  else
  {
    SEQ_OBJ *raw_str = new_full_seq(n);
    for (int i=0 ; i < n ; i++)
      raw_str->elems[i] = make_int(c_str[i]);
    raw_str_obj = make_obj(raw_str);
  }

  TAG_OBJ *str = new_tag_obj();
  str->tag = make_symb(symb_idx_string);
  str->obj = raw_str_obj;

  return make_obj(str);
}

void obj_to_str(OBJ str_obj, char *buffer, int size)
{
  OBJ raw_str_obj = get_tag_obj_ptr(str_obj)->obj;

  if (raw_str_obj != empty_seq)
  {
    SEQ_OBJ *raw_str = get_seq_ptr(raw_str_obj);
    int len = raw_str->length;
    internal_fail_if(len >= size);
    for (int i=0 ; i < len ; i++)
      buffer[i] = get_int_val(raw_str->elems[i]);
    buffer[len] = '\0';
  }
  else
  {
    buffer[0] = '\0';
  }
}

char *obj_to_byte_array(OBJ byte_seq_obj, int &size)
{
  if (byte_seq_obj == empty_seq)
  {
    size = 0;
    return NULL;
  }

  SEQ_OBJ *seq = get_seq_ptr(byte_seq_obj);
  int len = seq->length;
  OBJ *elems = seq->elems;
  char *buffer = new char[len];
  for (int i=0 ; i < len ; i++)
  {
    long long val = get_int_val(elems[i]);
    assert(val >= 0 && val <= 255);
    buffer[i] = (char) val;
  }
  size = len;
  return buffer;
}

int char_buffer_size(OBJ str_obj)
{
  OBJ raw_str_obj = get_tag_obj_ptr(str_obj)->obj;

  if (raw_str_obj == empty_seq)
    return 1;

  SEQ_OBJ *raw_str = get_seq_ptr(raw_str_obj);
  return raw_str->length + 1;
}

char *obj_to_str(OBJ str_obj)
{
  int size = char_buffer_size(str_obj);
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
  int count = cached_objs.size();
  for (int i=0 ; i < count ; i++)
    release(cached_objs[i]);
  cached_objs.clear();
}

////////////////////////////////////////////////////////////////////////////////

struct str_obj_cmp
{
  bool operator() (const OBJ &str1, const OBJ &str2) const
  {
    SEQ_OBJ *ptr1 = get_seq_ptr(get_inner_obj(str1));
    SEQ_OBJ *ptr2 = get_seq_ptr(get_inner_obj(str2));

    int len1 = ptr1->length;
    int len2 = ptr2->length;

    if (len1 != len2)
      return len1 < len2;

    OBJ *elems1 = ptr1->elems;
    OBJ *elems2 = ptr2->elems;

    for (int i=0 ; i < len1 ; i++)
    {
      OBJ elem1 = elems1[i];
      OBJ elem2 = elems2[i];
      if (elem1 != elem2)
        return elem1 < elem2;
    }

    return false;
  }
};


std::map<OBJ, int, str_obj_cmp> str_to_symb_map;
std::vector<OBJ> symb_strs;


static void initialize_symb_strs()
{
  static bool initialized = false;

  if (initialized)
    return;

  initialized = true;

  for (int i=0 ; i < generated::EMB_SYMB_COUNT ; i++)
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
  int idx = get_symb_idx(obj);
  assert(idx < symb_strs.size());
  OBJ str = symb_strs[idx];
  add_ref(str);
  return str;
}


OBJ to_symb(OBJ obj)
{
  initialize_symb_strs();

  std::map<OBJ, int, str_obj_cmp>::iterator it = str_to_symb_map.find(obj);
  if (it != str_to_symb_map.end())
    return make_symb(it->second);

  add_ref(obj);
  add_obj_to_cache(obj);

  int next_symb_id = symb_strs.size();
  symb_strs.push_back(obj);
  str_to_symb_map[obj] = next_symb_id;
  return make_symb(next_symb_id);
}
