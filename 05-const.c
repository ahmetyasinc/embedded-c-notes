/*
 * 05 - const
 *
 * const is a promise you make to the compiler: "I will not write to this."
 * The compiler then holds you to it, and refuses to build code that breaks
 * the promise. It is a compile-time contract, not a lock on the memory.
 *
 * The whole difficulty is one question. When const appears next to a '*',
 * does it protect the POINTER, or the thing the pointer POINTS AT?
 *
 * Build:  gcc -Wall -o 05-const 05-const.c
 * Run:    ./05-const
 * Look:   nm 05-const.o     (letter 'r' = read-only data)
 */

#include <stdio.h>
#include <stdint.h>

static void section(const char *title)
{
    printf("\n=== %s ===\n", title);
}

/* ------------------------------------------------------------------ */
/* 1. const on a plain variable                                        */
/* ------------------------------------------------------------------ */

static void basics(void)
{
    const int limit = 100;
    int normal = 5;

    section("1. the promise");

    printf("limit  = %d   (const - cannot be assigned to)\n", limit);
    printf("normal = %d   (ordinary)\n", normal);

    /* Uncomment this line and the build fails:
     *
     *     limit = 200;
     *     error: assignment of read-only variable 'limit'
     *
     * Notice WHEN it fails. Not at run time - the program never even
     * gets built. That is the point of const: the mistake is caught
     * while you are typing, not on the bench at 2am. */

    normal = 6;
    printf("normal changed to %d\n", normal);
}

/* ------------------------------------------------------------------ */
/* 2. The three pointer forms                                          */
/* ------------------------------------------------------------------ */

static void pointer_forms(void)
{
    char text_a[] = "first";
    char text_b[] = "second";

    section("2. where the const sits");

    /* Form 1: the DATA is const, the pointer is free. */
    const char *p1 = text_a;
    printf("p1 -> %s\n", p1);
    p1 = text_b;                    /* allowed: moving the pointer   */
    printf("p1 -> %s   (pointer moved)\n", p1);
    /* p1[0] = 'X';                    refused: writing the data     */

    /* Form 2: the POINTER is const, the data is free. */
    char * const p2 = text_a;
    printf("p2 -> %s\n", p2);
    p2[0] = 'F';                    /* allowed: writing the data     */
    printf("p2 -> %s   (data changed)\n", p2);
    /* p2 = text_b;                    refused: moving the pointer   */

    /* Form 3: both locked. */
    const char * const p3 = text_b;
    printf("p3 -> %s   (nothing may change)\n", p3);
    /* p3[0] = 'X';    refused */
    /* p3 = text_a;    refused */

    puts("");
    puts("Read the declaration RIGHT TO LEFT, stopping at each word:");
    puts("");
    puts("  const char *p        p is a pointer to a char that is const");
    puts("                       -> the DATA is protected");
    puts("");
    puts("  char * const p       p is a const pointer to a char");
    puts("                       -> the POINTER is protected");
    puts("");
    puts("The rule: const protects whatever is on its LEFT. If there is");
    puts("nothing on its left, it protects what is on its right.");
    puts("So 'const char *' and 'char const *' mean exactly the same thing.");
}

/* ------------------------------------------------------------------ */
/* 3. Why function parameters use it                                   */
/* ------------------------------------------------------------------ */

/* This is where const earns its keep. The signature alone tells the
 * caller: I will read your buffer, I will not touch it. */
static int count_ones(const uint8_t *data, int length)
{
    int total = 0;

    for (int i = 0; i < length; i++) {
        if (data[i]) {
            total++;
        }
    }
    /* data[0] = 0;  would be refused - and that refusal is the feature */
    return total;
}

/* No const here: this one is documented as a writer. */
static void clear_buffer(uint8_t *data, int length)
{
    for (int i = 0; i < length; i++) {
        data[i] = 0;
    }
}

static void parameters(void)
{
    uint8_t buffer[5] = {1, 0, 3, 0, 5};

    section("3. const in parameters");

    printf("non-zero bytes: %d\n", count_ones(buffer, 5));
    printf("buffer still:   %d %d %d %d %d\n",
           buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);

    clear_buffer(buffer, 5);
    printf("after clear:    %d %d %d %d %d\n",
           buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);

    puts("");
    puts("You can tell which function writes without reading either body.");
    puts("That is the real value: the signature is the documentation, and");
    puts("the compiler checks that the documentation is honest.");
}

/* ------------------------------------------------------------------ */
/* 4. String literals                                                  */
/* ------------------------------------------------------------------ */

static void string_literals(void)
{
    char        copy[]  = "hello";   /* an ARRAY, filled from the text */
    const char *direct  = "hello";   /* a POINTER to the text itself   */

    section("4. string literals");

    copy[0] = 'H';                   /* fine - you own this array */
    printf("copy   = %s\n", copy);
    printf("direct = %s\n", direct);

    printf("\ncopy lives at   %p  (stack)\n", (void *)copy);
    printf("direct lives at %p  (read-only section)\n", (void *)direct);

    puts("");
    puts("The two lines look almost identical and are completely different.");
    puts("The array is your own copy, on the stack. The pointer points at");
    puts("the original text, which the program stores in a read-only area.");
    puts("");
    puts("Writing through that pointer compiles if you drop the const -");
    puts("and then crashes at run time with SIGSEGV. This is why string");
    puts("literals should always be reached through 'const char *'.");
}

/* ------------------------------------------------------------------ */
/* 5. On a microcontroller: const means flash                          */
/* ------------------------------------------------------------------ */

/* A lookup table nobody will ever write to. */
static const uint8_t sine_table[16] = {
    128, 176, 218, 246, 255, 246, 218, 176,
    128,  79,  37,   9,   0,   9,  37,  79
};

/* The same table without const, for comparison. */
static uint8_t writable_table[16] = {
    128, 176, 218, 246, 255, 246, 218, 176,
    128,  79,  37,   9,   0,   9,  37,  79
};

static void on_a_microcontroller(void)
{
    section("5. const means flash");

    printf("sine_table[4]     = %d   at %p\n",
           sine_table[4], (void *)sine_table);
    printf("writable_table[4] = %d   at %p\n",
           writable_table[4], (void *)writable_table);

    puts("");
    puts("On a PC this is only about safety. On an MCU it is memory.");
    puts("");
    puts("A const table goes into .rodata, which the linker places in");
    puts("FLASH. It is never copied to RAM, because nothing can change it.");
    puts("Without const the same table goes into .data - so it occupies");
    puts("RAM as well, and the startup code copies it from flash at boot.");
    puts("");
    puts("A 1 KB table on a chip with 20 KB of RAM: with const it costs");
    puts("you nothing in RAM, without const it costs 5 percent of it.");
    puts("Run 'nm 05-const.o' and compare the letters:");
    puts("    r sine_table       read-only data");
    puts("    d writable_table   ordinary data");
}

/* ------------------------------------------------------------------ */
/* 6. const together with volatile                                     */
/* ------------------------------------------------------------------ */

static void const_and_volatile(void)
{
    section("6. const volatile");

    puts("These two look like opposites, and they appear together often:");
    puts("");
    puts("    const volatile uint32_t *status_register;");
    puts("");
    puts("They answer two different questions.");
    puts("");
    puts("  const     -> MY code will never write here.");
    puts("  volatile  -> but the value changes anyway, so re-read it.");
    puts("");
    puts("That is exactly a hardware status register. The chip updates it;");
    puts("you only ever read it. const stops you writing to it by mistake,");
    puts("and volatile stops the compiler from caching the value in a");
    puts("register and never looking again.");
    puts("");
    puts("Without volatile, 'while (!(*status & FLAG));' becomes the");
    puts("infinite jump from file 03. Without const, a typo of '=' instead");
    puts("of '==' would silently write to a register you must not touch.");
}

/* ------------------------------------------------------------------ */
/* 7. What const does not do                                           */
/* ------------------------------------------------------------------ */

static void limits(void)
{
    section("7. the limits");

    puts("const is a promise checked by the COMPILER. It is not a lock on");
    puts("the memory. A cast can throw it away:");
    puts("");
    puts("    const int x = 5;");
    puts("    int *p = (int *)&x;   /* the compiler stops complaining */");
    puts("    *p = 99;              /* undefined behaviour */");
    puts("");
    puts("It may change x, may crash, or may do nothing while x keeps");
    puts("reading as 5 because the compiler folded the constant into the");
    puts("code. All three are allowed. Never do this.");
    puts("");
    puts("Also: const is not the same as a compile-time constant. A const");
    puts("variable cannot size an array in older C, which is why sizes are");
    puts("still written as #define or as an enum.");
}

int main(void)
{
    basics();
    pointer_forms();
    parameters();
    string_literals();
    on_a_microcontroller();
    const_and_volatile();
    limits();

    printf("\n");
    return 0;
}
