#include "lib.h"

#include <algorithm>


using std::sort;
using std::unique;


uint64 pack(uint64 left, uint64 right)
{
  return (left << 32) | right;
}

uint64 swap(uint64 pair)
{
  return (pair >> 32) | (pair << 32);
}

uint32 left(uint64 pair)
{
  return pair >> 32;
}

uint32 right(uint64 pair)
{
  return pair;
}

template <typename T> void sort_unique(vector<T> &xs)
{
  sort(xs.begin(), xs.end());
  xs.erase(unique(xs.begin(), xs.end()), xs.end());
}

void take_left(vector<uint32> &ls, const vector<uint64> &ps)
{
  uint32 count = ps.size();
  ls.resize(count);
  for (uint32 i=0 ; i < count ; i++)
    ls[i] = left(ps[i]);
}

void take_right(vector<uint32> &rs, const vector<uint64> &ps)
{
  uint32 count = ps.size();
  rs.resize(count);
  for (uint32 i=0 ; i < count ; i++)
    rs[i] = right(ps[i]);
}

////////////////////////////////////////////////////////////////////////////////

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

bool binary_table_update_left_col_stays_unique(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates)
{
  set<uint64> &left_to_right = table->left_to_right;
  vector<uint64> &deletes = updates->deletes;
  vector<uint64> &inserts = updates->inserts;

  // Gathering and sorting all left column values from tuples to delete
  vector<uint32> deleted_values;
  take_left(deleted_values, deletes);
  sort(deleted_values.begin(), deleted_values.end());

  // Gathering all left column values from tuples to insert.
  // Since the tuples to insert are already sorted, the resulting array is sorted too
  vector<uint32> inserted_values;
  take_left(inserted_values, inserts);

  // Checking that there are no duplicates. Since the duplicates among the tuples
  // to insert have already been eliminated, the presence of a duplicate among
  // left column values implies a unicity conflict
  uint32 count = inserted_values.size();
  uint32 last = inserted_values[0];
  for (int i=1 ; i < count ; i++)
  {
    uint32 elem = inserted_values[i];
    if (elem == last)
      return false;
    assert(elem > last);
    last = elem;
  }

  // Checking that for each left column value to insert, either there's an
  // entry among the values to delete, or there's no entry in the current table
  for (int i=0 ; i < count ; i++)
  {
    uint32 value = inserted_values[i];
    if (!binary_search(deleted_values.begin(), deleted_values.end(), value))
    {
      set<uint64>::iterator it = left_to_right.lower_bound(pack(value, 0));
      if (it != left_to_right.end() && left(*it) == value)
        return false;
    }
  }

  // Everything seems fine...
  return true;
}

bool binary_table_update_right_col_stays_unique(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates)
{
  set<uint64> &right_to_left = table->right_to_left;
  vector<uint64> &deletes = updates->deletes;
  vector<uint64> &inserts = updates->inserts;

  // Gathering and sorting all right column values from tuples to delete
  vector<uint32> deleted_values;
  take_right(deleted_values, deletes);
  sort(deleted_values.begin(), deleted_values.end());

  // Gathering all left column values from tuples to insert. Here we also need to sort them
  vector<uint32> inserted_values;
  take_right(inserted_values, inserts);
  sort(inserted_values.begin(), inserted_values.end());

  // Checking that there are no duplicates. Since the duplicates among the tuples
  // to insert have already been eliminated, the presence of a duplicate among
  // right column values implies a unicity conflict
  uint32 count = inserted_values.size();
  uint32 last = inserted_values[0];
  for (int i=1 ; i < count ; i++)
  {
    uint32 elem = inserted_values[i];
    if (elem == last)
      return false;
    assert(elem > last);
    last = elem;
  }

  // Checking that for each right column value to insert, either there's an
  // entry among the values to delete, or there's no entry in the current table
  for (int i=0 ; i < count ; i++)
  {
    uint32 value = inserted_values[i];
    if (!binary_search(deleted_values.begin(), deleted_values.end(), value))
    {
      set<uint64>::iterator it = right_to_left.lower_bound(pack(value, 0));
      if (it != right_to_left.end() && left(*it) == value)
        return false;
    }
  }

  // Everything seems fine...
  return true;
}

bool binary_table_updates_check(BINARY_TABLE *table, BINARY_TABLE_UPDATES *updates, bool left_is_unique, bool right_is_unique)
{
  // If neither column is unique, any update is valid
  if (!left_is_unique & !right_is_unique)
    return true;

  // If there's nothing to insert, the update is trivially valid
  if (updates->inserts.empty())
    return true;

  // Sorting and removing duplicates from all insertions.
  sort_unique(updates->inserts);

  // Checking unicity in the left column, if need be
  if (left_is_unique && !binary_table_update_left_col_stays_unique(table, updates))
    return false;

  // Checking unicity in the right column, again if need be
  return !right_is_unique || binary_table_update_right_col_stays_unique(table, updates);
}
