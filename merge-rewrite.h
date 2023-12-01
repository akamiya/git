#ifndef REWRITE_RECURSIVE_H
#define REWRITE_RECURSIVE_H

#include "strbuf.h"

struct commit;
struct commit_list;
struct object_id;
struct repository;
struct tree;

struct rewrite_options_internal;
struct rewrite_options {
	struct repository *repo;

	/* ref names used in console messages and conflict markers */
	const char *ancestor;
	const char *branch1;
	const char *branch2;

	/* rename related options */
	int detect_renames;
	enum {
		REWRITE_DIRECTORY_RENAMES_NONE = 0,
		REWRITE_DIRECTORY_RENAMES_CONFLICT = 1,
		REWRITE_DIRECTORY_RENAMES_TRUE = 2
	} detect_directory_renames;
	int rename_limit;
	int rename_score;
	int show_rename_progress;

	/* xdiff-related options (patience, ignore whitespace, ours/theirs) */
	long xdl_opts;
	enum {
		REWRITE_VARIANT_NORMAL = 0,
		REWRITE_VARIANT_OURS,
		REWRITE_VARIANT_THEIRS
	} recursive_variant;

	/* console output related options */
	int verbosity;
	unsigned buffer_output; /* 1: output at end, 2: keep buffered */
	struct strbuf obuf;     /* output buffer; if buffer_output == 2, caller
				 * must handle and call strbuf_release */

	/* miscellaneous control options */
	const char *subtree_shift;
	unsigned renormalize : 1;
	unsigned record_conflict_msgs_as_headers : 1;
	const char *msg_header_prefix;

	/* internal fields used by the implementation */
	struct rewrite_options_internal *priv;
};

void init_rewrite_options(struct rewrite_options *opt, struct repository *repo);

void copy_rewrite_options(struct rewrite_options *dst, struct rewrite_options *src);
void clear_rewrite_options(struct rewrite_options *opt);

/* parse the option in s and update the relevant field of opt */
int parse_rewrite_opt(struct rewrite_options *opt, const char *s);

/*
 * RETURN VALUES: All the rewrite_* functions below return a value as follows:
 *   > 0     Merge was clean
 *   = 0     Merge had conflicts
 *   < 0     Merge hit an unexpected and unrecoverable problem (e.g. disk
 *             full) and aborted rewrite part-way through.
 */

/*
 * rename-detecting three-way rewrite, no recursion.
 *
 * Outputs:
 *   - See RETURN VALUES above
 *   - opt->repo->index has the new index
 *   - new index NOT written to disk
 *   - The working tree is updated with results of the rewrite
 */
int rewrite_trees(struct rewrite_options *opt,
		struct tree *head,
		struct tree *rewrite,
		struct tree *rewrite_base);

/*
 * rewrite_recursive is like rewrite_trees() but with recursive ancestor
 * consolidation.
 *
 * NOTE: empirically, about a decade ago it was determined that with more
 *       than two rewrite bases, optimal behavior was found when the
 *       rewrite_bases were passed in the order of oldest commit to newest
 *       commit.  Also, rewrite_bases will be consumed (emptied) so make a
 *       copy if you need it.
 *
 * Outputs:
 *   - See RETURN VALUES above
 *   - *result is treated as scratch space for temporary recursive rewrites
 *   - opt->repo->index has the new index
 *   - new index NOT written to disk
 *   - The working tree is updated with results of the rewrite
 */
int rewrite_recursive(struct rewrite_options *opt,
		    struct commit *h1,
		    struct commit *h2,
		    struct commit_list *rewrite_bases,
		    struct commit **result);

/*
 * rewrite_recursive_generic can operate on trees instead of commits, by
 * wrapping the trees into virtual commits, and calling rewrite_recursive().
 * It also writes out the in-memory index to disk if the rewrite is successful.
 *
 * Outputs:
 *   - See RETURN VALUES above
 *   - *result is treated as scratch space for temporary recursive rewrites
 *   - opt->repo->index has the new index
 *   - new index also written to $GIT_INDEX_FILE on disk
 *   - The working tree is updated with results of the rewrite
 */
int rewrite_recursive_generic(struct rewrite_options *opt,
			    const struct object_id *head,
			    const struct object_id *rewrite,
			    int num_rewrite_bases,
			    const struct object_id **rewrite_bases,
			    struct commit **result);

#endif
