/*
 * 06 - struct padding
 *
 * A struct is not just its members added together. The compiler inserts
 * invisible gaps between them, and the size you get depends on the ORDER
 * you wrote them in.
 *
 * The reason is alignment. Most processors want a 4-byte value to start
 * at an address divisible by 4. Some cannot read it any other way. So the
 * compiler slides members forward until they land correctly, and fills
 * the gaps with bytes that mean nothing.
 *
 * Build:  gcc -Wall -o 06-struct-padding 06-struct-padding.c
 * Run:    ./06-struct-padding
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>     /* offsetof */
#include <string.h>     /* memcmp, memset */

static void section(const char *title)
{
    printf("\n=== %s ===\n", title);
}

/* ------------------------------------------------------------------ */
/* 1. Same members, different order, different size                    */
/* ------------------------------------------------------------------ */

struct bad_order {
    uint8_t  flag;      /* 1 byte  */
    uint32_t value;     /* 4 bytes */
    uint8_t  id;        /* 1 byte  */
};

struct good_order {
    uint32_t value;     /* 4 bytes */
    uint8_t  flag;      /* 1 byte  */
    uint8_t  id;        /* 1 byte  */
};

static void two_orders(void)
{
    section("1. the same members, twice");

    printf("members add up to:      %zu bytes\n",
           sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t));
    printf("sizeof(struct bad_order)  = %zu\n", sizeof(struct bad_order));
    printf("sizeof(struct good_order) = %zu\n", sizeof(struct good_order));

    puts("");
    puts("Identical members. Only the order changed. The compiler is not");
    puts("allowed to reorder them for you - C guarantees that members stay");
    puts("in the order you wrote. So the waste is yours to fix.");
}

/* ------------------------------------------------------------------ */
/* 2. Where the gaps actually are                                      */
/* ------------------------------------------------------------------ */

/* offsetof(type, member) reports how many bytes into the struct a member
 * begins. Comparing that with the size of the member before it shows the
 * hole. */
static void draw_map(const char *name, size_t total,
                     const char *n1, size_t o1, size_t s1,
                     const char *n2, size_t o2, size_t s2,
                     const char *n3, size_t o3, size_t s3)
{
    printf("\n%s  (%zu bytes)\n", name, total);
    printf("  byte %2zu..%2zu   %s\n", o1, o1 + s1 - 1, n1);
    if (o2 > o1 + s1) {
        printf("  byte %2zu..%2zu   ---- padding ----\n", o1 + s1, o2 - 1);
    }
    printf("  byte %2zu..%2zu   %s\n", o2, o2 + s2 - 1, n2);
    if (o3 > o2 + s2) {
        printf("  byte %2zu..%2zu   ---- padding ----\n", o2 + s2, o3 - 1);
    }
    printf("  byte %2zu..%2zu   %s\n", o3, o3 + s3 - 1, n3);
    if (total > o3 + s3) {
        printf("  byte %2zu..%2zu   ---- padding ----\n", o3 + s3, total - 1);
    }
}

static void the_map(void)
{
    section("2. where the gaps are");

    draw_map("struct bad_order", sizeof(struct bad_order),
             "flag  (1)", offsetof(struct bad_order, flag),  1,
             "value (4)", offsetof(struct bad_order, value), 4,
             "id    (1)", offsetof(struct bad_order, id),    1);

    draw_map("struct good_order", sizeof(struct good_order),
             "value (4)", offsetof(struct good_order, value), 4,
             "flag  (1)", offsetof(struct good_order, flag),  1,
             "id    (1)", offsetof(struct good_order, id),    1);

    puts("");
    puts("In the bad one, 'value' needs an address divisible by 4. It sits");
    puts("right after a single byte, so three bytes are thrown away to push");
    puts("it into place.");
    puts("");
    puts("The padding at the END is the surprising one. It exists so that");
    puts("in an ARRAY of these structs, every element still starts at a");
    puts("correct address. The size of a struct is always a multiple of its");
    puts("largest member's alignment.");
}

/* ------------------------------------------------------------------ */
/* 3. The fix                                                          */
/* ------------------------------------------------------------------ */

static void the_fix(void)
{
    section("3. the fix");

    printf("array of 100 bad_order:  %zu bytes\n",
           sizeof(struct bad_order) * 100);
    printf("array of 100 good_order: %zu bytes\n",
           sizeof(struct good_order) * 100);
    printf("wasted: %zu bytes\n",
           (sizeof(struct bad_order) - sizeof(struct good_order)) * 100);

    puts("");
    puts("The rule is one line: declare members from LARGEST to SMALLEST.");
    puts("It costs nothing and it is the whole optimisation.");
    puts("");
    puts("On a chip with 20 KB of RAM, a sloppy 100-element buffer can lose");
    puts("you 400 bytes for no reason at all.");
}

/* ------------------------------------------------------------------ */
/* 4. Packing: removing the gaps by force                              */
/* ------------------------------------------------------------------ */

/* __attribute__((packed)) tells GCC to insert no padding at all. */
struct packed_frame {
    uint8_t  flag;
    uint32_t value;
    uint8_t  id;
} __attribute__((packed));

static void packing(void)
{
    section("4. packed structs");

    printf("sizeof(struct bad_order)    = %zu\n", sizeof(struct bad_order));
    printf("sizeof(struct packed_frame) = %zu\n", sizeof(struct packed_frame));

    puts("");
    puts("No gaps. Six bytes for six bytes of data. That looks like a pure");
    puts("win, and it is not free.");
    puts("");
    puts("'value' now starts at byte 1, an address not divisible by 4. On");
    puts("x86 the processor handles it and simply takes longer. On a");
    puts("Cortex-M0 an unaligned 32-bit load is a HardFault - your board");
    puts("stops dead. On a Cortex-M4 it works, but the compiler has to");
    puts("build the value from separate byte loads, so it is slower.");
    puts("");
    puts("So: pack a struct when the LAYOUT is the requirement - a network");
    puts("packet, a file header, a CAN frame. Never pack for tidiness.");
}

/* ------------------------------------------------------------------ */
/* 5. Why you must not send a struct over a wire                       */
/* ------------------------------------------------------------------ */

static void over_the_wire(void)
{
    section("5. structs on a wire");

    puts("It is tempting to do this on the sending node:");
    puts("");
    puts("    can_send((uint8_t *)&frame, sizeof(frame));");
    puts("");
    puts("and cast the bytes straight back into a struct on the receiver.");
    puts("It works perfectly until the day it does not.");
    puts("");
    puts("The padding is decided by the COMPILER and the ARCHITECTURE, not");
    puts("by your source code. Your STM32 and your ESP32 use different");
    puts("compilers. Change one optimisation flag, one compiler version,");
    puts("or one member, and the two sides disagree about where 'value'");
    puts("starts. The bytes still arrive. They are simply read as garbage.");
    puts("");
    puts("Worse, the padding bytes themselves are never initialised. They");
    puts("contain whatever was in that memory before - old stack data. You");
    puts("would be transmitting leftovers of your own program.");
    puts("");
    puts("The fix is to write the bytes out one field at a time:");
    puts("");
    puts("    buf[0] = frame.flag;");
    puts("    buf[1] = (uint8_t)(frame.value >> 24);");
    puts("    buf[2] = (uint8_t)(frame.value >> 16);");
    puts("    ...");
    puts("");
    puts("That is called serialisation. It is more typing, and it is the");
    puts("only version that keeps working. Endianness (file 07) is the");
    puts("second half of the same problem.");
}

/* ------------------------------------------------------------------ */
/* 6. Padding holds garbage - so never memcmp a struct                 */
/* ------------------------------------------------------------------ */

static void never_memcmp(void)
{
    struct bad_order a;
    struct bad_order b;

    section("6. why memcmp lies");

    /* Dirty the memory of each one differently, then set the same
     * values in both. Every MEMBER now matches. */
    memset(&a, 0x00, sizeof(a));
    memset(&b, 0xFF, sizeof(b));

    a.flag = 1;  a.value = 1000;  a.id = 7;
    b.flag = 1;  b.value = 1000;  b.id = 7;

    printf("a.flag == b.flag   : %s\n", a.flag  == b.flag  ? "yes" : "no");
    printf("a.value == b.value : %s\n", a.value == b.value ? "yes" : "no");
    printf("a.id == b.id       : %s\n", a.id    == b.id    ? "yes" : "no");

    printf("\nmemcmp(&a, &b, sizeof) says: %s\n",
           memcmp(&a, &b, sizeof(a)) == 0 ? "identical" : "DIFFERENT");

    puts("");
    puts("Every member is equal, and memcmp still reports a difference.");
    puts("It compared the padding too, and nothing ever wrote to those");
    puts("bytes. Compare structs field by field, never with memcmp.");
}

int main(void)
{
    two_orders();
    the_map();
    the_fix();
    packing();
    over_the_wire();
    never_memcmp();

    printf("\n");
    return 0;
}
