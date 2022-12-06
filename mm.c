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
    "3팀",
    "김현우",
    "kimhyn21@gmail.com",
    "주성우",
    "alskadmlcraz1@gmail.com",
    "안예인",
    "yein.ahn9@gmail.com",
};
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 매크로 상수, 매크로 함수 정의 */

#define WSIZE 4 /* 워드 사이즈 정의(bytes) */
#define DSIZE 8 /* 더블워드 사이즈 정의(bytes) */

#define CHUNKSIZE (1 << 12) /* 힙 확장을 위한 기본 크기(bytes) */
// 비트를 왼쪽으로 한 비트씩 이동시킬 때마다 2가 곱해지므로 1 << 12는 4096(bytes) = 4KB

/* 두 값을 비교하여 더 큰 값을 선택하는 매크로 함수 */
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* 크기(size)에 대한 정보와 할당 여부를 나타내는 가용 비트(allocated bit)를 하나의 워드 묶는 매크로 함수 */
#define PACK(size, alloc) ((size) | (alloc))
// 이 워드는 헤더 및 푸터를 구성함.

/* 주소 p의 워드를 읽고 쓰기 위한 매크로 함수 */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
// 인자로 전달받은 주소값을 unsigned int형으로 타입 캐스팅하며 간접 참조

/* 주소 p의 크기 정보를 읽는 매크로 함수 */
#define GET_SIZE(p) (GET(p) & ~0x7)
// 1. 0x7를 부정 연산하여 비트를 모두 반전시칸다. 0 .. 1 1 1 -> 1 .. 0 0 0
// 2. 논리곱 연산(&)으로 인해, 주소 p의 워드에서 마지막 3비트만 0으로 교체된 값이 반환된다.
// 3. 이 값이 바로 해당 워드에 저장되어 있던 크기 정보! (교재 p.816 참고)

/* 주소 p의 가용 비트 값을 읽는 매크로 함수 */
#define GET_ALLOC(p) (GET(p) & 0x1)
// 1. 논리곱 연산(&)으로 인해, 주소 p의 워드에서 마지막 1비트를 제외한 앞의 모든 비트는 0으로 교체된 값이 반환된다.
// 2. 이 값이 바로 해당 워드에 저장되어 있던 가용 비트 값! (교재 p.816 참고)

// ! 참고: malloc은 payload의 시작 주소를 반환한다. -> bp (교재 p.816 참고)
/* 블록을 가리키는 포인터인 bp를 인자로 받아 해당 블록의 헤더 주소를 계산하는 매크로 함수 */
#define HDRP(bp) ((char *)(bp) - WSIZE)
/* 블록을 가리키는 포인터인 bp를 인자로 받아 해당 블록의 푸터 주소를 계산하는 매크로 함수 */
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// ! GET_SIZE의 인자로 전달되는 주소 값은 payload의 시작 주소가 아니라 블록 자체의 시작 주소다.
// 블록을 가리키는 포인터인 bp를 인자로 받아 다음 블록의 주소를 계산하는 매크로 함수
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
// 블록을 가리키는 포인터인 bp를 인자로 받아 이전 블록의 주소를 계산하는 매크로 함수
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))
// #define PREV_BLKP(bp) ((char *)(bp) - DSIZE - GET_SIZE((char *)(bp) - DSIZE) + DSIZE)

/* 전역 변수 선언 */
static char *heap_listp;

/* 함수 선언 */
int mm_init(void);
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp, size_t asize);
void mm_free(void *bp);
void *mm_malloc(size_t size);
void *mm_realloc(void *ptr, size_t size);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* 비어있는 힙을 생성 */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) {
        return -1;
    }
    // mem_sbrk 함수의 반환값을 heap_listp에 할당함과 동시에 동등 관계 연산
    // mem_sbrk는 정상적으로 처리되지 않을 경우 -1 반환

    /* 방금 생성한 비어있는 힙을 초기화 */
    PUT(heap_listp, 0); /* 미사용 패딩 워드(정렬용) */
    PUT(heap_listp + (1 * WSIZE), PACK(8, 1)); /* 프롤로그 헤더 */
    PUT(heap_listp + (2 * WSIZE), PACK(8, 1)); /* 프롤로그 푸터 */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); /* 에필로그 푸터 */
    heap_listp += (2 * WSIZE);

    /* 초기 가용 블록 생성 */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }

    return 0;
}

/* 
 * extend_heap - 다음 두 가지 케이스에서 호출된다. :
 * 1) 힙을 초기화할 때
 * 2) mm_malloc이 적당한 fit을 찾지 못했을 때
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /*
     * double-word alignment 유지하기 위해, 요청 크기(words)를 인접 2워드의 배수로 반올림
     * p.811 참고
     */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }

    /* 새로운 가용 블록의 헤더/푸터와 에필로그 헤더를 정한다. */
    PUT(HDRP(bp), PACK(size, 0)); // 새로운 가용 블록의 헤더
    PUT(FTRP(bp), PACK(size, 0)); // 새로운 가용 블록의 푸터
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // 새로운 에필로그 헤더

    /* 이전 힙이 가용 블록으로 끝났었다면 두 개의 가용 블록 통합 작업 */
    return coalesce(bp);
}

/*
 * 가용 블록 간 연결 작업을 하는 함수
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 이전 블록의 할당 여부
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 다음 블록의 할당 여부
    size_t size = GET_SIZE(HDRP(bp)); // 현재 블록의 크기 정보

    /* Case 1 */
    if (prev_alloc && next_alloc) {
        // 이전, 다음 블록 모두 할당 블록이므로 연결 작업 X
        return bp;
    }

    /* Case 2 */
    if (prev_alloc && !next_alloc) {
        // 이전 블록은 할당 블록, 다음 블록은 가용 블록이므로
        // 다음 블록과의 연결 작업 진행
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // 현재 블록 크기에 다음 블록의 크기를 더한다.
        PUT(HDRP(bp), PACK(size, 0)); // 현재 블록의 헤더 정보 업데이트
        PUT(FTRP(bp), PACK(size, 0));
        // FTRP 값은 HDRP에 위치하는 size 값에 의존하는데, 바로 윗줄에서 이 size 값이 갱신되었으므로 FTRP 값도 변경된다.
        // 기존에는 연결 이전의 현재 블록의 푸터의 위치를 가리켰지만, 이 시점에선 연결 이전의 다음 블록의 푸터의 위치,
        // 즉 연결 이후의 현재 블록의 푸터의 위치를 가리키게 된다.
        // 결과적으로 연결 이후의 현재 블록의 푸터 정보를 업데이트하게 된다.
    }
    /* Case 3 */
    else if (!prev_alloc && next_alloc) {
        // 이전 블록은 가용 블록, 다음 블록은 할당 블록이므로
        // 이전 블록과의 연결 작업 진행
        size += GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

        // 가용 블록의 헤더 위치가 변경되었으니 다음과 같이 bp 갱신 작업 필요
        bp = PREV_BLKP(bp);
    }
    /* Case 4 */
    else {
        // 이전, 다음 블록 모두 가용 블록일 경우 양쪽 블록과 연결 작업 진행
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));

        // 가용 블록의 헤더 위치가 변경되었으니 다음과 같이 bp 갱신 작업 필요
        bp = PREV_BLKP(bp);
    }

    return bp;
}

/* first fit 정책에 따라 배치할 가용 블록을 찾는 함수 */
static void *find_fit(size_t asize)
{
    // 가용 리스트를 처음부터 검색한다.
    char *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (GET_ALLOC(HDRP(bp))) {
            // 할당 블록이라면 다음 턴으로 skip
            continue;
        }

        if (GET_SIZE(HDRP(bp)) >= asize) {
            return bp;
        }
    }

    return NULL;
}

/* 블록이 할당되거나 재할당될 때 여유 공간을 판단해 분할해 주는 함수 */
static void place(void *bp, size_t asize)
{
    size_t currSize = GET_SIZE(HDRP(bp));
    /* 
    * CASE1. 현재 블록에 size를 할당한 후에 남은 size가 최소 블록size(header와 footer를 포함한 4워드)보다 크거나 같을 때 
    * - 현재 블록의 size 정보를 변경하고 남은 size를 새로운 가용 블록으로 분할한다.
    */
    if ((currSize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        // * (분할) 남은 블록을 가용 영역으로 바꿔준다.
        PUT(HDRP(bp), PACK(currSize - asize, 0));
        PUT(FTRP(bp), PACK(currSize - asize, 0));
    }
    /* 
    * CASE2. 현재 블록에 size를 할당한 후에 남은 size가 최소 블록 size보다 작을 때
    * - 현재 블록의 size 정보만 바꿔준다.
    */
    else {
        PUT(HDRP(bp), PACK(currSize, 1));
        PUT(FTRP(bp), PACK(currSize, 1));
    }
    
}

/* 할당 블록을 반환(free)하는 함수 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    // 블록의 헤더, 푸터에 있는 할당 비트 값을 0으로 갱신
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    // false fragmentation 현상을 방지하기 위해 인접 가용 블록들과 연결하는 작업 진행
    coalesce(bp);
}

void *mm_malloc(size_t size)
{
    size_t asize; // 헤더/풋터 오버헤드와 더블워드 정렬 조건을 고려하여 조정된 블록 크기
    size_t extend_size; // 맞는 블록을 찾지 못할 경우 힙을 얼마나 증가시킬지
    char *bp;

    if (size == 0) { // 의미 없는 요청은 무시
        return NULL;
    }

    // 헤더/풋터 오버헤드와 더블워드 정렬 조건을 고려하여 블록 크기를 조정한다.
    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    }
    else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    // 할당하기에 적절한 가용 블록을 찾았다면,
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);

        return bp;
    }

    // 할당하기에 적절한 가용 블록을 찾지 못했다면,
    extend_size = MAX(asize, CHUNKSIZE);

    if ((bp = extend_heap(extend_size)) == NULL) {
        return NULL;
    }
    place(bp, asize);

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
    size_t new_size;
    
    // adjust block size to include overhead and alignment reqs.
    if (size <= DSIZE) {
        new_size = 2 * DSIZE;
    }
    else {
        new_size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }
    //새로 할당할 사이즈가 본래 사이즈보다 작거나 같을 때
    //즉시 반환
    copySize = GET_SIZE(HDRP(oldptr));
    if (new_size <= copySize)
    {
        if (copySize - new_size < 2*DSIZE)
        {
            PUT(HDRP(oldptr), PACK(copySize, 1));
            PUT(FTRP(oldptr), PACK(copySize, 1));
        }else{
            PUT(HDRP(oldptr), PACK(new_size, 1));
            PUT(FTRP(oldptr), PACK(new_size, 1));
            oldptr = NEXT_BLKP(oldptr);
            PUT(HDRP(oldptr), PACK(copySize - new_size, 0));
            PUT(FTRP(oldptr), PACK(copySize - new_size, 0));
            coalesce(oldptr);
        }
        
        return ptr;
    }
    
    // 사이즈가 본래 사이즈 보다 크고, 다음 블럭이 가용 블럭 일 경우
    size_t size_sum = GET_SIZE(HDRP(NEXT_BLKP(oldptr))) + GET_SIZE(HDRP(oldptr));
    if (!GET_ALLOC(HDRP(NEXT_BLKP(oldptr))) && size_sum >= new_size) {
        if (size_sum - new_size < 2*DSIZE) {
            PUT(HDRP(oldptr), PACK(size_sum, 1));
            PUT(FTRP(oldptr), PACK(size_sum, 1));
        }
        else {
            PUT(HDRP(oldptr), PACK(new_size, 1));
            PUT(FTRP(oldptr), PACK(new_size, 1));
            oldptr = NEXT_BLKP(oldptr);
            PUT(HDRP(oldptr), PACK(size_sum - new_size, 0));
            PUT(FTRP(oldptr), PACK(size_sum - new_size, 0));
        }
        return ptr;
    }


    // 일반적인 realloc 함수
    // malloc을 이용해서 새로운 사이즈를 할당하고 할당된 곳에
    // 데이터를 이전해서 보존한 이후, 본래 할당된 메모리를 가용 메모리로 전환
    newptr = mm_malloc(size);
    if (newptr == NULL) {
        return NULL;
    }
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}