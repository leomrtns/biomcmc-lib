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

/*! \file topology_randomise.h 
 *  \brief Creation of random topologies and modification of existing ones through branch swapping
 *
 */

#ifndef _biomcmc_randomise_h_
#define _biomcmc_randomise_h_

#include "topology_distance.h" 
#include "prob_distribution.h"

/*! \brief low level function that generates a random tree (equiv. to random refinement of a star topology) */
void randomise_topology (topology tree);
/*! \brief generates a random topology if sample_type==0, but can reuse some info later to create a "correlated" tree */
void quasi_randomise_topology (topology tree, int sample_type);

void create_parent_node_from_children (topology tree, int parent, int lchild, int rchild);
/*! \brief random rerooting */
void topology_apply_rerooting (topology tree, bool update_done);
/*! \brief recursive SPR over all internal nodes, assuming common prob of swap  per node */
void topology_apply_shortspr (topology tree, bool update_done);
/*! \brief recursive SPR over all internal nodes, using prob[] vector as rough guide of error rate for node */
void topology_apply_shortspr_weighted (topology tree, double *prob, bool update_done);
/*! \brief random Subtree Prune-and-Regraft branch swapping for subtree below lca node */
void topology_apply_spr_on_subtree (topology tree, topol_node lca, bool update_done);
/*! \brief random Subtree Prune-and-Regraft branch swapping */
void topology_apply_spr (topology tree, bool update_done);
/*! \brief random Subtree Prune-and-Regraft branch swapping generalized (neglecting root) */
void topology_apply_spr_unrooted (topology tree, bool update_done);
/*! \brief random Nearest Neighbor Interchange branch swapping (SPR where regraft node is close to prune node) */
void topology_apply_nni (topology tree, bool update_done);
/*! \brief check if it is possible to apply SPR/NNI without rerooting (used by topology_apply_spr() and MCMC functions) */
bool cant_apply_swap (topology tree);

#endif
