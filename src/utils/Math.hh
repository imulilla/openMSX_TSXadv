// $Id$

#ifndef MATH_HH
#define MATH_HH

#include "openmsx.hh"
#include "static_assert.hh"
#include "inline.hh"
#include "build-info.hh"
#include <algorithm>
#define _USE_MATH_DEFINES // needed for VC++
#include <cmath>

namespace openmsx {

namespace Math {

#ifdef _MSC_VER  // or replace this with     #ifndef HAVE_C99MATHOPS

	// Poor man's implementations to get things compiling

	// rint(): Round according to current floating-point rounding direction:
	//         Is potentially faster than floor(), but we don't care about
	//         that here.
	static ALWAYS_INLINE long lrint(double x)
	{
		return floor(x + (x > 0) ? 0.5 : -0.5);
	}

	static ALWAYS_INLINE long lrintf(float x)
	{
		return floor(x + (x > 0) ? 0.5 : -0.5);
	}

	// truncf(): round x to the nearest integer not larger in absolute value
	static ALWAYS_INLINE float truncf(float x)
	{
		return (x < 0) ? ceil(x) : floor(x);
	}

	// round(): round to nearest integer, away from zero
	static double round(double x)
	{
		double t;
		if (x >= 0) {
			t = ceil(x);
			if ((t - x) > 0.5) {
				t--;
			}
			return t;
		} else {
			t = ceil(-x);
			if ((t + x) > 0.5) {
				t--;
			}
			return -t;
		}
	}

#endif


/** Returns the smallest number that is both >=a and a power of two.
  */
unsigned powerOfTwo(unsigned a);

/** Returns two gaussian distributed random numbers.
  * We return two numbers instead of one because the second number comes for
  * free in the current implementation.
  */
void gaussian2(double& r1, double& r2);

/** Clips x to the range [LO,HI].
  * Slightly faster than    std::min(HI, std::max(LO, x))
  * especially when no clipping is required.
  */
template <int LO, int HI>
inline int clip(int x)
{
	return unsigned(x - LO) <= unsigned(HI - LO) ? x : (x < HI ? LO : HI);
}

/** Clip x to range [-32768,32767]. Special case of the version above.
  * Optimized for the case when no clipping is needed.
  */
inline short clipIntToShort(int x)
{
	STATIC_ASSERT((-1 >> 1) == -1); // right-shift must preserve sign
	return short(x) == x ? x : (0x7FFF - (x >> 31));
}

/** Clips r * factor to the range [LO,HI].
  */
template <int LO, int HI>
inline int clip(double r, double factor)
{
	int a = int(round(r * factor));
	return std::min(std::max(a, LO), HI);
}

/** Calculate greatest common divider of two strictly positive integers.
  * Classical implementation is like this:
  *    while (unsigned t = b % a) { b = a; a = t; }
  *    return a;
  * The following implementation avoids the costly modulo operation. It
  * is about 40% faster on my machine.
  *
  * require: a != 0  &&  b != 0
  */
inline unsigned gcd(unsigned a, unsigned b)
{
	unsigned k = 0;
	while (((a & 1) == 0) && ((b & 1) == 0)) {
		a >>= 1; b >>= 1; ++k;
	}

	// either a or b (or both) is odd
	while ((a & 1) == 0) a >>= 1;
	while ((b & 1) == 0) b >>= 1;

	// both a and b odd
	while (a != b) {
		if (a >= b) {
			a -= b;
			do { a >>= 1; } while ((a & 1) == 0);
		} else {
			b -= a;
			do { b >>= 1; } while ((b & 1) == 0);
		}
	}
	return b << k;
}

inline byte reverseByte(byte a)
{
	// classical implementation (can be extended to 16 and 32 bits)
	//   a = ((a & 0xF0) >> 4) | ((a & 0x0F) << 4);
	//   a = ((a & 0xCC) >> 2) | ((a & 0x33) << 2);
	//   a = ((a & 0xAA) >> 1) | ((a & 0x55) << 1);
	//   return a;

	// This only works for 8 bits (on a 32 bit machine) but it's slightly faster
	// Found trick on this page:
	//    http://graphics.stanford.edu/~seander/bithacks.html
	return ((a * 0x0802LU & 0x22110LU) | (a * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

/** Returns the smallest number of the form 2^n-1 that is greater or equal
  * to the given number.
  * The resulting number has the same number of leading zeros as the input,
  * but starting from the first 1-bit in the input all bits more to the right
  * are also 1.
  */
inline unsigned floodRight(unsigned x)
{
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x;
}

// Perform a 64bit divide-by 32-bit operation. The quotient must fit in
// 32-bit (results in a coredump otherwise)
inline unsigned div_64_32(unsigned long long dividend, unsigned divisor)
{
#ifdef ASM_X86
	unsigned quotient, remainder;
	asm (
		"divl %4;"
		: "=a"(quotient)
		, "=d"(remainder)
		: "a"(unsigned(dividend >>  0))
		, "d"(unsigned(dividend >> 32))
		, "rm"(divisor)
		: "cc"
	);
	return quotient;
#else
	return dividend / divisor;
#endif
}

// Perform a 64bit modulo 32-bit operation. The quotient must fit in
// 32-bit (results in a coredump otherwise)
inline unsigned mod_64_32(unsigned long long dividend, unsigned divisor)
{
#ifdef ASM_X86
	unsigned quotient, remainder;
	asm (
		"divl %4;"
		: "=a"(quotient)
		, "=d"(remainder)
		: "a"(unsigned(dividend >>  0))
		, "d"(unsigned(dividend >> 32))
		, "rm"(divisor)
		: "cc"
	);
	return remainder;
#else
	return dividend % divisor;
#endif
}

} // namespace Math

} // namespace openmsx

#endif // MATH_HH
