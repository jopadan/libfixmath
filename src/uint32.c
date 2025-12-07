#include "uint32.h"

uint32_t uint32_log2(uint32_t input)
{
    uint32_t retval = 0U;

    if (input != 0U)
    {
        uint32_t temp = input;
        if (temp >= ((uint32_t)1U << 16))
        {
            temp >>= 16U;
            retval += 16U;
        }

        if (temp >= ((uint32_t)1U << 8U))
        {
            temp >>= 8U;
            retval += 8U;
        }

        if (temp >= ((uint32_t)1U << 4U))
        {
            temp >>= 4U;
            retval += 4U;
        }

        if (temp >= ((uint32_t)1U << 2U))
        {
            temp >>= 2U;
            retval += 2U;
        }

        if (temp >= ((uint32_t)1U << 1U))
        {
            retval += 1U;
        }
    }

    return (retval);
}
