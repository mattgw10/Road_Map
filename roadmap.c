/**
 * Matthew Westbrook
 *
 * Reads a roadmap in binary format into memory.  The nodes of the
 * roadmap are stored in an array where the index in the array is the
 * number of the node minus 1.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>		/* for POSIX byte order conversions */

#include "roadmap.h"

static const size_t int_field_size = 4; /* bytes */
static const size_t ver_field_size = 1; /* byte */
static const unsigned char current_version = 2;


double sq_dist(int x, int y, int u, int v)
{
	double dx = x - u;
	double dy = y - v;
	return dx * dx + dy * dy;
}


/*
 * Read an unsigned integer field from the file.
 *
 * Return 1 on success and 0 on failure.
 */
static int read_uint_field(FILE *f, unsigned int *field)
{
	size_t ret;

	ret = fread(field, int_field_size, 1, f);
	if (ret != 1) {
		if (ret == 0)
			fprintf(stderr, "Unexpected end of file\n");
		else
			fprintf(stderr, "Short read\n");
		return 1;
	}
	*field = ntohl(*field);
	return 0;
}

/*
 * Read an integer field from the file.
 *
 * Return 1 on success and 0 on failure.
 */
static int read_int_field(FILE *f, int *field)
{
	size_t ret;

	ret = fread(field, int_field_size, 1, f);
	if (ret != 1) {
		if (ret == 0)
			fprintf(stderr, "Unexpected end of file\n");
		else
			fprintf(stderr, "Short read\n");
		return 1;
	}
	*field = ntohl(*field);

	return 0;
}


/*
 * Read the arc from the given file.
 *
 * Return 1 on success and 0 on failure.
 */
static int read_arc(FILE *f, struct arc *a)
{
	int err;

	err = read_uint_field(f, &a->target);
	a->target = a->target - 1; /* avoiding 1-indexing of input */
	if (err)
		return 1;
	err = read_uint_field(f, &a->wt);
	if (err)
		return 1;

	return 0;
}


/*
 * Read the arcs from the given file into the array of arcs.
 *
 * Return 1 on success and 0 on failure.
 */
static int read_arcs(FILE *f, struct arc arcs[], unsigned int narcs)
{
	unsigned int i;
	int err;

	for (i = 0; i < narcs; i += 1) {
		err = read_arc(f, &arcs[i]);
		if (err)
			break;
	}

	return err;
}


/*
 * Reads the next node from the given file.  On success, the 'arcv'
 * array is allocated and it must be freed by the caller.
 *
 * Return 1 on success, 0 on failure.
 */
static int read_node(FILE *f, struct node *n)
{
	int err;

	err = read_uint_field(f, &n->num);
	n->num = n->num - 1; /* avoiding 1-indexing */
	if (err)
		return 1;
	err = read_int_field(f, &n->x);
	if (err)
		return 1;
	err = read_int_field(f, &n->y);
	if (err)
		return 1;
	err = read_uint_field(f, &n->narcs);
	if (err)
		return 1;

	n->arcv = malloc(n->narcs * sizeof(*n->arcv));
	if (!(n->arcv)) {
		fprintf(stderr, "Failed allocating %lu bytes for %u arcs\n",
			(unsigned long) n->narcs * sizeof(*n->arcv), n->narcs);
		perror("malloc failed");
		return 1;
	}

	err = read_arcs(f, n->arcv, n->narcs);
	if (err) {
		free(n->arcv);
		return 1;
	}

	return 0;
}

/*
 * Reads the nodes into the given array.
 *
 * Return 0 on success and 1 on failure.
 */
static int read_nodes(FILE *f, struct node nodes[], unsigned int nnodes)
{
	unsigned int i;
	int err;

	for (i = 0; i < nnodes; i += 1) {
		err = read_node(f, &nodes[i]);
		if (err)
			break;
	}

	return err;
}


/*
 * Reads the header from the file.
 *
 * Return 1 on success and 0 on failure.
 */
static int read_header(FILE *f, unsigned char *ver, unsigned int *nnodes)
{
	int err;
	ssize_t ret;

	ret = fread(ver, ver_field_size, 1, f);
	if (ret != 1) {
		if (ret == 0)
			fprintf(stderr, "Unexpected end of file\n");
		else
			fprintf(stderr, "Short read\n");
		return 1;
	}
	err = read_uint_field(f, nnodes);
	if (err)
		return 1;
	if (*ver > 1) {
		unsigned int narcs;
		err = read_uint_field(f, &narcs);
		if (err)
			return 1;
	}

	return 0;
}


int load_map(FILE *f, struct node **nodesp, unsigned int *nnodes){
	unsigned char ver = 0;
	struct node *nodes;
	int err;

	err = read_header(f, &ver, nnodes);
	if (err) return 1;
	if (ver > current_version) {
		fprintf(stderr,
			"File version is %u, %u is the maximum version\n",
			(unsigned int) ver, (unsigned int) current_version);
		return 1;
	}

	if (*nnodes == 0) {
		fprintf(stderr, "No nodes to read\n");
		return 1;
	}

	nodes = malloc((*nnodes) * sizeof(*nodes));
	if (!nodes) {
		fprintf(stderr, "Failed allocating %lu bytes for nodes\n",
			(unsigned long) (*nnodes) * sizeof(*nodes));
		perror("malloc failed");
		return 1;
	}

	err = read_nodes(f, nodes, *nnodes);
	if (err) {
		free(nodes);
		return 1;
	}

	*nodesp = nodes;

	return 0;
}


void free_map(struct node *nodes, unsigned int nnodes)
{
	unsigned int i;

	for (i = 0; i < nnodes; i += 1)
		free(nodes[i].arcv);

	free(nodes);
}
