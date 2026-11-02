"""Libft subject v19.3 as data: single source of truth for prototypes, categories, allowed externals."""

SPEC_VERSION = "19.3"

# (name, category, return type, params, allowed external (libc) symbols)
_F = [
    ("ft_isalpha", "ctype", "int", "int c", ()),
    ("ft_isdigit", "ctype", "int", "int c", ()),
    ("ft_isalnum", "ctype", "int", "int c", ()),
    ("ft_isascii", "ctype", "int", "int c", ()),
    ("ft_isprint", "ctype", "int", "int c", ()),
    ("ft_toupper", "ctype", "int", "int c", ()),
    ("ft_tolower", "ctype", "int", "int c", ()),
    ("ft_strlen", "string", "size_t", "const char *s", ()),
    ("ft_strchr", "string", "char *", "const char *s, int c", ()),
    ("ft_strrchr", "string", "char *", "const char *s, int c", ()),
    ("ft_strncmp", "string", "int", "const char *s1, const char *s2, size_t n", ()),
    ("ft_strnstr", "string", "char *", "const char *big, const char *little, size_t len", ()),
    ("ft_strlcpy", "string", "size_t", "char *dst, const char *src, size_t size", ()),
    ("ft_strlcat", "string", "size_t", "char *dst, const char *src, size_t size", ()),
    ("ft_memset", "memory", "void *", "void *s, int c, size_t n", ()),
    ("ft_bzero", "memory", "void", "void *s, size_t n", ()),
    ("ft_memcpy", "memory", "void *", "void *dest, const void *src, size_t n", ()),
    ("ft_memmove", "memory", "void *", "void *dest, const void *src, size_t n", ()),
    ("ft_memchr", "memory", "void *", "const void *s, int c, size_t n", ()),
    ("ft_memcmp", "memory", "int", "const void *s1, const void *s2, size_t n", ()),
    ("ft_calloc", "memory", "void *", "size_t nmemb, size_t size", ("malloc",)),
    ("ft_strdup", "memory", "char *", "const char *s", ("malloc",)),
    ("ft_atoi", "convert", "int", "const char *nptr", ()),
    ("ft_itoa", "convert", "char *", "int n", ("malloc",)),
    ("ft_substr", "alloc", "char *", "char const *s, unsigned int start, size_t len", ("malloc",)),
    ("ft_strjoin", "alloc", "char *", "char const *s1, char const *s2", ("malloc",)),
    ("ft_strtrim", "alloc", "char *", "char const *s1, char const *set", ("malloc",)),
    ("ft_split", "alloc", "char **", "char const *s, char c", ("malloc", "free")),
    ("ft_strmapi", "alloc", "char *", "char const *s, char (*f)(unsigned int, char)", ("malloc",)),
    ("ft_striteri", "alloc", "void", "char *s, void (*f)(unsigned int, char *)", ()),
    ("ft_putchar_fd", "output", "void", "char c, int fd", ("write",)),
    ("ft_putstr_fd", "output", "void", "char *s, int fd", ("write",)),
    ("ft_putendl_fd", "output", "void", "char *s, int fd", ("write",)),
    ("ft_putnbr_fd", "output", "void", "int n, int fd", ("write",)),
    ("ft_lstnew", "list", "t_list *", "void *content", ("malloc",)),
    ("ft_lstadd_front", "list", "void", "t_list **lst, t_list *new", ()),
    ("ft_lstsize", "list", "unsigned int", "t_list *lst", ()),
    ("ft_lstlast", "list", "t_list *", "t_list *lst", ()),
    ("ft_lstadd_back", "list", "void", "t_list **lst, t_list *new", ()),
    ("ft_lstdelone", "list", "void", "t_list *lst, void (*del)(void *)", ("free",)),
    ("ft_lstclear", "list", "void", "t_list **lst, void (*del)(void *)", ("free",)),
    ("ft_lstiter", "list", "void", "t_list *lst, void (*f)(void *)", ()),
    ("ft_lstmap", "list", "t_list *", "t_list *lst, void *(*f)(void *), void (*del)(void *)", ("malloc", "free")),
]

FUNCS = [dict(name=n, cat=c, ret=r, params=p, ext=set(e)) for n, c, r, p, e in _F]
BY_NAME = {f["name"]: f for f in FUNCS}
CATEGORIES = ["ctype", "string", "memory", "convert", "alloc", "output", "list"]

# symbols the toolchain may legitimately reference
ALLOWED_RUNTIME = {"__stack_chk_fail", "_GLOBAL_OFFSET_TABLE_", "__gmon_start__", "_ITM_registerTMCloneTable",
                   "_ITM_deregisterTMCloneTable", "__cxa_finalize", "__stack_chk_guard"}
LIBC_HINTS = {
    "memset": "the compiler may have turned a loop into a memset call (check optimisation flags) - or you called it directly",
    "memcpy": "you called memcpy (or the compiler lowered a copy loop) - the subject forbids libc inside your functions",
    "strlen": "use your own ft_strlen instead of the libc one",
    "bzero": "use your own ft_bzero / ft_memset",
    "calloc": "ft_calloc must use malloc, not calloc",
    "realloc": "realloc is not an allowed function",
    "printf": "printf/puts are not allowed (leftover debug print?)",
    "puts": "printf/puts are not allowed (leftover debug print?)",
    "putchar": "putchar is not allowed (leftover debug print?)",
    "free": "free is only allowed in ft_split, ft_lstdelone, ft_lstclear, ft_lstmap",
    "malloc": "malloc is only allowed in the functions that allocate",
    "write": "write is only allowed in the ft_put*_fd functions",
}


def proto(f):
    ret = f["ret"]
    sep = "" if ret.endswith("*") else " "
    return f"{ret}{sep}{f['name']}({f['params']});"


def gen_proto_h():
    """Harness-private prototypes (NOT the student's libft.h) + presence table."""
    out = ["#ifndef PT_PROTO_H", "#define PT_PROTO_H", "#include <stddef.h>",
           "typedef struct s_list { void *content; struct s_list *next; } t_list;"]
    for f in FUNCS:
        out.append("extern " + proto(f)[:-1] + " __attribute__((weak));")
    out.append("static inline int pt_fn_present(const char *name)\n{\n\tstatic const struct { const char *n; const void *p; } tab[] = {")
    for f in FUNCS:
        out.append(f'\t\t{{"{f["name"]}", (const void *){f["name"]}}},')
    out.append("\t};\n\tfor (unsigned i = 0; i < sizeof(tab) / sizeof(tab[0]); i++)\n"
               "\t\tif (!__builtin_strcmp(tab[i].n, name))\n\t\t\treturn (tab[i].p != 0);\n\treturn (1);\n}")
    out.append("#endif")
    return "\n".join(out) + "\n"


def gen_conformance_c():
    """Compiled together with the student's libft.h: any conflict means the header disagrees with the subject."""
    lines = ['#include "libft.h"']
    lines += [proto(f) for f in FUNCS]
    lines.append("int main(void) { t_list l; l.content = 0; l.next = (t_list *)0; return (int)(l.content != 0); }")
    return "\n".join(lines) + "\n"
