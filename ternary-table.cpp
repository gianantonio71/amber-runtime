#include "lib.h"
#include "table-utils.h"

void ternary_table_init(TERNARY_TABLE *table)
{

}

void ternary_table_cleanup(TERNARY_TABLE *table)
{

}

////////////////////////////////////////////////////////////////////////////////

void ternary_table_updates_init(TERNARY_TABLE_UPDATES *updates)
{

}


void ternary_table_updates_cleanup(TERNARY_TABLE_UPDATES *updates)
{

}

////////////////////////////////////////////////////////////////////////////////

bool ternary_table_contains(TERNARY_TABLE *table, uint32 left_val, uint32 middle_val, uint32 right_val)
{
  tuple3 entry;
  build(entry, left_val, middle_val, right_val);

  set<tuple3> &unshifted = table->unshifted;
  set<tuple3>::iterator it = unshifted.find(entry);
  return it != unshifted.end();
}

////////////////////////////////////////////////////////////////////////////////

void ternary_table_delete_range_(TERNARY_TABLE_ITER *iter, TERNARY_TABLE_UPDATES *updates)
{
  int shift = iter->shift;
  vector<tuple3> &deletes = updates->deletes;
  while (!ternary_table_iter_is_out_of_range(iter))
  {
    if (shift == 0)
    {
      deletes.push_back(*iter->iter);
    }
    else
    {
      assert(shift == 1 || shift == 2);

      uint64 fields01 = iter->iter->fields01;
      uint32 field2 = iter->iter->field2;

      tuple3 entry;

      if (shift == 1)
        build(entry, field2, left(fields01), right(fields01));
      else
        build(entry, right(fields01), field2, left(fields01));

      deletes.push_back(entry);
    }

    ternary_table_iter_next(iter);
  }
}

void ternary_table_delete(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 left_val, uint32 middle_val, uint32 right_val)
{
  if (ternary_table_contains(table, left_val, middle_val, right_val))
  {
    tuple3 entry;
    build(entry, left_val, middle_val, right_val);
    updates->deletes.push_back(entry);
  }
}

void ternary_table_delete_by_cols_01(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 value0, uint32 value1)
{
  TERNARY_TABLE_ITER iter;
  ternary_table_get_iter_by_cols_01(table, &iter, value0, value1);
  ternary_table_delete_range_(&iter, updates);
}

void ternary_table_delete_by_cols_02(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 value0, uint32 value2)
{
  TERNARY_TABLE_ITER iter;
  ternary_table_get_iter_by_cols_02(table, &iter, value0, value2);
  ternary_table_delete_range_(&iter, updates);
}

void ternary_table_delete_by_cols_12(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 value1, uint32 value2)
{
  TERNARY_TABLE_ITER iter;
  ternary_table_get_iter_by_cols_12(table, &iter, value1, value2);
  ternary_table_delete_range_(&iter, updates);
}

void ternary_table_delete_by_col_0(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 value)
{
  TERNARY_TABLE_ITER iter;
  ternary_table_get_iter_by_col_0(table, &iter, value);
  ternary_table_delete_range_(&iter, updates);
}

void ternary_table_delete_by_col_1(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 value)
{
  TERNARY_TABLE_ITER iter;
  ternary_table_get_iter_by_col_1(table, &iter, value);
  ternary_table_delete_range_(&iter, updates);
}

void ternary_table_delete_by_col_2(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates, uint32 value)
{
  TERNARY_TABLE_ITER iter;
  ternary_table_get_iter_by_col_2(table, &iter, value);
  ternary_table_delete_range_(&iter, updates);
}

void ternary_table_clear(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates)
{
  TERNARY_TABLE_ITER iter;
  ternary_table_get_iter(table, &iter);
  ternary_table_delete_range_(&iter, updates);
}

////////////////////////////////////////////////////////////////////////////////

void ternary_table_insert(TERNARY_TABLE_UPDATES *updates, uint32 left_val, uint32 middle_val, uint32 right_val)
{
  tuple3 entry;
  build(entry, left_val, middle_val, right_val);
  updates->inserts.push_back(entry);
}

////////////////////////////////////////////////////////////////////////////////

void ternary_table_updates_apply(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates)
{
  set<tuple3> &unshifted = table->unshifted;
  set<tuple3> &shifted_once = table->shifted_once;
  set<tuple3> &shifted_twice = table->shifted_twice;

  vector<tuple3>::iterator it = updates->deletes.begin();
  vector<tuple3>::iterator end = updates->deletes.end();
  for ( ; it != end ; it++)
  {
    tuple3 entry = *it;
    unshifted.erase(entry);
    shift(entry);
    shifted_once.erase(entry);
    shift(entry);
    shifted_twice.erase(entry);
  }

  it = updates->inserts.begin();
  end = updates->inserts.end();
  for ( ; it != end ; it++)
  {
    tuple3 entry = *it;
    unshifted.insert(entry);
    shift(entry);
    shifted_once.insert(entry);
    shift(entry);
    shifted_twice.insert(entry);
  }
}

////////////////////////////////////////////////////////////////////////////////

void ternary_table_get_iter_by_cols_01(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter, uint32 value0, uint32 value1)
{
  set<tuple3> &target = table->unshifted;
  tuple3 lb;
  build(lb, value0, value1, 0);
  iter->iter = target.lower_bound(lb);
  iter->end = target.end();
  iter->excl_upper_bound = pack(value0, value1+1);
  iter->shift = 0;
}

void ternary_table_get_iter_by_cols_02(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter, uint32 value0, uint32 value2)
{
  set<tuple3> &target = table->shifted_twice;
  tuple3 lb;
  build(lb, value2, value0, 0);
  iter->iter = target.lower_bound(lb);
  iter->end = target.end();
  iter->excl_upper_bound = pack(value2, value0+1);
  iter->shift = 2;
}

void ternary_table_get_iter_by_cols_12(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter, uint32 value1, uint32 value2)
{
  set<tuple3> &target = table->shifted_once;
  tuple3 lb;
  build(lb, value1, value2, 0);
  iter->iter = target.lower_bound(lb);
  iter->end = target.end();
  iter->excl_upper_bound = pack(value1, value2+1);
  iter->shift = 1;
}

void ternary_table_get_iter_by_col_0(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter, uint32 value)
{
  set<tuple3> &target = table->unshifted;
  tuple3 lb;
  build(lb, value, 0, 0);
  iter->iter = target.lower_bound(lb);
  iter->end = target.end();
  iter->excl_upper_bound = pack(value+1, 0);
  iter->shift = 0;
}

void ternary_table_get_iter_by_col_1(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter, uint32 value)
{
  set<tuple3> &target = table->shifted_once;
  tuple3 lb;
  build(lb, value, 0, 0);
  iter->iter = target.lower_bound(lb);
  iter->end = target.end();
  iter->excl_upper_bound = pack(value+1, 0);
  iter->shift = 1;
}

void ternary_table_get_iter_by_col_2(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter, uint32 value)
{
  set<tuple3> &target = table->shifted_twice;
  tuple3 lb;
  build(lb, value, 0, 0);
  iter->iter = target.lower_bound(lb);
  iter->end = target.end();
  iter->excl_upper_bound = pack(value+1, 0);
  iter->shift = 2;
}

void ternary_table_get_iter(TERNARY_TABLE *table, TERNARY_TABLE_ITER *iter)
{
  set<tuple3> &target = table->unshifted;
  iter->iter = target.begin();
  iter->end = target.end();
  iter->excl_upper_bound = 0xFFFFFFFFFFFFFFFFULL;
  iter->shift = 0;
}

////////////////////////////////////////////////////////////////////////////////

bool ternary_table_iter_is_out_of_range(TERNARY_TABLE_ITER *iter)
{
  set<tuple3>::iterator it = iter->iter;
  return it == iter->end || it->fields01 >= iter->excl_upper_bound;
}

////////////////////////////////////////////////////////////////////////////////

uint32 ternary_table_iter_get_left_field(TERNARY_TABLE_ITER *iter)
{
  uint8 shift = iter->shift;
  assert(shift >= 0 && shift <= 2);
  if (shift == 0)
    return left(iter->iter->fields01);
  else if (shift == 1)
    return iter->iter->field2;
  else
    return right(iter->iter->fields01);
}

uint32 ternary_table_iter_get_middle_field(TERNARY_TABLE_ITER *iter)
{
  uint8 shift = iter->shift;
  assert(shift >= 0 && shift <= 2);
  if (shift == 0)
    return right(iter->iter->fields01);
  else if (shift == 1)
    return left(iter->iter->fields01);
  else
    return iter->iter->field2;
}

uint32 ternary_table_iter_get_right_field(TERNARY_TABLE_ITER *iter)
{
  uint8 shift = iter->shift;
  assert(shift >= 0 && shift <= 2);
  if (shift == 0)
    return iter->iter->field2;
  else if (shift == 1)
    return right(iter->iter->fields01);
  else
    return left(iter->iter->fields01);
}

////////////////////////////////////////////////////////////////////////////////

void ternary_table_iter_next(TERNARY_TABLE_ITER *iter)
{
  assert(!ternary_table_iter_is_out_of_range(iter));
  iter->iter++;
}

////////////////////////////////////////////////////////////////////////////////

bool ternary_table_updates_check_01(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates)
{
  sort_unique(updates->inserts);
  return table_updates_check_key<cols_01>(updates->inserts, updates->deletes, table->unshifted);
}

bool ternary_table_updates_check_01_2(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates)
{
  return ternary_table_updates_check_01(table, updates) &&
    table_updates_check_key<col_2>(updates->inserts, updates->deletes, table->shifted_twice);
}

bool ternary_table_updates_check_01_12(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates)
{
  return ternary_table_updates_check_01(table, updates) &&
    table_updates_check_key<cols_12>(updates->inserts, updates->deletes, table->shifted_once);
}

bool ternary_table_updates_check_01_12_20(TERNARY_TABLE *table, TERNARY_TABLE_UPDATES *updates)
{
  return ternary_table_updates_check_01_12(table, updates) &&
    table_updates_check_key<cols_20>(updates->inserts, updates->deletes, table->shifted_twice);
}
