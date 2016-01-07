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

void printed_obj_or_filename(Obj obj, bool add_path, char *buffer, int buff_size)
{
  const int MAX_OBJ_COUNT = 1024;
  static int filed_objs_count = 0;
  // Deliberate bug: storing objects without reference counting them.
  static Obj filed_objs[MAX_OBJ_COUNT];

  assert(buff_size >= 64);

  const char *file_template = add_path ? "<debug/obj_%02d.txt>" : "<obj_%02d.txt>";

  for (int i=0 ; i < filed_objs_count ; i++)
    if (filed_objs[i] == obj)
    {
      sprintf(buffer, file_template, i);
      return;
    }

  char fname[64];
  sprintf(fname, "debug/obj_%02d.txt", filed_objs_count);

  print_to_buffer_or_file(obj, buffer, buff_size, fname);

  if (buffer[0] == '\0')
  {
    // The object was written to a file
    sprintf(buffer, file_template, filed_objs_count);
    if (filed_objs_count < MAX_OBJ_COUNT)
      filed_objs[filed_objs_count++] = obj;
  }
}

////////////////////////////////////////////////////////////////////////////////

void print_indented_param(FILE *fp, Obj param, bool is_last)
{
  const int BUFF_SIZE = 512;
  char buffer[BUFF_SIZE];

  if (param != null_obj)
    printed_obj_or_filename(param, false, buffer, BUFF_SIZE);
  else
    strcpy(buffer, "<closure>");

  for (int i=0 ; buffer[i] != '\0' ; i++)
  {
    if (i == 0 || buffer[i-1] == '\n')
      fputs("  ", fp);
    fputc(buffer[i], fp);
  }

  if (!is_last)
    fputs(",", fp);
  fputs("\n", fp);
  fflush(fp);
}


void print_stack_frame(FILE *fp, int frame_idx)
{
  const char *fn_name = function_names[frame_idx];
  int arity = arities[frame_idx];
  Obj *params = param_lists[frame_idx];

  fputs(fn_name, fp);
  fputs("(", fp);
  if (arity > 0)
    fputs("\n", fp);
  for (int i=0 ; i < arity ; i++)
    print_indented_param(fp, params[i], i == arity-1);
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
  for (int i=0 ; i < size ; i++)
    print_stack_frame(fp, i);
  fputs(" done.\n\n", stderr);
  fclose(fp);
#endif
}


void dump_var(const char *name, Obj value)
{
  const int BUFF_SIZE = 512;
  char buffer[BUFF_SIZE];
  printed_obj_or_filename(value, true, buffer, BUFF_SIZE);
  fprintf(stderr, "%s = %s\n\n", name, buffer);
}

////////////////////////////////////////////////////////////////////////////////

void print_assertion_failed_msg(const char *file, int line, const char *text)
{
  if (text == NULL)
    fprintf(stderr, "\nAssertion failed. File: %s, line: %d\n\n", file, line);
  else
    fprintf(stderr, "\nAssertion failed: %s\nFile: %s, line: %d\n\n", text, file, line);
}

////////////////////////////////////////////////////////////////////////////////

void hard_fail(const char *message)
{
  if (message != NULL)
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
