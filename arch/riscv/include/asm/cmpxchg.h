/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2014 Regents of the University of California
 */

#ifndef _ASM_RISCV_CMPXCHG_H
#define _ASM_RISCV_CMPXCHG_H

#include <linux/bug.h>
#include <linux/mmdebug.h>

#include <asm/barrier.h>
#include <asm/fence.h>

#define __arch_xchg_masked(prepend, append, r, p, n)			\
({									\
	u32 *__ptr32b = (u32 *)((ulong)(p) & ~0x3);			\
	ulong __size_p = (sizeof(*p) > 2) ? 2 : sizeof(*p);		\
	ulong __s = ((ulong)(p) & (0x4 - __size_p)) * BITS_PER_BYTE;	\
	ulong __mask = GENMASK(((__size_p) * BITS_PER_BYTE) - 1, 0)	\
			<< __s;						\
	ulong __newx = (ulong)(n) << __s;				\
	ulong __retx;							\
	ulong __rc;							\
									\
	__asm__ __volatile__ (						\
	       prepend							\
	       "0:	lr.w %0, %2\n"					\
	       "	and  %1, %0, %z4\n"				\
	       "	or   %1, %1, %z3\n"				\
	       "	sc.w %1, %1, %2\n"				\
	       "	bnez %1, 0b\n"					\
	       append							\
	       : "=&r" (__retx), "=&r" (__rc), "+A" (*(__ptr32b))	\
	       : "rJ" (__newx), "rJ" (~__mask)				\
	       : "memory");						\
									\
	r = (__typeof__(*(p)))((__retx & __mask) >> __s);		\
})

#define __arch_xchg(sfx, prepend, append, r, p, n)			\
({									\
	__asm__ __volatile__ (						\
		prepend							\
		"	amoswap" sfx " %0, %2, %1\n"			\
		append							\
		: "=r" (r), "+A" (*(p))					\
		: "r" (n)						\
		: "memory");						\
})

#define _arch_xchg(ptr, new, sfx, prepend, append)			\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	__typeof__(*(__ptr)) __new = (new);				\
	__typeof__(*(__ptr)) __ret;					\
									\
	switch (sizeof(*__ptr)) {					\
	case 1:								\
	case 2:								\
		__arch_xchg_masked(prepend, append,			\
				   __ret, __ptr, __new);		\
		break;							\
	case 4:								\
		__arch_xchg(".w" sfx, prepend, append,			\
			      __ret, __ptr, __new);			\
		break;							\
	case 8:								\
		__arch_xchg(".d" sfx, prepend, append,			\
			      __ret, __ptr, __new);			\
		break;							\
	default:							\
		BUILD_BUG();						\
	}								\
	(__typeof__(*(__ptr)))__ret;					\
})

#define arch_xchg_relaxed(ptr, x)					\
	_arch_xchg(ptr, x, "", "", "")

#define arch_xchg_acquire(ptr, x)					\
	_arch_xchg(ptr, x, "", "", RISCV_ACQUIRE_BARRIER)

#define arch_xchg_release(ptr, x)					\
	_arch_xchg(ptr, x, "", RISCV_RELEASE_BARRIER, "")

#define arch_xchg(ptr, x)						\
	_arch_xchg(ptr, x, ".aqrl", "", "")

#define xchg32(ptr, x)							\
({									\
	BUILD_BUG_ON(sizeof(*(ptr)) != 4);				\
	arch_xchg((ptr), (x));						\
})

#define xchg64(ptr, x)							\
({									\
	BUILD_BUG_ON(sizeof(*(ptr)) != 8);				\
	arch_xchg((ptr), (x));						\
})

/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */

#define __arch_cmpxchg_masked(sc_sfx, prepend, append, r, p, o, n)	\
({									\
	u32 *__ptr32b = (u32 *)((ulong)(p) & ~0x3);			\
	ulong __size_p = (sizeof(*p) > 2) ? 2 : sizeof(*p);		\
	ulong __s = ((ulong)(p) & (0x4 - __size_p)) * BITS_PER_BYTE;	\
	ulong __mask = GENMASK(((__size_p) * BITS_PER_BYTE) - 1, 0)	\
			<< __s;						\
	ulong __newx = (ulong)(n) << __s;				\
	ulong __oldx = (ulong)(o) << __s;				\
	ulong __retx;							\
	ulong __rc;							\
									\
	__asm__ __volatile__ (						\
		prepend							\
		"0:	lr.w %0, %2\n"					\
		"	and  %1, %0, %z5\n"				\
		"	bne  %1, %z3, 1f\n"				\
		"	and  %1, %0, %z6\n"				\
		"	or   %1, %1, %z4\n"				\
		"	sc.w" sc_sfx " %1, %1, %2\n"			\
		"	bnez %1, 0b\n"					\
		append							\
		"1:\n"							\
		: "=&r" (__retx), "=&r" (__rc), "+A" (*(__ptr32b))	\
		: "rJ" ((long)__oldx), "rJ" (__newx),			\
		  "rJ" (__mask), "rJ" (~__mask)				\
		: "memory");						\
									\
	r = (__typeof__(*(p)))((__retx & __mask) >> __s);		\
})

#define __arch_cmpxchg(lr_sfx, sc_sfx, prepend, append, r, p, co, o, n)	\
({									\
	register unsigned int __rc;					\
									\
	__asm__ __volatile__ (						\
		prepend							\
		"0:	lr" lr_sfx " %0, %2\n"				\
		"	bne  %0, %z3, 1f\n"				\
		"	sc" sc_sfx " %1, %z4, %2\n"			\
		"	bnez %1, 0b\n"					\
		append							\
		"1:\n"							\
		: "=&r" (r), "=&r" (__rc), "+A" (*(p))			\
		: "rJ" (co o), "rJ" (n)					\
		: "memory");						\
})

#define _arch_cmpxchg(ptr, old, new, sc_sfx, prepend, append)		\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	__typeof__(*(__ptr)) __old = (old);				\
	__typeof__(*(__ptr)) __new = (new);				\
	__typeof__(*(__ptr)) __ret;					\
									\
	switch (sizeof(*__ptr)) {					\
	case 1:								\
	case 2:								\
		__arch_cmpxchg_masked(sc_sfx, prepend, append,		\
					__ret, __ptr, __old, __new);	\
		break;							\
	case 4:								\
		__arch_cmpxchg(".w", ".w" sc_sfx, prepend, append,	\
				__ret, __ptr, (long), __old, __new);	\
		break;							\
	case 8:								\
		__arch_cmpxchg(".d", ".d" sc_sfx, prepend, append,	\
				__ret, __ptr, /**/, __old, __new);	\
		break;							\
	default:							\
		BUILD_BUG();						\
	}								\
	(__typeof__(*(__ptr)))__ret;					\
})

#define arch_cmpxchg_relaxed(ptr, o, n)					\
	_arch_cmpxchg((ptr), (o), (n), "", "", "")

#define arch_cmpxchg_acquire(ptr, o, n)					\
	_arch_cmpxchg((ptr), (o), (n), "", "", RISCV_ACQUIRE_BARRIER)

#define arch_cmpxchg_release(ptr, o, n)					\
	_arch_cmpxchg((ptr), (o), (n), "", RISCV_RELEASE_BARRIER, "")

#define arch_cmpxchg(ptr, o, n)						\
	_arch_cmpxchg((ptr), (o), (n), ".rl", "", "	fence rw, rw\n")

#define arch_cmpxchg_local(ptr, o, n)					\
	arch_cmpxchg_relaxed((ptr), (o), (n))

#define arch_cmpxchg64(ptr, o, n)					\
({									\
	BUILD_BUG_ON(sizeof(*(ptr)) != 8);				\
	arch_cmpxchg((ptr), (o), (n));					\
})

#define arch_cmpxchg64_local(ptr, o, n)					\
({									\
	BUILD_BUG_ON(sizeof(*(ptr)) != 8);				\
	arch_cmpxchg_relaxed((ptr), (o), (n));				\
})

#ifdef CONFIG_ARCH_RV64ILP32
#define system_has_cmpxchg_double()	1

#define __cmpxchg_double_check(ptr1, ptr2)				\
({									\
	if (sizeof(*(ptr1)) != 4)					\
		BUILD_BUG();						\
	if (sizeof(*(ptr2)) != 4)					\
		BUILD_BUG();						\
	VM_BUG_ON((ulong *)(ptr2) - (ulong *)(ptr1) != 1);		\
	VM_BUG_ON(((ulong)ptr1 & 0x7) != 0);				\
})

#define __cmpxchg_double(old1, old2, new1, new2, ptr)			\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	register unsigned int __ret;					\
	u64 __old;							\
	u64 __new;							\
	u64 __tmp;							\
	switch (sizeof(*(ptr))) {					\
	case 4:								\
		__old = ((u64)old2 << 32) | (u64)old1;			\
		__new = ((u64)new2 << 32) | (u64)new1;			\
		__asm__ __volatile__ (					\
			"0:	lr.d %0, %2\n"				\
			"	bne %0, %z3, 1f\n"			\
			"	sc.d %1, %z4, %2\n"			\
			"	bnez %1, 0b\n"				\
			"1:\n"						\
			: "=&r" (__tmp), "=&r" (__ret), "+A" (*__ptr)	\
			: "rJ" (__old), "rJ" (__new)			\
			: "memory");					\
		__ret = (__old == __tmp);				\
		break;							\
	default:							\
		BUILD_BUG();						\
	}								\
	__ret;								\
})

#define arch_cmpxchg_double(ptr1, ptr2, o1, o2, n1, n2)			\
({									\
	int __ret;							\
	__cmpxchg_double_check(ptr1, ptr2);				\
	__ret = __cmpxchg_double((ulong)(o1), (ulong)(o2),		\
				 (ulong)(n1), (ulong)(n2),		\
				  ptr1);				\
	__ret;								\
})
#endif

#endif /* _ASM_RISCV_CMPXCHG_H */
