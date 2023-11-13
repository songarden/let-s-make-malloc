/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12) // Extend heap by this amount (bytes)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (DSIZE-1)) & ~0x7)



#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* ################################################ */

#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *coalesce_delay_v();
static void *first_fit(size_t asize);
static void *next_fit(size_t asize);
static void *best_fit(size_t asize);
static void *custom_best_fit(size_t asize);
static void *custom_best_fit_2(size_t asize);
static void place(void *bp, size_t asize);

static void *heap_listp;
static void *next_bp;

typedef enum  {
    ZERO_BLK = 0,
    FREE_BLK = 0,
    ALLOC_BLK = 1
} block_status_t;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1){
        return -1;
    }
    PUT(heap_listp,0);
    PUT(heap_listp + (1*WSIZE) , PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE) , PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE) , PACK(0, 1));
    heap_listp += (2*WSIZE);
    next_bp = heap_listp;

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL){
        return -1;
    }
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
	// return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
    /* ########################################## */

    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0){
        return NULL;
    }

    if (size <= DSIZE){
        asize = 2*DSIZE;
    }
    else{
        asize = DSIZE * ((size+(DSIZE) + (DSIZE-1)) / DSIZE);
    }

    if ((bp = custom_best_fit_2(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    // coalesce_delay_v();
    // //delay merging block
    // if ((bp = first_fit(asize)) != NULL){
    //     place(bp,asize);
    //     return bp;
    // }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp) , PACK(size,0));
    PUT(FTRP(bp) , PACK(size,0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */

void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t this_size = GET_SIZE(HDRP(ptr));
    size_t copySize,asize,bsize,csize;

    size_t original_size = GET_SIZE(HDRP(ptr));
    size += 2 * WSIZE;
    if (original_size >= size) {
        return ptr;
    }

    // if (this_size - DSIZE >= size){
    //     if (size <= DSIZE){
    //         asize = 2*DSIZE;
    //     }
    //     else{
    //         asize = DSIZE * ((size+(DSIZE) + (DSIZE-1)) / DSIZE);
    //     }
    //     if((this_size - asize) >= (2*DSIZE)) {
    //         PUT(HDRP(ptr) , PACK(asize , 1));
    //         PUT(FTRP(ptr) , PACK(asize , 1));
    //         PUT(HDRP(NEXT_BLKP(ptr)) , PACK(this_size - asize , 0));
    //         PUT(FTRP(NEXT_BLKP(ptr)) , PACK(this_size - asize , 0));
    //     }
    //     next_bp = ptr;
    //     return ptr;
    // }

    if (original_size < size) {
        size_t total_size = original_size + GET_SIZE(HDRP(NEXT_BLKP(ptr)));

        if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && total_size >= size) {
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 0));
            PUT(FTRP(ptr), PACK(0, 0));
            PUT(HDRP(ptr), PACK(total_size, 1));
            PUT(FTRP(ptr), PACK(total_size, 1));
            return ptr;
        }
    }


    // asize = DSIZE * ((size+(DSIZE) + (DSIZE-1)) / DSIZE);
    // csize = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + this_size;

    // if(csize >= asize && !GET_ALLOC(HDRP(NEXT_BLKP(ptr)))){
    //     bsize = csize - asize;
    //     PUT(FTRP(ptr) , PACK(0 , 0));
    //     PUT(FTRP(ptr)+WSIZE , PACK(0 , 0));
    //     if(bsize >= DSIZE){
    //         PUT(HDRP(ptr) , PACK(asize , 1));
    //         PUT(FTRP(ptr) , PACK(asize , 1));
    //         PUT(HDRP(NEXT_BLKP(ptr)) , PACK(bsize , 0));
    //         PUT(FTRP(NEXT_BLKP(ptr)) , PACK(bsize , 0));
    //     }
    //     else{
    //         PUT(HDRP(ptr) , PACK(csize , 1));
    //         PUT(FTRP(ptr) , PACK(csize , 1));
    //     }
    //     next_bp = ptr;
    //     return ptr;

    // }

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}


/* ##################  CUSTOMIZING  ################ */

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1){
        return NULL;
    }

    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    PUT(HDRP(NEXT_BLKP(bp)) , PACK(0,1));
    next_bp = bp;

    return coalesce(bp);

}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        return bp;
    }

    else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        next_bp = bp;
    }

    else if(!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp) , PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)) , PACK(size, 0));
        next_bp = PREV_BLKP(bp);
        bp = PREV_BLKP(bp);
        
    }

    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)) , PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)) , PACK(size,0));
        next_bp = PREV_BLKP(bp);
        bp = PREV_BLKP(bp);
    }

    return bp;
}

static void *coalesce_delay_v(void){
    void *bp;
    for(bp = heap_listp; GET_SIZE(HDRP(NEXT_BLKP(bp)))>0; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp))){
            while(!GET_ALLOC(HDRP(NEXT_BLKP(bp)))){
                size_t size = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
                PUT(HDRP(bp), PACK(size, 0));
                PUT(FTRP(bp), PACK(size, 0));
            }
            next_bp = bp;
        }
    }
}

static void *first_fit(size_t asize){

    void *bp;

    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }

    return NULL;
}

static void *next_fit(size_t asize){

    void *bp;

    for(bp = next_bp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            next_bp = bp;
            return bp;
        }
    }

    for(bp = heap_listp; bp < next_bp; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            next_bp = bp;
            return bp;
        }
    }
    next_bp = heap_listp;
    return NULL;
}

static void *best_fit(size_t asize){

    void *bp;
    void *best_bp = NULL;
    size_t besize = 0;
    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            if (best_bp == NULL){
                besize = (GET_SIZE(HDRP(bp)) - asize);
                best_bp = bp;
            }
            if (besize > (GET_SIZE(HDRP(bp)) - asize)){
                besize = (GET_SIZE(HDRP(bp)) - asize);
                best_bp = bp;
            }
        }
    }
    return best_bp;
}


/* custom best fit -v1 : bestfit and next_buffer check */
static void *custom_best_fit(size_t asize){

    void *bp;
    void *best_bp = NULL;
    size_t besize = 0;

    for(bp = heap_listp; bp < next_bp; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            if (best_bp == NULL){
                besize = (GET_SIZE(HDRP(bp)) - asize);
                best_bp = bp;
            }
            if (besize > (GET_SIZE(HDRP(bp)) - asize)){
                besize = (GET_SIZE(HDRP(bp)) - asize);
                best_bp = bp;
            }
        }
    }
    if (best_bp != NULL){
        next_bp = best_bp;
        return best_bp;
    }

    for(bp = next_bp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            next_bp = bp;
            return bp;
        }
    }

    return NULL;

    
}
/* custom best fit -v2 : next_buffer check and bestfit*/
static void *custom_best_fit_2(size_t asize){

    void *bp;
    void *best_bp = NULL;
    size_t besize = 0;

    for(bp = next_bp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            next_bp = bp;
            return bp;
        }
    }

    for(bp = heap_listp; bp < next_bp; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            if (best_bp == NULL){
                besize = (GET_SIZE(HDRP(bp)) - asize);
                best_bp = bp;
            }
            if (besize > (GET_SIZE(HDRP(bp)) - asize)){
                besize = (GET_SIZE(HDRP(bp)) - asize);
                best_bp = bp;
            }
        }
    }

    return best_bp;
}

static void place(void *bp , size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));

    if((csize - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp) , PACK(asize , 1));
        PUT(FTRP(bp) , PACK(asize , 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp) , PACK(csize - asize , 0));
        PUT(FTRP(bp) , PACK(csize-asize , 0));
    }
    else{
        PUT(HDRP(bp) , PACK(csize, 1));
        PUT(FTRP(bp) , PACK(csize, 1));
    }
}