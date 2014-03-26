/*****************************************************************************
 * Zoltan Library for Parallel Applications                                  *
 * Copyright (c) 2000,2001,2002, Sandia National Laboratories.               *
 * For more info, see the README file in the top-level Zoltan directory.     *  
 *****************************************************************************/
/*****************************************************************************
 * CVS File Information :
 *    $RCSfile$
 *    $Author$
 *    $Date$
 *    Revision: 1.24.4.1 $
 ****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zoltan_mem.h"
#include "zoltan_util.h"


#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif


/* Macro to get rank information for printing and error messages. */
/* If not using MPI, compile with -DZOLTAN_NO_MPI; all messages   */
/* will print with proc = 0.                                      */
#ifdef ZOLTAN_NO_MPI
#define GET_RANK(a) *(a)=0
#else
#include <mpi.h>
#define GET_RANK(a) MPI_Comm_rank(MPI_COMM_WORLD, (a))
#endif

#ifdef __cplusplus
/* if C++, define the rest of this header file as extern C */
extern "C" {
#endif

static int DEBUG_MEMORY = 0;	/* Flag for detecting memory leaks */
static size_t bytes_used = 0;	/* Sum of active allocations */
static size_t bytes_max = 0;	/* Largest total of active allocations */

static int nmalloc = 0;         /* number of calls to malloc */
static int nfree = 0;           /* number of calls to free */

#define MAX_STRING_LEN 50
static struct malloc_debug_data {
  int       order;			/* which malloc call is it? */
  size_t    size;			/* size of malloc invocation */
  double   *ptr;			/* memory location returned */
  char  file[MAX_STRING_LEN+1];   	/* file name */
  int       line;                       /* line number */
  struct malloc_debug_data *next;	/* pointer to next element */
} *top = NULL;


/******************************************************************************/
void Zoltan_Memory_Debug(int new_level) {
/*
 *  Routine to allow user to set DEBUG_MEMORY level.
 */

  DEBUG_MEMORY = new_level;
}

/******************************************************************************
 *
 *                    Dynamic Allocation of Multidimensional Arrays
 *-----------------------------------------------------------------------------
 *
 * Example Usage:
 *
 *     typedef  struct
 *       {      int     bus1;
 *              int     bus2;
 *              int     dest;
 *      }       POINT;
 *
 *      POINT    **points, corner;
 *
 *      points = (POINT **)Zoltan_Array_Alloc(file,lineno,2,x,y,sizeof(POINT));
 *                                             ^     ^    ^ ^ ^
 *                                             |     |    | | |
 *                 name of calling file--------*     |    | | |
 *                                                   |    | | |
 *                  line number of call--------------*    | | |
 *                                                        | | |
 *                 number of dimensions-------------------+ | |
 *                                                          | |
 *                  first dimension max---------------------+ |
 *                                                            |
 *                 second dimension max-----------------------+
 *
 *         (points may be now be used as if it were declared
 *          POINT points[x][y])
 *
 *          This particular version is limited to dimensions of 4 or less.
 *
 *      corner = points[2][3]; (refer to the structure as you would any array)
 *
 *      free (points); (frees the entire structure in one fell swoop)
 *
 *****************************************************************************/
/******************************************************************************
*       The following section is a commented section containing
*       an example main code:
*******************************************************************************
*double *Zoltan_Array_Alloc();
*main()
*{
*  int ***temp;
*   int *temp2;
*   int i, j, k;
*   int il, jl, kl;
*
*   malloc_debug(2);
*   il = 2;
*   jl = 3;
*   kl = 3;
*   temp = (int ***) Array_Alloc(__FILE__,__LINE__,3,il,jl,kl,sizeof(int));
*   for (i=0; i<il; i++) {
*      for (j=0; j<jl; j++) {
*         for (k=0; k<kl; k++) temp[i][j][k] = 1;
*      }
*   }
*
*   temp2 = (int *) malloc(10*sizeof(int));
*   for (i=0; i<10; i++) temp2[i] = 0;
*
*   for (i=0; i<il; i++) {
*      for (j=0; j<jl; j++) {
*         for (k=0; k<kl; k++) (void) printf(" %d\n", temp[i][j][k]);
*      }
*   }
*   malloc_verify();
*}
******************************************************************************/

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#ifdef __STDC__

double *Zoltan_Array_Alloc(char *file, int lineno, int numdim, ...)

#else

double *Zoltan_Array_Alloc(va_alist)
va_dcl

#endif

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

{
  char *yo = "Zoltan_Array_Alloc";
  int i, j;
  struct dimension {
    long index;  /* Number of elements in the dimension  */
    long total;  /* Total number of elements             */
    long size;   /* Size of a single element in bytes    */
    long off;    /* offset from beginning of array       */
  } dim[4];      /* Info about each dimension            */

#ifndef __STDC__
  char *file;           /* Filename of source code from call.   */
  int lineno;           /* Line number of call.                 */
  int numdim;           /* Number of dimensions                 */
#endif

  long    total;        /* Total size of the array              */
  double *dfield;       /* ptr to avoid lint complaints         */
  char   *field;        /* The multi-dimensional array          */
  char  **ptr;          /* Pointer offset                       */
  char   *data;         /* Data offset                          */
  va_list va;           /* Current pointer in the argument list */

#ifdef __STDC__
  va_start(va, numdim);
#else
  va_start(va);
  file = va_arg(va, char *);
  lineno = va_arg(va, int);
  numdim = va_arg(va, int);
#endif

  if (numdim <= 0) {
    fprintf(stderr, "%s (%s: %d) ERROR: number of dimensions, %d, is <=0\n",
            yo, file, lineno, numdim);
    return((double *) NULL);
  }
  else if (numdim > 4) {
    fprintf(stderr, "%s (%s: %d) ERROR: number of dimensions, %d, is > 4\n",
            yo, file, lineno, numdim);
    return((double *) NULL);
  }

  dim[0].index = va_arg(va, int);

  if (dim[0].index <= 0) {
    if (DEBUG_MEMORY > 0) {
      fprintf(stderr, "WARNING, %s (%s: %d) called with first "
            "dimension <= 0, %ld; will return NULL\n",
            yo, file, lineno, dim[0].index);
    }
    return((double *) NULL);
  }

  dim[0].total = dim[0].index;
  dim[0].size  = sizeof(void *);
  dim[0].off   = 0;
  for (i = 1; i < numdim; i++) {
    dim[i].index = va_arg(va, int);
    if (dim[i].index <= 0) {
      fprintf(stderr, "WARNING: %s (%s: %d) called with dimension %d <= 0, "
              "%ld; will return NULL\n",
              yo, file, lineno, i+1, dim[i].index);
      return((double *) NULL);
    }
    dim[i].total = dim[i-1].total * dim[i].index;
    dim[i].size  = sizeof(void *);
    dim[i].off   = dim[i-1].off + dim[i-1].total * dim[i-1].size;
  }

  dim[numdim-1].size = va_arg(va, int);
  va_end(va);

  /* Round up the last offset value so data is properly aligned. */

  dim[numdim-1].off = dim[numdim-1].size *
    ((dim[numdim-1].off+dim[numdim-1].size-1)/dim[numdim-1].size);

  total = dim[numdim-1].off + dim[numdim-1].total * dim[numdim-1].size;

  dfield = (double *) Zoltan_Malloc(total, file, lineno);

  if (dfield != NULL) {
    field  = (char *) dfield;
    for (i = 0; i < numdim - 1; i++) {
      ptr  = (char **) (field + dim[i].off);
      data = (char *) (field + dim[i+1].off);
      for (j = 0; j < dim[i].total; j++) {
        ptr[j] = data + j * dim[i+1].size * dim[i+1].index;
      }
    }
  }

  return dfield;

} /* Zoltan_Array_Alloc */

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/* Safe version of calloc.  */

double *Zoltan_Calloc (size_t num, size_t size, char *filename, int lineno)
{
double *p ;
  p = Zoltan_Malloc (num*size, filename, lineno) ;
  if (p) memset ((void *) p, '\0', num*size) ;
  return p ;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/* Safe version of malloc.  Does not initialize memory .*/

double *Zoltan_Malloc(size_t n, char *filename, int lineno)
{
  char *yo = "Zoltan_Malloc";
  struct malloc_debug_data *new_ptr;     /* data structure for malloc data */
  int       proc;             /* processor ID for debugging msg */
  double *pntr;           /* return value */
  char *basefile;

  if (n > 0) {
    pntr = (double *) malloc(n);
    if (pntr == NULL) {
      GET_RANK(&proc);
      fprintf(stderr, "%s (from %s,%d) No space on proc %d - number of bytes "
              "requested = %lu\n", yo, filename, lineno, proc,
              (unsigned long) n);
      return ((double *) NULL);
    }
    nmalloc++;
  }
  else if (n == 0)
    pntr = NULL;
  else {		/* n < 0 */
    GET_RANK(&proc);
    fprintf(stderr, "%s (from %s,%d) ERROR on proc %d: "
	    "Negative malloc argument. (%lu)\n", yo, filename, lineno, proc, 
            (unsigned long) n);
    return ((double *) NULL);
  }

  if (DEBUG_MEMORY > 1 && pntr != NULL) {
    /* Add allocation to linked list of active allocations. */
    new_ptr = (struct malloc_debug_data *)
      malloc(sizeof(struct malloc_debug_data));

    if (new_ptr == NULL) {
      GET_RANK(&proc);
      fprintf(stderr, "WARNING: No space on proc %d for malloc_debug %lu.\n",
	proc, (unsigned long) n);
      return (pntr);
    }

    new_ptr->order = nmalloc;
    new_ptr->size = n;
    new_ptr->ptr = pntr;
#ifdef SHORT_FILE
    basefile = strrchr(filename, '/');
    basefile = ((basefile != NULL)?basefile:filename-1)+1;
#else
    basefile = filename;
#endif /* SHORT_FILE */
    strncpy(new_ptr->file, basefile, MAX_STRING_LEN);
    new_ptr->line = lineno;
    new_ptr->next = top;
    top = new_ptr;
    bytes_used += n;
    if (bytes_used > bytes_max) {
      bytes_max = bytes_used;
    }
  }

  if (DEBUG_MEMORY > 2) {
    /* Print out details of allocation. */
    GET_RANK(&proc);
<<<<<<< mem.c
    fprintf(stderr, "Proc %d: order=%d, size=%lu, location=0x%lx, "
=======
    fprintf(stderr, "Proc %d: order=%d, size=%d, location=%p, "
>>>>>>> 1.8
      "file=%s, line=%d\n",
<<<<<<< mem.c
      proc, nmalloc, (unsigned long) n, (long) pntr, filename, lineno);
=======
      proc, nmalloc, n, pntr, filename, lineno);
>>>>>>> 1.8
  }

  return pntr;

} /* Zoltan_Malloc */

/* Safe version of realloc. Does not initialize memory. */

double *Zoltan_Realloc(void *ptr, size_t n, char *filename, int lineno)
{
  char *yo = "Zoltan_Realloc";
  struct malloc_debug_data *dbptr;   /* loops through debug list */
  int       proc;             /* processor ID */
  double   *p;                /* returned pointer */

  if (ptr == NULL) {	/* Previous allocation not important */
    if (n == 0) {
      p = NULL;
    }
    else {
      p = Zoltan_Malloc(n, filename, lineno);
    }
  }
  else {
    if (n == 0) {
      Zoltan_Free((void **) &ptr, filename, lineno);
      p = NULL;
    }
    else {
      p = (double *) realloc((char *) ptr, n);

      if (DEBUG_MEMORY > 1) {
        /* Need to replace item in allocation list */
        for (dbptr = top; dbptr != NULL && (void *) (dbptr->ptr) != ptr;
	   dbptr = dbptr->next);
	if (dbptr == NULL) {	/* previous allocation not found in list. */
           GET_RANK(&proc);
	   fprintf(stderr, "Proc %d: Memory error: "
	     "In realloc, address not found in debug list (%p)\n",
	     proc, ptr);
	}
	else {	/* Update entry in allocation list */
	  bytes_used += (n - dbptr->size);
	  dbptr->size = n;
	  dbptr->ptr = p;
	  if (bytes_used > bytes_max) {
	    bytes_max = bytes_used;
	  }
	}
      }

      if (p == NULL) {
        GET_RANK(&proc);
        fprintf(stderr, "%s (from %s,%d) No space on proc %d - "
		"number of bytes requested = %lu\n",
		yo, filename, lineno, proc, (unsigned long) n);
      }
    }
  }

  return (p);
} /* Zoltan_Realloc */


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

void Zoltan_Free (void **ptr, char *filename, int lineno)
{
  struct malloc_debug_data *dbptr;   /* loops through debug list */
  struct malloc_debug_data **prev;   /* holds previous pointer */
  int       proc;             /* processor ID */

/*
 *  This version of free calls the system's free function.  It doesn't call
 *  free if ptr is the NULL pointer.
 */
 
  if (ptr == NULL || *ptr == NULL) 
    return;

  nfree++;

  if (DEBUG_MEMORY > 1) {
    /* Remove allocation of list of active allocations */
    prev = &top;
    for (dbptr = top; dbptr != NULL && (void *) (dbptr->ptr) != *ptr;
      dbptr = dbptr->next) {
      prev = &(dbptr->next);
    }
    if (dbptr == NULL) {
      GET_RANK(&proc);
      fprintf(stderr, "Proc %d: Memory error: In free, address (%p) "
	"not found in debug list. File=%s, line=%d.\n", proc, 
        *ptr, filename, lineno);
   }
   else {
       *prev = dbptr->next;
       bytes_used -= dbptr->size;
       free((char *) dbptr);

      if (DEBUG_MEMORY > 2){
        GET_RANK(&proc);
        fprintf(stderr, "Proc %d: free, address (0x%lx) "
  	"File=%s, line=%d.\n", proc, (long) *ptr, filename, lineno);
       }
     }
   }
 
  free(*ptr);
 
  /* Set value of ptr to NULL, to flag further references to it. */
  *ptr = NULL;

}  /* Zoltan_Free */


/* Free n pointers. Variable number of arguments is allowed.  */

#ifdef __STDC__

void Zoltan_Multifree(char *filename, int lineno, int n, ...)
{
  int i;
  va_list va;
  
  va_start(va, n);
  for (i=0; i<n; i++){
    Zoltan_Free(va_arg(va, void **), filename, lineno);
  }
  va_end(va);
}

#else

void Zoltan_Multifree(va_alist)
va_dcl
{
  int i, n, lineno;
  char *filename;
  va_list va;
   
  va_start(va);
  filename = va_arg(va, char *);
  lineno = va_arg(va, int);
  n = va_arg(va, int);
  for (i=0; i<n; i++){
    Zoltan_Free(va_arg(va, void **), filename, lineno);
  }
  va_end(va);
}

#endif

/* Print out status of malloc/free calls.  Flag any memory leaks. */

void Zoltan_Memory_Stats()
{
    struct malloc_debug_data *dbptr;	/* loops through debug list */
    int       proc;		/* processor ID */


    if (DEBUG_MEMORY == 1) {
        GET_RANK(&proc);
	fprintf(stderr, "Proc %d: Calls to malloc = %d,  Calls to free = %d\n",
                         proc, nmalloc, nfree);
        if (nmalloc > nfree)
          fprintf(stderr, "Proc %d: Possible memory error: "
                          "# malloc > # free.\n", proc);
        else if (nfree > nmalloc)
          fprintf(stderr, "Proc %d: Possible memory error: "
                          "# free > # malloc.\n", proc);
    }
    else if (DEBUG_MEMORY > 1) {
        GET_RANK(&proc);
	fprintf(stderr, "Proc %d: Calls to malloc = %d,  Calls to free = %d, "
                        "Max bytes = %lu, total bytes = %lu\n", 
                         proc, nmalloc, nfree,
                         (unsigned long) bytes_max, (unsigned long) bytes_used);
        if (nmalloc > nfree) 
          fprintf(stderr, "Proc %d: Possible memory error: "
                          "# malloc > # free.\n", proc);
        else if (nfree > nmalloc)
          fprintf(stderr, "Proc %d: Possible memory error: "
                          "# free > # malloc.\n", proc);
	if (top != NULL) {
	    fprintf(stderr, "Proc %d: Remaining allocations:\n", proc);
	    for (dbptr = top; dbptr != NULL; dbptr = dbptr->next) {
<<<<<<< mem.c
		fprintf(stderr, " order=%d, size=%lu, location=0x%lx, "
=======
		fprintf(stderr, " order=%d, size=%d, location=%p, "
>>>>>>> 1.8
                  "file=%s, line=%d\n", 
<<<<<<< mem.c
                  dbptr->order, (unsigned long) (dbptr->size),
                  (long) dbptr->ptr,
=======
                  dbptr->order, dbptr->size, dbptr->ptr,
>>>>>>> 1.8
                  dbptr->file, dbptr->line);
	    }
	}
    }
} /* Zoltan_Memory_Stats */


int       Zoltan_Malloc_Num(void)
{
/* Return number associated with the most recent malloc call. */
  return (nmalloc);
} /* Zoltan_Malloc_Num */


size_t Zoltan_Memory_Usage (int type)
{
/* Return memory usage information:  total bytes used currently or  *
 * maximum bytes used at any point.  Default is maximum bytes used. */
   if (type == ZOLTAN_MEM_STAT_TOTAL)
      return bytes_used ;

   return bytes_max ;
}

void Zoltan_Memory_Reset (int type)
{
/* Reset total bytes used currently or maximum bytes used at any point 
   to zero. */

   if (type == ZOLTAN_MEM_STAT_TOTAL)
      bytes_used = 0;
   else
      bytes_max = 0;
}


/*****************************************************************************/
/*                      END of mem.c                                         */
/*****************************************************************************/

#ifdef __cplusplus
} /* closing bracket for extern "C" */
#endif
