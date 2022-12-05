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

/* 크기(size)에 대한 정보와 할당 여부를 나타내는 가용 비트(allocated bit)를 하나의 워드로 묶는 매크로 함수 */
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

/* 블록을 가리키는 포인터인 bp를 인자로 받아 해당 블록의 헤더 주소를 계산하는 매크로 함수 */
// ! 참고: malloc은 payload의 시작 주소를 반환한다. -> bp (교재 p.816 참고)
#define HDRP(bp) ((char *)(bp) - WSIZE)
/* 블록을 가리키는 포인터인 bp를 인자로 받아 해당 블록의 푸터 주소를 계산하는 매크로 함수 */
// ! 참고: malloc은 payload의 시작 주소를 반환한다. -> bp (교재 p.816 참고)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* 블록을 가리키는 포인터인 bp를 인자로 받아 다음 블록의 주소를 계산하는 매크로 함수 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp) - WSIZE))
/* 블록을 가리키는 포인터인 bp를 인자로 받아 이전 블록의 주소를 계산하는 매크로 함수 */
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))
// #define PREV_BLKP(bp) ((char *)(bp) - DSIZE - GET_SIZE((char *)(bp) - DSIZE) + DSIZE)
