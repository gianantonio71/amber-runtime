#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "generated.h"



Obj str_to_obj(const char *c_str)
{
  int n = strlen(c_str);
  
  Obj raw_str_obj = null_obj;
  
  if (n == 0)
  {
    raw_str_obj = empty_seq;
  }
  else
  {
    Seq *raw_str = new_full_seq(n);
    for (int i=0 ; i < n ; i++)
      raw_str->elems[i] = to_obj((long long) c_str[i]);
    raw_str_obj = make_obj(raw_str);
  }

  TagObj *str = new_tag_obj();
  str->tag = generated::S_string;
  str->obj = raw_str_obj;

  return make_obj(str);
}

void obj_to_str(Obj str_obj, char *buffer, int size)
{
  Obj raw_str_obj = get_tag_obj_ptr(str_obj)->obj;
  
  if (raw_str_obj != empty_seq)
  {
    Seq *raw_str = get_seq_ptr(raw_str_obj);
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

int char_buffer_size(Obj str_obj)
{
  Obj raw_str_obj = get_tag_obj_ptr(str_obj)->obj;

  if (raw_str_obj == empty_seq)
    return 1;

  Seq *raw_str = get_seq_ptr(raw_str_obj);
  return raw_str->length + 1;
}

////////////////////////////////////////////////////////////////////////////////

const int MAX_SYMBS = 10000;

int symb_count = generated::EMB_SYMB_COUNT;
Obj symb_strs[MAX_SYMBS];


static void initialize_symb_strs()
{
  static bool initialized = false;
  
  if (initialized)
    return;
  
  initialized = true;
  
  for (int i=0 ; i < generated::EMB_SYMB_COUNT ; i++)
  {
    assert(symb_strs[i] == 0);
    symb_strs[i] = str_to_obj(generated::map_symb_to_str[i]);
  }
}


Obj to_str(Obj obj)
{
  initialize_symb_strs();
  
  assert(is_symb(obj));
  
  int idx = get_symb_idx(obj);
  
  assert(idx < symb_count);
  assert(idx < generated::EMB_SYMB_COUNT || symb_strs[idx] != 0);

  if (symb_strs[idx] == 0)
    symb_strs[idx] = str_to_obj(generated::map_symb_to_str[idx]);

  add_ref(symb_strs[idx]);

  return symb_strs[idx];
}


Obj to_symb(Obj obj)
{
  initialize_symb_strs();

  //## BAD BAD BAD VERY INEFFICIENT. THIS HAS TO BE REWRITTEN COMPLETELY
  for (int i=0 ; i < symb_count ; i++)
    if (are_eq(obj, symb_strs[i]))
      return make_symb(i);
  symb_strs[symb_count] = obj;
  add_ref(obj);
  return make_symb(symb_count++);
}

void release_all_cached_strings()
{
  for (int i=0 ; i < symb_count ; i++)
    if (symb_strs[i] != 0)
    {
      release(symb_strs[i]);
      symb_strs[i] = 0;    
    }
}

////////////////////////////////////////////////////////////////////////////////

void print(Obj obj)
{
  generated::Env env;
  memset(&env, 0, sizeof(generated::Env));
  Obj str_obj = generated::To_Text(obj, to_obj(80LL), to_obj(0LL), env);
  int buff_size = char_buffer_size(str_obj);
  char *buffer = (char *) new_obj(nblocks16(buff_size));
  obj_to_str(str_obj, buffer, buff_size);
  puts(buffer);
  puts("");
  fflush(stdout);
  release(str_obj);
  free_obj(buffer, nblocks16(buff_size));
}
