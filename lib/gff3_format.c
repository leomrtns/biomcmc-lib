/* SPDX-License-Identifier: GPL-3.0-or-later 
 * This file is part of biomcmc-lib, a low-level library for phylogenomic analysis.
 * Copyright (C) 2019-today  Leonardo de Oliveira Martins [ leomrtns at gmail.com;  http://www.leomartins.org ]
 *
 * biomcmc is free software; you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later
 * version.

 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more 
 * details (file "COPYING" or http://www.gnu.org/copyleft/gpl.html).
 */

/*! \file 
 *  \brief GFF3 format 
 */

#include "gff3_format.h"

int compare_gff3_fields_increasing (const void *a, const void *b);

gff3_fields gff3_fields_from_char_line (const char *line);
void free_strings_gff3_fields (gff3_fields gff);
void get_gff3_string_from_field (const char *start, const char *end, gff3_string *string);
uint64_t return_gff3_hashed_string (const char *str, type_t len);

gff3_t new_gff3_t (void);
void add_fields_to_gff3_t (gff3_t g3, gff3_fields gfield);

// static char *feature_type_list[] = {"gene", "cds", "mrna", "region"}; // many more, but I dont use them
// TODO: prokka generates many of these, one per contig: ##sequence-region seqid start end 
// and fasta sequences are one seqid each

int
compare_gff3_fields_increasing (const void *a, const void *b)
{
  // arbitrary order of distinct genomes 
  if ((*(gff3_fields*)a)->seqid.hash > (*(gff3_fields*)b)->seqid.hash) return 1;
  if ((*(gff3_fields*)a)->seqid.hash < (*(gff3_fields*)b)->seqid.hash) return -1;
  // arbitrary order of feature types (all genes first, then all CDS, etc.)
  if ((*(gff3_fields*)a)->type.hash > (*(gff3_fields*)b)->type.hash) return 1;
  if ((*(gff3_fields*)a)->type.hash < (*(gff3_fields*)b)->type.hash) return -1;
  int result = (*(gff3_fields*)a)->start - (*(gff3_fields*)b)->start;
  if (result) return result; // this is main sorting, by genome location
  return (*(gff3_fields*)a)->end - (*(gff3_fields*)b)->end; // if same type and start, try to sort by end
}

gff3_fields
gff3_fields_from_char_line (const char *line)
{
  gff3_fields gff;
  char *f_start, *f_end;
  int i;
  // check if proper gff3 fields line, otherwise return NULL before doing anything else
  for (i = 0, f_start = line; f_start != NULL; f_start = strstr (f_start, '\t'), i++);
  if (i != 8) return NULL;

  f_end = line; // will become f_start as son as enters loop 
  for (i = 0; (i < 9) && (f_end != NULL); i++) {
    f_start = f_end; 
    if (f_start[0] == '\t') f_start++;
    f_end = strstr (f_start, '\t');
    if (f_end == NULL) f_end = line + strlen (line); // last column

    switch (i) {
      case 0: // col 1 = SEQID (genome id)
        get_gff3_string_from_field (f_start, f_end, &(gff.seqid)); break;
      case 1: // col 2 = source (RefSeq, genbank)
        get_gff3_string_from_field (f_start, f_end, &(gff.source)); break;
      case 2: // gene, mRNA, CDS
        get_gff3_string_from_field (f_start, f_end, &(gff.type)); break;
      case 3: 
        sscanf (f_start, "%d\t", &gff.start); gff.start--; break; // gff3 is one-based but we are zero-based
      case 4: 
        sscanf (f_start, "%d\t", &gff.end);   gff.end--;   break; // gff3 is one-based but we are zero-based
      case 6: // + or - strand (relative to landmark in column 1); can be "." for irrelevant or "?" for unknown 
        if (f_start == '+') gff.pos_strand = 1;
        else if (f_start == '-') gff.pos_strand = 0;
        else gff.pos_strand = 2; break;
      case 7:  // where codon starts, in CDS. It can be 0,1,2 (relative to start if + or to end if -) 
        sscanf (f_start, "%d\t", &gff.phase); break;
      case 8: get_gff3_attributes_from_field (f_start, f_end, &gff.attr_id, &gff.attr_parent); break;
      default: break; // skip 'score' field  (6 of 9)
    };
  }
//  if (i < 9) biomcmc_error ("Malformed GFF3 file, found only %d (tab-separated) fields instead of 9 on line \n%s", i, line);
//  if (f_end != NULL) biomcmc_error ("Malformed GFF3, more than 9 tab-separating fields found on line\n%s", line);
  if ((i < 9) || (f_end != NULL)) {free_strings_gff3_fields (gff); return NULL; }
  return gff;
}

void
free_strings_gff3_fields (gff3_fields gff)
{
  if (gff.seqid.str)   free (gff.seqid.str);
  if (gff.source.str)  free (gff.source.str);
  if (gff.type.str)    free (gff.type.str);
  if (gff.attr_id.str) free (gff.id.str);
  if (gff.attr_parent.str) free (gff.parent.str);
}

void
get_gff3_string_from_field (const char *start, const char *end, gff3_string *string)
{
  type_t len = end - start; // len actually points to final "\t"; spaces _are_part_of_string_ 
  if (len == 1) { string->str = NULL; string->hash = 0ULL; string->id = -1; return; }

  string->str = (char*) biomcmc_malloc (sizeof (char) * len);
  strncpy (string->str, start, len - 1); // do not copy trailing tab
  string->str[len-1] = '\0';
  /* hash is 64 bits, formed by concatenating 2 uint32_t hash values */
  string->hash = return_gff3_hash_string (string->str, len-1);
  string->id = -1; // defined when creating char_vector 
  return;
}

uint64_t
return_gff3_hashed_string (const char *str, type_t len)
{
  uint64_t hash = (biomcmc_hashbyte_salted (str, len, 4) << 32); // 4 (salt) = CRC algo
  hash |= (biomcmc_hashbyte_salted (str, len, 2) & 0xffffffff);  // 2 (salt) = dbj2 algo
  return hash;
}

void      
get_gff3_attributes_from_field (const char *start, const char *end, gff3_string *attr_id, gff3_string *attr_parent)
{
  char *s1, *e1;
  s1 = strstr (start, "ID="); // protected attributes start with uppercase 
  if (s1 == NULL) { attr_id->str = NULL; attr_id->hash = 0ULL; attr_id->id = -1; }
  else {
    s1 += 3; // everything after '=', even spaces
    e1 = strstr (s1, ";");
    if (e1 == NULL) e1 = end;
    if (s1 + 1 == e1) { attr_id->str = NULL; attr_id->hash = 0ULL; attr_id->id = -1; }
    else get_gff3_string_from_field (s1, e1, attr_id);
  } 

  s1 = strstr (start, "Parent="); // protected attributes start with uppercase 
  if (s1 == NULL) { attr_parent->str = NULL; attr_parent->hash = 0ULL; attr_parent->id = -1; }
  else { // Parent can be like "Parent=mRNA00001,mRNA00002" (i.e. several parents; here I don't care)
    s1 += 7; // everything after '=', even spaces
    e1 = strstr (s1, ";");
    if (e1 == NULL) e1 = end;
    if (s1 + 1 == e1) { attr_parent->str = NULL; attr_parent->hash = 0ULL; attr_parent->id = -1; }
    else get_gff3_string_from_field (s1, e1, attr_parent);
  } 
  return;
}

gff3_t
read_gff3_from_file (char *gff3filename)
{
  FILE *seqfile;
  char *line = NULL, *line_read = NULL, *delim = NULL, *tmpc = NULL;
  size_t linelength = 0;
  int stage = 0, i1, i2;
  gff3_fields gfield;
  gff3_t g3;
  char_vector seqreg = new_char_vector (1);

  seqfile = biomcmc_fopen (gff3filename, "r");
  g3 = new_gff3_t ();
  tmpc = (char*) biomcmc_malloc (1024 * sizeof (char));

  while (biomcmc_getline (&line_read, &linelength, seqfile) != -1) {
    line = line_read; /* the variable *line_read should point always to the same value (no line++ or alike) */
    if (nonempty_gff3_string (line)) {
      if (stage == 0) && (strcasestr (line, "##gff-version")) stage = 1; // obligatory first line to keep going on

      else if (stage == 1) { /* initial pragmas */
        if (delim = strcasestr (line, "##sequence-region")) {
          sscanf (delim, "##sequence-region %s %d %d", tmpc, &i1, &i2); i2--; // TODO: use i2 (contig length) 
          char_vector_add_string (seqreg, tmpc); // add chromosome/contig name;
        }
        else if (strcasestr (line, "##")) /*DO NOTHING */;
        else if ((gfield = gff3_fields_from_char_line (line)) != NULL) {
          add_fields_to_gff3_t (g3, gfield);
          stage = 2;
        }
      }

      else if (stage == 2) { /* regular fields */
        if ((gff = gff3_fields_from_char_line (line)) != NULL)  add_fields_to_gff3_t (g3, gfield);
        else if (strcasestr (line, "##fasta")) stage = 3;
      }

      else if (stage == 3) {  /* FASTA file */
        if (nonempty_fasta_line (line)) { /* each line can be either the sequence or its name, on a strict order */
          if ((delim = strchr (line, '>'))) char_vector_add_string (g3->seqname, ++delim); /* sequence name (description, in FASTA jargon) */
          else {/* the sequence itself, which may span several lines */
            line = remove_space_from_string (line);
            line = uppercase_string (line);
            char_vector_append_string_big_at_position (g3->sequence, line, g3->seqname->next_avail-1); // counter from name NOT sequence
          }
        } // if non_empty
      } // if stage 3 
    } // if gff3 line not empty
  } // while readline ()
  fclose (seqfile);

  char_vector_finalise_big (g3->sequence);
  gff3_finalise (g3, seqreg);

  del_char_vector (seqreg);
  if (line_read) free (line_read);
  if (tmpc) free (tmpc);
}

gff3_t
new_gff3_t (void)
{
  gff3_t g3 = (gff3_t) biomcmc_malloc (sizeof (struct gff3_file_struct));
  g3->sequence = new_char_vector_big (1);
  g3->seqname = new_char_vector (1);
  g3->f0 = NULL;
  g3->cds = g3->gene = NULL;
  g3->n_f0 = g3->n_cds = g3->n_gene = 0;
  g3->seqname_hash = NULL; // created when finalising 
  g3->ref_counter = 1;
  return gff3;
}

void
del_gff3_t (gff3_t g3)
{
  if (!g3) return;
  if (--g3->ref_counter) return;
  del_char_vector (g3->sequence);
  del_char_vector (g3->seqname);
  del_hashtable (g3->seqname_hash);
  if (g3->f0) free (g3->f0);
  if (g3->cds) free (g3->cds);
  if (g3->gene) free (g3->gene);
  free (g3);
}

void
add_fields_to_gff3_t (gff3_t g3, gff3_fields gfield)
{
  g3->f0 = (gff3_fields*) biomcmc_realloc ((gff3_fields*) g3->f0, (g3->n_f0 + 1) * sizeof (gff3_fields));
  g3->f0[g3->n_f0++] = gfield;
}

void merge_seqid_from_fields_and_pragma (gff3_t g3, char_vector s);

void
gff3_finalise (gff3_t g3, char_vector seqreg)
{
  /* 1. sort fields, map seqids to char_vector, and point to specific features (cds, gene) */
  merge_seqid_from_fields_and_pragma (g3, seqreg); // updates seqreg to match fields->seqid
  generate_feature_type_pointers (g3); // vectors of pointers (CDS, gene)

  /* 2. if fasta is incomplete or missing, just copy seqids to seqname */
  if ((!g3->sequence->next_avail) || (g3->seqname->next_avail < seqreg->next_avail)) { 
    del_char_vector (g3->sequence); g3->sequence = NULL;
    del_char_vector (g3->seqname);
    g3->seqname = seqreg;
    g3->seqname->ref_counter++; // seqreg will be deleted later
    if (g3->seqname && (g3->seqname->next_avail < seqreg->next_avail)) 
      biomcmc_warning ("incomplete fasta pragma in GFF3; ignoring DNA sequences from file\n");
    return;
  }

  /* 3. map seqnames from fasta pragma to seqid from fields; assume fasta have spurious seqs */
  int i, n_extra, hid, *order;
  order = (int*) biomcmc_malloc (g3->seqname->nstrings * sizeof (int));
  n_extra = seqreg->nstrings; // OK for us for fasta to have more sequences than needed
  for (i = 0; i < g3->seqname->nstrings; i++) {
    hid = lookup_hashtable (g3->seqname_hash, g3->seqname->string[i]);
    if ((hid < 0) && (n_extra == g3->seqname->nstrings)) {
      biomcmc_warning ("fasta pragma in GFF3 doesn't correspond to field seqids; ignoring DNA sequences from file\n");
      del_char_vector (g3->sequence); g3->sequence = NULL;
      del_char_vector (g3->seqname);
      g3->seqname = seqreg;
      g3->seqname->ref_counter++; // seqreg will be deleted later
      if (order) free (order);
      return;
    }
    else if (hid < 0) order[n_extra++] = i; // last elements, hopefully just extra fasta sequences
    else order[hid] = i;
  }
  /* 4. use order from fields seqids (hash sorted) on fasta */  
  if (seqreg->nstrings < g3->seqname->nstrings) {
    char_vector_reorder_strings_from_external_order (g3->seqname,  order);
    char_vector_reorder_strings_from_external_order (g3->sequence, order);
    char_vector_reduce_to_trimmed_size (g3->seqname,  seqreg->nstrings);	
    char_vector_reduce_to_trimmed_size (g3->sequence, seqreg->nstrings);	
  }

  if (order) free (order);
  return;
}

void
merge_seqid_from_fields_and_pragma (gff3_t g3, char_vector s)
{ // FIXME: currently we ignore completely sequence-region (which might be used to define order)
  int i;
  qsort (g3->f0, g3->n_f0, sizeof (gff3_fields), compare_gff3_fields_increasing);
  del_char_vector (s);
  s = new_char_vector (1); // TODO: this should be compared with original s 

  char_vector_add_string (s, g3->f0[0].seqid.str);
  g3->f0[i].seqid.id = s->next_avail - 1; // which is zero, btw, since next_avail is one after add_string()
  for (i = 1; i < g3->n_f0; i++) {
    if (strcmp(g3->f0[i].seqid.str, s->string[s->next_avail-1])) char_vector_add_string (s, g3->f0[i].seqid.str);
    g3->f0[i].seqid.id = s->next_avail - 1;
  }

  g3->seqname_hash = new_hashtable (s->nstrings);
  for (i = 0; i < s->nstrings; i++) insert_hashtable (g3->seqname_hash, s->string[i], i);
}

void
generate_feature_type_pointers (gff3_t g3)
{
  int i;
  g3->cds  = (gff3_fields**) biomcmc_malloc (g3->n_f0 * sizeof (gff3_fields*));
  g3->gene = (gff3_fields**) biomcmc_malloc (g3->n_f0 * sizeof (gff3_fields*));
  g3->n_gene = g3->n_cds = 0;

  for (i = 0; i < g3->n_f0; i++) {
    if (!strcasecmp (g3->f0[i].type.str, "cds")) {
      g3->f0[i].attr_id.id = g3->n_cds; // experimental (not used); several CDS (rows) can have same ID in gff3
      g3->cds[g3->n_cds++] = &(g3->f0[i]);
    }
    else if (!strcasecmp (g3->f0[i].type.str, "gene")) {
      g3->f0[i].attr_id.id = g3->n_gene; // experimental (not used yet)
      g3->cds[g3->n_gene++] = &(g3->f0[i]);
    }
  }
  g3->cds  = (gff3_fields**) biomcmc_realloc ((gff3_fields**) g3->cds,  g3->n_cds  * sizeof (gff3_fields*));
  g3->gene = (gff3_fields**) biomcmc_realloc ((gff3_fields**) g3->gene, g3->n_gene * sizeof (gff3_fields*));
  return;
}

