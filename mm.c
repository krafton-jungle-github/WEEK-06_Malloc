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

/* 기본 size 상수 정의 */
#define WSIZE                4              /* Word and header/footer size (bytes) */
#define DSIZE                8              /* Double word size (bytes) */
#define CHUNKSIZE           (1 << 12)       /* Extend heap by this amount (bytes) */ // 512 bytes

#define MAX(x, y)           ((x) > (y) ? (x) : (y)) // x, y 중 큰 값을 출력

/* 매크로 정의 */
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)   ((size) | (alloc))                  // size와 status_bit를 통합해서 헤더|풋터에 저장하는 값을 리턴

/* Read and write a word at address p */
#define GET(p)              (*(unsigned int *)(p))              // 인자 p가 참조하는 word를 읽어서 리턴
#define PUT(p, val)         (*(unsigned int *)(p) = (val))      // 인자 p가 가리키는 word에 val를 저장

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)         (GET(p) & ~0x7)                     // 주소 p에 있는 헤더 또는 풋터의 size 리턴
#define GET_ALLOC(p)        (GET(p) & 0x1)                      // 주소 p에 있는 할당 비트를 리턴

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)            ((char *)(bp) - WSIZE)                           // 자신의 헤더를 가리키는 포인터를 리턴 // HRDP는 bp 포인터에서 -1 워드로 움직임
#define FTRP(bp)            ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)     // 자신의 풋터를 가리키는 포인터를 리턴 // FTRP는 bp 포인터에서 헤더의 사이즈를 가져와 다음 블록의 bp까지 움직인 다음 -DSIZE만큼가서 자신의 풋터로 갈 수 있음

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)       ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))   // 다음 블럭 bp포인터 리턴 // 현재 블록의 header에서 size을 가지고 온 뒤 현재 bp 포인터에서 size만큼을 더해 다음 블록으로 포인터가 넘어감
#define PREV_BLKP(bp)       ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))   // 이전 블럭 bp포인터 리턴 // 이전 블록의 footer에서 size을 가지고 온 뒤 현재 bp 포인터에서 size만큼을 빼서 이전 블록으로 포인터가 넘어감

static char *heap_listp; // 프롤로그 블록을 가리키는 정적 전역변수

/*  mm_init - initialize the malloc package. */
int mm_init(void)
{
    /* create the inital empty heap */
    
    // 초기 heap_listp 값은 0(mem_sbrk 함수의 old_brk값 return)
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    
    // 빈 가용 리스트 초기화
    PUT(heap_listp, 0);                                 /* Alignment padding */ 
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));      /* Prologue header */ 
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));      /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));          /* Epilogue header */ 
    heap_listp += (2 * WSIZE);                          

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    // CHUNKSIZE/WSIZE == 1024 bytes == 2^10
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
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
