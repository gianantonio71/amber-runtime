#include "lib.h"
#include "table-utils.h"


void binary_table_init(BINARY_TABLE *table)
{

}

void binary_table_cleanup(BINARY_TABLE *table)
{

}

void binary_table_updates_init(BINARY_TABLE_UPDATES *updates)
{

}

void binary_table_updates_cleanup(BINARY_TABLE_UPDATES *updates)
{

}

////////////////////////////////////////////////////////////////////////////////

bool binary_table_contains(BINARY_TABLE *table, uint32 left_val, uint32 right_val)
{
  set<uint64> &left_to_right = table->left_to_right;
  set<uint64>::iterator it = left_to_right.find(pack(left_val, right_val));
  return it != left_to_right.end();
}

////////////////////////////////////////////////////////////////////////////////

void binary_table_delete_range_(BINARY_TABLE_ITER *iter, BINARY_TABLE_UPDATES *updates)
{
  bool reversed = iter->reversed;
  vector<uint64> &deletes = updates->deletes;
  while (!binary_table_iter_is_out_of_range(iter))
  {
    uint64 pair = *iter->iter;
    deletes.push_back(reversed ? swap(pair) : pair);
    binary_table_iter_next(iter);
  }
}

void binary_table_delete(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates, uint32 left_val, uint32 right_val)
{
  if (binary_table_contains(table, left_val, right_val))
    updates->deletes.push_back(pack(left_val, right_val));
}

void binary_table_delete_by_col_0(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates, uint32 value)
{
  BINARY_TABLE_ITER iter;
  binary_table_get_iter_by_col_0(table, &iter, value);
  binary_table_delete_range_(&iter, updates);
}

void binary_table_delete_by_col_1(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates, uint32 value)
{
  BINARY_TABLE_ITER iter;
  binary_table_get_iter_by_col_1(table, &iter, value);
  binary_table_delete_range_(&iter, updates);
}

void binary_table_clear(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates)
{
  BINARY_TABLE_ITER iter;
  binary_table_get_iter(table, &iter);
  binary_table_delete_range_(&iter, updates);
}

void binary_table_insert(BINARY_TABLE_UPDATES *updates, uint32 left_val, uint32 right_val)
{
  updates->inserts.push_back(pack(left_val, right_val));
}

void binary_table_updates_apply(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates)
{
  set<uint64> &left_to_right = table->left_to_right;
  set<uint64> &right_to_left = table->right_to_left;

  vector<uint64>::iterator it = updates->deletes.begin();
  vector<uint64>::iterator end = updates->deletes.end();
  for ( ; it != end ; it++)
  {
    uint64 pair = *it;
    left_to_right.erase(pair);
    right_to_left.erase(swap(pair));
  }

  it = updates->inserts.begin();
  end = updates->inserts.end();
  for ( ; it != end ; it++)
  {
    uint64 pair = *it;
    left_to_right.insert(pair);
    right_to_left.insert(swap(pair));
  }
}

////////////////////////////////////////////////////////////////////////////////

void binary_table_get_iter_by_col_0(BINARY_TABLE *table, BINARY_TABLE_ITER *iter, uint32 value)
{
  set<uint64> &left_to_right = table->left_to_right;
  iter->iter = left_to_right.lower_bound(pack(value, 0));
  iter->end = left_to_right.end();
  iter->value = value;
  iter->reversed = false;
}

void binary_table_get_iter_by_col_1(BINARY_TABLE *table, BINARY_TABLE_ITER *iter, uint32 value)
{
  set<uint64> &right_to_left = table->right_to_left;
  iter->iter = right_to_left.lower_bound(pack(value, 0));
  iter->end = right_to_left.end();
  iter->value = value;
  iter->reversed = true;
}

void binary_table_get_iter(BINARY_TABLE *table, BINARY_TABLE_ITER *iter)
{
  set<uint64> &left_to_right = table->left_to_right;
  iter->iter = left_to_right.begin();
  iter->end = left_to_right.end();
  iter->value = 0xFFFFFFFFU;
  iter->reversed = false;
}

////////////////////////////////////////////////////////////////////////////////

bool binary_table_iter_is_out_of_range(BINARY_TABLE_ITER *iter)
{
  set<uint64>::iterator it = iter->iter;
  return it == iter->end || (*it >> 32) > iter->value;
}

uint32 binary_table_iter_get_left_field(BINARY_TABLE_ITER *iter)
{
  return *iter->iter >> (iter->reversed ? 0 : 32);
}

uint32 binary_table_iter_get_right_field(BINARY_TABLE_ITER *iter)
{
  return *iter->iter >> (iter->reversed ? 32 : 0);
}

void binary_table_iter_next(BINARY_TABLE_ITER *iter)
{
  assert(!binary_table_iter_is_out_of_range(iter));
  iter->iter++;
}

////////////////////////////////////////////////////////////////////////////////

bool binary_table_updates_check_0(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates)
{
  sort_unique(updates->inserts);
  return table_updates_check_key<col_0>(updates->inserts, updates->deletes, table->left_to_right);
}

bool binary_table_updates_check_1(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates)
{
  sort_unique(updates->inserts);
  return table_updates_check_key<col_1>(updates->inserts, updates->deletes, table->right_to_left);
}

bool binary_table_updates_check_0_1(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates)
{
  return binary_table_updates_check_0(table, updates) &&
    table_updates_check_key<col_1>(updates->inserts, updates->deletes, table->right_to_left);
}
