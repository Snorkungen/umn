#define UMN_CORE_IMPLEMENTATION

#include "umn/core.h"

#include <inttypes.h>
int main(void)
{
    /* umn_memfree(umn_memalloc(16));*/
    /* umn_memalloc(16);*/
    /* umn_memfree(umn_memalloc(16));*/
    /* umn_memfree(umn_memalloc(16));*/

    umn_printf("Hello, %s\n", "James");
    // umn_printf("Hello, World %f\n", 10.1);

    const char *s = "Hello";
    umn_printf("Strings:\n"); // same as puts("Strings");
    umn_printf(" padding:\n");
    umn_printf("\t[%10s]\n", s);
    umn_printf("\t[%-10s]\n", s);
    umn_printf("\t[%*s]\n", 10, s);
    umn_printf(" truncating:\n");
    umn_printf("\t%.4s\n", s);
    umn_printf("\t%.*s\n", 3, s);

    umn_printf("Characters:\t%c %%\n", 'A');
    umn_printf("Integers:\n");

    umn_printf("\tDecimal:\t%i %d %.6i %i %.0i %+i %i\n",
               1, 2, 3, 0, 0, 4, -4);

    umn_printf("\tHexadecimal:\t%x %x %X %#x\n", 5, 10, 10, 6);
    umn_printf("\tOctal:\t\t%o %#o %#o\n", 10, 10, 4);
    umn_printf("\tBinary:\t\t%b %#b %#b\n", 10, 10, 4);

    if (0)
    {
        umn_printf("Floating-point:\n");
        umn_printf("\tRounding:\t%f %.0f %.32f\n", 1.5, 1.5, 1.3);
        umn_printf("\tPadding:\t%05.2f %.2f %5.2f\n", 1.5, 1.5, 1.5);
        umn_printf("\tScientific:\t%E %e\n", 1.5, 1.5);
        umn_printf("\tHexadecimal:\t%a %A\n", 1.5, 1.5);
        umn_printf("\tSpecial values:\t0/0=%g 1/0=%g\n", 0.0 / 0.0, 1.0 / 0.0);
    }
    umn_printf("Fixed-width types:\n");
    umn_printf("\tLargest 32-bit value is %" PRIu32 " or %#" PRIx32 "\n",
               UINT32_MAX, UINT32_MAX);

    return 0;
}
