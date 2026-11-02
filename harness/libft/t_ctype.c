#include "../core.h"
#include "proto.h"

static const char *cdesc(int c) { return (c >= 32 && c < 127) ? pt_fmt(" '%c'", c) : ""; }
static int r_alpha(int c) { return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')); }
static int r_digit(int c) { return (c >= '0' && c <= '9'); }
static int r_alnum(int c) { return (r_alpha(c) || r_digit(c)); }
static int r_ascii(int c) { return (c >= 0 && c <= 127); }
static int r_print(int c) { return (c >= 32 && c <= 126); }
static int r_up(int c) { return ((c >= 'a' && c <= 'z') ? c - 32 : c); }
static int r_low(int c) { return ((c >= 'A' && c <= 'Z') ? c + 32 : c); }

#define IS_TEST(fn, ref) \
TEST(#fn, "ctype", "returns exactly 1 or 0 for 0..255 and EOF", T_SPEC) \
{ \
	HINT("the subject requires exactly 1 (match) or 0 (no match), not just a non-zero value"); \
	for (int c = -1; c < 256; c++) \
	{ \
		CALLING(#fn "(%d)", c); \
		EXPECT_INT(ref(c), fn(c), #fn "(%d%s)", c, cdesc(c)); \
	} \
} \
TEST(#fn, "ctype", "values outside unsigned char range (-300..600)", T_CONV) \
{ \
	for (int c = -300; c < 600; c++) \
	{ \
		CALLING(#fn "(%d)", c); \
		EXPECT_INT(ref(c), fn(c), #fn "(%d)", c); \
	} \
}

IS_TEST(ft_isalpha, r_alpha)
IS_TEST(ft_isdigit, r_digit)
IS_TEST(ft_isalnum, r_alnum)
IS_TEST(ft_isascii, r_ascii)
IS_TEST(ft_isprint, r_print)

#define CASE_TEST(fn, ref) \
TEST(#fn, "ctype", "converts only the right letters for 0..255 and EOF", T_SPEC) \
{ \
	for (int c = -1; c < 256; c++) \
	{ \
		CALLING(#fn "(%d)", c); \
		EXPECT_INT(ref(c), fn(c), #fn "(%d%s)", c, cdesc(c)); \
	} \
} \
TEST(#fn, "ctype", "values outside unsigned char range are returned unchanged", T_CONV) \
{ \
	for (int c = -300; c < 600; c++) \
	{ \
		CALLING(#fn "(%d)", c); \
		EXPECT_INT(ref(c), fn(c), #fn "(%d)", c); \
	} \
}

CASE_TEST(ft_toupper, r_up)
CASE_TEST(ft_tolower, r_low)
