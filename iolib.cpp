#include "lib.h"
#include "os-interface.h"

#include <stdio.h> //## MAYBE THIS SHOULD NOT BE HERE...


namespace generated {
  struct ENV;
}


OBJ FileRead_P(OBJ filename, generated::ENV &) {
  char *fname = obj_to_str(filename);
  int size;
  char *data = file_read(fname, size);
  delete [] fname;

  if (size == -1)
    return make_symb(symb_idx_nothing);

  OBJ seq_obj = make_empty_seq();
  if (size > 0) {
    SEQ_OBJ *seq = new_seq(size);
    for (uint32 i=0 ; i < size ; i++)
      seq->buffer[i] = make_int((uint8) data[i]);
    delete [] data;
    seq_obj = make_seq(seq, size);
  }

  return make_tag_obj(symb_idx_just, seq_obj);
}


OBJ FileWrite_P(OBJ filename, OBJ mode, OBJ data, generated::ENV &) {
  char *fname = obj_to_str(filename);
  bool append = get_bool(mode);
  uint32 size;
  char *buffer = obj_to_byte_array(data, size);
  bool res;
  if (size > 0) {
    res = file_write(fname, buffer, size, append);
    delete [] buffer;
  }
  else {
    char empty_buff[1];
    res = file_write(fname, empty_buff, 0, append);
  }
  delete [] fname;
  return make_bool(true);
}


OBJ Print_P(OBJ str_obj, generated::ENV &env) {
  char *str = obj_to_str(str_obj);
  fputs(str, stdout);
  fflush(stdout);
  delete [] str;
  return make_blank_obj();
}


OBJ GetChar_P(generated::ENV &env) {
  int ch = getchar();
  if (ch == EOF)
    return make_symb(symb_idx_nothing);
  return make_int(ch);
}
