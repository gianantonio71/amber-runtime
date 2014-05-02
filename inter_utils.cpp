#include "lib.h"
#include "generated.h"

#include <stdio.h>
#include <string.h>



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
    Seq *raw_str = new_seq(n);
    for (int i=0 ; i < n ; i++)
      raw_str->elems[i] = to_obj(c_str[i]);
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
    if (len >= size)
      fail();
    for (int i=0 ; i < len ; i++)
      buffer[i] = get_int_val(raw_str->elems[i]);
    buffer[len] = '\0';
  }
  else
  {
    buffer[0] = '\0';
  }
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

int indent_delta(char ch)
{
  if (ch == '(' || ch == '[' || ch == '{')
    return 1;
    
  if (ch == ')' || ch == ']' || ch == '}')
    return -1;
  
  return 0;
}


void print_indented(const char *str)
{
  int len = strlen(str);

  putchar('\n');

  int indent = 0;
  for (int i=0 ; i < len ; i++)
  {
    char ch = str[i];

    int dind = indent_delta(ch);
    
    if (dind == 0)
    {
      putchar(ch);
      if (ch == ',')
      {
        putchar('\n');
        for (int j=0 ; j < indent ; j++)
          printf("  ");
        assert(str[i+1] == ' ');
        i++;
      }
    }  

    if (dind == 1)
    {
      int loc_ind = 1;

      for (int j=1 ; j < 60 && i+j < len ; j++)
      {
        loc_ind += indent_delta(str[i+j]);
        assert(loc_ind >= 0);
        if (loc_ind == 0)
        {
          for (int k=0 ; k <= j ; k++)
            putchar(str[i+k]);
          i += j;
          break;                  
        }
      }
      
      if (loc_ind > 0)
      {
        indent++;
        putchar(ch);
        putchar('\n');
        for (int j=0 ; j < indent ; j++)
          printf("  ");
      }
    }

    if (dind == -1)
    {
      indent--;
      putchar('\n');
      for (int j=0 ; j < indent ; j++)
        printf("  ");
      putchar(ch);
    }  
  }

  putchar('\n');
  //delete [] str;
}

void print(Obj obj)
{
  const int BUFF_SIZE = 16 * 1024 *1024;
  static char buffer[BUFF_SIZE];
  
  //Obj fn_to_text(Obj);
  
  generated::Env env;
  memset(&env, 0, sizeof(generated::Env));
  
  Obj str_obj = generated::To_Text(obj, env);
  obj_to_str(str_obj, buffer, BUFF_SIZE);
  release(str_obj);
  
  //puts(buffer);
  print_indented(buffer);
  fflush(stdout);
}

