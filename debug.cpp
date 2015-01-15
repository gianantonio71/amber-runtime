#include <vector>
#include <cstdio>
#include <cstring>

#include "lib.h"
#include "generated.h"

using namespace std;


std::vector<const char *> function_names;
std::vector<int>    arities;
std::vector<Obj *>  param_lists;

void push_call_info(const char *fn_name, int arity, Obj *params)
{
#ifndef NDEBUG
  function_names.push_back(fn_name);
  arities.push_back(arity);
  param_lists.push_back(params);
#endif
}

void pop_call_info()
{
#ifndef NDEBUG
  int arity = arities.back();
  if (arity > 0)
    delete_obj_array(param_lists.back(), arity);

  function_names.pop_back();
  arities.pop_back();
  param_lists.pop_back();
#endif
}

////////////////////////////////////////////////////////////////////////////////

void print_indented_param(FILE *fp, Obj param, bool is_last, int &file_idx)
{
  int buff_size;
  char *buffer;
  if (param != null_obj)
  {
    generated::Env env;
    memset(&env, 0, sizeof(generated::Env));
    Obj str_obj = generated::To_Text(param, to_obj(80LL), to_obj(1LL), env);

    buff_size = char_buffer_size(str_obj);
    buffer = (char *) new_obj(nblocks16(buff_size));
    obj_to_str(str_obj, buffer, buff_size);

    release(str_obj);
  }
  else
  {
    static const char *CLOSURE_PLACEHOLDER_TEXT = "  <closure>";
    buff_size = strlen(CLOSURE_PLACEHOLDER_TEXT) + 1;
    buffer = (char *) new_obj(nblocks16(buff_size));
    strcpy(buffer, CLOSURE_PLACEHOLDER_TEXT);
  }

  char *final_text = buffer;
  char msg[1024];

  if (buff_size > 500)
  {
    char fname[1024];
    sprintf(fname, "debug/obj_%02d.txt", file_idx);

    FILE *fp = fopen(fname, "w");
    if (fp != 0)
    {
      fputs(buffer, fp);
      fclose(fp);
    }

    sprintf(msg, "  <obj_%02d.txt>", file_idx);
    final_text = msg;
    file_idx++;
  }

  fputs(final_text, fp);
  if (!is_last)
    fputs(",", fp);
  fputs("\n", fp);
  fflush(fp);

  free_obj(buffer, nblocks16(buff_size));
}


void print_stack_frame(FILE *fp, int frame_idx, int &file_idx)
{
  const char *fn_name = function_names[frame_idx];
  int arity = arities[frame_idx];
  Obj *params = param_lists[frame_idx];

  fputs(fn_name, fp);
  fputs("(", fp);
  if (arity > 0)
    fputs("\n", fp);
  for (int i=0 ; i < arity ; i++)
    print_indented_param(fp, params[i], i == arity-1, file_idx);
  fputs(")\n\n", fp);
}


void print_stack_frame(int frame_idx)
{
  const char *fn_name = function_names[frame_idx];
  int arity = arities[frame_idx];
  fprintf(stderr, "%s/%d\n", fn_name, arity);
}


void print_call_stack()
{
#ifndef NDEBUG
  int size = function_names.size();
  for (int i=0 ; i < size ; i++)
    print_stack_frame(i);
  fputs("\nNow trying to write a full dump of the stack to the file debug/stack_trace.txt.\nPlease be patient. This may take a while...", stderr);
  fflush(stderr);
  FILE *fp = fopen("debug/stack_trace.txt", "w");
  if (fp == NULL)
  {
    fputs("\nFailed to open file debug/stack_trace.txt\n", stderr);
    return;
  }
  int file_idx = 0;
  for (int i=0 ; i < size ; i++)
    print_stack_frame(fp, i, file_idx);
  fputs(" done.\n\n", stderr);
  fclose(fp);
#endif
}

////////////////////////////////////////////////////////////////////////////////

void hard_fail(const char *message)
{
  fprintf(stderr, "%s\n\n", message);
  print_call_stack();
  *(char *)0 = 0;
}

void hard_fail_if(bool condition, const char *message)
{
  if (condition)
    hard_fail(message);
}

void hard_fail_if_not(bool condition, const char *message)
{
  if (!condition)
    hard_fail(message);
}

void fail_if(bool condition, const char *message)
{
#ifndef NOCHECKS
  hard_fail_if(condition, message);
#endif
}

void fail_if_not(bool condition, const char *message)
{
  fail_if(!condition, message);
}

////////////////////////////////////////////////////////////////////////////////

void internal_fail()
{
  fputs("Internal error!\n", stderr);
  fflush(stderr);
  print_call_stack();
  *(char *)0 = 0;
}

void internal_fail_if(bool condition)
{
  if (condition)
    internal_fail();
}