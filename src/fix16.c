#include "fix16.h"
#include "int64.h"

////////////////////////////////////////////////////////////////////////////////
// STATIC INLINE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Perform signed shift right operation on 32-bit integer. This has been
 * separated to suppress cppcheck/MISRA warnings about shifting signed integers,
 * which are unpredictable depending on implementation, but work predictably on
 * ARM.
 *
 * @param value Value to shift
 * @param shift Number of bits to shift
 *
 * @return int32_t Shifted value
 */
static inline int32_t signed_shift_right(int32_t value, int32_t shift)
{
    int32_t retval;

    // NOTE: ARM processors will perform this shift predictably
    // cppcheck-suppress-begin misra-c2012-10.1
    if (shift < 0)
    {
        retval = (value << -shift);
    }
    else
    {
        retval = (value >> shift);
    }
    // cppcheck-suppress-end misra-c2012-10.1

    return (retval);
}

/**
 * @brief Perform signed shift left operation on 32-bit integer. This has been
 * separated to suppress cppcheck/MISRA warnings about shifting signed integers,
 * which are unpredictable depending on implementation, but work predictably on
 * ARM.
 *
 * @param value Value to shift
 * @param shift Number of bits to shift
 *
 * @return int32_t Shifted value
 */
static inline int32_t signed_shift_left(int32_t value, int32_t shift)
{
    int32_t retval;

    // NOTE: ARM processors will perform this shift predictably
    // cppcheck-suppress-begin misra-c2012-10.1
    if (shift < 0)
    {
        retval = (value >> -shift);
    }
    else
    {
        retval = (value << shift);
    }
    // cppcheck-suppress-end misra-c2012-10.1

    return (retval);
}

/* Subtraction and addition with overflow detection.
 * The versions without overflow detection are inlined in the header.
 */
#ifndef FIXMATH_NO_OVERFLOW
fix16_t fix16_add(fix16_t a, fix16_t b)
{
    // Use unsigned integers because overflow with signed integers is
    // an undefined operation (http://www.airs.com/blog/archives/120).
    uint32_t _a     = (uint32_t)a;
    uint32_t _b     = (uint32_t)b;
    uint32_t sum    = _a + _b;

    fix16_t  retval = (fix16_t)sum;

    // Overflow can only happen if sign of a == sign of b, and then
    // it causes sign of sum != sign of a.
    if (!((_a ^ _b) & 0x80000000U) && ((_a ^ sum) & 0x80000000U))
    {
        retval = fix16_overflow;
    }

    return (retval);
}

fix16_t fix16_sub(fix16_t a, fix16_t b)
{
    uint32_t _a     = (uint32_t)a;
    uint32_t _b     = (uint32_t)b;
    uint32_t diff   = _a - _b;

    fix16_t  retval = (fix16_t)diff;

    // Overflow can only happen if sign of a != sign of b, and then
    // it causes sign of diff != sign of a.
    if (((_a ^ _b) & 0x80000000U) && ((_a ^ diff) & 0x80000000U))
    {
        retval = fix16_overflow;
    }

    return (retval);
}

/* Saturating arithmetic */
fix16_t fix16_sadd(fix16_t a, fix16_t b)
{
    fix16_t result = fix16_add(a, b);

    if (result == fix16_overflow)
    {
        result = (a >= 0) ? fix16_maximum : fix16_minimum;
    }

    return (result);
}

fix16_t fix16_ssub(fix16_t a, fix16_t b)
{
    fix16_t result = fix16_sub(a, b);

    if (result == fix16_overflow)
    {
        result = (a >= 0) ? fix16_maximum : fix16_minimum;
    }

    return (result);
}
#endif

/* 64-bit implementation for fix16_mul. Fastest version for e.g. ARM Cortex M3.
 * Performs a 32*32 -> 64bit multiplication. The middle 32 bits are the result,
 * bottom 16 bits are used for rounding, and upper 16 bits are used for overflow
 * detection.
 */

#if !defined(FIXMATH_NO_64BIT) && !defined(FIXMATH_OPTIMIZE_8BIT)
fix16_t fix16_mul(fix16_t inArg0, fix16_t inArg1)
{
    int64_t product = (int64_t)inArg0 * inArg1;

#ifndef FIXMATH_NO_OVERFLOW
    // The upper 17 bits should all be the same (the sign).
    uint32_t upper = (product >> 47U);
#endif

    if (product < 0)
    {
#ifndef FIXMATH_NO_OVERFLOW
        if (upper != 0xFFFFFFFFU)
        {
            return (fix16_overflow);
        }
#endif

#ifndef FIXMATH_NO_ROUNDING
        // This adjustment is required in order to round -1/2 correctly
        product--;
#endif
    }
    else
    {
#ifndef FIXMATH_NO_OVERFLOW
        if (upper != 0U)
        {
            return (fix16_overflow);
        }
#endif
    }

#ifdef FIXMATH_NO_ROUNDING
    return product >> 16U;
#else
    fix16_t result = product >> 16U;
    result += (product & 0x8000) >> 15U;

    return (result);
#endif
}
#endif

/* 32-bit implementation of fix16_mul. Potentially fast on 16-bit processors,
 * and this is a relatively good compromise for compilers that do not support
 * uint64_t. Uses 16*16->32bit multiplications.
 */
#if defined(FIXMATH_NO_64BIT) && !defined(FIXMATH_OPTIMIZE_8BIT)
fix16_t fix16_mul(fix16_t inArg0, fix16_t inArg1)
{
    // Each argument is divided to 16-bit parts.
    //					AB
    //			*	 CD
    // -----------
    //					BD	16 * 16 -> 32 bit products
    //				 CB
    //				 AD
    //				AC
    //			 |----| 64 bit product
    int32_t  A          = signed_shift_right(inArg0, 16);
    int32_t  C          = signed_shift_right(inArg1, 16);
    uint32_t B          = ((uint32_t)inArg0 & 0xFFFFU);
    uint32_t D          = ((uint32_t)inArg1 & 0xFFFFU);

    int32_t  AC         = A * C;
    uint32_t BD         = B * D;

    int32_t  AD         = A * (int32_t)D;
    int32_t  CB         = C * (int32_t)B;
    int32_t  AD_CB      = AD + CB;

    int32_t  product_hi = AC + signed_shift_right(AD_CB, 16);

    // Handle carry from lower 32 bits to upper part of result.
    uint32_t ad_cb_temp = (uint32_t)AD_CB << 16U;
    uint32_t product_lo = BD + ad_cb_temp;
    if (product_lo < BD)
    {
        product_hi++;
    }

    fix16_t retval = 0;

#ifndef FIXMATH_NO_OVERFLOW
    // The upper 17 bits should all be the same (the sign).
    if (product_hi >> 31U != product_hi >> 15U)
        return fix16_overflow;
#endif

#ifdef FIXMATH_NO_ROUNDING

    retval = (product_hi << 16) | (product_lo >> 16U);

#else

    if (retval != fix16_overflow)
    {
        // Subtracting 0x8000 (= 0.5) and then using signed right shift
        // achieves proper rounding to result-1, except in the corner
        // case of negative numbers and lowest word = 0x8000.
        // To handle that, we also have to subtract 1 for negative numbers.
        uint32_t product_lo_tmp = product_lo;
        product_lo -= 0x8000U;
        product_lo -= (uint32_t)product_hi >> 31U;
        if (product_lo > product_lo_tmp)
        {
            product_hi--;
        }

        // Discard the lowest 16 bits. Note that this is not exactly the same
        // as dividing by 0x10000. For example if product = -1, result will
        // also be -1 and not 0. This is compensated by adding +1 to the result
        // and compensating this in turn in the rounding above.
        fix16_t  result     = signed_shift_left(product_hi, 16);

        uint32_t lo_shifted = product_lo >> 16U;
        result |= (fix16_t)(lo_shifted);

        retval = result + 1;
    }
#endif

    return (retval);
}
#endif

/* 8-bit implementation of fix16_mul. Fastest on e.g. Atmel AVR.
 * Uses 8*8->16bit multiplications, and also skips any bytes that
 * are zero.
 */
#if defined(FIXMATH_OPTIMIZE_8BIT)
fix16_t fix16_mul(fix16_t inArg0, fix16_t inArg1)
{
    uint32_t _a    = fix_abs(inArg0);
    uint32_t _b    = fix_abs(inArg1);

    uint8_t  va[4] = {_a, (_a >> 8U), (_a >> 16U), (_a >> 24U)};
    uint8_t  vb[4] = {_b, (_b >> 8U), (_b >> 16U), (_b >> 24U)};

    uint32_t low   = 0;
    uint32_t mid   = 0;

    // Result column i depends on va[0..i] and vb[i..0]

#ifndef FIXMATH_NO_OVERFLOW
    // i = 6
    if ((va[3] && vb[3]) != 0U)
    {
        return (fix16_overflow);
    }
#endif

    // i = 5
    if ((va[2] && vb[3]) != 0U)
    {
        mid += (uint16_t)va[2] * vb[3];
    }
    if ((va[3] && vb[2]) != 0U)
    {
        mid += (uint16_t)va[3] * vb[2];
    }
    mid <<= 8;

    // i = 4
    if ((va[1] && vb[3]) != 0U)
    {
        mid += (uint16_t)va[1] * vb[3];
    }
    if ((va[2] && vb[2]) != 0U)
    {
        mid += (uint16_t)va[2] * vb[2];
    }
    if ((va[3] && vb[1]) != 0U)
    {
        mid += (uint16_t)va[3] * vb[1];
    }

#ifndef FIXMATH_NO_OVERFLOW
    if (mid & 0xFF000000U)
    {
        return (fix16_overflow);
    }
#endif
    mid <<= 8;

    // i = 3
    if ((va[0] && vb[3]) != 0U)
    {
        mid += (uint16_t)va[0] * vb[3];
    }
    if ((va[1] && vb[2]) != 0U)
    {
        mid += (uint16_t)va[1] * vb[2];
    }
    if ((va[2] && vb[1]) != 0U)
    {
        mid += (uint16_t)va[2] * vb[1];
    }
    if ((va[3] && vb[0]) != 0U)
    {
        mid += (uint16_t)va[3] * vb[0];
    }

#ifndef FIXMATH_NO_OVERFLOW
    if (mid & 0xFF000000U)
    {
        return (fix16_overflow);
    }
#endif
    mid <<= 8;

    // i = 2
    if ((va[0] && vb[2]) != 0U)
    {
        mid += (uint16_t)va[0] * vb[2];
    }
    if ((va[1] && vb[1]) != 0U)
    {
        mid += (uint16_t)va[1] * vb[1];
    }
    if ((va[2] && vb[0]) != 0U)
    {
        mid += (uint16_t)va[2] * vb[0];
    }

    // i = 1
    if ((va[0] && vb[1]) != 0U)
    {
        low += (uint16_t)va[0] * vb[1];
    }
    if ((va[1] && vb[0]) != 0U)
    {
        low += (uint16_t)va[1] * vb[0];
    }
    low <<= 8;

    // i = 0
    if ((va[0] && vb[0]) != 0U)
    {
        low += (uint16_t)va[0] * vb[0];
    }
#ifndef FIXMATH_NO_ROUNDING
    low += 0x8000;
#endif
    mid += (low >> 16U);

#ifndef FIXMATH_NO_OVERFLOW
    if (mid & 0x80000000U)
    {
        return (fix16_overflow);
    }
#endif

    fix16_t result = mid;

    /* Figure out the sign of result */
    if ((inArg0 >= 0) != (inArg1 >= 0))
    {
        result = -result;
    }

    return (result);
}
#endif

#ifndef FIXMATH_NO_OVERFLOW
/* Wrapper around fix16_mul to add saturating arithmetic. */
fix16_t fix16_smul(fix16_t inArg0, fix16_t inArg1)
{
    fix16_t result = fix16_mul(inArg0, inArg1);

    if (result == fix16_overflow)
    {
        if ((inArg0 >= 0) == (inArg1 >= 0))
        {
            result = fix16_maximum;
        }
        else
        {
            result = fix16_minimum;
        }
    }

    return (result);
}
#endif

/* 32-bit implementation of fix16_div. Fastest version for e.g. ARM Cortex M3.
 * Performs 32-bit divisions repeatedly to reduce the remainder. For this to
 * be efficient, the processor has to have 32-bit hardware division.
 */
#if !defined(FIXMATH_NO_HARD_DIVISION)
#ifdef __GNUC__
// Count leading zeros, using processor-specific instruction if available.
#define clz(x) (__builtin_clzl(x) - (8 * sizeof(long) - 32))
#else
static uint8_t clz_fn(uint32_t x)
{
    uint8_t result = 0;
    if (x == 0U)
    {
        return (32U);
    }

    while (!(x & 0xF0000000U))
    {
        result += 4U;
        x <<= 4U;
    }

    while (!(x & 0x80000000U))
    {
        result += 1U;
        x <<= 1U;
    }

    return (result);
}

#define clz(x) clz_fn(x)
#endif

fix16_t fix16_div(fix16_t a, fix16_t b)
{
    // This uses a hardware 32/32 bit division multiple times, until we have
    // computed all the bits in (a<<17)/b. Usually this takes 1-3 iterations.

    if (b == 0)
    {
        return (fix16_minimum);
    }

    uint32_t remainder = fix_abs(a);
    uint32_t divider   = fix_abs(b);
    uint64_t quotient  = 0;
    int      bit_pos   = 17;

    // Kick-start the division a bit.
    // This improves speed in the worst-case scenarios where N and D are large
    // It gets a lower estimate for the result by N/(D >> 17U + 1).
    if (divider & 0xFFF00000U)
    {
        uint32_t shifted_div = ((divider >> 17U) + 1);
        quotient             = remainder / shifted_div;
        uint64_t tmp         = ((uint64_t)quotient * (uint64_t)divider) >> 17U;
        remainder -= (uint32_t)(tmp);
    }

    // If the divider is divisible by 2^n, take advantage of it.
    while (!(divider & 0xF) && bit_pos >= 4)
    {
        divider >>= 4;
        bit_pos -= 4;
    }

    while (remainder && bit_pos >= 0)
    {
        // Shift remainder as much as we can without overflowing
        int shift = clz(remainder);
        if (shift > bit_pos)
        {
            shift = bit_pos;
        }
        remainder <<= shift;
        bit_pos -= shift;

        uint32_t div = remainder / divider;
        remainder    = remainder % divider;
        quotient += (uint64_t)div << bit_pos;

#ifndef FIXMATH_NO_OVERFLOW
        if (div & ~(0xFFFFFFFFU >> bit_pos))
        {
            return (fix16_overflow);
        }
#endif

        remainder <<= 1;
        bit_pos--;
    }

#ifndef FIXMATH_NO_ROUNDING
    // Quotient is always positive so rounding is easy
    quotient++;
#endif

    fix16_t result = quotient >> 1U;

    // Figure out the sign of the result
    if ((a ^ b) & 0x80000000U)
    {
#ifndef FIXMATH_NO_OVERFLOW
        if (result == fix16_minimum)
        {
            return (fix16_overflow);
        }
#endif

        result = -result;
    }

    return (result);
}
#endif /* !defined(FIXMATH_NO_HARD_DIVISION) */

/* Alternative 32-bit implementation of fix16_div. Fastest on e.g. Atmel AVR.
 * This does the division manually, and is therefore good for processors that
 * do not have hardware division.
 */
#if defined(FIXMATH_NO_HARD_DIVISION)
fix16_t fix16_div(fix16_t a, fix16_t b)
{
    // This uses the basic binary restoring division algorithm.
    // It appears to be faster to do the whole division manually than
    // trying to compose a 64-bit divide out of 32-bit divisions on
    // platforms without hardware divide.

    fix16_t retval = fix16_minimum;
    if (b != 0)
    {
        uint32_t remainder = fix_abs(a);
        uint32_t divider   = fix_abs(b);

        uint32_t quotient  = 0U;
        uint32_t bit       = 0x10000U;

        /* The algorithm requires D >= R */
        while (divider < remainder)
        {
            divider <<= 1;
            bit <<= 1;
        }

#ifndef FIXMATH_NO_OVERFLOW
        if (!bit)
        {
            return (fix16_overflow);
        }
#endif

        if ((divider & 0x80000000U) != 0U)
        {
            // Perform one step manually to avoid overflows later.
            // We know that divider's bottom bit is 0 here.
            if (remainder >= divider)
            {
                quotient |= bit;
                remainder -= divider;
            }
            divider >>= 1;
            bit >>= 1;
        }

        /* Main division loop */
        while (bit && remainder)
        {
            if (remainder >= divider)
            {
                quotient |= bit;
                remainder -= divider;
            }

            remainder <<= 1;
            bit >>= 1;
        }

#ifndef FIXMATH_NO_ROUNDING
        if (remainder >= divider)
        {
            quotient++;
        }
#endif

        fix16_t result = (fix16_t)quotient;

        /* Figure out the sign of result */
        if (((a ^ b) & (fix16_t)0x80000000U) != 0)
        {
#ifndef FIXMATH_NO_OVERFLOW
            if (result == fix16_minimum)
            {
                return (fix16_overflow);
            }
#endif

            result = -result;
        }

        retval = result;
    }

    return (retval);
}
#endif /* defined(FIXMATH_NO_HARD_DIVISION) */

#ifndef FIXMATH_NO_OVERFLOW
/* Wrapper around fix16_div to add saturating arithmetic. */
fix16_t fix16_sdiv(fix16_t inArg0, fix16_t inArg1)
{
    fix16_t result = fix16_div(inArg0, inArg1);

    if (result == fix16_overflow)
    {
        if ((inArg0 >= 0) == (inArg1 >= 0))
        {
            result = fix16_maximum;
        }
        else
        {
            result = fix16_minimum;
        }
    }

    return (result);
}
#endif

fix16_t fix16_mod(fix16_t x, fix16_t y)
{
#ifdef FIXMATH_NO_HARD_DIVISION
    /* The reason we do this, rather than use a modulo operator
     * is that if you don't have a hardware divider, this will result
     * in faster operations when the angles are close to the bounds.
     */

    fix16_t x_calc = x;
    fix16_t y_calc = y;

    while (x_calc >= y_calc)
    {
        x_calc -= y_calc;
    }
    while (x_calc <= -y_calc)
    {
        x_calc += y_calc;
    }
#else
    /* Note that in C90, the sign of result of the modulo operation is
     * undefined. in C99, it's the same as the dividend (aka numerator).
     */
    fix16_t x_calc = x % y;
#endif

    return (x_calc);
}

fix16_t fix16_lerp8(fix16_t inArg0, fix16_t inArg1, uint8_t inFract)
{
    uint32_t temp_u = (uint32_t)1U << 8U;
    int32_t  temp1  = (int32_t)(temp_u);
    temp1 -= (int32_t)inFract;

    int64_t tempOut = int64_mul_i32_i32(inArg0, temp1);
    tempOut         = int64_add(tempOut, int64_mul_i32_i32(inArg1, inFract));
    tempOut         = int64_shift(tempOut, -8);
    return ((fix16_t)int64_lo(tempOut));
}

fix16_t fix16_lerp16(fix16_t inArg0, fix16_t inArg1, uint16_t inFract)
{
    uint32_t temp_u = (uint32_t)1U << 16U;
    int32_t  temp1  = (int32_t)(temp_u);
    temp1 -= (int32_t)inFract;

    int64_t tempOut = int64_mul_i32_i32(inArg0, temp1);
    tempOut         = int64_add(tempOut, int64_mul_i32_i32(inArg1, inFract));
    tempOut         = int64_shift(tempOut, -16);
    return ((fix16_t)int64_lo(tempOut));
}

fix16_t fix16_lerp32(fix16_t inArg0, fix16_t inArg1, uint32_t inFract)
{
    fix16_t retval = inArg0;
    if (inFract != 0U)
    {
        int64_t inFract64 = int64_const(0, inFract);
        int64_t subbed    = int64_sub(int64_const(1, 0), inFract64);
        int64_t temp_64   = int64_mul_i64_i32(subbed, inArg0);
        temp_64 = int64_add(temp_64, int64_mul_i64_i32(inFract64, inArg1));
        retval  = (fix16_t)int64_hi(temp_64);
    }

    return (retval);
}

// CUSTOM FUNCTIONS ////////////////////////////////////////////////////////////

// Assert on overflow fns //////////////////////////////////////////////////////

fix16_t fix16_aadd(fix16_t a, fix16_t b)
{
    fix16_t result = fix16_add(a, b);

    if (result == fix16_overflow)
    {
        fix16_assert(0);
    }

    return (result);
}

fix16_t fix16_asub(fix16_t a, fix16_t b)
{
    fix16_t result = fix16_sub(a, b);

    if (result == fix16_overflow)
    {
        fix16_assert(0);
    }

    return (result);
}

fix16_t fix16_amul(fix16_t a, fix16_t b)
{
    fix16_t result = fix16_mul(a, b);

    if (result == fix16_overflow)
    {
        fix16_assert(0);
    }

    return (result);
}

fix16_t fix16_adiv(fix16_t a, fix16_t b)
{
    fix16_t result = fix16_div(a, b);

    if (result == fix16_overflow)
    {
        fix16_assert(0);
    }

    return (result);
}

// Custom large int division fns
///////////////////////////////////////////////

#ifdef FIXMATH_NO_64BIT

/**
 * @brief Perform [a * b] calculation with overflow detection.
 *
 * @param afix16 Fixed point value for a.
 * @param b32 Signed 32-bit integer value for b.
 *
 * @return fix16_t Fixed point result. Returns 0x80000000U on overflow.
 */
fix16_t fix16_amul_int32(fix16_t afix16, int32_t b32)
{
    int64_t result = int64_mul_i32_i32(afix16, b32);

    fix16_t retval = (fix16_t)result.lo;

    // High word should be 0x00000000 or 0xFFFFFFFFU
    if ((result.hi != 0) && ((uint32_t)result.hi != 0xFFFFFFFFU))
    {
        retval = fix16_overflow;
    }
    else
    {
        // MSB of low word must match bits of high word
        if (((uint32_t)result.hi & 0x80000000U) != (result.lo & 0x80000000U))
        {
            retval = fix16_overflow;
        }
    }

    if (retval == fix16_overflow)
    {
        fix16_assert(0);
        // printf("ASSERT!\t\t");
    }

    return (retval);
}

/**
 * @brief Perform [a * b / c] calculation with overflow detection.
 *
 * @param afix16 Fixed point value for a.
 * @param b32 Signed 32-bit integer value for b.
 * @param c32 Signed 32-bit integer value for c.
 *
 * @return fix16_t Fixed point result. Returns 0x80000000U on overflow.
 */
fix16_t fix16_axb_c(fix16_t afix16, int32_t b32, int32_t c32)
{
    fix16_t retval = fix16_overflow;

    if (c32 != 0)
    {
        int64_t a_x_b  = int64_mul_i32_i32(afix16, b32);
        int64_t result = int64_div_i64_i32(a_x_b, c32);

        retval         = (fix16_t)result.lo;

        // High word should be 0x00000000 or 0xFFFFFFFFU
        if ((result.hi != 0) && ((uint32_t)result.hi != 0xFFFFFFFFU))
        {
            retval = fix16_overflow;
        }
        else
        {
            // MSB of low word must match bits of high word
            if (((uint32_t)result.hi & 0x80000000U) !=
                (result.lo & 0x80000000U))
            {
                retval = fix16_overflow;
            }
        }

        if (retval == fix16_overflow)
        {
            fix16_assert(0);
        }
    }

    return (retval);
}

fix16_t fix16_div_big_int(int32_t a32, int32_t b32)
{
    fix16_t retval = fix16_overflow;

    // division by zero
    if (b32 != 0)
    {
        int64_t a64       = int64_from_int32(a32);
        a64               = int64_shift(a64, 32);

        int64_t result_64 = int64_div_i64_i32(a64, b32);

        if ((result_64.hi <= INT16_MAX) && (result_64.hi >= INT16_MIN))
        {
            retval              = signed_shift_left(result_64.hi, 16);
            uint32_t lo_shifted = result_64.lo >> 16U;
            retval |= (fix16_t)lo_shifted;
        }
        else
        {
            fix16_assert(0);
        }
    }

    return (retval);
}

fix16_t fix16_div_huge_int(int32_t a64_hi, uint32_t a64_lo, int32_t b32)
{

    fix16_t retval = fix16_overflow;

    // division by zero
    if (b32 != 0)
    {
        int64_t a64       = {.hi = a64_hi, .lo = a64_lo};
        a64               = int64_shift(a64, 24);

        int64_t result_64 = int64_div_i64_i32(a64, b32);

        if ((result_64.hi <= INT8_MAX) && (result_64.hi >= INT8_MIN))
        {
            retval              = (fix16_t)signed_shift_left(result_64.hi, 24);
            uint32_t lo_shifted = result_64.lo >> 8U;
            retval |= (fix16_t)lo_shifted;
        }
        else
        {
            fix16_assert(0);
        }
    }

    return (retval);
}

#endif /* FIXMATH_NO_64BIT */

/*** end of file ***/
