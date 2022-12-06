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

/* single word (4 bytes) or double word (8 bytes) alignment */

#define ALIGNMENT 8 // 

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


// basic constants and macros
#define WSIZE 4 // word, header, footer size (bytes)
#define DSIZE 8 // double word size (bytes)
#define CHUNKSIZE (1<<12) // extend heap by this amount (512 bytes)

#define MAX(x, y) ((x) > (y)? (x) : (y))

//pack a size and allocated bit into a word
#define PACK(size, alloc) ((size) | (alloc)) // 할당 사이즈와 할당 여부를 한 워드에 합친다.

//read and write a word at address p
#define GET(p) (*(unsigned int *)(p)) // p주소에 있는 값을 읽음
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // p 주소에 val을 할당함

//read the size and allocated fields from address p
#define GET_SIZE(p) (GET(p) & ~0x7) // p 주소의 사이즈를 읽음
#define GET_ALLOC(p) (GET(p) & 0x1) 

// given block ptr bp, compute address of its header and footer
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

//given block ptr bp, compute address of next and previous block
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *heap_listp;
char *find_last_search;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);





/* 
 * mm_init - initialize the malloc package.
 */
// create a heap with a initial free block2
int mm_init(void)
{
    //create the initial empty heap
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *) - 1)
        return -1;
    PUT(heap_listp, 0); // Alignment padding
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); // prologue header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); // prologue footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); //epilogue header
    heap_listp += (2*WSIZE);
    find_last_search = heap_listp;
    //extend the empty heap with a free block of CHUNKSIZE bytes
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

//extend the heap with a new free block
static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    // allocate an even number of words to maintain alignment
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    // initialize free block header/footer and the epilogue header
    PUT(HDRP(bp), PACK(size, 0)); // free block header
    PUT(FTRP(bp), PACK(size, 0)); // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); // new epilogue header

    //coalesce if the previous block was free
    return coalesce(bp);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
//allocate a block from the free list
void *mm_malloc(size_t size)
{
    size_t asize;  //adjusted block size
    size_t extendsize; // amount to extend heap if no fit
    char *bp;

    // ignore spurious requests
    if(size == 0) return NULL;

    // adjust block size to include overhead and aligment reqs.
    if(size <= DSIZE) asize = 2 * DSIZE;
    else{
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    // search the free list for a fit
    if((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    // no fit found. get more memory and place the block
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL) return NULL;
    place(bp, asize);
    return bp;
}

static void place(void *bp, size_t asize)
{   
    size_t old_size = GET_SIZE(HDRP(bp));
    size_t left_size = old_size - asize;
    if (left_size < 2*DSIZE)
    {
        PUT(HDRP(bp), PACK(old_size, 1));
        PUT(FTRP(bp), PACK(old_size, 1));
    }else{
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(left_size, 0));
        PUT(FTRP(bp), PACK(left_size, 0));
    }
}
static void *find_fit(size_t asize)
{
    void *bp;

    for (bp = find_last_search; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) 
    {
        if (asize <= GET_SIZE(HDRP(bp)) && !GET_ALLOC(HDRP(bp)))
        {
            find_last_search = bp;
            return bp;
        }
    }

    for (bp = heap_listp; bp < find_last_search; bp = NEXT_BLKP(bp)) 
    {
        if (asize <= GET_SIZE(HDRP(bp)) && !GET_ALLOC(HDRP(bp)))
        {
            find_last_search = bp;
            return bp;
        }
            
    }
    return NULL;
}
/*
 * mm_free - Freeing a block does nothing.
 */
// free a block and uses boundary-tag coalescing to merge it
// with any adjacent free block in constant time.
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // CASE 1
    if(prev_alloc && next_alloc) {
        find_last_search = bp;
        return bp;
    }

    // CASE 2
    if(prev_alloc && !next_alloc) { 
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    // CASE 3
    else if(!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {
    //CASE 4
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);   
    }
    find_last_search = bp;
    return bp;
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    // adjust block size to include overhead and aligment reqs.
    size_t new_size;
    if(size <= DSIZE)
        new_size = 2 * DSIZE;
    else{
        new_size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }
    // (size/WSIZE % 2) ? (size/WSIZE + 1) * WSIZE : size/WSIZE * WSIZE;
    //새로 할당할 사이즈가 본래 사이즈보다 작거나 같을 때
    //즉시 반환
    copySize = GET_SIZE(HDRP(oldptr));
    if (new_size <= copySize)
    {
        //find_last_search = oldptr;
        return oldptr;
    }
    
    // 사이즈가 본래 사이즈 보다 크고, 다음 블럭이 가용 블럭 일 경우
    size_t size_sum = GET_SIZE(HDRP(NEXT_BLKP(oldptr))) + GET_SIZE(HDRP(oldptr));
    if (!GET_ALLOC(HDRP(NEXT_BLKP(oldptr))) && size_sum >= new_size)
    {
        if (size_sum - new_size < 2*DSIZE)
        {
            PUT(HDRP(oldptr), PACK(size_sum, 1));
            PUT(FTRP(oldptr), PACK(size_sum, 1));
        }else{
            PUT(HDRP(oldptr), PACK(new_size, 1));
            PUT(FTRP(oldptr), PACK(new_size, 1));
            oldptr = NEXT_BLKP(oldptr);
            PUT(HDRP(oldptr), PACK(size_sum - new_size, 0));
            PUT(FTRP(oldptr), PACK(size_sum - new_size, 0));
        }
        //find_last_search = oldptr;  
        return ptr;
    }


    // 일반적인 raalloc 함수
    // malloc을 이용해서 새로운 사이즈를 할당하고 할당된 곳에
    // 데이터를 이전해서 보존한 이후, 본래 할당된 메모리를 가용 메모리로 전환
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}