#include "lib.h"
#include "os_interface.h"
#include "generated.h"


Obj io_File_Read(Obj filename, generated::Env &)
{
  char *fname = obj_to_str(filename);
  int size;
  char *data = file_read(fname, size);
  delete [] fname;

  if (size == -1)
    return generated::S_nil;

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
  tag_obj->tag = generated::S_just;
  tag_obj->obj = seq_obj;
  return make_obj(tag_obj);
}


Obj io_File_Write(Obj filename, Obj mode, Obj data, generated::Env &)
{
  char *fname = obj_to_str(filename);
  bool append = mode == generated::S_true;
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
  return generated::S_true;
}
