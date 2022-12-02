# 3팀 Malloc Lab

## 🧚🏻‍♂️ 팀원

1. 김현우 [@woe3](https://github.com/woe3)

2. 주성우 [nickhealthy](https://github.com/nickhealthy)

3. 안예인 [ahnanne](https://github.com/ahnanne)

## 📘 학습 계획

|날짜|일정|
|------|---|
|12/2(금) - 12/4(일)|개인학습|
|12/4(일) 밤|<b>[개념설명]</b> 김현우: 힙 구성, sbrk, 메모리 할당 정책 / 주성우: 동적 메모리 할당 / 안예인: malloc/free, 가용블록, 경계태그|
||코딩 컨벤션 및 협업 방식 논의 🤔|
||merge 작업 일정 정하기|
|12/5(월) 오전|구현 시작|
|12/5(월) 밤|merge 작업 (범위 미정)|
|12/6(화) 밤|merge 작업 (범위 미정)|
|12/7(수) 밤|merge 작업 (범위 미정)|


> 이하 원본 저장소의 README.md 내용입니다.

#####################################################################
# CS:APP Malloc Lab
# Handout files for students
#
# Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
# May not be used, modified, or copied without permission.
#
######################################################################

***********
Main Files:
***********

mm.{c,h}	
	Your solution malloc package. mm.c is the file that you
	will be handing in, and is the only file you should modify.

mdriver.c	
	The malloc driver that tests your mm.c file

short{1,2}-bal.rep
	Two tiny tracefiles to help you get started. 

Makefile	
	Builds the driver

**********************************
Other support files for the driver
**********************************

config.h	Configures the malloc lab driver
fsecs.{c,h}	Wrapper function for the different timer packages
clock.{c,h}	Routines for accessing the Pentium and Alpha cycle counters
fcyc.{c,h}	Timer functions based on cycle counters
ftimer.{c,h}	Timer functions based on interval timers and gettimeofday()
memlib.{c,h}	Models the heap and sbrk function

*******************************
Building and running the driver
*******************************
To build the driver, type "make" to the shell.

To run the driver on a tiny test trace:

	unix> mdriver -V -f short1-bal.rep

The -V option prints out helpful tracing and summary information.

To get a list of the driver flags:

	unix> mdriver -h

