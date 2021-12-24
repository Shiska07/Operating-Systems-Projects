#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)             (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)         ((b) + 1)
#define BLOCK_HEADER(ptr)     ((struct _block *)(ptr) - 1)


static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;


void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev;  /* Pointer to the previous _block of allcated memory   */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};

struct _block *heapList = NULL;        /* Free list to track the _blocks available */
static struct _block *nflast = NULL;   /* track last block for next fit */
struct _block *best = NULL;            /* track best block for best fit */
struct _block *worst = NULL;           /* track wirst block for worst fit */
static int size_diff = -1;  
static int lowest_size_diff = -1;
static int highest_size_diff = -1;

struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;

#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   /* Best fit */
   
   while (curr) // keep traversing till the end 
   {
      if (curr->free && curr->size >= size)
      {
         // calculate size difference between curr and reuqested
         size_diff = curr->size - size;

         // if size difference is lowest till now save block address
         if ( (lowest_size_diff != -1) && (size_diff < lowest_size_diff) )
         {
            lowest_size_diff = size_diff;
            best = curr;
         }
         else if (lowest_size_diff == -1)
         {
            lowest_size_diff = size_diff;
            best = curr;
         }
      }
      *last = curr;
      curr  = curr->next;
   }

   // save best block address into curr
   curr = best;

   // reset variables
   size_diff = -1;
   lowest_size_diff = -1;
   best = NULL;

#endif

#if defined WORST && WORST == 0
   /* Worst fit */

   while (curr) // keep traversing till the end 
   {
      if (curr->free && curr->size >= size)
      {
         // calculate size difference between curr and reuqested
         size_diff = curr->size - size;

         // if size difference is highest till now save block address
         if (highest_size_diff != -1 && size_diff > highest_size_diff)
         {
            highest_size_diff = size_diff;
            worst = curr;
         }
         else if (highest_size_diff == -1)
         {
            highest_size_diff = size_diff;
            worst = curr;
         }
      }
      *last = curr;
      curr  = curr->next;
   }

   // save worst block address to curr
   curr = worst;

   // reset variables
   size_diff = -1;
   highest_size_diff = -1;
   worst = NULL;
   
#endif

#if defined NEXT && NEXT == 0
   /* Next fit */

   // if list empty
   if (curr == NULL)
   {
      curr = *last;
   }
   // if not empty start from last block address saved
   else
   {
      curr = nflast;
   }

   // keep traversing and find next first fit
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }

   // save address of last block
   nflast = *last;

#endif
   return curr;
}


struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      // address of the very first block
      heapList = curr;
   }

   /* Attach new _block to prev _block */
   if (last) 
   {
      last->next = curr;
      curr->prev = last;
   }

   /* Update _block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   num_blocks += 1;

   /* keep track of no. of grows and total size */
   num_grows += 1;
   max_heap = max_heap + size + sizeof(struct _block);

   return curr;
}


void *malloc(size_t size) 
{

   /* keep track of total memory requested */
   num_requested = num_requested + size;

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free _block */
   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
   }

   // split the block if a free block is found
   else
   {
                               
      if (next->size > (size + sizeof(struct _block) + 1))
      {
         struct _block *temp = next->next;                         /* store address of next block */
         // splitting 
         struct _block *new = BLOCK_DATA(next);                    /* initialize pointer to store address */
         char *p = (char*)new;
         p = p + size;
         new = (struct _block *)p;
         next->next = new;                                         /* current block will point to a new address */
         new->prev = next;
         new->next = temp;                                         /* new block points to next block */

         // if next block is not null
         if (temp)
         {
            new->next->prev = new;
         }

         new->size = next->size - sizeof(struct _block)- size;
         new->free = true;
         next->size = size;

         // Increment no_of_splits and blocks
         num_splits += 1;
         num_blocks += 1;
         num_reuses += 1;
      }
      
   }
   

   // Could not find free _block or grow heap, so just return NULL
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark _block as in use */
   next->free = false;
   num_mallocs += 1;

   /* Return data address associated with _block */
   return BLOCK_DATA(next);
}

void *calloc(size_t nmemb, size_t size)
{
   
   size_t total_size = nmemb*size;                 /* total bytes to be allocated */
   struct _block *ptr = malloc(total_size);   /* malloc allocates and returns start address of the block */
   memset(ptr, 0, total_size);                /* set all bytes to 0 */

   return ptr;
}


void free(void *ptr) 
{
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;
   num_frees += 1;

   
   // keep freeing as long as block before the current block is free 
   while ( (curr->prev != NULL) && (curr->prev->free == true) )
   {
      curr->prev->next = curr->next;
      curr->prev->size = curr->prev->size + sizeof(struct _block) + curr->size;
      curr = curr->prev;
      num_coalesces += 1;
      num_blocks -= 1;
   }

   // keep freeing as long as block after the current block is free
   while ( (curr->next != NULL) && (curr->next->free == true) )
   {
      curr->size = curr->size + sizeof(struct _block) + curr->next->size;
      curr->next = curr->next->next;
      num_coalesces += 1;
      num_blocks -= 1;
   }

}

void *realloc(void *ptr, size_t size)
{

   struct _block *reall_ptr = NULL;

   // if ptr is NULL behave like malloc
   if (ptr == NULL)
   {
      reall_ptr = malloc(size);
      return reall_ptr;
   }
   // if size is 0 behave like free
   else if (ptr && size == 0)
   {
      free(ptr);
      return NULL;
   }
   else
   {

      reall_ptr = malloc(size);                          /* allocate required memory */
      memcpy(reall_ptr, ptr, BLOCK_HEADER(ptr)->size);   /* copy data */
      free(ptr);                                         /* free old block */

      return reall_ptr;                                  /* return new block address */
   }
}
