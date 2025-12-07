#include "fix16.h"
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdbool.h>
#endif
#if defined(FIXMATH_NO_CTYPE) || defined(__KERNEL__)
static inline int isdigit(int c)
{
    return c >= '0' && c <= '9';
}

static inline int isspace(int c)
{
    return c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\v' ||
           c == '\f';
}
#else
#include <ctype.h>
#endif

static char* itoa_loop(char* buf_start, uint32_t scale_start,
                       uint32_t value_start, bool skip_start)
{
    char*    buf   = buf_start;
    uint32_t scale = scale_start;
    uint32_t value = value_start;
    bool     skip  = skip_start;

    while (scale != 0U)
    {
        uint8_t digit = (uint8_t)(value / scale);

        if ((!skip) || (digit != 0U) || (scale == 1U))
        {
            skip        = false;

            uint8_t chr = ((uint8_t)'0' + digit);
            *buf        = (char)chr;
            buf++;

            value %= scale;
        }

        scale /= 10U;
    }

    return (buf);
}

uint32_t fix16_to_str(fix16_t value, char* buf_start, int decimals)
{
    char*    buf    = buf_start;

    uint32_t uvalue = (value >= 0) ? (uint32_t)value : (uint32_t)(-value);
    if (value < 0)
    {
        *buf = '-';
        buf++;
    }

    static const uint32_t scales[8] = {
        /* 5 decimals is enough for full fix16_t precision */
        1U, 10U, 100U, 1000U, 10000U, 100000U, 100000U, 100000U};

    /* Separate the integer and decimal parts of the value */
    unsigned intpart  = uvalue >> 16U;
    uint32_t fracpart = uvalue & 0xFFFFU;
    uint32_t scale    = scales[decimals & 7];

    fracpart = (uint32_t)fix16_mul((fix16_t)fracpart, (fix16_t)(scale));

    if (fracpart >= scale)
    {
        /* Handle carry from decimal part */
        intpart++;
        fracpart -= scale;
    }

    /* Format integer part */
    buf = itoa_loop(buf, 10000, intpart, true);

    /* Format decimal part (if any) */
    if (scale != 1U)
    {
        *buf = '.';
        buf++;

        buf = itoa_loop(buf, scale / 10U, fracpart, false);
    }

    *buf = '\0';

    return (buf - buf_start);
}

fix16_t fix16_from_str(const char* buf_start)
{
    const char* buf = buf_start;

    while (isspace((unsigned char)*buf) != 0)
    {
        buf++;
    }

    /* Decode the sign */
    bool negative = (*buf == '-');
    if ((*buf == '+') || (*buf == '-'))
    {
        buf++;
    }

    /* Decode the integer part */
    uint32_t intpart = 0;
    int      count   = 0;
    while (isdigit((unsigned char)*buf) != 0)
    {
        intpart *= 10;
        intpart += *buf - '0';
        buf++;
        count++;
    }

#ifdef FIXMATH_NO_OVERFLOW
    if (count == 0)
    {
        return (fix16_overflow);
    }
#else
    if ((count == 0) || (count > 5) || (intpart > 0x8000U) ||
        (!negative && (intpart > 0x7FFFU)))
    {
        return (fix16_overflow);
    }
#endif

    fix16_t value = (fix16_t)(intpart << 16);

    /* Decode the decimal part */
    if ((*buf == '.') || (*buf == ','))
    {
        buf++;

        uint32_t fracpart = 0U;
        uint32_t scale    = 1U;
        while (isdigit((unsigned char)*buf) && (scale < 100000U))
        {
            scale *= 10;
            fracpart *= 10;
            fracpart += *buf - '0';
            buf++;
        }

        value += fix16_div((fix16_t)fracpart, (fix16_t)scale);
    }

    /* Verify that there is no garbage left over */
    while (*buf != '\0')
    {
        if (!isdigit((unsigned char)*buf) && !isspace((unsigned char)*buf))
        {
            return (fix16_overflow);
        }

        buf++;
    }

    return negative ? -value : value;
}
