#pragma once
/*
 * rjn_remapping.h
 * Converted from rjn_remapping.f90
 * Copyright (c) 2026, arakawa@climtech.jp
 */

#include <stddef.h>

/* -----------------------------------------------------------------------
 * init_remapping
 * Must be called before any other remapping function.
 * Stores local_comm and queries size/rank from MPI.
 * ----------------------------------------------------------------------- */
void init_remapping(int local_comm);

/* -----------------------------------------------------------------------
 * make_grid_rank_file
 * Rank 0 gathers all (sorted) local grid indices, performs a merge sort,
 * then writes two binary files:
 *   jlt.<my_comp>.<my_grid>.GRID_INDEX   (global sorted grid indices)
 *   jlt.<my_comp>.<my_grid>.GRID_RANK    (corresponding MPI rank, 0-based)
 * All other ranks send their sorted local index array to rank 0 and return.
 * ----------------------------------------------------------------------- */
void make_grid_rank_file(const char* my_comp, const char* my_grid,
                         const int* grid_index, int n);

/* -----------------------------------------------------------------------
 * delete_grid_rank_file
 * Root rank (rank 0) deletes the two grid-rank files created by
 * make_grid_rank_file.  Non-root ranks return immediately.
 * ----------------------------------------------------------------------- */
void delete_grid_rank_file(const char* my_comp, const char* my_grid);

/* -----------------------------------------------------------------------
 * get_target_grid_rank
 * Reads the GRID_INDEX / GRID_RANK files for (target_comp, target_grid)
 * in chunks of 20 entries.  For each entry in target_index[], looks up
 * the corresponding rank and stores it in target_rank[].
 * target_index and target_rank must both have length n.
 * ----------------------------------------------------------------------- */
void get_target_grid_rank(const char* target_comp, const char* target_grid,
                          const int* target_index, int n,
                          int* target_rank);

/* -----------------------------------------------------------------------
 * make_local_mapping_table_no_sort
 * Filters the global mapping table (global_index[], global_target[],
 * global_coef[], length n_global) to keep only entries whose global_index
 * appears in grid_index[] (length n_grid).
 * Does NOT sort global_index first; sorts only grid_index for binary search.
 *
 * Outputs (all malloc'd by callee, caller must free):
 *   *local_size_out   : number of matching entries
 *   *local_index_out  : malloc'd int[*local_size_out]
 *   *local_target_out : malloc'd int[*local_size_out]
 *   *local_coef_out   : malloc'd double[*local_size_out]
 * ----------------------------------------------------------------------- */
void make_local_mapping_table_no_sort(
    const int* global_index, const int* global_target, const double* global_coef,
    int n_global,
    const int* grid_index, int n_grid,
    int* local_size_out,
    int** local_index_out, int** local_target_out, double** local_coef_out);

/* -----------------------------------------------------------------------
 * make_local_mapping_table
 * Like make_local_mapping_table_no_sort but sorts global_index first,
 * then uses a two-pointer merge scan for O((n_global+n_grid) log n) total.
 *
 * Outputs (all malloc'd by callee, caller must free):
 *   *local_size_out   : number of matching entries
 *   *local_index_out  : malloc'd int[*local_size_out]
 *   *local_target_out : malloc'd int[*local_size_out]
 *   *local_coef_out   : malloc'd double[*local_size_out]
 * ----------------------------------------------------------------------- */
void make_local_mapping_table(
    const int* global_index, const int* global_target, const double* global_coef,
    int n_global,
    const int* grid_index, int n_grid,
    int* local_size_out,
    int** local_index_out, int** local_target_out, double** local_coef_out);

/* -----------------------------------------------------------------------
 * reorder_index_by_target_rank
 * Sorts target_rank[] in ascending order, applying the same permutation
 * to send_index[], recv_index[], and coef[] (all length n).
 * All arrays are modified in-place.
 * ----------------------------------------------------------------------- */
void reorder_index_by_target_rank(int* send_index, int* recv_index,
                                  double* coef, int* target_rank, int n);

/* -----------------------------------------------------------------------
 * delete_same_index
 * Returns a sorted array of unique values from index_in[] (length n).
 * Output is malloc'd by callee; caller must free *index_out.
 * ----------------------------------------------------------------------- */
void delete_same_index(const int* index_in, int n,
                       int** index_out, int* n_out);

/* -----------------------------------------------------------------------
 * make_exchange_table
 * Given target_rank[] (length n, already sorted/grouped by rank) and
 * local_table[] (length n, corresponding local grid indices), builds:
 *   *num_of_rank_out  : number of distinct target ranks
 *   *exchange_rank_out: malloc'd int[num_of_rank] — distinct ranks in order
 *   *num_of_index_out : malloc'd int[num_of_rank] — unique exchange indices per rank
 *   *offset_out       : malloc'd int[num_of_rank] — cumulative offset into exchange_index
 *   *exchange_index_out: malloc'd int[exchange_data_size] — unique sorted exchange indices
 *   *exchange_data_size_out: total number of unique exchange indices (sum of num_of_index)
 *
 * If n <= 0, sets *num_of_rank_out = 0 and all pointers to NULL.
 * ----------------------------------------------------------------------- */
void make_exchange_table(
    const int* target_rank, const int* local_table, int n,
    int* num_of_rank_out,
    int** exchange_rank_out, int** num_of_index_out,
    int** offset_out, int** exchange_index_out,
    int* exchange_data_size_out);

/* -----------------------------------------------------------------------
 * make_conversion_table
 * For each entry in remapping_index[] (length n_remap), binary-searches
 * grid_index[] (length n_grid) and stores the 1-based position in
 * (*conv_table_out)[].  The output array is malloc'd by the callee.
 * Caller must free *conv_table_out.
 * ----------------------------------------------------------------------- */
void make_conversion_table(const int* remapping_index, int n_remap,
                           const int* grid_index, int n_grid,
                           int** conv_table_out);
