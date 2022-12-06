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
 * 2) mm_malloc이 적당한 fit을 찾지 못했을 때 // TODO:
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
