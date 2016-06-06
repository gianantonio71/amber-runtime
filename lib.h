#include "utils.h"


typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

typedef signed char      int8;
typedef signed short     int16;
typedef signed int       int32;
typedef signed long long int64;


typedef long long OBJ;


struct REF_OBJ
{
  unsigned int ref_count;
};


struct SET_OBJ : public REF_OBJ
{
  int size;
  OBJ elems[1];
};


struct SEQ_OBJ : public REF_OBJ
{
  unsigned int length;
  OBJ *elems;
};


struct FULL_SEQ_OBJ : public SEQ_OBJ
{
  unsigned int capacity;
  unsigned int used_capacity;
  OBJ buffer[1];
};


struct REF_SEQ_OBJ : public SEQ_OBJ
{
  FULL_SEQ_OBJ *full_seq;
};


struct MAP_OBJ : public REF_OBJ
{
  int size;
  OBJ buffer[1];
};


struct TAG_OBJ : public REF_OBJ
{
  OBJ tag;
  OBJ obj;
};


struct FLOAT : public REF_OBJ
{
  int _;
  double value;
};

////////////////////////////////////////////////////////////////////////////////

struct SET_ITER
{
  SET_OBJ *set;
  int idx;
  int size;
};

struct SEQ_ITER
{
  SEQ_OBJ *seq;
  int idx;
  int len;
};

struct MAP_ITER
{
  MAP_OBJ *map;
  int idx;
  int size;
};

struct STREAM
{
  OBJ *buffer;
  int capacity;
  int count;
};

////////////////////////////////////////////////////////////////////////////////

enum SHORT_TYPE_TAG {
  short_type_tag_integer    = 0,  //  00      Inline integer
  short_type_tag_float      = 1,  //  01      Inline floating point
  short_type_tag_inline     = 2,  //  10      Other inline objects
  short_type_tag_reference  = 3   //  11      Object containing a pointer
};

enum TYPE_TAG {
  type_tag_null         = 2,   //    0  10    Null object
  type_tag_empty_set    = 6,   //    1  10    Empty set
  type_tag_empty_seq    = 10,  //   10  10    Empty sequence
  type_tag_empty_map    = 14,  //   11  10    Empty map
  type_tag_symb         = 18,  //  100  10    Symbol

  type_tag_set          = 7,   //    1  11    Set
  type_tag_seq          = 11,  //   10  11    Seq
  type_tag_map          = 15,  //   11  11    Map
  type_tag_tag_obj      = 19,  //  100  11    Tagged object
  type_tag_float        = 23   //  101  11    Floating point number
};

const int SHORT_TAG_SIZE  = 2;
const int FULL_TAG_SIZE   = 8;
const int POINTER_SHIFT   = 24;

const int SHORT_TAG_MASK  = (1 << SHORT_TAG_SIZE) - 1;
const int FULL_TAG_MASK   = (1 << FULL_TAG_SIZE) - 1;

#define symb(idx) ((idx << FULL_TAG_SIZE) | type_tag_symb)

const OBJ null_obj  = type_tag_null;
const OBJ blank_obj = (1 << FULL_TAG_SIZE) | type_tag_null;

const OBJ empty_set = type_tag_empty_set;
const OBJ empty_seq = type_tag_empty_seq;
const OBJ empty_map = type_tag_empty_map;

///////////////////////////////// mem_core.cpp /////////////////////////////////

void *new_obj(int nblocks16);
void *new_obj(int nblocks16_requested, int &nblocks16_returned);
void free_obj(void *obj, int nblocks16);

bool is_alive(void *obj);

int get_live_objs_count();
int get_max_live_objs_count();
int get_total_objs_count();

int get_live_mem_usage();
int get_max_live_mem_usage();
int get_total_mem_requested();

void print_all_live_objs();

/////////////////////////////////// mem.cpp ////////////////////////////////////

void add_ref(REF_OBJ *);
void add_ref(OBJ obj);
void release(OBJ obj);

void mult_add_ref(OBJ obj, int count);

void vec_add_ref(OBJ *objs, int len);
void vec_release(OBJ *objs, int len);

SET_OBJ      *new_set(int size);                                        // Sets ref_count and size
// SEQ_OBJ      *new_seq(int length);                                      // Sets ref_count and length
FULL_SEQ_OBJ *new_full_seq(int length);                                 // Sets ref_count, length, capacity, used_capacity and elems
REF_SEQ_OBJ  *new_ref_seq();                                            // Sets ref_count
REF_SEQ_OBJ  *new_ref_seq(FULL_SEQ_OBJ *full_seq, OBJ *elems, int length);   // Sets all fields
MAP_OBJ      *new_map(int size);                                        // Sets ref_count and size
TAG_OBJ      *new_tag_obj();                                            // Sets ref_count
FLOAT        *new_float();                                              // Sets ref_count

SET_OBJ *shrink_set(SET_OBJ *set, int new_size);

OBJ *new_obj_array(int size);
void delete_obj_array(OBJ *buffer, int size);

int *new_int_array(int size);
void delete_int_array(int *buffer, int size);

void **new_ptr_array(int size);
void delete_ptr_array(void **buffer, int size);

//int get_ref_count(OBJ obj);

//bool is_valid(OBJ obj);
//bool are_valid(OBJ *objs, int count);

//////////////////////////////// mem_utils.cpp /////////////////////////////////

bool is_inline_obj(OBJ obj);
bool is_ref_obj(OBJ obj);

bool is_full_seq(SEQ_OBJ *seq);
bool is_ref_seq(SEQ_OBJ *seq);

bool is_symb(OBJ obj);
bool is_int(OBJ obj);
bool is_float(OBJ obj);

bool is_ne_set(OBJ obj);
bool is_ne_seq(OBJ obj);
bool is_ne_map(OBJ obj);
bool is_tag_obj(OBJ obj);

bool is_set(OBJ obj);
bool is_seq(OBJ obj);
bool is_map(OBJ obj);

SHORT_TYPE_TAG get_short_type_tag(OBJ obj);
TYPE_TAG get_full_type_tag(OBJ obj);

OBJ make_symb(int idx);
int get_symb_idx(OBJ obj);

OBJ *get_key_array_ptr(MAP_OBJ *map);
OBJ *get_value_array_ptr(MAP_OBJ *map);

OBJ make_obj(SET_OBJ *ptr);
OBJ make_obj(SEQ_OBJ *ptr);
OBJ make_obj(MAP_OBJ *ptr);
OBJ make_obj(TAG_OBJ *ptr);
OBJ make_obj(FLOAT *obj);

REF_OBJ      *get_ref_obj_ptr(OBJ obj);
SET_OBJ      *get_set_ptr(OBJ obj);
SEQ_OBJ      *get_seq_ptr(OBJ obj);
FULL_SEQ_OBJ *get_full_seq_ptr(OBJ obj);
REF_SEQ_OBJ  *get_ref_seq_ptr(OBJ obj);
MAP_OBJ      *get_map_ptr(OBJ obj);
TAG_OBJ      *get_tag_obj_ptr(OBJ obj);
FLOAT        *get_float_ptr(OBJ obj);

//////////////////////////////// basic_ops.cpp /////////////////////////////////

bool inline_eq(OBJ obj1, OBJ obj2);
bool are_eq(OBJ obj1, OBJ obj2);
bool is_out_of_range(SET_ITER &it);
bool is_out_of_range(SEQ_ITER &it);
bool is_out_of_range(MAP_ITER &it);
bool has_elem(OBJ set, OBJ elem);

long long get_int_val(OBJ obj);
int get_set_size(OBJ set);
int get_seq_len(OBJ seq);
int get_map_size(OBJ map);
long long mantissa(OBJ obj);
int dec_exp(OBJ obj);
int rand_nat(int max);        // Non-deterministic
long long unique_nat();       // Non-deterministic

// OBJ to_obj(bool b);
// OBJ to_obj(int n);            //## THIS HAS TO GO
// OBJ to_obj(long long n);
OBJ make_bool(bool b);
OBJ make_int(long long n);
OBJ obj_neg(OBJ obj);
OBJ at(OBJ seq, int idx);
OBJ get_tag(OBJ obj);
OBJ get_inner_obj(OBJ obj);
OBJ get_curr_obj(SET_ITER &it);
OBJ get_curr_obj(SEQ_ITER &it);
OBJ get_curr_key(MAP_ITER &it);
OBJ get_curr_value(MAP_ITER &it);
OBJ rand_set_elem(OBJ set);   // Non-deterministic

////////////////////////////////// instrs.cpp //////////////////////////////////

void init(STREAM &s);
void append(STREAM &s, OBJ obj);                // obj must be already reference-counted
OBJ build_set(OBJ *elems, int size);
OBJ build_set(STREAM &s);
OBJ build_seq(OBJ *elems, int length);           // Objects in elems must be already reference-counted
OBJ build_seq(STREAM &s);
OBJ build_map(OBJ *keys, OBJ *values, int size);
OBJ build_map(STREAM &key_stream, STREAM &value_stream);
OBJ build_tagged_obj(OBJ tag, OBJ obj);          // obj must be already reference-counted
OBJ build_float(double val);
OBJ neg_float(OBJ val);
OBJ add_floats(OBJ val1, OBJ val2);
OBJ sub_floats(OBJ val1, OBJ val2);
OBJ mult_floats(OBJ val1, OBJ val2);
OBJ div_floats(OBJ val1, OBJ val2);
OBJ square_root(OBJ val);
OBJ floor(OBJ val);
OBJ ceiling(OBJ val);
OBJ int_to_float(OBJ val);
OBJ blank_array(int size);
OBJ get_seq_slice(OBJ seq, int idx_first, int len);
OBJ append_to_seq(OBJ seq, OBJ obj);            // Both seq and obj must already be reference counted
OBJ join_seqs(OBJ left, OBJ right);
OBJ join_mult_seqs(OBJ seqs);
OBJ rev_seq(OBJ seq);
OBJ get_at(OBJ seq, int idx);                   // Increases reference count
void set_at(OBJ seq, int idx, OBJ value);       // Value must be already reference counted
OBJ lookup(OBJ map, OBJ key);                   // Does not increase reference count
OBJ lookup(OBJ map, OBJ key, bool &found);      // Does not increase reference count
OBJ ext_lookup(OBJ map, OBJ key);               // Does not increase reference count
OBJ ext_lookup(OBJ map, OBJ key, bool &found);  // Does not increase reference count
OBJ merge_sets(OBJ sets);
OBJ merge_maps(OBJ maps);
OBJ seq_to_set(OBJ seq);
OBJ seq_to_mset(OBJ seq);
OBJ list_to_seq(OBJ list);
OBJ internal_sort(OBJ set);
OBJ add_attachment(OBJ target, OBJ data);
OBJ fetch_attachments(OBJ obj);
void get_set_iter(SET_ITER &it, OBJ set);
void get_seq_iter(SEQ_ITER &it, OBJ seq);
void get_map_iter(MAP_ITER &it, OBJ map);
void move_forward(SET_ITER &it);
void move_forward(SEQ_ITER &it);
void move_forward(MAP_ITER &it);
void fail();
void runtime_check(OBJ cond);

/////////////////////////////////// debug.cpp //////////////////////////////////

void push_call_info(const char *fn_name, int arity, OBJ *params);
void pop_call_info();
void print_call_stack();
void dump_var(const char *name, OBJ value);
void print_assertion_failed_msg(const char *file, int line, const char *text);
void fail_if(bool condition, const char *message);
void fail_if_not(bool condition, const char *message);
void hard_fail(const char *message);
void hard_fail_if(bool condition, const char *message);
void hard_fail_if_not(bool condition, const char *message);
void internal_fail();
void internal_fail_if(bool condition);

/////////////////////////////////// algs.cpp ///////////////////////////////////

int  sort_group_and_count(OBJ *objs, int len, int *idxs, OBJ *counters);
int  sort_and_release_dups(OBJ *objs, int size);
void sort_and_check_no_dups(OBJ *keys, OBJ *values, int size);
int  find_obj(OBJ *sorted_array,  int len, OBJ obj);
int  comp_objs(OBJ obj1, OBJ obj2);

/////////////////////////////// inter_utils.cpp ////////////////////////////////

void add_obj_to_cache(OBJ obj);
void release_all_cached_objs();

OBJ to_str(OBJ obj);
OBJ to_symb(OBJ obj);

OBJ str_to_obj(const char *c_str);
void obj_to_str(OBJ str_obj, char *buffer, int size);
char *obj_to_str(OBJ str_obj);

char *obj_to_byte_array(OBJ byte_seq_obj, int &size);

int char_buffer_size(OBJ str_obj);

//////////////////////////////// printing.cpp //////////////////////////////////

void print(OBJ obj);
void print_to_buffer_or_file(OBJ obj, char *buffer, int max_size, const char *fname);
void printed_obj(OBJ obj, char *buffer, int max_size);

///////////////////////////// os_interface_xxx.cpp /////////////////////////////

int get_tick_count();   // Impure


const uint16 symb_idx_false = 0;
const uint16 symb_idx_true = 1;
const uint16 symb_idx_nil = 2;
const uint16 symb_idx_string = 3;
const uint16 symb_idx_just = 4;

OBJ make_blank_obj();
OBJ make_null_obj();
OBJ make_empty_seq();
OBJ make_empty_set();
OBJ make_empty_map();
bool is_symb(OBJ obj, uint16 symb_idx);
bool is_int(OBJ obj, int64 value);
bool is_empty_seq(OBJ obj);
bool is_empty_set(OBJ obj);
bool is_empty_map(OBJ obj);
bool is_blank_obj(OBJ obj);
bool is_null_obj(OBJ obj);
bool get_bool(OBJ obj);
uint32 get_seq_length(OBJ obj);
SEQ_OBJ *new_seq(uint32 len);
OBJ *get_seq_buffer_ptr(OBJ obj);
OBJ make_seq(SEQ_OBJ *seq, uint32 len);


OBJ build_const_uint8_seq(const uint8* buffer, uint32 len);
OBJ build_const_uint16_seq(const uint16* buffer, uint32 len);
OBJ build_const_uint31_seq(const uint32* buffer, uint32 len);
OBJ build_const_int8_seq(const int8* buffer, uint32 len);
OBJ build_const_int16_seq(const int16* buffer, uint32 len);
OBJ build_const_int32_seq(const int32* buffer, uint32 len);
OBJ build_const_int64_seq(const int64* buffer, uint32 len);
