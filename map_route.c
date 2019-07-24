/**
 * Matthew Westbrook
 *
 * Plans a route in a roadmap.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "system.h"

/*
 * Single source shortest path solver using Djikstra's algorithm.
 * 'source' is the ID value of the source node (its node structure can
 * be found at nodes[source - 1]) and 'target' is the ID value of the
 * target node.  The 'costp' argument should be used to return the
 * final path cost value.
 *
 * Return 0 on success and 1 on failure.
 */

 void swap_Heap(struct heap_node *A, int x, int y, int *track){
	struct heap_node heap_swap;
	
	heap_swap = A[x];
	A[x] = A[y];
	A[y] = heap_swap;
	
	track[A[x].v_ind] = x;
	track[A[y].v_ind] = y;
}

int Pull_Up(struct heap_node *A, int i, int *track){
	int P;
	
	if (i == 0)
		P = -1;
	else if (i == 1 || i == 2)
		P = 0;
	else if (i%2 == 0)
		P = (i-2)/2;
	else
		P = (i-1)/2;
	
	if (((A[P].d>A[i].d)||(A[P].d==-1))&&P!=-1){
		swap_Heap(A, i, P, track);
		return Pull_Up(A, P, track);
	}
	else
		return i;
}

void Min_Heapify(struct heap_node *A, int i, int size, int *track){
	int L, R, smallest;
	
	L = 2*i+1;
	R = 2*i+2;
	
	if ((L<=(size)) && ((A[L].d)<(A[i].d))&&((A[L].d)!=-1))
		smallest = L;
	else
		smallest = i;
	if ((R<=(size)) && ((A[R].d)<(A[smallest].d))&&((A[R].d)!=-1))
		smallest = R;
	if (smallest!=i){
		swap_Heap(A, i, smallest, track);
		Min_Heapify(A, smallest, size, track);
	}
}

int djikstra(FILE * outfile, struct node nodes[], unsigned int nnodes,
	     unsigned int source, unsigned int target, unsigned int *costp)
{
    /* Find the shortest path and output it.  Return the cost via
     * 'costp'. */
	
	int i, size = nnodes-1, *heap_track, u, v, *solution, loc;
	struct heap_node *Q;
	
	Q = malloc(sizeof(struct heap_node)*nnodes);
	heap_track = malloc(sizeof(int)*nnodes);
	solution = malloc(sizeof(int)*nnodes);
	
        for (i = 0; i < nnodes; i+=1){ 
		Q[i].d = -1;
		Q[i].pi = -1;
		Q[i].v_ind = nodes[i].num;
		heap_track[nodes[i].num] = i;
        }
        Q[source].d = 0;
	swap_Heap(Q, source, 0, heap_track);
	
        while (size>0) {
		u = Q[0].v_ind;
		swap_Heap(Q, size, 0, heap_track);
		size-=1;
		Min_Heapify(Q, 0, size, heap_track);
		for (i=0; i<(nodes[u].narcs); i+=1){
			v = heap_track[nodes[u].arcv[i].target];
			if (((Q[v].d ==-1)||((Q[v].d )>(Q[size+1].d+nodes[u].arcv[i].wt)))&&(v<=size)){
				Q[v].d = Q[size+1].d+nodes[u].arcv[i].wt;
				Q[v].pi = u;
				loc = Pull_Up(Q, v, heap_track);
				Min_Heapify(Q, loc, size, heap_track);
			}
		}
		if (u==target)
			size = -1;
	} 
	
	*costp = Q[heap_track[target]].d;
	solution[0] = target;
	u = Q[heap_track[target]].pi;
	size = 1;
	while (u!=source){
		solution[size] = u;
		u = Q[heap_track[u]].pi;
		size+=1;
	}
	solution[size] = u;
	while(size>=0){
		printf("%d\n", solution[size]);
		size-=1;
	}
	
	free(Q);
	free(heap_track);
	free(solution);
	
    return 0;
}



/* Read the user's input and call the search. */
static int input_and_search(FILE * infile, struct node nodes[],
			    unsigned int nnodes)
{
    int err = 0;
    unsigned int s, t;
    unsigned int cost = 0;
    double start, end;

    while (fscanf(infile, "%u %u", &s, &t) == 2) {
	s = s - 1; /* avoiding 1-indexing */
	t = t - 1;
	if (s >= nnodes) {
	    fprintf(stderr, "Start node is invalid\n");
	    continue;
	}
	if (t >= nnodes) {
	    fprintf(stderr, "Target node is invalid\n");
	    continue;
	}
	printf("finding a route from %d to %d\n", s, t);
	start = get_current_seconds();
	err = djikstra(stdout, nodes, nnodes, s, t, &cost);
	end = get_current_seconds();
	if (err)
	    break;
	printf("cost: %u\n", cost);
	printf("time: %f seconds\n", end - start);
    }

    return err;
}


/* Print the usage string. */
static void usage(void)
{
    fprintf(stderr, "Usage:\nmap-route <datafile> <file>\n");
    exit(EXIT_FAILURE);
}


int main(int argc, const char *const argv[])
{
    int err, ret = EXIT_FAILURE;
    FILE *f, *infile = stdin;
    double start, end;
    unsigned int nnodes;
    struct node *nodes;

    if (argc != 3)
	usage();

    f = fopen(argv[1], "r");
    if (!f) {
	perror("Error opening data file");
	return EXIT_FAILURE;
    }
    if (strcmp(argv[2], "-") != 0) {
	infile = fopen(argv[2], "r");
	if (!infile) {
	    perror("Error opening data file");
	    fclose(f);
	    return EXIT_FAILURE;
	}
    }

    start = get_current_seconds();
    err = load_map(f, &nodes, &nnodes);
    if (err)
	goto out_map;
    end = get_current_seconds();
    fclose(f);
    if (err)
	return EXIT_FAILURE;
    printf("Loaded %d nodes in %f seconds\n", nnodes, end - start);
    printf("Using %d MB\n", peak_memory_usage());

    err = input_and_search(infile, nodes, nnodes);
    if (err)
	goto out;

    printf("Peak memory usage %d MB\n", peak_memory_usage());

    ret = EXIT_SUCCESS;
  out:
    free_map(nodes, nnodes);
  out_map:
    if (infile != stderr)
	fclose(infile);
    return ret;
}
