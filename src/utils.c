#define UMN_UTILS_IMPLEMENTATION
#include "./umn/utils.h"

#include <inttypes.h>

int main(void)
{
    umn_sb_t sb = {0};
    sb.capacity = 128 << 4;
    sb.items = malloc(sb.capacity);
    memset(sb.items, 'X', sb.capacity);

    const char *s = "Hello";
    umn_sb_pushf(&sb, "Strings:\n"); // same as puts("Strings");
    umn_sb_pushf(&sb, " padding:\n");
    umn_sb_pushf(&sb, "\t[%10s]\n", s);
    umn_sb_pushf(&sb, "\t[%-10s]\n", s);
    umn_sb_pushf(&sb, "\t[%*s]\n", 10, s);
    umn_sb_pushf(&sb, " truncating:\n");
    umn_sb_pushf(&sb, "\t%.4s\n", s);
    umn_sb_pushf(&sb, "\t%.*s\n", 3, s);

    umn_sb_pushf(&sb, "Characters:\t%c %%\n", 'A');
    umn_sb_pushf(&sb, "Integers:\n");

    umn_sb_pushf(&sb, "\tDecimal:\t%i %d %.6i %i %.0i %+i %i\n",
                 1, 2, 3, 0, 0, 4, -4);

    umn_sb_pushf(&sb, "\tHexadecimal:\t%x %x %X %#x\n", 5, 10, 10, 6);
    umn_sb_pushf(&sb, "\tOctal:\t\t%o %#o %#o\n", 10, 10, 4);

    if (0)
    {
        umn_sb_pushf(&sb, "Floating-point:\n");
        umn_sb_pushf(&sb, "\tRounding:\t%f %.0f %.32f\n", 1.5, 1.5, 1.3);
        umn_sb_pushf(&sb, "\tPadding:\t%05.2f %.2f %5.2f\n", 1.5, 1.5, 1.5);
        umn_sb_pushf(&sb, "\tScientific:\t%E %e\n", 1.5, 1.5);
        umn_sb_pushf(&sb, "\tHexadecimal:\t%a %A\n", 1.5, 1.5);
        umn_sb_pushf(&sb, "\tSpecial values:\t0/0=%g 1/0=%g\n", 0.0 / 0.0, 1.0 / 0.0);
    }
    umn_sb_pushf(&sb, "Fixed-width types:\n");
    umn_sb_pushf(&sb, "\tLargest 32-bit value is %" PRIu32 " or %#" PRIx32 "\n",
                 UINT32_MAX, UINT32_MAX);

    puts(sb.items), sb.count = 0;
    return 0;
}
