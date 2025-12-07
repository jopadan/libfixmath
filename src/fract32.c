#include "fract32.h"

fract32_t fract32_create(uint32_t inNumerator, uint32_t inDenominator)
{
    fract32_t retval = 0xFFFFFFFFU;

    if (inDenominator > inNumerator)
    {
        uint32_t tempMod = (inNumerator % inDenominator);
        uint32_t tempDiv = (0xFFFFFFFFU / (inDenominator - 1U));
        retval           = (tempMod * tempDiv);
    }

    return retval;
}

fract32_t fract32_invert(fract32_t inFract)
{
    return (0xFFFFFFFFU - inFract);
}

#ifndef FIXMATH_NO_64BIT
uint32_t fract32_usmul(uint32_t inVal, fract32_t inFract)
{
    return (uint32_t)(((uint64_t)inVal * (uint64_t)inFract) >> 32);
}

int32_t fract32_smul(int32_t inVal, fract32_t inFract)
{
    int32_t retval;

    if (inVal < 0)
    {
        retval = -(int32_t)fract32_usmul((uint32_t)(-inVal), inFract);
    }
    else
    {
        retval = ((int32_t)fract32_usmul((uint32_t)inVal, inFract));
    }

    return (retval);
}
#endif
