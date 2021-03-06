/* 
 * This file is part of biomcmc-lib, a low-level library for phylogenomic analysis.
 * Copyright (C) 2019-today  Leonardo de Oliveira Martins [ leomrtns at gmail.com;  http://www.leomartins.org ]
 *
 * biomcmc is free software; you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more 
 * details (file "COPYING" or http://www.gnu.org/copyleft/gpl.html).
 */

/*! \file parsimony.h
 *  \brief binary and multistate parsimony matrices, together with bipartition extraction for MRP  
 *
 */

#ifndef _biomcmc_parsimony_
#define _biomcmc_parsimony_

#include "topology_distance.h"

typedef struct binary_parsimony_datamatrix_struct* binary_parsimony_datamatrix; 
typedef struct binary_parsimony_struct* binary_parsimony;

/*! \brief used by matrix representation with parsimony (01 10 11 sequences) */
struct binary_parsimony_datamatrix_struct {
  int ntax, nchar, i;  /*!< \brief number of taxa, distinct sites (patterns), and index to current (last) column */
  bool **s;            /*!< \brief 1 (01) and 2 (10) are the two binary states, with 3 (11) being undetermined */
  int *freq, freq_sum; /*!< \brief frequency of pattern. */
  int *occupancy;      /*!< \brief how many species represented by each bipartition */
  uint32_t *col_hash;  /*!< \brief hash value of each column, to speed up comparisons */
  int ref_counter;     /*!< \brief how many places have a pointer to this instance */
};

struct binary_parsimony_struct {
  int *score;      /*!< \brief parsimony score per pattern */
  binary_parsimony_datamatrix external, internal; /*!< \brief binary matrices for leaves and for internal nodes */
  double costs[4];
  int ref_counter; /*!< \brief how many places have a pointer to this instance */
};

binary_parsimony_datamatrix new_binary_parsimony_datamatrix (int n_sequences);
binary_parsimony_datamatrix new_binary_parsimony_datamatrix_fixed_length (int n_sequences, int n_sites);
void del_binary_parsimony_datamatrix (binary_parsimony_datamatrix mrp);
binary_parsimony new_binary_parsimony (int n_sequences);
binary_parsimony new_binary_parsimony_fixed_length (int n_sequences, int n_sites);
void del_binary_parsimony (binary_parsimony pars);
/*! \brief given a map[] with location in sptree of gene tree leaves, update binary matrix with splits from genetree */
void update_binary_parsimony_from_topology (binary_parsimony pars, topology t, int *map, int n_species);
int binary_parsimony_score_of_topology (binary_parsimony pars, topology t);
void pairwise_distances_from_binary_parsimony_datamatrix (binary_parsimony_datamatrix mrp, double **dist, int n_dist);

#endif
