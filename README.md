# 3ν Malloc Lab

## π§π»ββοΈ νμ

1. κΉνμ° [@woe3](https://github.com/woe3)

2. μ£Όμ±μ° [@nickhealthy](https://github.com/nickhealthy)

3. μμμΈ [@ahnanne](https://github.com/ahnanne)

## π νμ΅ κ³ν

|λ μ§|μΌμ |
|------|---|
|12/2(κΈ) - 12/4(μΌ)|κ°μΈνμ΅|
|12/4(μΌ) λ°€|<b>[κ°λμ€λͺ]</b> κΉνμ°: ν κ΅¬μ±, sbrk, λ©λͺ¨λ¦¬ ν λΉ μ μ± / μ£Όμ±μ°: λμ  λ©λͺ¨λ¦¬ ν λΉ / μμμΈ: malloc/free, κ°μ©λΈλ‘, κ²½κ³νκ·Έ|
||μ½λ© μ»¨λ²€μ λ° νμ λ°©μ λΌμ π€|
||merge μμ μΌμ  μ νκΈ°|
|12/5(μ) μ€μ |κ΅¬ν μμ|
|12/5(μ) ~λ°€|<b>[μ½λλ¦¬λ·°]</b> κΉνμ°: `mm_free`, `mm_malloc` / μ£Όμ±μ°: `mm_init` / μμμΈ: κΈ°λ³Έ μμ λ° λ§€ν¬λ‘ μ μ, `extend_heap`|
||`find_fit`, `place`, `realloc` κ΅¬ν μμ (κ°μ μ΅μν νλμ© κ΅¬ν)|
|12/6(ν) λ°€(21μ)|<b>[μ½λλ¦¬λ·°]</b> κΉνμ°: `realloc` / μ£Όμ±μ°: `place` / μμμΈ: `find_fit`|
||νμ€νΈ + merge
|12/7(μ) μ€ν|(κ°μ) λͺμμ  κ°μ© λ¦¬μ€νΈ λ° λΆλ¦¬ κ°μ© λ¦¬μ€νΈλ‘ κ΅¬ν|
|12/7(μ) λ°€|μ΅μ’ μ κ² + νμ€νΈ + merge + λλ²κΉ λ±..|

## π₯ͺ νμ λ°©μ

- μ½λ μ»¨λ²€μμ CSAPP μμ μ½λ μ»¨λ²€μμ λ°λ₯΄κΈ°λ‘..

- νμ λ°©μ

  - κ°μμ κ°μΈ λΈλμΉμμ μμνλ κ²μ κΈ°λ³ΈμΌλ‘ ν¨.

	```
	# λ©μΈ λΈλμΉλ‘ μ΄λ
	git checkout main

	# μκ²© λ©μΈ λΈλμΉμ λ‘μ»¬ λ©μΈ λΈλμΉ λκΈ°ν
	git pull origin main

	# νμ κ°μΈ λΈλμΉλ‘ μ΄λ
	git checkout κ°μΈλΈλμΉλͺ

	# λ©μΈ λΈλμΉμ κ°μΈ λΈλμΉ λκΈ°ν
	git merge main

	# μμ μμ! π
	```

  - κ³΅ν΅ μ½λ(CSAPP μμ μ½λ)μ μ§μ  κ΅¬νν΄μΌ νλ μ½λ λͺ¨λ κ°μΈ λΈλμΉμμ μμ±ν©λλ€.

    - κ³΅ν΅ μ½λ: κΈ°λ³Έ μμ λ° λ§€ν¬λ‘ μ μ μ½λ, `mm_init`, `extend_heap`, `mm_free`, `mm_malloc`

  - μ§μ  κ΅¬νν΄μΌ νλ μ½λμ κ²½μ° νμ΅ κ³ν μΌμ μ λ§μΆ°, νμλ€κ³Ό νμνμ¬ λ©μΈ λΈλμΉλ‘μ merge μμμ μ§νν©λλ€.

> μ΄ν μλ³Έ μ μ₯μμ README.md λ΄μ©μλλ€.

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

