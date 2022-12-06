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
    "KJ-R-8team",
    /* First member's full name */
    "gitae Kim",
    /* First member's email address */
    "rlarlxo2628@gmail.com",
    /* Second member's full name (leave blank if none) */
    "dasol Park",
    /* Second member's email address (leave blank if none) */
    "davidtwins6970@gmail.com"
    /* Third member's full name (leave blank if none) */
};

/* 기본 size 상수 정의 */
#define WSIZE                4              /* Word and header/footer size (bytes) */
#define DSIZE                8              /* Double word size (bytes) */
#define CHUNKSIZE           (1 << 12)       /* Extend heap by this amount (bytes) */ // 512 bytes
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

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
#define HDRP(bp)            ((char *)(bp) - WSIZE)                           // 자신의 헤더를 가리키는 포인터를 리턴 // HDRP는 bp 포인터에서 -1 워드로 움직임
#define FTRP(bp)            ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)     // 자신의 풋터를 가리키는 포인터를 리턴 // FTRP는 bp 포인터에서 헤더의 사이즈를 가져와 다음 블록의 bp까지 움직인 다음 -DSIZE만큼가서 자신의 풋터로 갈 수 있음

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)       ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))   // 다음 블럭 bp포인터 리턴 // 현재 블록의 header에서 size을 가지고 온 뒤 현재 bp 포인터에서 size만큼을 더해 다음 블록으로 포인터가 넘어감
#define PREV_BLKP(bp)       ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))   // 이전 블럭 bp포인터 리턴 // 이전 블록의 footer에서 size을 가지고 온 뒤 현재 bp 포인터에서 size만큼을 빼서 이전 블록으로 포인터가 넘어감

static char *heap_listp; // 프롤로그 블록을 가리키는 정적 전역변수
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
void *mm_malloc(size_t size);
static void *coalesce(void *bp);
void mm_free(void *bp);
static void *extend_heap(size_t words);
int mm_init(void);
void *mm_realloc(void *ptr, size_t size);

// * 할당 정책 - 적절한 위치의 블럭을 찾으면 return bp
static void *find_fit(size_t asize)
{
    // first fit
    // void *bp = (char*)heap_listp;

    void *bp;

    for(bp = (char*)heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp))  && GET_SIZE(HDRP(bp)) >= asize){
            return bp;
        }
    }
    return NULL;

}

/* 블록이 할당되거나 재할당될 때 여유 공간을 판단해 분할해 주는 함수 */
/* 
* CASE1. 현재 블록에 size를 할당한 후에 남은 size가 최소 블록size(header와 footer를 포함한 4워드)보다 크거나 같을 때 
* - 현재 블록의 size 정보를 변경하고 남은 size를 새로운 가용 블록으로 분할한다.
*/

/* 
* CASE2. 현재 블록에 size를 할당한 후에 남은 size가 최소 블록 size보다 작을 때
* - 현재 블록의 size 정보만 바꿔준다.
*/
static void place(void *bp, size_t asize)
{
    size_t currSize = GET_SIZE(HDRP(bp));
    if ((currSize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        // * (분할) 남은 블록을 가용 영역으로 바꿔준다.
        PUT(HDRP(bp), PACK(currSize - asize, 0));
        PUT(FTRP(bp), PACK(currSize - asize, 0));
    }
    else {
        PUT(HDRP(bp), PACK(currSize, 1));
        PUT(FTRP(bp), PACK(currSize, 1));
    }
    
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;
    
    /* Adjust block size to include overhead and alignment reqs */
    /* 
    * 최소 16 bytes 크기의 블록을 구성
    * - 8bytes는 헤더와 풋터의 오버헤드를 위해, 오버헤드란 - 어떤 일을 처리하기 위해 들어가는 간접적인 처리 시간
    * - 8bytes는 정렬 요건을 맞추기 위해
    */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    /* 
    * 8bytes는 헤더와 풋더의 오베헤드를 위해
    * 인접 8의 배수로 증가시켜 정렬 요건을 맞춤
    ? 최소 24 bytes로 구성하게 되는데, 매개변수 size 값은 (헤더와 풋터를 뺀)data size이므로 최소 DSIZE만큼씩을 늘려줘야 함. 왜? 더블 워드 정렬 조건을 맞추면서 늘려줘야 하니까 공간이 부족하다면 DSIZE씩 만큼 늘려줘야함. 따라서! 8의 배수로 늘려줘야한다.
    */
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    /* Search the free list for a fit */
    /* 
    * curr_heap에서 할당 정책(find_fit)을 통해 요청한 블록을 배치시킨다.
    */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // prev_bp 포인트까지 간 후, prev_bp의 payload size만큼 이동하여 prev_footer 할당 비트를 확인
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // next_bp 포인트까지 간 후, -word로 next_header 할당 비트를 확인
    size_t size = GET_SIZE(HDRP(bp));                   // bp의 header 를 통해 size 확인

    /* Case 1 */
    if (prev_alloc && next_alloc) {                     // CASE1. prev, next block 모두 allocate면 pass
        return bp;
    }
    
    /* Case 2 */
    else if (prev_alloc && !next_alloc) {               // CASE2. prev는 allocate, next는 free block이면 curr_size + next_size를 추가
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    /* Case 3 */
    else if (!prev_alloc && next_alloc) {               // CASE3. prev는 free, next는 allocate block이면 curr_size + prev_size를 추가
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    /* Case 4 */
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));  // CASE4. prev는 free, next도 free block이면 prev size + curr_size + next size 추가
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;          // 옮겨진 위치의 bp 포인터를 반환
}



/*
 * mm_free - Freeing a block does nothing.
 * 블록 할당 해제하기
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp)); // 현재 블록의 header 사이즈를 알아옴

    PUT(HDRP(bp), PACK(size, 0));   // header의 할당 비트를 0으로 초기화
    PUT(FTRP(bp), PACK(size, 0));   // footer의 할당 비트를 0으로 초기화
    coalesce(bp);                   // (이전, 다음)블록이 가용 가능한 경우 병합
}

// 힙 확장하기
static void *extend_heap(size_t words) 
{
    char *bp;           // 블록 포인터
    size_t size;        // 힙을 확장할 size(바이트)

    /* Allocate an even number of words to maintain alignment */
    // 정렬을 유지하기 위해 짝수 개의 word 할당
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; // 4096 bytes, 즉 1024개의 워드 블록을 만듬
    // mem_sbrk 함수에서 max_addr 의 크기를 넘긴다면 -1 return
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    /* 가용 블럭 헤더/풋터 와 에필로그 헤더를 초기화 */ 
    
    PUT(HDRP(bp), PACK(size, 0));               /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));               /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));       /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp); // (이전, 다음)블록이 가용 가능한 경우 병합
}

/* mm_init - initialize the malloc package. */
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
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}