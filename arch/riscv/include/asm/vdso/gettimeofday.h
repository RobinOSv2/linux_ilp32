/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_VDSO_GETTIMEOFDAY_H
#define __ASM_VDSO_GETTIMEOFDAY_H

#ifndef __ASSEMBLY__

#include <asm/barrier.h>
#include <asm/unistd.h>
#include <asm/csr.h>
#include <uapi/linux/time.h>

/*
 * 32-bit land is lacking generic time vsyscalls as well as the legacy 32-bit
 * time syscalls like gettimeofday. Skip these definitions since on 32-bit.
 */
#ifdef CONFIG_GENERIC_TIME_VSYSCALL

#define VDSO_HAS_CLOCK_GETRES	1

#ifdef __NR_gettimeofday
static __always_inline
int gettimeofday_fallback(struct __kernel_old_timeval *_tv,
			  struct timezone *_tz)
{
	register struct __kernel_old_timeval *tv asm("a0") = _tv;
	register struct timezone *tz asm("a1") = _tz;
	register long ret asm("a0");
	register long nr asm("a7") = __NR_gettimeofday;

	asm volatile ("ecall\n"
		      : "=r" (ret)
		      : "r"(tv), "r"(tz), "r"(nr)
		      : "memory");

	return ret;
}
#endif

#if __SIZEOF_POINTER__ == 8
#ifdef __NR_clock_gettime
static __always_inline
long clock_gettime_fallback(clockid_t _clkid, struct __kernel_timespec *_ts)
{
	register clockid_t clkid asm("a0") = _clkid;
	register struct __kernel_timespec *ts asm("a1") = _ts;
	register long ret asm("a0");
	register long nr asm("a7") = __NR_clock_gettime;

	asm volatile ("ecall\n"
		      : "=r" (ret)
		      : "r"(clkid), "r"(ts), "r"(nr)
		      : "memory");

	return ret;
}
#endif

#ifdef __NR_clock_getres
static __always_inline
int clock_getres_fallback(clockid_t _clkid, struct __kernel_timespec *_ts)
{
	register clockid_t clkid asm("a0") = _clkid;
	register struct __kernel_timespec *ts asm("a1") = _ts;
	register long ret asm("a0");
	register long nr asm("a7") = __NR_clock_getres;

	asm volatile ("ecall\n"
		      : "=r" (ret)
		      : "r"(clkid), "r"(ts), "r"(nr)
		      : "memory");

	return ret;
}
#endif

#elif __SIZEOF_POINTER__ == 4

#define BUILD_VDSO32		1

#ifdef __NR_clock_gettime64
static __always_inline
long clock_gettime_fallback(clockid_t _clkid, struct __kernel_timespec *_ts)
{
	register clockid_t clkid asm("a0") = _clkid;
	register struct __kernel_timespec *ts asm("a1") = _ts;
	register long ret asm("a0");
	register long nr asm("a7") = __NR_clock_gettime64;

	asm volatile ("ecall\n"
		      : "=r" (ret)
		      : "r"(clkid), "r"(ts), "r"(nr)
		      : "memory");

	return ret;
}
#endif

#ifdef __NR_clock_getres_time64
static __always_inline
int clock_getres_fallback(clockid_t _clkid, struct __kernel_timespec *_ts)
{
	register clockid_t clkid asm("a0") = _clkid;
	register struct __kernel_timespec *ts asm("a1") = _ts;
	register long ret asm("a0");
	register long nr asm("a7") = __NR_clock_getres_time64;

	asm volatile ("ecall\n"
		      : "=r" (ret)
		      : "r"(clkid), "r"(ts), "r"(nr)
		      : "memory");

	return ret;
}
#endif

#ifdef __NR_clock_gettime
static __always_inline
int clock_gettime32_fallback(clockid_t _clkid, struct old_timespec32 *_ts)
{
	register clockid_t clkid asm("a0") = _clkid;
	register struct old_timespec32 *ts asm("a1") = _ts;
	register long ret asm("a0");
	register long nr asm("a7") = __NR_clock_gettime;

	asm volatile ("ecall\n"
		      : "=r" (ret)
		      : "r"(clkid), "r"(ts), "r"(nr)
		      : "memory");

	return ret;
}
#endif

#ifdef __NR_clock_getres
static __always_inline
int clock_getres32_fallback(clockid_t _clkid, struct old_timespec32 *_ts)
{
	register clockid_t clkid asm("a0") = _clkid;
	register struct old_timespec32 *ts asm("a1") = _ts;
	register long ret asm("a0");
	register long nr asm("a7") = __NR_clock_getres;

	asm volatile ("ecall\n"
		      : "=r" (ret)
		      : "r"(clkid), "r"(ts), "r"(nr)
		      : "memory");

	return ret;
}
#endif

#endif /* __SIZEOF_POINTER__ */
#endif /* CONFIG_GENERIC_TIME_VSYSCALL */

static __always_inline u64 __arch_get_hw_counter(s32 clock_mode,
						 const struct vdso_data *vd)
{
	/*
	 * The purpose of csr_read(CSR_TIME) is to trap the system into
	 * M-mode to obtain the value of CSR_TIME. Hence, unlike other
	 * architecture, no fence instructions surround the csr_read()
	 */
#if __riscv_xlen == 64
	return csr_read(CSR_TIME);
#else
	u32 hi, lo;

	do {
		hi = csr_read(CSR_TIMEH);
		lo = csr_read(CSR_TIME);
	} while (hi != csr_read(CSR_TIMEH));

	return ((u64)hi << 32) | lo;
#endif
}

static __always_inline const struct vdso_data *__arch_get_vdso_data(void)
{
	return _vdso_data;
}

#ifdef CONFIG_TIME_NS
static __always_inline
const struct vdso_data *__arch_get_timens_vdso_data(const struct vdso_data *vd)
{
	return _timens_data;
}
#endif
#endif /* !__ASSEMBLY__ */

#endif /* __ASM_VDSO_GETTIMEOFDAY_H */
