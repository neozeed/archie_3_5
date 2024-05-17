#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "case.h"
#include "defs.h"
#include "sort.h"
#include "timing.h"
#include "utils.h"


struct infix_stats {
  unsigned long textSize;       /* size of text file */
  unsigned long tempSize;       /* size of all temporary files */
  unsigned long infixSize;      /* size of infix file */
  unsigned long memSize;        /* maximum amount of memory for buffers */
  unsigned long txtBufSize;
  struct _patrie_timing runsTime; /* time to create runs */
  struct _patrie_timing mergeTime; /* time to merge runs */
  struct _patrie_timing infixTime; /* total time to create infix file */
};

typedef struct {
  int c;                        /* count */
  int s;
  unsigned short lh;
  unsigned short rh;
} RunElement;


typedef struct {
  RunElement *elt;              /* current buffer of run elements */
  size_t nelts;                 /* # of elements in `elts' buffer */
  size_t curr;                  /* offset of current element in `elts' */
  
  long offset;                  /* offset of this run in runs file */

  size_t nitems;                /* # of elements written to this run */
  size_t outposi;               /* # of elements written to infix file */
  size_t outsofar;
} Run;


/*  
 *
 *
 *                             Stats routines.
 *
 *
 */  

static void printStats(struct infix_stats s)
{
  fprintf(stderr, "# text_size temp_size infix_size max_mem_size text_buf_size "
          "runs_time merge_time total_time\n");
  fprintf(stderr, "infix %lu %lu %lu %lu %lu %.3f %.3f %.3f\n",
          s.textSize, s.tempSize, s.infixSize, s.memSize, s.txtBufSize,
          _patrieTimingDiff(&s.runsTime),
          _patrieTimingDiff(&s.mergeTime),
          _patrieTimingDiff(&s.infixTime));
}


/*  
 *
 *
 *                            Internal routines.
 *
 *
 */  

/*
 *  Create a temporary file, open for update.  Unlink the file in case we
 *  crash.
 */
static FILE *tempFile(const char *tmpdir)
{
  char *name;
  FILE *fp;
  
  if ( ! (name = tempnam(tmpdir, 0))) {
    fprintf(stderr, "%s: tempFile: can't get temporary file name: ",
            prog); perror("tempnam");
    return 0;
  }

  if ( ! (fp = fopen(name, "w+b"))) {
    fprintf(stderr, "%s: tempFile: can't open `%s' with mode `%s': ",
            prog, name, "w+b"); perror("fopen"); free(name);
    return 0;
  }

  (void)unlink(name);
  free(name);

  return fp;
}


static int initRuns(FILE *runFP, size_t memsz, void *mem, int nruns, Run run[])
{
  int i;
  size_t nelts = memsz / (nruns * sizeof(RunElement));

  /* Initialize the run queues. */

  for (i = 0; i < nruns; i++) {
    run[i].elt = (RunElement *)mem + i * nelts;
    run[i].curr = 0;
    run[i].outsofar = run[i].outposi = 0;

    if (fseek(runFP, run[i].offset, SEEK_SET) == -1) {
      fprintf(stderr, "%s: initRuns: can't seek to %d'th run (offset %ld): ",
              prog, i, run[i].offset); perror("fseek");
      return 0;
    }
    if ((run[i].nelts = fread(run[i].elt, sizeof run[i].elt[0], nelts, runFP)) == 0) {
      fprintf(stderr, "%s: initRuns: can't read %lu items from %d'th run: ",
              prog, (unsigned long)nelts, i); perror("fread");
      return 0;
    }
  }

  return 1;
}


/*
 *  Read another chunk of run elements into the buffer.  Use the previous
 *  number of elements as the number to read this time.
 */
static int moreFromRun(FILE *runFP, Run *run)
{
  long off = run->offset + run->outposi * sizeof *run->elt;

  if (fseek(runFP, off, SEEK_SET) == -1) {
    fprintf(stderr, "%s: moreFromRun: can't seek to offset %ld in runs file: ",
            prog, off); perror("fseek");
    return 0;
  }
  
  if ((run->nelts = fread(run->elt, sizeof *run->elt, run->nelts, runFP)) == 0) {
    if (feof(runFP)) {
      run->outposi = -1; /* signals the end of the run */
    } else {
      fprintf(stderr, "%s: moreFromRun: error reading %lu elements from offset %ld in runs file: ",
              prog, (unsigned long)run->nelts, off); perror("fread");
      return 0;
    }
  }

  run->curr = 0;

  return 1;
}


/*  
 *  Add an index point to the current run.
 */
static void markPos(void *run, size_t indexPoint)
{
  Run *r = run;

  r->elt[r->nitems++].s = indexPoint;
}


/*  
 *  Entering this routine, we have a set of runs (or queues) of items.  Each
 *  item contains two heights and a count of the number of items, from all
 *  previous runs, that fall between it and its predecessor.  (The count of
 *  the first item of each run is the number of items, from all previous
 *  runs, that are less than it.)
 *  
 *  To select the next item to output we look at the count field of the next
 *  item from each run, starting from the last run, and choose the first one
 *  whose count is equal to the length of the last sequence of items output
 *  from previous runs.  [Can there be only one of these?]  This is
 *  implemented by maintaining a counter, for each run, that is incremented
 *  when an item is selected from a previous run.  The counter for the run
 *  containing the selected item is zeroed.  [Insert succinct explanation of
 *  why this is so.]
 */
static int cmerge(size_t memsz,
                  void *mem,
                  size_t tbs,
                  int nruns,
                  Run run[],
                  FILE *runFP,
                  FILE *infixFP)
{
  int flag = 1;
  int h, s, lasth = 0;
  int i, j;

  if ( ! initRuns(runFP, memsz, mem, nruns, run)) {
    fprintf(stderr, "%s: can't initialize runs from runs file.\n", prog);
    return 0;
  }
  
  s = 0;
  if (fwrite(&s, sizeof s, 1, infixFP) == 0) {
    fprintf(stderr, "%s: cmerge: error writing to infix file: ", prog);
    perror("fwrite");
    return 0;
  }

  while (1) {
    /*  
     *  Find the run containing the next item to output.  The item's count
     *  field will equal the length of the sequence of items output from
     *  previous runs.
     */

    for (i = nruns - 1; i >= 0; --i) {
      if (run[i].outposi != -1 && run[i].elt[run[i].curr].c == run[i].outsofar) {
        break;
      }
    }

    if (i < 0) break;           /* All run queues empty */

    /*  
     *  Zero the counter for the current run and increment those for all
     *  succeeding runs.
     */
    
    run[i].outsofar = 0;
    for (j = i + 1; j < nruns; j++) {
      run[j].outsofar++;
    }
      
    h = MAX(lasth, run[i].elt[run[i].curr].lh); /* CHANGED to be comp. with Bill's code */
    lasth = run[i].elt[run[i].curr].rh;

    if (flag) {
      flag = 0;
    } else {
      fwrite(&h, sizeof h, 1, infixFP);
    }

    /*  
     *  Calculate the start value, write it, and increment our position in
     *  the current run queue.
     */
    
    s = i * tbs + run[i].elt[run[i].curr].s;
    s = -(s + 1);
    if (fwrite(&s, sizeof s, 1, infixFP) == 0) {
      fprintf(stderr, "%s: cmerge: error writing start to infix file: ", prog);
      perror("fwrite");
      return 0;
    }
    run[i].outposi++;

    if (run[i].outposi >= run[i].nitems) {
      /*  
       *  No more items left in the i'th run.
       */
      run[i].outposi = -1;
    } else {
      /*  
       *  Read next item from i'th run.
       */
      if (++(run[i].curr) == run[i].nelts && ! moreFromRun(runFP, &run[i])) {
        fprintf(stderr, "%s: cmerge: error reading more from run %d.\n", prog, i);
        return 0;
      }
    }
  }

  s = 0;
  if (fwrite(&s, sizeof s, 1, infixFP) == 0) {
    fprintf(stderr, "%s: cmerge: error writing final 0 to infix file: ", prog);
    perror("fwrite");
    return 0;
  }

  return 1;
}


/*  
 *  Compare two run elements based on the strings they point to.  Avoid
 *  calling the comparison function on very long equal strings by first
 *  checking for equality of the text offsets.
 */
static int cmpRunElt(int (*cmp)(const char *, const char *),
                      const char *txtBase,
                      RunElement *a, RunElement *b)
{
  return (a->s == b->s) ? 0 : cmp(txtBase + a->s, txtBase + b->s);
}


/*  
 *  Sort an array of run elements.  Based on the Quicksort algorithm on page
 *  389 of "Handbook of Algorithms and Data Structures" by G. H. Gonnet and
 *  R. Baeza-Yates.  (Also, see page 159 for a discussion of the algorithm.)
 *  
 *  bug: try Bentley and McIlroy's Quicksort...
 */
static void sort(RunElement *relt, size_t lo, size_t up, const char *txtBase,
                 int (*cmp)(const char *, const char *))
{
  RunElement pivot;
  size_t i, j;
  
  while (up > lo) {
    i = lo; j = up;
    pivot = relt[i];            /* bug: would relt[(i+j)/2] work? (Maybe not.) */

    /* Partition array around pivot. */

    while (i < j) {
      for (; cmpRunElt(cmp, txtBase, &relt[j], &pivot) > 0; --j) {
        continue;
      }
      for (relt[i] = relt[j];
           i < j && cmpRunElt(cmp, txtBase, &relt[i], &pivot) <= 0;
           i++) {
        continue;
      }
      relt[j] = relt[i];
    }
    relt[i] = pivot;

    /* Sort recursively, the smallest subarray first. */

    if (i - lo < up - i) {
      if (lo + 1 < i) sort(relt, lo, i - 1, txtBase, cmp);
      lo = i + 1;
    } else {
      if (up > i + 1) sort(relt, i + 1, up, txtBase, cmp);
      up = i - 1;
    }
  }
}


/*  
 *  Sort an array of run elements.
 */
static void sortRun(RunElement *run, size_t nelts, const char *txtBase,
                    int (*cmp)(const char *, const char *))
{
  sort(run, 0, nelts - 1, txtBase, cmp);
}


/*  
 *  For each sistring in the current run increment its count by the number of
 *  sistrings in the previous run that fall between it and its predecessor.
 *  
 *  Also, do stuff with the heights.
 */
static int generateRun(struct patrie_config *cf,
                       RunElement *r1, char *t1, int n1, RunElement *r2, char *t2, int n2,
                       int (*cmp)(const char *, const char *))
{
  int i = 0, j = 0;
  long diff;

  /*  
   *  Increment the count of the current run's initial sistring by the number
   *  of sistrings, in the previous run, strictly less than it.
   */

  while (i < n1 && cmp(t1 + r1[i].s, t2 + r2[0].s) < 0) {
    r2[0].c++;
    diff = cf->diffbit(t1 + r1[i].s, t2 + r2[0].s);
    r2[0].lh = MAX(r2[0].lh, diff);
    i++;
  }

  /*  
   *  Now, `i' is either equal to `n1' or it corresponds to the first
   *  sistring, in the previous run, that is greater than, or equal to, the
   *  initial sistring of the current run.
   */

  /*  
   *  bug: we may be able to speed up this loop, since successive trips
   *  through the else branch overwrite existing values.  (Although, the
   *  MAX() may prevent this.)
   */
  
  while (i < n1 && j < n2) {
    if (cmp(t1 + r1[i].s, t2 + r2[j].s) >= 0) {
      j++;
    } else {
      r2[j].c++;
      diff = cf->diffbit(t1 + r1[i].s, t2 + r2[j].s);
      r2[j].lh = MAX(r2[j].lh, diff);

      diff = cf->diffbit(t2 + r2[j-1].s, t1 + r1[i].s);
      r2[j-1].rh = MAX(r2[j-1].rh, diff);
      i++;
    }
  }

  /*  
   *  Now, either i == n1 or j == n2, or both.
   */

  /*  
   *  bug: check this against Shang's thesis.  If correct it can definitely
   *  be optimized, as `j < n2' is guaranteed to be true.  Also, since `j'
   *  never changes we need only find the greatest value of `i' satisfying
   *  the comparison function before doing a height calculation.  (Again, the
   *  MAX() may prevent this.)
   */
  
  if (j == n2) j = n2 - 1;

  for (; i < n1 && j < n2; i++) {
    if (cmp(t1 + r1[i].s, t2 + r2[j].s) > 0) {
      diff = cf->diffbit(t2 + r2[j].s, t1 + r1[i].s);
      r2[j].rh = MAX(r2[j].rh, diff);
    }
  }

  return 1;
}


/*  
 *  cf->sortMaxMem is an upper limit on the amount of memory we can use for
 *  buffers, etc.
 *  
 *  Ensure `*nruns' is always up to date with the current number of runs; in
 *  case of an error we want the caller to know how many file pointers to
 *  close, and how many files to unlink.
 */
static int csort(struct patrie_config *cf,
                 size_t memsz,
                 void *mem,
                 FILE *runFP,
                 size_t *tbs,
                 int *nruns,
                 Run **r)
{
  Run *run = 0;
  RunElement *currRun, *prevRun;
  char *currText, *prevText;
  int i, j, nbytes;
  int runNum = 0;
  long old;
  size_t textsz;
  
  *r = 0;                       /* to tell whether anything was malloc()ed */

  /*  
   *  Set up various buffers to fit within our memory constraints.
   *
   *  The formula for `textsz' is a consequence of the relation:
   *  
   *  memsz >= 2 * (textsz + cf->sortPadLen + 1)
   *           + 2 * textsz * sizeof(RunElement)
   */

  textsz = (memsz - 2 * (cf->sortPadLen + 1)) / (2 * (1 + sizeof(RunElement)));

  currRun = mem;
  prevRun = currRun + textsz;
  currText = (char *)(prevRun + textsz);
  prevText = currText + textsz + cf->sortPadLen + 1;

  while ((nbytes = fread(currText, 1, textsz + cf->sortPadLen, cf->textFP)) != 0) {
    currText[nbytes] = '\0';

    old = ftell(cf->textFP);

    /*  
     *  Append a new run structure to the array.
     */
    
    {
      void *new;
      
      if ( ! _patrieReAlloc(run, (runNum + 1) * sizeof *run, &new)) {
        fprintf(stderr, "%s: csort: can't allocate %lu bytes for run structures: ",
                prog, (unsigned long)(runNum + 1) * sizeof *run); perror("malloc");
        if (run) free(run);
        return 0;
      }
      run = new;
    }

    /*  
     *  Set the index points for the current run.
     */
    
    run[runNum].elt = currRun;
    run[runNum].nitems = 0;

    cf->setIndexPoints(run + runNum, markPos, MIN(nbytes, textsz), currText, cf->indexArg);

    for (i = 0; i < run[runNum].nitems; i++) {
      currRun[i].c = 0;
      currRun[i].lh = currRun[i].rh = 0;
    }

    sortRun(currRun, run[runNum].nitems, currText, cf->strcomp);

    for (i = 0; i < run[runNum].nitems - 1; i++) {
      currRun[i].c = 0;
      currRun[i].rh = cf->diffbit(currText + currRun[i].s, currText + currRun[i+1].s);
      currRun[i+1].lh = currRun[i].rh;
    }

    for (j = 0; j < runNum; j++) {
      int nb;

      if (fseek(runFP, run[j].offset, SEEK_SET) == -1) {
        fprintf(stderr, "%s: csort: can't seek to %d'th run (offset %lu) in runs file.\n",
                prog, j, (unsigned long)(run[j].nitems * sizeof(RunElement)));
        perror("fseek"); free(run);
        return 0;
      }

      if (fread(prevRun, sizeof *prevRun, run[j].nitems, runFP) < 1) {
        fprintf(stderr, "%s: csort: error reading from temporary runs file: ", prog);
        perror("fread"); free(run);
        return 0;
      }

      if (fseek(cf->textFP, j * textsz, 0) < 0) {
        fprintf(stderr, "%s: csort: can't seek to offset %ld in text file: ",
                prog, (long)(j * textsz)); perror("fseek"); free(run);
        return 0;
      }

      if ((nb = fread(prevText, 1, textsz + cf->sortPadLen, cf->textFP)) < 1) {
        fprintf(stderr, "%s: csort: error reading %ld bytes from text file: ",
                prog, (long)(textsz + cf->sortPadLen)); perror("fread"); free(run);
        return 0;
      }
      prevText[nb] = '\0';

      generateRun(cf, prevRun, prevText, run[j].nitems,
                  currRun, currText, run[runNum].nitems, cf->strcomp);
    }

    /*
     *  Append the current run to the runs file.
     */

    (void)fseek(runFP, 0, SEEK_END);

    run[runNum].offset = ftell(runFP);

    if (fwrite(currRun, sizeof *currRun, run[runNum].nitems, runFP) == 0 ||
        fflush(runFP) < 0) {
      fprintf(stderr, "%s: csort: can't write current run to temporary runs file: ", prog);
      perror("fwrite or fflush"); free(run);
      return 0;
    }

    if (fseek(cf->textFP, old, SEEK_SET) < 0) {
      fprintf(stderr, "%s: csort: can't seek to previous location, `%ld', in text file: ",
              prog, old); perror("fseek"); free(run);
      return 0;
    }

    /*  
     *  Seek backwards by the number of bytes in the text buffer greater than
     *  `textsz'.
     */

    if (nbytes > textsz) {
      fseek(cf->textFP, -(long)(nbytes - textsz), SEEK_CUR);
    }

    (runNum)++;
  }

  *tbs = textsz;
  *nruns = runNum;
  *r = run;

  return 1;
}


/*  
 *
 *
 *                            External routines.
 *
 *
 */  

int _patrieInfixBuild(struct patrie_config *cf)
{
  FILE *runFP;
  Run *run;
  int nruns;
  size_t tbs;                   /* text buffer size */
  size_t fsz;                   /* file size */
  struct infix_stats stats;
  void *mem;

  if (cf->printStats) {
    _patrieTimingStart(&stats.infixTime);
  }

  _patrieInitDiffTab();
  fsz = _patrieFpSize(cf->textFP);

  if ( ! (mem = malloc(cf->sortMaxMem))) {
    fprintf(stderr, "%s: _patrieInfixBuild: can't allocate %lu bytes for buffers: ",
            prog, (unsigned long)cf->sortMaxMem); perror("malloc");
    return 0;
  }

  if ( ! (runFP = tempFile(cf->sortTempDir))) {
    fprintf(stderr, "%s: _patrieInfixBuild: can't open temporary runs file.\n", prog);
    free(mem);
    return 0;
  }

  if (cf->printStats) {
    _patrieTimingStart(&stats.runsTime);
  }

  if ( ! csort(cf, cf->sortMaxMem, mem, runFP, &tbs, &nruns, &run)) {
    free(mem); if (run) free(run);
    return 0;
  }
  
  if (cf->printStats) {
    _patrieTimingEnd(&stats.runsTime);
    _patrieTimingStart(&stats.mergeTime);
  }

  if ( ! cmerge(cf->sortMaxMem, mem, tbs, nruns, run, runFP, cf->infixTrieFP)) {
    free(mem); if (run) free(run);
    return 0;
  }

  if (cf->printStats) {
    _patrieTimingEnd(&stats.mergeTime);
    _patrieTimingEnd(&stats.infixTime);

    stats.textSize = fsz;
    stats.tempSize = _patrieFpSize(runFP);
    stats.infixSize = _patrieFpSize(cf->infixTrieFP);
    stats.memSize = cf->sortMaxMem;
    stats.txtBufSize = tbs;

    printStats(stats);
  }

  fclose(runFP);
  free(mem); free(run);

  return 1;
}


/*  
 *  The default function to set index points.  Every character is an index
 *  point.
 */
void _patrieSetIndexPoints(void *run,
                           void (*mark)(void *run, size_t indexPoint),
                           size_t tlen, const char *text, void *arg)
{
  int i;
  
  for (i = 0; i < tlen; i++) {
    mark(run, i);
  }
}
