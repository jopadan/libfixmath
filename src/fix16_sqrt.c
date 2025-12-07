#include "fix16.h"

/* The square root algorithm is quite directly from
 * http://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Binary_numeral_system_.28base_2.29
 * An important difference is that it is split to two parts
 * in order to use only 32-bit operations.
 *
 * Note that for negative numbers we return -sqrt(-inValue).
 * Not sure if someone relies on this behaviour, but not going
 * to break it for now. It doesn't slow the code much overall.
 */
fix16_t fix16_sqrt(fix16_t inValue)
{
    uint8_t  neg    = (inValue < 0) ? (uint8_t)1U : (uint8_t)0U;
    uint32_t num    = fix_abs(inValue);
    uint32_t result = 0;
    uint32_t bit;
    uint8_t  n;

    // Many numbers will be less than 15, so
    // this gives a good balance between time spent
    // in if vs. time spent in the while loop
    // when searching for the starting value.
    if ((num & 0xFFF00000U) != 0U)
    {
        bit = (uint32_t)1U << 30U;
    }
    else
    {
        bit = (uint32_t)1U << 18U;
    }

    while (bit > num)
    {
        bit >>= 2U;
    }

    // The main part is executed twice, in order to avoid
    // using 64 bit values in computations.
    for (n = 0; n < 2U; n++)
    {
        // First we get the top 24 bits of the answer.
        while (bit != 0U)
        {
            if (num >= (result + bit))
            {
                num -= result + bit;
                result = (result >> 1U) + bit; ///
            }
            else
            {
                result = (result >> 1U);
            }
            bit >>= 2U;
        }

        if (n == 0U)
        {
            // Then process it again to get the lowest 8 bits.
            if (num > 65535U)
            {
                // The remainder 'num' is too large to be shifted left
                // by 16, so we have to add 1 to result manually and
                // adjust 'num' accordingly.
                // num = a - (result + 0.5)^2
                //	 = num + result^2 - (result + 0.5)^2
                //	 = num - result - 0.5
                num -= result;
                num    = (num << 16U) - 0x8000U;
                result = (result << 16U) + 0x8000U;
            }
            else
            {
                num <<= 16U;
                result <<= 16U;
            }

            bit = (uint32_t)1U << 14U;
        }
    }

#ifndef FIXMATH_NO_ROUNDING
    // Finally, if next bit would have been 1, round the result upwards.
    if (num > result)
    {
        result++;
    }
#endif

    return (neg ? -(fix16_t)result : (fix16_t)result);
}
