#include "clustering_goptics.h" 

typedef struct {
  int id;
  double coreDist;  // Minimum distance that makes this point a core. If it does not exist, set to DBL_MAX.
  double reachDist; // reachability of this point from other points in the same cluster. DBL_MAX in case it does not exist.
  bool processed;
  int pqPos; // Position in priority queue
} point; 

typedef struct edgearray_item { int id; double distance; } edgearray_item;
typedef struct element { point *p; } element;
typedef struct PriorityQueue { element *pq; int n, heap_size; } PriorityQueue;

static void expand_cluster_order (goptics_cluster gop, point *current);
static void update_results_from_current_point (goptics_cluster gop, point *current);
static void set_core_dist (goptics_cluster gop, point *current);
static void order_seeds_update (goptics_cluster gop, point *this);
static int compare_edgearray_item_increasing (const void *a, const void *b); 
edgearray_item* generate_graph (goptics_cluster gop); // cannot declare static (internal linkage) since -Wall would complain
static void aux_generate_Va_n (goptics_cluster gop, int idx);
edgearray_item* generate_graph_multithread (goptics_cluster gop);
static PriorityQueue* createHeap (int size);
static void destroyHeap (PriorityQueue *heap);
static int insertHeap (PriorityQueue *heap, point *p);
static void promoteElementHeap (PriorityQueue *heap, int child);
static point* getNextHeap (PriorityQueue *heap);
void demoteElementHeap (PriorityQueue *heap, int parent);

goptics_cluster
new_goptics_cluster (distance_generator dg, int min_points, double epsilon)
{
  int i;
  goptics_cluster gop = (goptics_cluster) biomcmc_malloc (sizeof (struct goptics_cluster_struct));
  gop->d = dg; dg->ref_counter++;
  gop->epsilon = epsilon;
  if (min_points > dg->n_samples) min_points = dg->n_samples;
  gop->min_points = min_points;
  gop->n_order = 0;
  gop->max_distance = -1.;
  gop->num_edges = 0;
  gop->n_clusters = 0;

  gop->core   = (bool*) biomcmc_malloc (dg->n_samples * sizeof (bool));
  gop->order   = (int*) biomcmc_malloc (dg->n_samples * sizeof (int));
  gop->cluster = (int*) biomcmc_malloc (dg->n_samples * sizeof (int));
  gop->core_distance  = (double*) biomcmc_malloc (dg->n_samples * sizeof (double));
  gop->reach_distance = (double*) biomcmc_malloc (dg->n_samples * sizeof (double));

  gop->Va_i = (int*) biomcmc_malloc (dg->n_samples * sizeof (int)); 
  gop->Va_n = (int*) biomcmc_malloc (dg->n_samples * sizeof (int));
  gop->Ea = NULL;
  gop->heap = NULL;
  gop->points = NULL;

  for (i = 0; i < dg->n_samples; i++) {
    gop->order[i] = gop->cluster[i] = -1;
    gop->core[i] = false;
    gop->core_distance[i] = 0.; 
    gop->reach_distance[i] = DBL_MAX;
  }
  return gop;
}

void
del_goptics_cluster (goptics_cluster gop)
{
  if (!gop) return;
  if (gop->core) free (gop->core);
  if (gop->order) free (gop->order);
  if (gop->cluster) free (gop->cluster);
  if (gop->core_distance) free (gop->core_distance);
  if (gop->reach_distance) free (gop->reach_distance);
  if (gop->Va_i) free (gop->Va_i);
  if (gop->Va_n) free (gop->Va_n);
  if (gop->Ea) free (gop->Ea);
  if (gop->points) free (gop->points);
  destroyHeap (gop->heap);
  del_distance_generator (gop->d);
  free (gop);
}

goptics_cluster
new_goptics_cluster_run (distance_generator dg, int min_points, double epsilon)
{
  int i;
  goptics_cluster gop = new_goptics_cluster (dg, min_points, epsilon);
  edgearray_item *Ea = NULL;
  PriorityQueue *heap = NULL;
  point *points = NULL;
  clock_t time1, time0;
//  if ((gop->Ea != NULL) || (gop->heap != NULL)) biomcmc_error ("goptics_cluster_run() was called before; please now use rerun() instead.");
  time0 = clock ();
  heap = createHeap (gop->d->n_samples);
  points = (point*) biomcmc_malloc (gop->d->n_samples * sizeof (point));
  for(i = 0; i < gop->d->n_samples; ++i) {
    points[i].id = i; points[i].pqPos = -1; points[i].coreDist  = 0.; points[i].reachDist = DBL_MAX; points[i].processed = false;
  }
#ifdef _OPENMP
  Ea = generate_graph_multithread (gop); // will update max_distance and num_edges
#else
  Ea = generate_graph (gop);  // will update max_distance and num_edges
#endif

  gop->Ea = (edgearray_item*) Ea;
  gop->heap = (PriorityQueue*) heap;
  gop->points = (point*) points;

  for (i = 0; i < gop->d->n_samples; ++i) if (!points[i].processed) expand_cluster_order (gop, &points[i]);
  time1 = clock (); gop->timing_secs += (double)(time1-time0)/(double)(CLOCKS_PER_SEC); 
  return gop;
}

void
assign_goptics_clusters (goptics_cluster gop, double cluster_eps)
{
  int i, j, cluster = -1;
  if (cluster_eps > 0.999 * gop->epsilon) cluster_eps = 0.999 * gop->epsilon;
  for(j = 0; j < gop->d->n_samples; j++) {
    i = gop->order[j]; // only place that uses it is cluster[i] (others must be ordered by point *current)
    if (gop->reach_distance[j] > cluster_eps) { 
      if (gop->core_distance[j] <= cluster_eps) {
      cluster++;
      gop->cluster[i] = cluster;
      }
      else gop->cluster[i] = -1;
    } 
    else gop->cluster[i] = cluster;
  }
  gop->n_clusters = cluster + 1;
}

static void 
expand_cluster_order (goptics_cluster gop, point *current)
{
  PriorityQueue *heap = (PriorityQueue*) gop->heap;

  current->processed = true;
  set_core_dist (gop, current);	// Define the core distance of the current point (or DBL_MAX)
  update_results_from_current_point (gop, current);

  if (current->coreDist != DBL_MAX) order_seeds_update (gop, current);

  while (heap->n > 0) {
    current = getNextHeap (heap);
    current->processed = true;
    set_core_dist (gop, current);
    update_results_from_current_point (gop, current);
    if (current->coreDist != DBL_MAX) order_seeds_update (gop, current);
  }
}

static void
update_results_from_current_point (goptics_cluster gop, point *current)
{
  gop->order[gop->n_order] = current->id; // cannot update on [current->id] directly since last pass is in order of *current
  gop->core_distance[gop->n_order] = current->coreDist;
  gop->reach_distance[gop->n_order] = current->reachDist;
  /* just cosmetic change, to replace DBL_MAX values */
  if (gop->reach_distance[gop->n_order] > gop->max_distance) gop->reach_distance[gop->n_order] = 2 * gop->max_distance;
  if (gop->core_distance[gop->n_order] > gop->max_distance) gop->core_distance[gop->n_order] = 2 * gop->max_distance;
  if (current->coreDist < gop->epsilon) gop->core[gop->n_order] = true;
  else  gop->core[gop->n_order] = false;
  gop->n_order++;
}

static void 
set_core_dist (goptics_cluster gop, point *current)
{ 
  edgearray_item *Ea = (edgearray_item*) gop->Ea; // gop->Ea is type void
  if (gop->Va_n[current->id] >=  gop->min_points - 1) { // If enough neighbours (and we always have min_points <= n_samples)
    qsort ((void*) &(Ea[gop->Va_i[current->id]]), gop->Va_n[current->id], sizeof (edgearray_item), compare_edgearray_item_increasing);
    current->coreDist = Ea[gop->Va_i[current->id] + gop->min_points - 2].distance; // -2 in Ea b/c itself was not counted as neighbour
  } else current->coreDist = DBL_MAX;
}

static void 
order_seeds_update (goptics_cluster gop, point *this)
{
  int i = 0;
  double cdist, newrdist;
  point *points = (point*) gop->points, *p;
  edgearray_item *Ea = (edgearray_item*) gop->Ea;
  PriorityQueue *heap = (PriorityQueue*) gop->heap;

  cdist = this->coreDist;
  // for all neighbours of this
  for(i = gop->Va_i[this->id]; (i < gop->Va_i[this->id] + gop->Va_n[this->id]) && (gop->Va_n[this->id] > 0); ++i) { 
    p = &(points[Ea[i].id]);
    if(!p->processed) {
      newrdist = ((cdist >= Ea[i].distance) ? cdist : Ea[i].distance);
      if(p->reachDist == DBL_MAX) { // if not in heap, find reachDist and insert on heap
        p->reachDist = newrdist;
        insertHeap (heap, p);
      } else {
        if (newrdist < p->reachDist){ // if already in heap, update reachDist
          p->reachDist = newrdist;
          promoteElementHeap (heap, p->pqPos);
        }
      }
    }
  }
}

static int 
compare_edgearray_item_increasing (const void *a, const void *b) 
{
  edgearray_item *x = (edgearray_item *) a;
  edgearray_item *y = (edgearray_item *) b;
  if (x->distance > y->distance) return 1;
  if (x->distance < y->distance) return -1;
  return 0;
}

edgearray_item* 
generate_graph (goptics_cluster gop)
{ 
  edgearray_item *Ea;
  int i = 0, j = 0, auxEa = 0, n_neighbours = 0, neighbour_list = 0;
  double de = 0;

  for(j = 1; j < gop->d->n_samples; j++) for(i = 0; i < j; i++) { // just to find size of edge_array
    de = distance_generator_get (gop->d, i, j);
    if (de > gop->max_distance) gop->max_distance = de;
    if (de <=  gop->epsilon) gop->num_edges += 2; 
  }

  Ea = (edgearray_item*) biomcmc_malloc ((sizeof (edgearray_item) * gop->num_edges));

  for (i = 0; i < gop->d->n_samples; i++) {	
    gop->Va_i[i] = auxEa;
    gop->Va_n[i] = 0;
    for(j = 0; j < gop->d->n_samples; ++j) if (i != j) {
      de = distance_generator_get (gop->d, i, j);
      if (de <= gop->epsilon) {
        Ea[auxEa].id = j;
        Ea[auxEa].distance = de;
        auxEa += 1;
        gop->Va_n[i] += 1;
      }
    }
    n_neighbours = gop->Va_n[i];
    neighbour_list = gop->Va_i[i];	
    qsort((void*) &Ea[neighbour_list], n_neighbours, sizeof (edgearray_item), compare_edgearray_item_increasing);
  }
  return Ea;
}

static void 
aux_generate_Va_n (goptics_cluster gop, int idx)
{ // aux function for CPU parallel
  int i;
  double de;
  gop->Va_n[idx] = 0;
  for(i = 0; i < gop->d->n_samples; ++i) if (idx != i) {
    de = distance_generator_get (gop->d, i, idx);
    if (de > gop->max_distance) gop->max_distance = de;
    if ( de <=  gop->epsilon) gop->Va_n[idx] += 1;
  }
}

edgearray_item* 
generate_graph_multithread (goptics_cluster gop)
{
  edgearray_item *Ea;
  int i, n_neighbours = 0, neighbour_list = 0;
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
  for(i = 0; i < gop->d->n_samples; i++) aux_generate_Va_n (gop, i);

  gop->Va_i[0] = 0;
  gop->num_edges = gop->Va_n[0];
  for(i = 1; i < gop->d->n_samples; i++) {
    gop->Va_i[i] = gop->Va_i[i-1] + gop->Va_n[i];
    gop->num_edges += gop->Va_n[i];
  }
  Ea = (edgearray_item*) biomcmc_malloc ((sizeof (edgearray_item) * gop->num_edges));

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
  for(i=0; i < gop->d->n_samples; ++i) {
    int idx = i, j; // declared here to avoid race condition
    double de;
    int pointer = gop->Va_i[idx];
    if (gop->Va_n[idx] > 0) for (j = 0; j < gop->d->n_samples; ++j) if (idx != j) {
      de = distance_generator_get (gop->d, j, idx); // original has i, idx
      if (de > gop->max_distance) gop->max_distance = de;
      if ( de <=  gop->epsilon) {
        Ea[pointer].id = j;
        Ea[pointer].distance = de;
        pointer++;
      }
    }
  }
  for (i = 0; i < gop->d->n_samples; ++i) {  // sort vector of neighbours // #pragma omp parallel for schedule(dynamic)
    n_neighbours   = gop->Va_n[i];
    neighbour_list = gop->Va_i[i];	
    qsort ((void*) &Ea[neighbour_list], n_neighbours, sizeof (edgearray_item), compare_edgearray_item_increasing);
  }
  return Ea;
}

static PriorityQueue* createHeap (int size)
{
  PriorityQueue *heap = (PriorityQueue*) biomcmc_malloc (sizeof (PriorityQueue));
  heap->pq = (element*) biomcmc_malloc (size * sizeof (element));
  heap->n = 0;
  heap->heap_size = size;
  return heap;
}

static void destroyHeap (PriorityQueue *heap) 
{
  if (!heap) return;
  if (heap->pq) free (heap->pq);
  free (heap);
}

static int insertHeap (PriorityQueue *heap, point *p) 
{ //MinHeap
  if (heap == NULL) { fprintf(stderr, "Could not insert on priority queue.\n");return 0; }
  if (heap->n == heap->heap_size) { printf("Heap is full\n"); return 0; }
  heap->pq[heap->n].p = p;
  heap->pq[heap->n].p->pqPos = heap->n;
  promoteElementHeap (heap, heap->n);
  heap->n++; // notice that in biomcmc default heaps we leave pq[0] empty
  return 1;
}

static void promoteElementHeap (PriorityQueue *heap, int child) 
{
  int parent = (child - 1)/2; // biomcmc default heap has child/2 
  element temp;
  // if child == 0 (first insertion)  we do nothing
  while((child > 0) && (heap->pq[parent].p->reachDist > heap->pq[child].p->reachDist)){
    temp = heap->pq[child];
    heap->pq[child] = heap->pq[parent];
    heap->pq[parent] = temp;
    heap->pq[child].p->pqPos = child;
    heap->pq[parent].p->pqPos = parent;
    child = parent;
    parent = (parent - 1) / 2;
  }
  if(child != heap->pq[child].p->pqPos) biomcmc_error ("could not promote OPTICS MinHeap element");
}

static point* 
getNextHeap (PriorityQueue *heap)
{
  if (heap == NULL) { printf("could not find heap in getNextHeap\n"); return NULL; }
  point *temp;
  temp = heap->pq[0].p;// Copies the first element to temp;
  heap->pq[0] = heap->pq[heap->n - 1];
  heap->pq[0].p->pqPos = 0;
  heap->n--;
  demoteElementHeap (heap, 0);// Then demote the first element to the position he should be according to his Priority
  temp->pqPos = -1;
  return temp;
}

void
demoteElementHeap (PriorityQueue *heap, int parent)
{
  element temp;
  int child = 2 * parent + 1;
  while (child < heap->n) {
    if (child < heap->n - 1) if (heap->pq[child].p->reachDist > heap->pq[child + 1].p->reachDist) child++;
    if (heap->pq[parent].p->reachDist <= heap->pq[child].p->reachDist) break;

    temp = heap->pq[parent]; // swap positions
    heap->pq[parent] = heap->pq[child];
    heap->pq[child] = temp;
    heap->pq[child].p->pqPos = child; // update positions indices
    heap->pq[parent].p->pqPos = parent;
    parent = child;
    child = 2 * parent + 1;
  }
  if (parent != heap->pq[parent].p->pqPos) biomcmc_error ("could not promote OPTICS MinHeap element");
}
