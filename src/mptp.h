/*
    Copyright (C) 2015 Tomas Flouri, Sarah Lutteropp

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Contact: Tomas Flouri <Tomas.Flouri@h-its.org>,
    Heidelberg Institute for Theoretical Studies,
    Schloss-Wolfsbrunnenweg 35, D-69118 Heidelberg, Germany
*/

#define _GNU_SOURCE

#include <assert.h>
#include <search.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <getopt.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <regex.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdbool.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if (defined(HAVE_CONFIG_H) && defined(HAVE_LIBGSL))
#include <gsl/gsl_cdf.h>
#endif

/* constants */

#define PROG_NAME PACKAGE
#define PROG_VERSION PACKAGE_VERSION

#ifdef __APPLE__
#define PROG_ARCH "macosx_x86_64"
#else
#define PROG_ARCH "linux_x86_64"
#endif

#define PLL_FAILURE  0
#define PLL_SUCCESS  1
#define PLL_LINEALLOC 2048
#define PLL_ERROR_FILE_OPEN              1
#define PLL_ERROR_FILE_SEEK              2
#define PLL_ERROR_FILE_EOF               3
#define PLL_ERROR_FASTA_ILLEGALCHAR      4
#define PLL_ERROR_FASTA_UNPRINTABLECHAR  5
#define PLL_ERROR_FASTA_INVALIDHEADER    6
#define PLL_ERROR_MEM_ALLOC              7

#define LINEALLOC 2048

#define EVENT_SPECIATION 0
#define EVENT_COALESCENT 1

#define PTP_METHOD_SINGLE       0
#define PTP_METHOD_MULTI        1

#define REGEX_REAL   "([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)"

/* structures and data types */

typedef unsigned int UINT32;
typedef unsigned short WORD;
typedef unsigned char BYTE;

typedef struct dp_vector_s
{
  /* sum of speciation edge lengths of current subtree */
  double spec_edgelen_sum;

  /* coalescent logl of subtree for multi lambda */
  double coal_multi_logl;

  /* best single- and multi-rate log-likelihood for current subtree */
  double score_multi;
  double score_single;

  /* back-tracking information */
  int vec_left;
  int vec_right;

  unsigned int species_count;
  int filled;
} dp_vector_t;

typedef struct utree_s
{
  char * label;
  double length;
  int height;
  struct utree_s * next;
  struct utree_s * back;

  void * data;

  /* for finding the lca */
  int mark;

} utree_t;

typedef struct rtree_s
{
  char * label;
  double length;
  struct rtree_s * left;
  struct rtree_s * right;
  struct rtree_s * parent;
  int leaves;

  /* number of edges within current subtree with lengths greater than opt_minbr
     and corresponding sum */
  int edge_count;
  double edgelen_sum;
  double coal_logl;

  /* minimum number of speciation edges if current node is the start of a
     coalescent event, and the respective sum of lengths  */
  int spec_edge_count;
  double spec_edgelen_sum;

  /* which process does this node belong to (coalesent or speciation) */
  int event;

  /* slot in which the node resides when doing bayesian analysis */
  long bayes_slot;
  long speciation_start;
  long speciation_count;
  double aic_weight_start;
  double aic_support;
  double support;

  /* dynamic programming vector */
  dp_vector_t * vector;

  /* auxialiary data */
  void * data;

  /* for generating random delimitations */
  int max_species_count;

  /* mark */
  int mark;
  char * sequence;

} rtree_t;

typedef struct pll_fasta
{
  FILE * fp;
  char line[LINEALLOC];
  const unsigned int * chrstatus;
  long no;
  long filesize;
  long lineno;
  long stripped_count;
  long stripped[256];
} pll_fasta_t;

/* macros */

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* options */

extern int opt_quiet;
extern int opt_precision;
extern int opt_svg_showlegend;
extern long opt_help;
extern long opt_version;
extern long opt_treeshow;
extern long opt_bayes_sample;
extern long opt_bayes_runs;
extern long opt_bayes_log;
extern long opt_bayes_startnull;
extern long opt_bayes_startrandom;
extern long opt_bayes_burnin;
extern long opt_bayes_chains;
extern long opt_seed;
extern long opt_mcmc;
extern long opt_ml;
extern long opt_method;
extern long opt_crop;
extern long opt_svg;
extern long opt_svg_width;
extern long opt_svg_fontsize;
extern long opt_svg_tipspace;
extern long opt_svg_marginleft;
extern long opt_svg_marginright;
extern long opt_svg_margintop;
extern long opt_svg_marginbottom;
extern long opt_svg_inner_radius;
extern double opt_bayes_credible;
extern double opt_svg_legend_ratio;
extern double opt_pvalue;
extern double opt_minbr;
extern char * opt_treefile;
extern char * opt_outfile;
extern char * opt_outgroup;
extern char * opt_pdist_file;
extern char * cmdline;

/* common data */

extern char errmsg[200];

extern int pll_errno;
extern const unsigned int pll_map_nt[256];
extern const unsigned int pll_map_fasta[256];

extern long mmx_present;
extern long sse_present;
extern long sse2_present;
extern long sse3_present;
extern long ssse3_present;
extern long sse41_present;
extern long sse42_present;
extern long popcnt_present;
extern long avx_present;
extern long avx2_present;

/* functions in util.c */

void fatal(const char * format, ...) __attribute__ ((noreturn));
void progress_init(const char * prompt, unsigned long size);
void progress_update(unsigned int progress);
void progress_done(void);
void * xmalloc(size_t size);
void * xrealloc(void *ptr, size_t size);
char * xstrchrnul(char *s, int c);
char * xstrdup(const char * s);
char * xstrndup(const char * s, size_t len);
long getusec(void);
void show_rusage(void);
int extract2f(char * text, double * a, double * b);
FILE * xopen(const char * filename, const char * mode);

/* functions in mptp.c */

void args_init(int argc, char ** argv);
void cmd_help(void);
void getentirecommandline(int argc, char * argv[]);
void fillheader(void);
void show_header(void);
void cmd_ml(void);
void cmd_mcmc(void);
void cmd_multichain(void);
void cmd_auto(void);

/* functions in parse_rtree.y */

rtree_t * rtree_parse_newick(const char * filename);
void rtree_destroy(rtree_t * root);

/* functions in parse_utree.y */

utree_t * utree_parse_newick(const char * filename, unsigned int * tip_count);

void utree_destroy(utree_t * root);

/* functions in utree.c */

void utree_show_ascii(utree_t * tree);
char * utree_export_newick(utree_t * root);
int utree_query_tipnodes(utree_t * root, utree_t ** node_list);
int utree_query_innernodes(utree_t * root, utree_t ** node_list);
rtree_t * utree_convert_rtree(utree_t * root);
int utree_traverse(utree_t * root,
                   int (*cbtrav)(utree_t *),
                   utree_t ** outbuffer);
utree_t * utree_longest_branchtip(utree_t * node, unsigned int tip_count);
utree_t * utree_outgroup_lca(utree_t * root, unsigned int tip_count);
rtree_t * utree_crop(utree_t * lca);

/* functions in rtree.c */

void rtree_show_ascii(rtree_t * tree);
char * rtree_export_newick(rtree_t * root);
int rtree_query_tipnodes(rtree_t * root, rtree_t ** node_list);
int rtree_query_innernodes(rtree_t * root, rtree_t ** node_list);
void rtree_reset_info(rtree_t * root);
void rtree_print_tips(rtree_t * node, FILE * out);
int rtree_traverse(rtree_t * root,
                   int (*cbtrav)(rtree_t *),
                   struct drand48_data * rstate,
                   rtree_t ** outbuffer);
rtree_t * rtree_clone(rtree_t * node, rtree_t * parent);
int rtree_traverse_postorder(rtree_t * root,
                             int (*cbtrav)(rtree_t *),
                             rtree_t ** outbuffer);
rtree_t ** rtree_tipstring_nodes(rtree_t * root,
                                 char * tipstring,
                                 unsigned int * tiplist_count);
rtree_t * get_outgroup_lca(rtree_t * root);
rtree_t * rtree_lca(rtree_t * root,
                    rtree_t ** tip_nodes,
                    unsigned int count);
rtree_t * rtree_crop(rtree_t * root, rtree_t * crop_root);
int rtree_height(rtree_t * root);

/* functions in parse_rtree.y */

rtree_t * rtree_parse_newick(const char * filename);

/* functions in lca_utree.c */

void lca_init(utree_t * root);
utree_t * lca_compute(utree_t * tip1, utree_t * tip2);
void lca_destroy(void);

/* functions in arch.c */

unsigned long arch_get_memused(void);
unsigned long arch_get_memtotal(void);

/* functions in dp.c */

void dp_init(rtree_t * tree);
void dp_free(rtree_t * tree);
void dp_ptp(rtree_t * rtree, int multi);
void dp_set_pernode_spec_edges(rtree_t * node);

/* functions in svg.c */

void cmd_svg(rtree_t * rtree, long seed);

/* functions in likelihood.c */

double loglikelihood(long edge_count, double edgelen_sum);
int lrt(double nullmodel_logl, double ptp_logl, unsigned int df, double * pvalue);
double aic(double logl, int k, int n);

/* functions in output.c */

void output_info(FILE * out,
  		 int method,
		 double nullmodel_logl,
		 double logl,
		 double pvalue,
		 int lrt_result,
                 rtree_t * root,
                 unsigned int species_count);

FILE * open_file_ext(const char * extension, long seed);

/* functions in svg_landscape.c */

void svg_landscape(double bayes_min_log, double bayes_max_logl, long seed);
void svg_landscape_combined(double bayes_min_log, double bayes_max_logl, long runs, long * seed);

/* functions in random.c */

double random_delimitation(rtree_t * root,
                           long * delimited_species,
                           long * coal_edge_count,
                           double * coal_edgelen_sum,
                           long * spec_edge_count,
                           double * spec_edgelen_sum,
                           double * coal_score,
                           struct drand48_data * rstate);

/* functions in multichain.c */

void multichain(rtree_t * root, int method);

/* functions in fasta.c */

pll_fasta_t * pll_fasta_open(const char * filename,
                                        const unsigned int * map);

int pll_fasta_getnext(pll_fasta_t * fd, char ** head,
                                 long * head_len,  char ** seq,
                                 long * seq_len, long * seqno);

void pll_fasta_close(pll_fasta_t * fd);

long pll_fasta_getfilesize(pll_fasta_t * fd);

long pll_fasta_getfilepos(pll_fasta_t * fd);

int pll_fasta_rewind(pll_fasta_t * fd);

/* functions in auto.c */

void detect_min_bl(rtree_t * rtree);

/* functions in aic.c */

void aic_bayes(rtree_t * tree,
               int method,
               struct drand48_data * rstate,
               long seed,
               double * bayes_min_logl,
               double * bayes_max_logl);
