#include "utils.h"


typedef signed char       int8;
typedef signed short      int16;
typedef signed int        int32;
typedef signed long long  int64;

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;




enum OBJ_TYPE {
  // Always inline
  TYPE_BLANK_OBJ  = 0,
  TYPE_NULL_OBJ   = 1,
  TYPE_SYMBOL     = 2,
  TYPE_INTEGER    = 3,
  TYPE_FLOAT      = 4,
  // Inline if empty, references otherwise
  TYPE_SEQUENCE   = 5,
  TYPE_SET        = 6,
  TYPE_MAP        = 7,
  // Always references
  TYPE_TAG_OBJ    = 8,
  TYPE_SLICE      = 9,
};


const uint32 MAX_INLINE_OBJ_TYPE_VALUE  = TYPE_FLOAT;
const uint32 MAX_OBJ_TYPE_VALUE         = TYPE_SLICE;


struct OBJ {
  union {
    int64   int_;
    double  float_;
    void*   ptr;
  } core_data;

  union {
    struct {
      uint16   symb_idx;
      uint16   inner_tag;
      uint16   tag;
      uint8    unused_byte;
      unsigned type        : 4;
      unsigned has_ref     : 1;
      unsigned num_tags    : 2;
      unsigned unused_flag : 1;
    } std;

    struct {
      uint32   length;
      uint16   tag;
      uint8    unused_byte;
      unsigned type        : 4;
      unsigned has_ref     : 1;
      unsigned num_tags    : 2;
      unsigned unused_flag : 1;
    } seq;

    struct {
      uint32   length;
      unsigned offset      : 24;
      unsigned type        : 4;
      unsigned has_ref     : 1;
      unsigned num_tags    : 2;
      unsigned unused_flag : 1;
    } slice;

    uint64 word;
  } extra_data;
};

////////////////////////////////////////////////////////////////////////////////

struct REF_OBJ {
  uint32 ref_count;
};


struct SEQ_OBJ {
  uint32 ref_count;
  uint32 capacity;
  uint32 size;
  OBJ    buffer[1];
};


struct SET_OBJ {
  uint32 ref_count;
  uint32 size;
  OBJ    buffer[1];
};


struct MAP_OBJ {
  uint32 ref_count;
  uint32 size;
  OBJ    buffer[1];
};


struct TAG_OBJ {
  uint32 ref_count;
  uint16 tag_idx;
  //## ADD SOME PADDING HERE?
  OBJ    obj;
};

////////////////////////////////////////////////////////////////////////////////

struct SEQ_ITER {
  OBJ    *buffer;
  uint32  idx;
  uint32  len;
};


struct SET_ITER {
  OBJ    *buffer;
  uint32  idx;
  uint32  size;
};


struct MAP_ITER {
  OBJ    *buffer;
  uint32  idx;
  uint32  size;
};


struct STREAM {
  OBJ    *buffer;
  uint32  capacity;
  uint32  count;
};

////////////////////////////////////////////////////////////////////////////////

struct VALUE_STORE {
  OBJ *slots;
  uint32 size;
  uint32 used;
  uint32 first_free;
};


struct UNARY_TABLE {
  uint64 *bitmap;
  uint32 size;
  uint32 count;
};


struct UNARY_TABLE_UPDATES {
  vector<uint32> deletes;
  vector<uint32> inserts;
};


struct UNARY_TABLE_ITER {
  uint64 *bitmap;
  uint32 size;
  uint32 curr_value;
};


struct BINARY_TABLE {
  set<uint64> left_to_right;
  set<uint64> right_to_left;
};


struct BINARY_TABLE_UPDATES {
  vector<uint64> deletes;
  vector<uint64> inserts;
};


struct BINARY_TABLE_ITER {
  set<uint64>::iterator iter;
  set<uint64>::iterator end;
  uint32 value;
  bool reversed;
};


struct tuple3 {
  uint64 fields01;
  uint32 field2;

  bool operator == (const tuple3 &o) const {
    return fields01 == o.fields01 & field2 == o.field2;
  }

  bool operator < (const tuple3 &o) const {
    return fields01 != o.fields01 ? fields01 < o.fields01 : field2 < o.field2;
  }
};

// bool operator < (const tuple3 &l, tuple3 &r) {
//   return l.fields01 != r.fields01 ? l.fields01 < r.fields01 : l.field2 < r.field2;
// }


struct TERNARY_TABLE {
  set<tuple3> unshifted;
  set<tuple3> shifted_once;
  set<tuple3> shifted_twice;
};


struct TERNARY_TABLE_UPDATES {
  vector<tuple3> deletes;
  vector<tuple3> inserts;
};


struct TERNARY_TABLE_ITER {
  set<tuple3>::iterator iter;
  set<tuple3>::iterator end;
  uint64 excl_upper_bound;
  uint8 shift;
};

///////////////////////////////////////////////////////////////

const uint64 MAX_SEQ_LEN = 0xFFFFFFFF;

const uint16 symb_idx_false   = 0;
const uint16 symb_idx_true    = 1;
const uint16 symb_idx_nil     = 2;
const uint16 symb_idx_string  = 3;
const uint16 symb_idx_just    = 4;

///////////////////////////////// mem-core.cpp /////////////////////////////////

void* new_obj(uint32 nblocks16);
void* new_obj(uint32 nblocks16_requested, uint32 &nblocks16_returned);
void  free_obj(void* obj, uint32 nblocks16);

bool is_alive(void* obj);

uint32 get_live_objs_count();
uint32 get_max_live_objs_count();
uint32 get_total_objs_count();

uint32 get_live_mem_usage();
uint32 get_max_live_mem_usage();
uint32 get_total_mem_requested();

void print_all_live_objs();

/////////////////////////////////// mem.cpp ////////////////////////////////////

void add_ref(REF_OBJ *);
void add_ref(OBJ);
void release(OBJ);

void mult_add_ref(OBJ obj, uint32 count);

void vec_add_ref(OBJ* objs, uint32 len);
void vec_release(OBJ* objs, uint32 len);

SET_OBJ* new_set(uint32 size);      // Sets ref_count and size
SEQ_OBJ* new_seq(uint32 length);    // Sets ref_count, length, capacity, used_capacity and elems
MAP_OBJ* new_map(uint32 size);      // Sets ref_count and size
TAG_OBJ* new_tag_obj();             // Sets ref_count

SET_OBJ* shrink_set(SET_OBJ* set, uint32 new_size);

OBJ* new_obj_array(uint32 size);
void delete_obj_array(OBJ* buffer, uint32 size);

uint32 *new_uint32_array(uint32 size);
void delete_uint32_array(uint32 *buffer, uint32 size);

// int* new_int_array(uint32 size);
// void delete_int_array(int* buffer, uint32 size);

void** new_ptr_array(uint32 size);
void delete_ptr_array(void** buffer, uint32 size);

//uint32 get_ref_count(OBJ);

//bool is_valid(OBJ);
//bool are_valid(OBJ* objs, uint32 count);

//////////////////////////////// mem-utils.cpp /////////////////////////////////

OBJ_TYPE get_logical_type(OBJ); //## RENAME TO JUST get_type() WHEN ALL IS DONE

bool is_blank_obj(OBJ);
bool is_null_obj(OBJ);
bool is_symb(OBJ);
bool is_bool(OBJ);
bool is_int(OBJ);
bool is_float(OBJ);
bool is_seq(OBJ);
bool is_empty_seq(OBJ);
bool is_ne_seq(OBJ);
bool is_set(OBJ);
bool is_empty_set(OBJ);
bool is_ne_set(OBJ);
bool is_map(OBJ);
bool is_empty_map(OBJ);
bool is_ne_map(OBJ);
bool is_tag_obj(OBJ);

bool is_symb(OBJ, uint16);
bool is_int(OBJ, int64);

uint16 get_symb_idx(OBJ);
bool   get_bool(OBJ);
int64  get_int(OBJ);
double get_float(OBJ);
uint32 get_seq_length(OBJ);
uint16 get_tag_idx(OBJ);
OBJ    get_inner_obj(OBJ);

OBJ make_blank_obj();
OBJ make_null_obj();
OBJ make_empty_seq();
OBJ make_empty_set();
OBJ make_empty_map();
OBJ make_symb(uint16 symb_idx);
OBJ make_bool(bool b);
OBJ make_int(uint64 value);
OBJ make_float(double value);
OBJ make_seq(SEQ_OBJ* ptr, uint32 length);
OBJ make_slice(SEQ_OBJ* ptr, uint32 offset, uint32 length);
OBJ make_set(SET_OBJ* ptr);
OBJ make_map(MAP_OBJ* ptr);
OBJ make_tag_obj(uint16 tag_idx, OBJ obj);

// These functions exist in a limbo between the logical and physical world

uint32 get_seq_offset(OBJ seq);
OBJ* get_seq_buffer_ptr(OBJ);

OBJ* get_key_array_ptr(MAP_OBJ* map);
OBJ* get_value_array_ptr(MAP_OBJ* map);

SEQ_OBJ* get_seq_obj_ptr(OBJ);
SET_OBJ* get_set_ptr(OBJ);
MAP_OBJ* get_map_ptr(OBJ);

// Purely physical representation functions

bool is_inline_obj(OBJ);
bool is_ref_obj(OBJ);

OBJ_TYPE get_ref_obj_type(OBJ);
REF_OBJ* get_ref_obj_ptr(OBJ);

bool are_shallow_eq(OBJ, OBJ);
int shallow_cmp(OBJ, OBJ);

//////////////////////////////// basic-ops.cpp /////////////////////////////////

bool inline_eq(OBJ obj1, OBJ obj2);
bool are_eq(OBJ obj1, OBJ obj2);
bool is_out_of_range(SET_ITER &it);
bool is_out_of_range(SEQ_ITER &it);
bool is_out_of_range(MAP_ITER &it);
bool has_elem(OBJ set, OBJ elem);

int64 get_int_val(OBJ);
uint32 get_set_size(OBJ set);
uint32 get_seq_len(OBJ seq);
uint32 get_map_size(OBJ map);
int64 mantissa(OBJ);
int64 dec_exp(OBJ);
int64 rand_nat(int64 max);  // Non-deterministic
int64 unique_nat();         // Non-deterministic

OBJ obj_neg(OBJ);
OBJ at(OBJ seq, int64 idx);
OBJ get_tag(OBJ);
OBJ get_curr_obj(SET_ITER &it);
OBJ get_curr_obj(SEQ_ITER &it);
OBJ get_curr_key(MAP_ITER &it);
OBJ get_curr_value(MAP_ITER &it);
OBJ rand_set_elem(OBJ set);   // Non-deterministic

OBJ search_or_lookup(OBJ coll, OBJ value);

////////////////////////////////// instrs.cpp //////////////////////////////////

void init(STREAM &s);
void append(STREAM &s, OBJ obj);                // obj must be already reference-counted
OBJ build_set(OBJ* elems, uint32 size);
OBJ build_set(STREAM &s);
OBJ build_seq(OBJ* elems, uint32 length);           // Objects in elems must be already reference-counted
OBJ build_seq(STREAM &s);
OBJ build_map(OBJ* keys, OBJ* values, uint32 size);
OBJ build_map(STREAM &key_stream, STREAM &value_stream);
OBJ build_tagged_obj(OBJ tag, OBJ obj);          // obj must be already reference-counted
// OBJ make_float(double val); // Already defined in mem_utils.cpp
OBJ neg_float(OBJ val);
OBJ add_floats(OBJ val1, OBJ val2);
OBJ sub_floats(OBJ val1, OBJ val2);
OBJ mult_floats(OBJ val1, OBJ val2);
OBJ div_floats(OBJ val1, OBJ val2);
OBJ square_root(OBJ val);
OBJ floor(OBJ val);
OBJ ceiling(OBJ val);
OBJ int_to_float(OBJ val);
OBJ blank_array(int64 size);
OBJ get_seq_slice(OBJ seq, int64 idx_first, int64 len);
OBJ append_to_seq(OBJ seq, OBJ obj);            // Both seq and obj must already be reference counted
OBJ join_seqs(OBJ left, OBJ right);
// OBJ join_mult_seqs(OBJ seqs);
OBJ rev_seq(OBJ seq);
OBJ get_at(OBJ seq, uint32 idx);                   // Increases reference count
void set_at(OBJ seq, uint32 idx, OBJ value);       // Value must be already reference counted
OBJ lookup(OBJ map, OBJ key);                   // Does not increase reference count
OBJ lookup(OBJ map, OBJ key, bool &found);      // Does not increase reference count
OBJ ext_lookup(OBJ map, OBJ key);               // Does not increase reference count
OBJ ext_lookup(OBJ map, OBJ key, bool &found);  // Does not increase reference count
OBJ merge_sets(OBJ sets);
OBJ merge_maps(OBJ maps);
OBJ seq_to_set(OBJ seq);
OBJ seq_to_mset(OBJ seq);
// OBJ list_to_seq(OBJ list);
OBJ internal_sort(OBJ set);
OBJ add_attachment(OBJ target, OBJ data);
OBJ fetch_attachments(OBJ);
void get_set_iter(SET_ITER &it, OBJ set);
void get_seq_iter(SEQ_ITER &it, OBJ seq);
void get_map_iter(MAP_ITER &it, OBJ map);
void move_forward(SET_ITER &it);
void move_forward(SEQ_ITER &it);
void move_forward(MAP_ITER &it);
void fail();
void runtime_check(OBJ cond);

OBJ build_const_uint8_seq(const uint8* buffer, uint32 len);
OBJ build_const_uint16_seq(const uint16* buffer, uint32 len);
OBJ build_const_uint32_seq(const uint32* buffer, uint32 len);
OBJ build_const_int8_seq(const int8* buffer, uint32 len);
OBJ build_const_int16_seq(const int16* buffer, uint32 len);
OBJ build_const_int32_seq(const int32* buffer, uint32 len);
OBJ build_const_int64_seq(const int64* buffer, uint32 len);

/////////////////////////////////// debug.cpp //////////////////////////////////

void push_call_info(const char* fn_name, uint32 arity, OBJ* params);
void pop_call_info();
void print_call_stack();
void dump_var(const char* name, OBJ value);
void print_assertion_failed_msg(const char* file, uint32 line, const char* text);
void fail_if(bool condition, const char* message);
void fail_if_not(bool condition, const char* message);
void hard_fail(const char* message);
void hard_fail_if(bool condition, const char* message);
void hard_fail_if_not(bool condition, const char* message);
void internal_fail();
void internal_fail_if(bool condition);

/////////////////////////////////// algs.cpp ///////////////////////////////////

uint32 sort_group_and_count(OBJ* objs, uint32 len, uint32* idxs, OBJ* counters);
uint32 sort_and_release_dups(OBJ* objs, uint32 size);
void sort_and_check_no_dups(OBJ* keys, OBJ* values, uint32 size);
uint32 find_obj(OBJ* sorted_array, uint32 len, OBJ obj, bool &found); //## WHAT SHOULD THIS RETURN? ANY VALUE IN THE [0, 2^32-1] IS A VALID SEQUENCE INDEX, SO WHAT COULD BE USED TO REPRESENT "NOT FOUND"?
int comp_objs(OBJ obj1, OBJ obj2);

/////////////////////////////// inter-utils.cpp ////////////////////////////////

void add_obj_to_cache(OBJ);
void release_all_cached_objs();

OBJ to_str(OBJ);
OBJ to_symb(OBJ);

OBJ str_to_obj(const char* c_str);
// void obj_to_str(OBJ str_obj, char* buffer, uint32 size);
char* obj_to_str(OBJ str_obj);

char* obj_to_byte_array(OBJ byte_seq_obj, uint32 &size);

uint64 char_buffer_size(OBJ str_obj);

//////////////////////////////// printing.cpp //////////////////////////////////

void print(OBJ);
void print_to_buffer_or_file(OBJ obj, char* buffer, uint32 max_size, const char* fname);
void printed_obj(OBJ obj, char* buffer, uint32 max_size);

///////////////////////////// os-interface-xxx.cpp /////////////////////////////

uint64 get_tick_count();   // Impure

//////////////////////////////// value-store.cpp ///////////////////////////////

void value_store_init(VALUE_STORE *store);
void value_store_cleanup(VALUE_STORE *store);

OBJ lookup_surrogate(VALUE_STORE *store, int64 surr);
int64 lookup_value(VALUE_STORE *store, OBJ value);

int64 insert_value(VALUE_STORE *store, OBJ value);

//////////////////////////////// unary-table.cpp ///////////////////////////////

void unary_table_init(UNARY_TABLE *table);
void unary_table_cleanup(UNARY_TABLE *table);

void unary_table_updates_init(UNARY_TABLE_UPDATES *table);
void unary_table_updates_cleanup(UNARY_TABLE_UPDATES *table);

bool unary_table_contains(UNARY_TABLE *table, uint32 value);

void unary_table_insert(UNARY_TABLE_UPDATES *updates, uint32 value);

void unary_table_delete(UNARY_TABLE *table, UNARY_TABLE_UPDATES *updates, uint32 value);
void unary_table_clear(UNARY_TABLE *table, UNARY_TABLE_UPDATES *updates);

bool unary_table_updates_check(UNARY_TABLE *table, UNARY_TABLE_UPDATES *updates);
void unary_table_updates_apply(UNARY_TABLE *table, UNARY_TABLE_UPDATES *updates);

void unary_table_get_iter(UNARY_TABLE *table, UNARY_TABLE_ITER *iter);
void unary_table_iter_next(UNARY_TABLE_ITER *iter);

bool unary_table_iter_is_out_of_range(UNARY_TABLE_ITER *iter);

uint32 unary_table_iter_get_field(UNARY_TABLE_ITER *iter);

/////////////////////////////// binary-table.cpp ///////////////////////////////

void binary_table_init(BINARY_TABLE *table);
void binary_table_cleanup(BINARY_TABLE *table);

void binary_table_updates_init(BINARY_TABLE_UPDATES *updates);
void binary_table_updates_cleanup(BINARY_TABLE_UPDATES *updates);

bool binary_table_contains(BINARY_TABLE *table, uint32 left_val, uint32 right_val);

void binary_table_delete(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates, uint32 left_val, uint32 right_val);
void binary_table_delete_by_col_0(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates, uint32 value);
void binary_table_delete_by_col_1(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates, uint32 value);
void binary_table_clear(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates);

void binary_table_insert(BINARY_TABLE_UPDATES *updates, uint32 left_val, uint32 right_val);

bool binary_table_updates_check_0(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates);
bool binary_table_updates_check_1(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates);
bool binary_table_updates_check_0_1(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates);

void binary_table_updates_apply(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates);

void binary_table_get_iter_by_col_0(BINARY_TABLE *table, BINARY_TABLE_ITER *iter, uint32 value);
void binary_table_get_iter_by_col_1(BINARY_TABLE *table, BINARY_TABLE_ITER *iter, uint32 value);
void binary_table_get_iter(BINARY_TABLE *table, BINARY_TABLE_ITER *iter);

bool binary_table_iter_is_out_of_range(BINARY_TABLE_ITER *iter);

uint32 binary_table_iter_get_left_field(BINARY_TABLE_ITER *iter);
uint32 binary_table_iter_get_right_field(BINARY_TABLE_ITER *iter);

void binary_table_iter_next(BINARY_TABLE_ITER *iter);

/////////////////////////////// ternary-table.cpp ///////////////////////////////

void ternary_table_init(TERNARY_TABLE *table);
void ternary_table_cleanup(TERNARY_TABLE *table);

void ternary_table_updates_init(TERNARY_TABLE_UPDATES *updates);
void ternary_table_updates_cleanup(TERNARY_TABLE_UPDATES *updates);

bool ternary_table_contains(TERNARY_TABLE *table, uint32 left_val, uint32 middle_val, uint32 right_val);

void ternary_table_delete(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 left_val, uint32 middle_val, uint32 right_val);
void ternary_table_delete_by_cols_01(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 value0, uint32 value1);
void ternary_table_delete_by_cols_02(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 value0, uint32 value2);
void ternary_table_delete_by_cols_12(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 value1, uint32 value2);
void ternary_table_delete_by_col_0(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 value);
void ternary_table_delete_by_col_1(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 value);
void ternary_table_delete_by_col_2(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 value);
void ternary_table_clear(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates);

void ternary_table_insert(TERNARY_TABLE_UPDATES *updates, uint32 left_val, uint32 middle_val, uint32 right_val);

bool ternary_table_updates_check_01(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates);
bool ternary_table_updates_check_01_2(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates);
bool ternary_table_updates_check_01_12(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates);
bool ternary_table_updates_check_01_12_20(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates);

void ternary_table_updates_apply(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates);

void ternary_table_get_iter_by_cols_01(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter, uint32 value0, uint32 value1);
void ternary_table_get_iter_by_cols_02(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter, uint32 value0, uint32 value2);
void ternary_table_get_iter_by_cols_12(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter, uint32 value1, uint32 value2);
void ternary_table_get_iter_by_col_0(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter, uint32 value);
void ternary_table_get_iter_by_col_1(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter, uint32 value);
void ternary_table_get_iter_by_col_2(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter, uint32 value);
void ternary_table_get_iter(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter);

bool ternary_table_iter_is_out_of_range(TERNARY_TABLE_ITER *iter);

uint32 ternary_table_iter_get_left_field(TERNARY_TABLE_ITER *iter);
uint32 ternary_table_iter_get_middle_field(TERNARY_TABLE_ITER *iter);
uint32 ternary_table_iter_get_right_field(TERNARY_TABLE_ITER *iter);

void ternary_table_iter_next(TERNARY_TABLE_ITER *iter);
