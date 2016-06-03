#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "generated.h"


typedef enum {TEXT, SUB_START, SUB_END} EMIT_ACTION;


void print_obj(OBJ obj, void (*emit)(void *, const void *, EMIT_ACTION), void *data);
void print_obj_inline(OBJ obj, bool print_delimiters, void (*emit)(void *, const void *, EMIT_ACTION), void *data);


bool is_str(OBJ tag, OBJ obj)
{
  if (tag != make_symb(symb_idx_string))
    return false;

  if (obj == empty_seq)
    return true;

  if (!is_ne_seq(obj))
    return false;

  SEQ_OBJ *seq = get_seq_ptr(obj);

  if (seq == NULL)
    return true;

  unsigned int len = seq->length;
  OBJ *elems = seq->elems;

  for (int i=0 ; i < len ; i++)
  {
    OBJ elem = elems[i];

    if (!is_int(elem))
      return false;

    long long value = get_int_val(elem);
    if (value < 0 | value >= 65536)
      return false;
  }

  return true;
}


bool is_record(OBJ obj)
{
  if (!is_ne_map(obj))
    return false;

  MAP_OBJ *map = get_map_ptr(obj);
  unsigned int size = map->size;
  OBJ *keys = get_key_array_ptr(map);

  for (int i=0 ; i < size ; i++)
    if (!is_symb(keys[i]))
      return false;

  return true;
}


void print_bare_str(OBJ str, void (*emit)(void *, const void *, EMIT_ACTION), void *data)
{
  char buffer[64];

  assert(is_str(get_tag_obj_ptr(str)->tag, get_tag_obj_ptr(str)->obj));

  OBJ char_seq_obj = get_tag_obj_ptr(str)->obj;
  if (char_seq_obj == empty_seq)
    return;

  SEQ_OBJ *char_seq = get_seq_ptr(char_seq_obj);
  unsigned int len = char_seq->length;
  OBJ *chars = char_seq->elems;

  for (int i=0 ; i < len ; i++)
  {
    long long ch = get_int_val(chars[i]);
    assert(ch >= 0 & ch < 65536);
    if (ch >= ' ' & ch <= '~')
    {
      buffer[0] = '\\';
      buffer[1] = ch;
      buffer[2] = '\0';
      emit(data, buffer + (ch == '"' | ch == '\\' ? 0 : 1), TEXT);
    }
    else
    {
      sprintf(buffer, "\\%04llx", ch);
      emit(data, buffer, TEXT);
    }
  }
}


void print_int(OBJ obj, void (*emit)(void *, const void *, EMIT_ACTION), void *data)
{
  long long n = get_int_val(obj);
  char buffer[1024];
  sprintf(buffer, "%lld", n);
  emit(data, buffer, TEXT);
}


void print_float(OBJ obj, void (*emit)(void *, const void *, EMIT_ACTION), void *data)
{
  FLOAT *x = get_float_ptr(obj);
  char buffer[1024];
  sprintf(buffer, "%g", x->value);
  emit(data, buffer, TEXT);
}


void print_symb(OBJ obj, void (*emit)(void *, const void *, EMIT_ACTION), void *data)
{
  OBJ str = to_str(obj);
  print_bare_str(str, emit, data);
  release(str);
}


void print_seq(OBJ obj, bool print_parentheses, void (*emit)(void *, const void *, EMIT_ACTION), void *data)
{
  if (print_parentheses)
    emit(data, "(", TEXT);
  if (obj != empty_seq)
  {
    SEQ_OBJ *seq = get_seq_ptr(obj);
    unsigned int len = seq->length;
    OBJ *elems = seq->elems;
    for (int i=0 ; i < len ; i++)
    {
      if (i > 0)
        emit(data, ", ", TEXT);
      print_obj(elems[i], emit, data);
    }
  }
  if (print_parentheses)
    emit(data, ")", TEXT);
}


void print_set(OBJ obj, void (*emit)(void *, const void *, EMIT_ACTION), void *data)
{
  emit(data, "[", TEXT);
  if (obj != empty_set)
  {
    SET_OBJ *set = get_set_ptr(obj);
    unsigned int size = set->size;
    OBJ *elems = set->elems;
    for (int i=0 ; i < size ; i++)
    {
      if (i > 0)
        emit(data, ", ", TEXT);
      print_obj(elems[i], emit, data);
    }
  }
  emit(data, "]", TEXT);
}


void print_map(OBJ obj, bool print_brackets, void (*emit)(void *, const void *, EMIT_ACTION), void *data)
{
  if (obj == empty_map)
  {
    emit(data, "[:]", TEXT);
    return;
  }

  if (print_brackets)
    emit(data, "[", TEXT);

  MAP_OBJ *map = get_map_ptr(obj);
  unsigned int size = map->size;
  OBJ *keys = get_key_array_ptr(map);
  OBJ *values = get_value_array_ptr(map);

  for (int i=0 ; i < size ; i++)
  {
    if (i > 0)
      emit(data, ", ", TEXT);
    emit(data, NULL, SUB_START);
    if (is_record(obj))
    {
      print_symb(keys[i], emit, data);
      emit(data, ": ", TEXT);
      print_obj_inline(values[i], true, emit, data);
    }
    else
    {
      print_obj(keys[i], emit, data);
      emit(data, " -> ", TEXT);
      print_obj(values[i], emit, data);
    }
    emit(data, NULL, SUB_END);
  }

  if (print_brackets)
    emit(data, "]", TEXT);
}


void print_tag_obj(OBJ obj, void (*emit)(void *, const void *, EMIT_ACTION), void *data)
{
  TAG_OBJ *tag_obj = get_tag_obj_ptr(obj);
  OBJ tag = tag_obj->tag;
  OBJ inner_obj = tag_obj->obj;
  if (is_str(tag, inner_obj))
  {
    emit(data, "\"", TEXT);
    print_bare_str(obj, emit, data);
    emit(data, "\"", TEXT);
  }
  else
  {
    print_symb(tag, emit, data);
    emit(data, "(", TEXT);
    bool skip_delimiters = is_record(inner_obj) || (is_ne_seq(inner_obj) && get_seq_len(inner_obj) > 1);
    print_obj_inline(inner_obj, !skip_delimiters, emit, data);
    emit(data, ")", TEXT);
  }
}


void print_obj_inline(OBJ obj, bool print_delimiters, void (*emit)(void *, const void *, EMIT_ACTION), void *data)
{
  if (is_set(obj))
    print_set(obj, emit, data);
  else if (is_seq(obj))
    print_seq(obj, print_delimiters, emit, data);
  else if (is_map(obj))
    print_map(obj, print_delimiters, emit, data);
  else
    print_obj(obj, emit, data);
}


void print_obj(OBJ obj, void (*emit)(void *, const void *, EMIT_ACTION), void *data)
{
  emit(data, NULL, SUB_START);

  if (obj == null_obj)
    emit(data, "NULL", TEXT);

  else if (obj == blank_obj)
    emit(data, "BLANK", TEXT);

  else if (is_int(obj))
    print_int(obj, emit, data);

  else if (is_float(obj))
    print_float(obj, emit, data);

  else if (is_symb(obj))
    print_symb(obj, emit, data);

  else if (is_seq(obj))
    print_seq(obj, true, emit, data);

  else if (is_set(obj))
    print_set(obj, emit, data);

  else if (is_map(obj))
    print_map(obj, true, emit, data);

  else // is_tag_obj(obj)
    print_tag_obj(obj, emit, data);

  emit(data, NULL, SUB_END);
}


struct TEXT_FRAG
{
  int depth;
  int start;
  int length;
};


struct PRINT_BUFFER
{
  int str_len;
  char buffer[64 * 1024 * 1024];

  int frag_count;
  TEXT_FRAG fragments[16 * 1024 * 1024];

  int curr_depth;
};


void init(PRINT_BUFFER *pb)
{
  pb->str_len = 0;
  pb->buffer[0] = '\0';
  pb->frag_count = 0;
  // for (int i=0 ; i < 1024 * 1024 ; i++)
  // {
  //   TEXT_FRAG *f = pb->fragments + i;
  //   f->depth = -100000;
  //   f->start = -100000;
  //   f->length = -100000;
  // }
  pb->curr_depth = -1;
}


int printable_frags_count(PRINT_BUFFER *pb)
{
  int fc = pb->frag_count;
  TEXT_FRAG *fs = pb->fragments;

  TEXT_FRAG *lf = fs + fc - 1;
  assert(pb->curr_depth == lf->depth);

  if (lf->depth == -1)
    return fc - 1;

  int curr_length = lf->length;

  for (int i=fc-2 ; i >= 0 ; i--)
  {
    TEXT_FRAG *f = fs + i;
    curr_length += f->length;
    if (curr_length > 100)
      return i + 1;
  }

  return 0;
}


void calculate_subobjects_lengths(PRINT_BUFFER *pb, int *ls)
{
  int fc = pb->frag_count;
  TEXT_FRAG *fs = pb->fragments;

  for (int i=0 ; i < fc ; i++)
  {
    TEXT_FRAG *f = fs + i;

    int pd;
    if (i == 0)
      pd = -1;
    else
      pd = fs[i-1].depth;

    if (f->depth > pd)
    {
      int len = 0;
      for (int j=i ; j < fc ; j++)
      {
        TEXT_FRAG *f2 = fs + j;
        if (f2->depth < f->depth)
          break;
        len += f2->length;
      }
      ls[i] = len;
    }
    else
      ls[i] = -1;
  }
}


void emit_known(PRINT_BUFFER *pb, void (*emit)(void *, const char *, int), void *data)
{
  int pfc = printable_frags_count(pb);

  int *ls = new int[pb->frag_count];
  calculate_subobjects_lengths(pb, ls);

  char *buff = pb->buffer;
  TEXT_FRAG *fs = pb->fragments;

  int split_depth = ls[0] > 100 ? 0 : -1;

  for (int i=0 ; i < pfc ; i++)
  {
    TEXT_FRAG *f = fs + i;
    TEXT_FRAG *nf = f + 1;

    int len = f->length;

    int d = f->depth;
    int nd = nf->depth;

    assert(d == nd - 1 || d == nd + 1);
    assert(split_depth <= d);

    if (nd > d)
    {
      emit(data, buff + f->start, len);

      if (d <= split_depth)
      {
        if (len >= 2)
        {
          emit(data, "\n", 1);
          for (int j=0 ; j < nd ; j++)
            emit(data, "  ", 2);
        }
        else if (len == 1)
        {
          emit(data, " ", 1);
        }
      }

      if (ls[i+1] > 100)
      {
        assert(split_depth == d);
        split_depth = nd;
      }
    }
    else
    {
      assert(nd < d);
      if (nd < split_depth)
      {
        assert(split_depth == d);
        split_depth = nd;
        if (len > 0)
        {
          emit(data, "\n", 1);
          for (int j=0 ; j <= nd ; j++)
            emit(data, "  ", 2);
        }
      }
      emit(data, buff + f->start, len);
    }
  }
  delete ls;
}


void process_text(PRINT_BUFFER *pb, const char *text)
{
  strcpy(pb->buffer + pb->str_len, text);
  int len = strlen(text);
  pb->str_len += len;
  TEXT_FRAG *curr_frag = pb->fragments + pb->frag_count - 1;
  assert(curr_frag->depth == pb->curr_depth);
  curr_frag->length += len;
}


void subobj_start(PRINT_BUFFER *pb)
{
  int new_depth = pb->curr_depth + 1;
  pb->curr_depth = new_depth;

  int new_frag_idx = pb->frag_count;
  pb->frag_count = new_frag_idx + 1;

  TEXT_FRAG *new_frag = pb->fragments + new_frag_idx;
  new_frag->depth = new_depth;
  new_frag->start = pb->str_len;
  new_frag->length = 0;
}


void subobj_end(PRINT_BUFFER *pb)
{
  int new_depth = pb->curr_depth - 1;
  pb->curr_depth = new_depth;

  int new_frag_idx = pb->frag_count;
  pb->frag_count = new_frag_idx + 1;

  TEXT_FRAG *new_frag = pb->fragments + new_frag_idx;
  new_frag->depth = new_depth;
  new_frag->start = pb->str_len;
  new_frag->length = 0;
}


////////////////////////////////////////////////////////////////////////////////

void emit_store(void *pb_, const void *data, EMIT_ACTION action)
{
  PRINT_BUFFER *pb = (PRINT_BUFFER *) pb_;

  switch (action)
  {
    case TEXT:
      process_text(pb, (char *) data);
      break;
    case SUB_START:
      subobj_start(pb);
      break;
    case SUB_END:
      subobj_end(pb);
      break;
  }
}


void stdout_print(void *, const char *text, int len)
{
  fwrite(text, 1, len, stdout);
  fflush(stdout);
}


void print(OBJ obj)
{
  PRINT_BUFFER *pb = new PRINT_BUFFER;

  init(pb);
  print_obj(obj, emit_store, pb);
  fputs("\n", stdout);
  emit_known(pb, stdout_print, NULL);
  fputs("\n", stdout);

  delete pb;
}


void write_to_file(void *fp, const char *text, int len)
{
  fwrite(text, 1, len, (FILE *) fp);
}


void append_to_string(void *ptr, const char *text, int len)
{
  char *str = (char *) ptr;
  int curr_len = strlen(str);
  memcpy(str + curr_len, text, len);
  str[curr_len + len] = '\0';
}


void calc_length(void *ptr, const char *text, int len)
{
  int *total_len = (int *) ptr;
  *total_len += len;
}


void print_to_buffer_or_file(OBJ obj, char *buffer, int max_size, const char *fname)
{
  PRINT_BUFFER *pb = new PRINT_BUFFER;

  init(pb);
  print_obj(obj, emit_store, pb);

  int len = 0;
  emit_known(pb, calc_length, &len);

  buffer[0] = '\0';
  if (len < max_size)
  {
    emit_known(pb, append_to_string, buffer);
  }
  else
  {
    FILE *fp = fopen(fname, "w");
    emit_known(pb, write_to_file, fp);
    fclose(fp);
  }
}


void printed_obj(OBJ obj, char *buffer, int max_size)
{
  PRINT_BUFFER *pb = new PRINT_BUFFER;

  init(pb);
  print_obj(obj, emit_store, pb);

  int len = 0;
  emit_known(pb, calc_length, &len);

  if (len + 1 < max_size)
  {
    memcpy(buffer, pb->buffer, len + 1);
  }
  else
  {
    memcpy(buffer, pb->buffer, max_size - 1);
    buffer[max_size-1] = '\0';
  }
}
