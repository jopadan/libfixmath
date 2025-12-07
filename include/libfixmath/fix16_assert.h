#ifndef libfixmath_fix16_assert_h__
#define libfixmath_fix16_assert_h__

#include <stdint.h>

// Assert function
/**
 * @brief  The assert_param macro is used for functions parameters check.
 * @param  expr If expr is false, it calls fix16_assert_failed function
 *         which reports the name of the source file and the source
 *         line number of the call that failed.
 *         If expr is true, it returns no value.
 * @retval None
 */
#define fix16_assert(expr)                                                     \
    ((expr) ? (void)0U                                                         \
            : fix16_assert_failed((const uint8_t*)__FILE__, __LINE__))
/* Exported functions
 * ------------------------------------------------------- */
void fix16_assert_failed(const uint8_t* file, uint32_t line);

#endif
