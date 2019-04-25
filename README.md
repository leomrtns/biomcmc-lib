# Biomcmc-lib 

This library provides low level functions used by other phylogenetic programs. 
It borrows many functions from the [guenomu software](https://bitbucket.org/leomrtns/guenomu/) for phylogenomic species tree inference and 
extends some ideas from the [genefam-dist library](https://github.com/leomrtns/genefam-dist).

It includes a version of [argtable3](https://www.argtable.org/) downloaded from
[github](https://github.com/argtable/argtable3) on 2019.03.12. Argtable3 is released under a BSD license. 

## Installation
This library is usually not installed directly, but as a submodule of another project. 
It includes, however, the `makefile.am` and `configure.ac` for autotools, and it provides unit tests from the
[libcheck](https://github.com/libcheck/check) library as well as custom checks. 

## Assumptions and limitations
This library works mainly with phylogenetic trees, but also contain a few functions to work with sequences. 
The trees can be in nexus or newick formats, and the sequences can be in nexus or fasta formats.
* Multifurcations are allowed, and are resolved into branches of length zero. e.g
```[python]
(A,B,C)  -->  ((A,B):0,C)     # notice the single branch length of zero
```
* All missing branch lengths are given a value of one. This way we can work with topologies (a.k.a. cladograms, trees without  
  lengths) transparently.
* A file is assumed to be in the nexus format if one of the first lines start with the keyword `#NEXUS`, followed by
  some other nexus keyword (`DATA` or `TREES`). Otherwise it's treated as newick (or fasta, for sequence files). 
* All trees from a single nexus file must contain the same taxa (e.g. output from a Bayesian inference algorithm). 
  This means that more compact representations are allowed, leading to smaller files. This also allow for some
  optimisations when reading large files.
  1. The taxon names can come first, in a `TRANSLATE` table; the individual trees then just point to the IDs in this
     table. BTW, technically each tree in a nexus file is represented in newick format.
  2. If the same topology is observed, then some programs (e.g. MrBayes, guenomu) just output the distinct topologies, with 
     their equivalent *frequencies* as nexus comments. The trees are ordered, with the most frequent trees at the top.
     The individual branch length information is lost, however, as just the mean values are reported.
  3. Since nexus files may represent MCMC samples, you can do "thinning", i.e. you may subsample by skipping consecutive
     trees, and also by skipping the first ones (from the "burnin" period). 
  4. Two topologies can be equal at the unrooted level but not at the rooted (i.e. the only difference is the root
     location). Please pay attention to this information, as this should be solved by the calling program.
* The so-called "newick files", however, can have trees from different leaf sets. That is, trees in the same file are not 
  assumed to have all the same leaves. 
* Newick files are allowed to have the same leaf name for several leaves in the same tree. However nexus files must have
  unique taxon names (to allow for interoperability, although I may change this in the future).
* All trees are stored internally as rooted trees, and are saved/printed as rooted, without the `[U]` flag some nexus
  implementations assume. The downstream program/user must take into account if the rooting is relevant or not. Having
  said that, all algorithms know when to ignore the root node (e.g. gene tree / species tree reconciliation assumes
  both trees are rooted, but we optimise over all gene rootings effectively looking at unrooted gene trees).
* The nexus format uses square brackets (`[]`) for comments, which can span several lines. These comments are ignored by
  all reading functions, including those for fasta and newick files. The only exception so far are the tree frequencies, which 
  follow the convention of the `.trprobs` files of MrBayes and guenomu (`p` for frequency, `P` for cummulative
  frequency):
```[bash]
tree t_1 [p = 0.51, P = 0.51] = [&W 0.51] ((((((((5,4),6),((2,1),3)),7),(9,8)),((((12,11),10),13),14)),(16,15)),((((((25,24),23),22),21),(20,19)),(18,17)));
```

### The rooting saga
Most phylogenetic inference software work with or assume unrooted trees, but sometimes we want to know the root
location. 
On top of that there is no single way of storing trees as rooted or unrooted. To give an example of valid options:
```[python]
(A,B,C)           ## this tree is unrooted
[U] ((A,B),C)     ## this tree is also unrooted
((A:1,B:1):0,C:1) ## also unrooted?
((A,B,C),D)       ## for all effects, this tree is rooted
```
This is a feature, not a bug: sometimes you want to remind users that the tree is unrooted (and therefore avoids
offering an arbitrary rooting), and sometimes you "know" the root location but some algorithms will neglect it nonetheless.
And sometimes it's just easier to work with a rooted, binary tree &mdash; as is our case.

UPGMA gives rooted trees, as well as strict/relaxed clock models, and irreversible substitution models.
Phylogenomic models (like guenomu) can also use duplications and losses to estimate the root location.
You can also provide the outgroup (i.e., which clade should have the furthest lowest common ancestor with the other
taxa), or use midpoint/MAD/minimum variance to find the best root location for your tree.

In any case, it is good idea to always assume that the trees are unrooted unless explicitly stated otherwise.
Irrespective of what the newick string tells you. 


## License 
Copyright (C) 2019-today  [Leonardo de Oliveira Martins](https://github.com/leomrtns)

biomcmc-lib is free software; you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later
version (http://www.gnu.org/copyleft/gpl.html).

The files `argtable3.c` and `argtable3.h` are part of the [argtable3](https://www.argtable.org/) library, maintained by
Tom G. Huang at the time of this writting  and are distributed under a BSD license. Please refer to 
https://github.com/argtable/argtable3 for the list of authors.
