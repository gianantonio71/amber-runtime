#include "lib.h"

#include <cstdlib>
#include <cstring>

#include <algorithm>


using std::malloc;
using std::free;
using std::memset;
using std::max_element;
using std::size_t;

////////////////////////////////////////////////////////////////////////////////

void unary_table_init(UNARY_TABLE *table)
{
  const uint32 INIT_SIZE = 1024;
  uint64 *bitmap = (uint64 *) malloc(INIT_SIZE/8);
  memset(bitmap, 0, INIT_SIZE/8);
  table->bitmap = bitmap;
  table->size = INIT_SIZE;
  table->count = 0;
}

void unary_table_cleanup(UNARY_TABLE *table)
{
  free(table->bitmap);
}

void unary_table_updates_init(UNARY_TABLE_UPDATES *table)
{

}

void unary_table_updates_cleanup(UNARY_TABLE_UPDATES *table)
{

}

////////////////////////////////////////////////////////////////////////////////

bool unary_table_contains(UNARY_TABLE *table, uint32 value)
{
  assert(value < table->size);

  uint64 *bitmap = table->bitmap;
  uint32 idx = value >> 6;
  uint64 mask = 1ULL << (value % 64);
  return bitmap[idx] & mask;
}

////////////////////////////////////////////////////////////////////////////////

void unary_table_insert(UNARY_TABLE_UPDATES *updates, uint32 value)
{
  updates->inserts.push_back(value);
}

void unary_table_delete(UNARY_TABLE *table, UNARY_TABLE_UPDATES *updates, uint32 value)
{
  updates->deletes.push_back(value);
}

void unary_table_clear(UNARY_TABLE *table, UNARY_TABLE_UPDATES *updates)
{
  uint64 *bitmap = table->bitmap;
  vector<uint32> &deletes = updates->deletes;
  uint32 cell_count = table->size / 64;
  for (int i=0 ; i < cell_count ; i++)
  {
    uint64 cell = bitmap[i];
    for (int j=0 ; j < 64 ; j++)
      if ((cell >> j) & 1)
        deletes.push_back(64 * i + j);
  }
}

bool unary_table_updates_check(UNARY_TABLE *table, UNARY_TABLE_UPDATES *updates)
{
  return true;
}

void unary_table_updates_apply(UNARY_TABLE *table, UNARY_TABLE_UPDATES *updates)
{
  vector<uint32> &deletes = updates->deletes;
  vector<uint32> &inserts = updates->inserts;

  uint32 max_val = inserts.empty() ? 0 : *max_element(inserts.begin(), inserts.end());

  uint32 size = table->size;
  uint64 *bitmap = table->bitmap;
  if (max_val >= size)
  {
    // Reallocating the table
    uint32 new_size = 2 * size;
    while (max_val >= new_size)
      new_size *= 2;
    uint64 *bitmap = (uint64 *) realloc(bitmap, new_size / 8);
    memset(bitmap + (size / 64), 0, (new_size - size) / 8);
    size = new_size;
    table->size = size;
    table->bitmap = bitmap;
  }

  size_t count = deletes.size();
  for (int i=0 ; i < count ; i++)
  {
    uint32 value = deletes[i];
    uint32 idx = value >> 6;
    uint64 mask = 1ULL << (value % 64);
    uint64 cell = bitmap[idx];
    if (cell & mask)
    {
      cell &= ~mask;
      bitmap[idx] = cell;
      table->count--;
    }
  }

  count = inserts.size();
  for (int i=0 ; i < count ; i++)
  {
    uint32 value = inserts[i];
    uint32 idx = value >> 6;
    uint64 mask = 1ULL << (value % 64);
    uint64 cell = bitmap[idx];
    if (!(cell & mask))
    {
      cell |= mask;
      bitmap[idx] = cell;
      table->count++;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void unary_table_get_iter(UNARY_TABLE *table, UNARY_TABLE_ITER *iter)
{
  if (table->count != 0)
  {
    uint64 *bitmap = table->bitmap;
    uint32 size = table->size;

    iter->bitmap = bitmap;
    iter->size = size;

    uint32 cell_count = size / 64;
    for (uint32 i=0 ; i < cell_count ; i++)
    {
      uint64 cell = bitmap[i];
      if (cell != 0)
        for (int j=0 ; j < 64 ; j++)
          if (((cell >> j) & 1) != 0)
          {
            iter->curr_value = 64 * i + j;
            return;
          }
    }
    throw;
  }
  else
  {
    iter->bitmap = NULL;
    iter->size = 0;
    iter->curr_value = 0;
  }
}

uint32 unary_table_iter_get_field(UNARY_TABLE_ITER *iter)
{
  assert(!unary_table_iter_is_out_of_range(iter));

  return iter->curr_value;
}

void unary_table_iter_next(UNARY_TABLE_ITER *iter)
{
  assert(!unary_table_iter_is_out_of_range(iter));

  uint64 *bitmap = iter->bitmap;
  uint32 size = iter->size;
  uint32 curr_value = iter->curr_value;

  uint32 cell_count = size / 64;

  uint32 idx = curr_value / 64;
  uint32 offset = curr_value % 64 + 1;

  uint64 cell = bitmap[idx];
  if (cell >> offset != 0)
  {
    for (int i=offset ; i < 64 ; i++)
      if (((cell >> i) & 1) != 0)
      {
        iter->curr_value = 64 * idx + i;
        return;
      }
    internal_fail();
  }

  for (uint32 i=idx+1 ; i < cell_count ; i++)
  {
    cell = bitmap[i];
    if (cell != 0)
      for (int j=0 ; j < 64 ; j++)
        if (((cell >> j) & 1) != 0)
        {
          iter->curr_value = 64 * i + j;
          return;
        }
  }

  iter->bitmap = NULL;
  iter->size = 0;
  iter->curr_value = 0;
}

bool unary_table_iter_is_out_of_range(UNARY_TABLE_ITER *iter)
{
  return iter->bitmap == NULL;
}
