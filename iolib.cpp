#include "lib.h"
#include "os_interface.h"
#include "generated.h"

#include <stdio.h> //## MAYBE THIS SHOULD NOT BE HERE...


OBJ FileRead_P(OBJ filename, generated::ENV &)
{
  char *fname = obj_to_str(filename);
  int size;
  char *data = file_read(fname, size);
  delete [] fname;

  if (size == -1)
    return make_symb(symb_idx_nil);

  OBJ seq_obj = empty_seq;
  if (size > 0)
  {
    FULL_SEQ_OBJ *seq = new_full_seq(size);
    for (int i=0 ; i < size ; i++)
      seq->buffer[i] = make_int(data[i]);
    delete [] data;
    seq_obj = make_obj(seq);
  }

  TAG_OBJ *tag_obj = new_tag_obj();
  tag_obj->tag = make_symb(symb_idx_just);
  tag_obj->obj = seq_obj;
  return make_obj(tag_obj);
}


OBJ FileWrite_P(OBJ filename, OBJ mode, OBJ data, generated::ENV &)
{
  char *fname = obj_to_str(filename);
  bool append = mode == make_symb(1);
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
  return make_symb(1);
}


OBJ Print_P(OBJ str_obj, generated::ENV &env)
{
  char *str = obj_to_str(str_obj);
  fputs(str, stdout);
  delete [] str;
  return make_symb(symb_idx_nil);
}


OBJ GetChar_P(generated::ENV &env)
{
  int ch = getchar();
  if (ch == EOF)
    return make_symb(symb_idx_nil);
  return make_int(ch);
}
