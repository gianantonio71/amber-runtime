#include "utils.h"


typedef long long Obj;


struct RefObj
{
  unsigned int ref_count;
};


struct Set : public RefObj
{
  int size;
  Obj elems[1];
};


struct Seq : public RefObj
{
  unsigned int length;
  Obj *elems;
};


struct FullSeq : public Seq
{
  unsigned int capacity;
  unsigned int used_capacity;
  Obj buffer[1];
};


struct RefSeq : public Seq
{
  FullSeq *full_seq;
};


struct Map : public RefObj
{
  int size;
  Obj buffer[1];
};


struct TagObj : public RefObj
{
  Obj tag;
  Obj obj;
};


struct Float : public RefObj
{
  int _;
  double value;
};

////////////////////////////////////////////////////////////////////////////////

struct SetIter
{
  Set *set;
  int idx;
  int size;
};

struct SeqIter
{
  Seq *seq;
  int idx;
  int len;
};

struct MapIter
{
  Map *map;
  int idx;
  int size;
};

struct Stream
{
  Obj *buffer;
  int capacity;
  int count;
};

////////////////////////////////////////////////////////////////////////////////

enum ShortTypeTag {
  short_type_tag_integer    = 0,  //  00      Inline integer
  short_type_tag_float      = 1,  //  01      Inline floating point
  short_type_tag_inline     = 2,  //  10      Other inline objects
  short_type_tag_reference  = 3   //  11      Object containing a pointer
};

enum TypeTag {
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

const Obj null_obj  = type_tag_null;
const Obj empty_set = type_tag_empty_set;
const Obj empty_seq = type_tag_empty_seq;
const Obj empty_map = type_tag_empty_map;

const int SHORT_TAG_SIZE  = 2;
const int FULL_TAG_SIZE   = 8;
const int POINTER_SHIFT   = 24;

const int SHORT_TAG_MASK  = (1 << SHORT_TAG_SIZE) - 1;
const int FULL_TAG_MASK   = (1 << FULL_TAG_SIZE) - 1;

#define symb(idx) ((idx << FULL_TAG_SIZE) | type_tag_symb)

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

void add_ref(RefObj *);
void add_ref(Obj obj);
void release(Obj obj);

void mult_add_ref(Obj obj, int count);

void vec_add_ref(Obj *objs, int len);
void vec_release(Obj *objs, int len);

Set      *new_set(int size);                                        // Sets ref_count and size
// Seq      *new_seq(int length);                                      // Sets ref_count and length
FullSeq  *new_full_seq(int length);                                 // Sets ref_count, length, capacity, used_capacity and elems
RefSeq   *new_ref_seq();                                            // Sets ref_count
RefSeq   *new_ref_seq(FullSeq *full_seq, Obj *elems, int length);   // Sets all fields
Map      *new_map(int size);                                        // Sets ref_count and size
TagObj   *new_tag_obj();                                            // Sets ref_count
Float    *new_float();                                              // Sets ref_count

Set *shrink_set(Set *set, int new_size);

Obj *new_obj_array(int size);
void delete_obj_array(Obj *buffer, int size);

int *new_int_array(int size);
void delete_int_array(int *buffer, int size);

void **new_ptr_array(int size);
void delete_ptr_array(void **buffer, int size);

//int get_ref_count(Obj obj);

//bool is_valid(Obj obj);
//bool are_valid(Obj *objs, int count);

//////////////////////////////// mem_utils.cpp /////////////////////////////////

bool is_inline_obj(Obj obj);
bool is_ref_obj(Obj obj);

bool is_full_seq(Seq *seq);
bool is_ref_seq(Seq *seq);

bool is_symb(Obj obj);
bool is_int(Obj obj);
bool is_float(Obj obj);

bool is_ne_set(Obj obj);
bool is_ne_seq(Obj obj);
bool is_ne_map(Obj obj);
bool is_tag_obj(Obj obj);

bool is_set(Obj obj);
bool is_seq(Obj obj);
bool is_map(Obj obj);

ShortTypeTag get_short_type_tag(Obj obj);
TypeTag get_full_type_tag(Obj obj);

Obj make_symb(int idx);
int get_symb_idx(Obj obj);

Obj *get_key_array_ptr(Map *map);
Obj *get_value_array_ptr(Map *map);

Obj make_obj(Set *ptr);
Obj make_obj(Seq *ptr);
Obj make_obj(Map *ptr);
Obj make_obj(TagObj *ptr);
Obj make_obj(Float *obj);

RefObj   *get_ref_obj_ptr(Obj obj);
Set      *get_set_ptr(Obj obj);
Seq      *get_seq_ptr(Obj obj);
FullSeq  *get_full_seq_ptr(Obj obj);
RefSeq   *get_ref_seq_ptr(Obj obj);
Map      *get_map_ptr(Obj obj);
TagObj   *get_tag_obj_ptr(Obj obj);
Float    *get_float_ptr(Obj obj);

//////////////////////////////// basic_ops.cpp /////////////////////////////////

bool inline_eq(Obj obj1, Obj obj2);
bool are_eq(Obj obj1, Obj obj2);
bool is_out_of_range(SetIter &it);
bool is_out_of_range(SeqIter &it);
bool is_out_of_range(MapIter &it);
bool has_elem(Obj set, Obj elem);

long long get_int_val(Obj obj);
int get_set_size(Obj set);
int get_seq_len(Obj seq);
int get_map_size(Obj map);
long long mantissa(Obj obj);
int dec_exp(Obj obj);
int rand_nat(int max);        // Non-deterministic
long long unique_nat();       // Non-deterministic

Obj to_obj(bool b);
Obj to_obj(int n);
Obj to_obj(long long n);
Obj obj_neg(Obj obj);
Obj at(Obj seq, int idx);
Obj get_tag(Obj obj);
Obj get_inner_obj(Obj obj);
Obj get_curr_obj(SetIter &it);
Obj get_curr_obj(SeqIter &it);
Obj get_curr_key(MapIter &it);
Obj get_curr_value(MapIter &it);
Obj rand_set_elem(Obj set);   // Non-deterministic

////////////////////////////////// instrs.cpp //////////////////////////////////

void init(Stream &s);
void append(Stream &s, Obj obj);                // obj must be already reference-counted
Obj make_set(Obj *elems, int size);
Obj make_set(Stream &s);
Obj make_seq(Obj *elems, int length);           // Objects in elems must be already reference-counted
Obj make_seq(Stream &s);
Obj make_map(Obj *keys, Obj *values, int size);
Obj make_map(Stream &key_stream, Stream &value_stream);
Obj make_tagged_obj(Obj tag, Obj obj);          // obj must be already reference-counted
Obj make_float(double val);
Obj neg_float(Obj val);
Obj add_floats(Obj val1, Obj val2);
Obj sub_floats(Obj val1, Obj val2);
Obj mult_floats(Obj val1, Obj val2);
Obj div_floats(Obj val1, Obj val2);
Obj square_root(Obj val);
Obj floor(Obj val);
Obj ceiling(Obj val);
Obj int_to_float(Obj val);
Obj make_array(int size, Obj value);
Obj get_seq_slice(Obj seq, int idx_first, int len);
Obj append_to_seq(Obj seq, Obj obj);            // Both seq and obj must already be reference counted
Obj join_seqs(Obj left, Obj right);
Obj join_mult_seqs(Obj seqs);
Obj rev_seq(Obj seq);
Obj get_at(Obj seq, int idx);                   // Increases reference count
void set_at(Obj seq, int idx, Obj value);       // Value must be already reference counted
Obj lookup(Obj map, Obj key);                   // Does not increase reference count
Obj lookup(Obj map, Obj key, bool &found);      // Does not increase reference count
Obj ext_lookup(Obj map, Obj key);               // Does not increase reference count
Obj ext_lookup(Obj map, Obj key, bool &found);  // Does not increase reference count
Obj merge_sets(Obj sets);
Obj merge_maps(Obj maps);
Obj seq_to_set(Obj seq);
Obj seq_to_mset(Obj seq);
Obj list_to_seq(Obj list);
Obj internal_sort(Obj set);
Obj add_attachment(Obj target, Obj data);
Obj fetch_attachments(Obj obj);
void get_set_iter(SetIter &it, Obj set);
void get_seq_iter(SeqIter &it, Obj seq);
void get_map_iter(MapIter &it, Obj map);
void move_forward(SetIter &it);
void move_forward(SeqIter &it);
void move_forward(MapIter &it);
void fail();
void runtime_check(Obj cond);

/////////////////////////////////// debug.cpp //////////////////////////////////

void push_call_info(const char *fn_name, int arity, Obj *params);
void pop_call_info();
void print_call_stack();
void fail_if(bool condition, const char *message);
void fail_if_not(bool condition, const char *message);
void hard_fail(const char *message);
void hard_fail_if(bool condition, const char *message);
void hard_fail_if_not(bool condition, const char *message);
void internal_fail();
void internal_fail_if(bool condition);

/////////////////////////////////// algs.cpp ///////////////////////////////////

int  sort_group_and_count(Obj *objs, int len, int *idxs, Obj *counters);
int  sort_and_release_dups(Obj *objs, int size);
void sort_and_check_no_dups(Obj *keys, Obj *values, int size);
int  find_obj(Obj *sorted_array,  int len, Obj obj);
int  comp_objs(Obj obj1, Obj obj2);

/////////////////////////////// inter_utils.cpp ////////////////////////////////

void add_obj_to_cache(Obj obj);
void release_all_cached_objs();

Obj to_str(Obj obj);
Obj to_symb(Obj obj);

void print(Obj obj);

Obj str_to_obj(const char *c_str);
void obj_to_str(Obj str_obj, char *buffer, int size);
char *obj_to_str(Obj str_obj);

char *obj_to_byte_array(Obj byte_seq_obj, int &size);

int char_buffer_size(Obj str_obj);

///////////////////////////// os_interface_xxx.cpp /////////////////////////////

int get_tick_count();   // Impure
