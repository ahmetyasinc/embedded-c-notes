/*
 * 04 - static
 *
 * One keyword, three jobs. They look unrelated until you separate the
 * three questions C asks about every variable:
 *
 *   scope     - where is this NAME visible?
 *   linkage   - can other .c files see this name at link time?
 *   storage   - how long does the memory live?
 *
 * 'static' answers a different one of those depending on where you put
 * it. Inside a function it changes STORAGE. At file scope it changes
 * LINKAGE. It never changes scope - that is decided by the braces.
 *
 * Build:  gcc -Wall -o 04-static 04-static.c
 * Run:    ./04-static
 * Look:   nm 04-static.o        (lowercase letter = static)
 *         size 04-static        (.data and .bss are the static ones)
 */

#include <stdio.h>
#include <stdint.h>

static void section(const char *title)
{
    printf("\n=== %s ===\n", title);
}

/* ------------------------------------------------------------------ */
/* 1. static inside a function: the memory outlives the call           */
/* ------------------------------------------------------------------ */

static int counts_nothing(void)
{
    int n = 0;      /* born on the stack at every call, dies on return */
    n++;
    return n;
}

static int counts_properly(void)
{
    static int n = 0;   /* born once, at program start, never dies */
    n++;
    return n;
}

static void persistence(void)
{
    section("1. persistence inside a function");

    for (int i = 0; i < 5; i++) {
        printf("call %d:  plain local = %d    static local = %d\n",
               i + 1, counts_nothing(), counts_properly());
    }

    /* The plain local is a fresh box on the stack each time, so it is
     * always 1. The static local is one box that exists for the entire
     * program, so it counts.
     *
     * Note what 'static int n = 0;' really means: the initialiser is NOT
     * an instruction that runs on entry. It is a value baked into the
     * program image before main() starts. The line runs zero times at
     * runtime, no matter how often the function is called. */
}

/* ------------------------------------------------------------------ */
/* 2. Where the memory actually lives                                  */
/* ------------------------------------------------------------------ */

static int   initialised_static   = 42;   /* goes to .data */
static int   uninitialised_static;        /* goes to .bss  */
static char  big_static_buffer[256];      /* goes to .bss  */

static void where_it_lives(void)
{
    int stack_variable = 7;

    section("2. where the memory lives");

    printf("stack variable        %p\n", (void *)&stack_variable);
    printf("initialised static    %p   (.data)\n", (void *)&initialised_static);
    printf("uninitialised static  %p   (.bss)\n", (void *)&uninitialised_static);
    printf("static buffer         %p   (.bss)\n", (void *)big_static_buffer);

    printf("\nuninitialised static = %d  <- guaranteed zero\n",
           uninitialised_static);

    /* Two things to read out of those addresses. The statics sit close
     * together and far away from the stack variable - they are in a
     * different region of memory entirely.
     *
     * And the zero is a promise, not luck. Anything static with no
     * initialiser goes into a section called .bss, which the startup code
     * clears to zero before main() runs. A plain local gets no such
     * treatment: it holds whatever the stack happened to contain.
     *
     * On an MCU this distinction is budget, not trivia. .data must be
     * COPIED from flash to RAM at boot, so it costs both. .bss costs only
     * RAM. A 256-byte buffer initialised to zeros belongs in .bss; write
     * '= {0}' on it and some toolchains will put 256 bytes of zeros in
     * your flash image for no reason.
     *
     * Run 'size 04-static' to see the three numbers. */
}

/* ------------------------------------------------------------------ */
/* 3. static at file scope: the name stops at this file                */
/* ------------------------------------------------------------------ */

int shared_with_other_files = 1;        /* external linkage - visible */
static int private_to_this_file = 2;    /* internal linkage - invisible */

static void linkage(void)
{
    section("3. linkage");

    printf("shared_with_other_files = %d\n", shared_with_other_files);
    printf("private_to_this_file    = %d\n", private_to_this_file);

    /* Nothing observable at runtime - this one is about the LINKER.
     *
     * Without static, the name is published in the object file's symbol
     * table. If another .c file defines a global with the same name, the
     * link fails with "multiple definition of ...". With static, the name
     * is not published, and two files may each have their own.
     *
     * You can see it directly:
     *
     *     gcc -c 04-static.c
     *     nm 04-static.o
     *
     * UPPERCASE letters mark global symbols, lowercase mark local ones:
     *     D  shared_with_other_files    published
     *     d  private_to_this_file       private
     *     T  a non-static function      published
     *     t  a static function          private
     *
     * The rule of thumb in embedded code: static by default, external
     * only when another file genuinely needs it. A published symbol is a
     * promise you did not mean to make. */
}

/* ------------------------------------------------------------------ */
/* 4. static functions                                                 */
/* ------------------------------------------------------------------ */

/* Same idea applied to a function name. Every function in these files is
 * static for this reason - and there is a second benefit on a small
 * target: because the compiler can see every call site, it may inline a
 * static function, or delete it entirely if nobody calls it. A published
 * function has to be kept, because some other file might want it. */

static void helper_nobody_outside_can_call(void)
{
    puts("called from inside this file only");
}

static void static_functions(void)
{
    section("4. static functions");
    helper_nobody_outside_can_call();
}

/* ------------------------------------------------------------------ */
/* 5. The trap: a static local is shared by every caller               */
/* ------------------------------------------------------------------ */

/* This looks harmless. It returns a text version of a byte, using a
 * static buffer so the memory survives the return. */
static const char *to_binary(uint8_t value)
{
    static char buffer[9];      /* 8 digits + the terminating zero */

    for (int i = 0; i < 8; i++) {
        buffer[i] = ((value >> (7 - i)) & 1u) ? '1' : '0';
    }
    buffer[8] = '\0';

    return buffer;
}

static void the_trap(void)
{
    section("5. one buffer, two callers");

    printf("first  call alone: %s\n", to_binary(0b00000101));
    printf("second call alone: %s\n", to_binary(0b11110000));

    /* Now ask for both in a single call: */
    printf("both at once:      %s and %s\n",
           to_binary(0b00000101), to_binary(0b11110000));

    puts("");
    puts("Both arguments printed the SAME text. There is only one buffer,");
    puts("both calls wrote into it, and printf received two copies of the");
    puts("same address - so whichever call ran last decided what you see.");
    puts("(Which one runs first is not even specified by the language.)");

    /* The same flaw with worse consequences on hardware: if a function
     * holding static state is called from main AND from an interrupt
     * handler, the interrupt can land mid-update and both callers end up
     * working with a corrupted half-written value. A function whose
     * behaviour depends on nothing but its arguments is called reentrant,
     * and a static local is the usual reason a function is not.
     *
     * Recursion breaks for the same reason: every level of the recursion
     * shares one variable instead of getting its own.
     *
     * The fix is to let the caller own the memory:
     *
     *     void to_binary(uint8_t value, char *out);
     *
     * Now two callers pass two different buffers and the problem is gone.
     * This is the same idea as min_max() in file 01 - results travel back
     * through a pointer the caller supplies. */
}

int main(void)
{
    persistence();
    where_it_lives();
    linkage();
    static_functions();
    the_trap();

    printf("\n");
    return 0;
}
