#include <cstring>
#include <iostream>
#include <cstdio>

using namespace std;

#include "lib.h"
#include "os_interface.h"
#include "generated.h"


int main(int argc, char **argv)
{
  OBJ args = make_empty_seq();
  if (argc > 1)
  {
    OBJ *arg_buff = new OBJ[argc-1];
    for (int i=0 ; i < argc-1 ; i++)
      arg_buff[i] = str_to_obj(argv[i+1]);
    args = build_seq(arg_buff, argc-1);
  }

  generated::ENV env;
  memset(&env, 0, sizeof(generated::ENV));

  OBJ res = Main_P(args, env);

  release(args);
  release(res);

  release_all_cached_objs();

#ifndef NDEBUG
  cerr << "\nNumber of live objects: " << get_live_objs_count() << endl;
  cerr << "Max number of live objects: " << get_max_live_objs_count() << endl;
  cerr << "Total number of allocated objects: " << get_total_objs_count() << endl;

  cerr << "\nLive memory usage: " << get_live_mem_usage() << endl;
  cerr << "Max live memory usage: " << get_max_live_mem_usage() << endl;
  cerr << "Total memory requested: " << get_total_mem_requested() << endl;

  // print_all_live_objs();
#endif

  return is_int(res) ? get_int_val(res) : 0;
}
