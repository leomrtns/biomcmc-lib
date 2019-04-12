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

/*! \file biomcmc.h 
 *  \brief biomcmc library interface to external programs, specific to super_sptree repo.
 *
 *  The idea is for biomcmc-lib is to be general for several sofware, including treesignal and super_sptree.
 *  This library started branching from the biomcmc library from the guenomu software.
 */

#ifndef _biomcmc_h_
#define _biomcmc_h_

#include "argtable3.h"
#include "prob_distribution.h" 
#include "alignment.h"
#include "parsimony.h"
#include "genetree.h"
#include "read_newick_trees.h"

#ifdef THESE_ARE_COMMENTS
#include "lowlevel.h"            // called by char_vector, argtable, bipartition, prob_distribution 
#include "char_vector.h"         // called by hashtable, random_number, nexus_common, empirical_frequency
#include "hashtable.h"           // called by random_number_gen 
#include "bipartition.h"         // called by hashtable
#include "random_number_gen.h"   // called by random_number
#include "random_number.h"       // called by prob_distribution
#include "topology_common.h"     // called by parsimony, reconciliation, read_nexus_trees
#include "reconciliation.h"      // opaque header/library, called by genetree
#include "distance_matrix.h"     // called by alignment
#include "empirical_frequency.h" // called by nexus_common  
#include "nexus_common.h"        // called by read_nexus_trees 
#include "splitset_distances.h"  // opaque header/library, called by genetree
#endif // of THESE_ARE_COMMENTS

#endif //define biomcmc