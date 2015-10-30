#include "lib.h"
#include "os_interface.h"
#include "generated.h"

#include <stdio.h> //## MAYBE THIS SHOULD NOT BE HERE...


Obj FileRead_P(Obj filename, generated::Env &)
{
  char *fname = obj_to_str(filename);
  int size;
  char *data = file_read(fname, size);
  delete [] fname;

  if (size == -1)
    return generated::Nil_S;

  Obj seq_obj = empty_seq;
  if (size > 0)
  {
    FullSeq *seq = new_full_seq(size);
    for (int i=0 ; i < size ; i++)
      seq->buffer[i] = to_obj(data[i]);
    delete [] data;
    seq_obj = make_obj(seq);
  }

  TagObj *tag_obj = new_tag_obj();
  tag_obj->tag = generated::Just_S;
  tag_obj->obj = seq_obj;
  return make_obj(tag_obj);
}


Obj FileWrite_P(Obj filename, Obj mode, Obj data, generated::Env &)
{
  char *fname = obj_to_str(filename);
  bool append = mode == generated::True_S;
  int size;
  char *buffer = obj_to_byte_array(data, size);
  bool res;
  if (size > 0)
  {
    res = file_write(fname, buffer, size, append);
    delete [] buffer;
  }
  else
  {
    char empty_buff[1];
    res = file_write(fname, empty_buff, 0, append);
  }
  delete [] fname;
  return generated::True_S;
}


Obj Print_P(Obj str_obj, generated::Env &env)
{
  char *str = obj_to_str(str_obj);
  fputs(str, stdout);
  delete [] str;
  return generated::Nil_S;
}


Obj GetChar_P(generated::Env &env)
{
  int ch = getchar();
  if (ch == EOF)
    return generated::Nil_S;
  return to_obj(ch);
}
