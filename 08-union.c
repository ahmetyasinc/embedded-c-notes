/*
 * 08 - union
 *
 * A struct gives every member its own memory. A union gives every member
 * the SAME memory.
 *
 * So a union is not a container. It is one block of bytes with several
 * names, and each name says how to read those bytes. Only one of those
 * readings is meaningful at a time - and the union does not remember
 * which one. That is your job.
 *
 * Build:  gcc -Wall -o 08-union 08-union.c
 * Run:    ./08-union
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

static void section(const char *title)
{
    printf("\n=== %s ===\n", title);
}

/* ------------------------------------------------------------------ */
/* 1. struct and union, side by side                                   */
/* ------------------------------------------------------------------ */

struct three_members {
    uint8_t  a;
    uint32_t b;
    uint16_t c;
};

union same_three {
    uint8_t  a;
    uint32_t b;
    uint16_t c;
};

static void side_by_side(void)
{
    struct three_members s;
    union  same_three    u;

    section("1. struct vs union");

    printf("sizeof(struct) = %zu   (members side by side, plus padding)\n",
           sizeof(s));
    printf("sizeof(union)  = %zu   (just the largest member)\n\n",
           sizeof(u));

    printf("struct member addresses:\n");
    printf("  &s.a = %p\n", (void *)&s.a);
    printf("  &s.b = %p\n", (void *)&s.b);
    printf("  &s.c = %p\n\n", (void *)&s.c);

    printf("union member addresses:\n");
    printf("  &u.a = %p\n", (void *)&u.a);
    printf("  &u.b = %p\n", (void *)&u.b);
    printf("  &u.c = %p\n", (void *)&u.c);

    puts("");
    puts("The struct addresses are different. The union addresses are all");
    puts("the same. That single fact explains everything else about unions.");
}

/* ------------------------------------------------------------------ */
/* 2. Writing one member changes the others                            */
/* ------------------------------------------------------------------ */

static void overwriting(void)
{
    union same_three u;

    section("2. they overwrite each other");

    u.b = 0x11223344;
    printf("after u.b = 0x11223344:  u.b = 0x%08X\n", u.b);

    u.a = 0xFF;
    printf("after u.a = 0xFF:        u.b = 0x%08X   <- b changed too\n", u.b);

    puts("");
    puts("Nothing was copied and nothing went wrong. There is one block of");
    puts("four bytes. Writing 0xFF through the name 'a' changed the first");
    puts("byte, and 'b' is a name for all four bytes including that one.");
    puts("");
    puts("So a union holds ONE value, not three. Storing something new");
    puts("destroys what was there before.");
}

/* ------------------------------------------------------------------ */
/* 3. Use one: looking at the same bytes two ways                      */
/* ------------------------------------------------------------------ */

union word_and_bytes {
    uint32_t word;
    uint8_t  byte[4];
};

static void two_views(void)
{
    union word_and_bytes u;

    section("3. one value, two views");

    u.word = 0x12345678;

    printf("u.word = 0x%08X\n", u.word);
    printf("u.byte = %02X %02X %02X %02X\n",
           u.byte[0], u.byte[1], u.byte[2], u.byte[3]);

    puts("");
    puts("This is the endianness probe from file 07. You write a number and");
    puts("read it back as separate bytes, with no copying and no shifting.");
    puts("");
    puts("Careful, though: this view shows the MACHINE's byte order. That is");
    puts("exactly what you want when you are inspecting hardware, and");
    puts("exactly what you must avoid when building a message for the wire.");
    puts("For the wire, use shifts (file 07).");

    /* One warning about the language. In C, writing one member and then
     * reading a different one is allowed, and the bytes are reinterpreted.
     * This is called type punning. It is legal C - but it is NOT legal
     * C++, where the same code is undefined behaviour. If a header of
     * yours is ever compiled by a C++ toolchain, use memcpy instead. */
}

/* ------------------------------------------------------------------ */
/* 4. Use two: taking a float apart                                    */
/* ------------------------------------------------------------------ */

union float_bits {
    float    f;
    uint32_t bits;
};

static void inside_a_float(void)
{
    union float_bits u;

    section("4. inside a float");

    u.f = 1.0f;
    printf("1.0f  is stored as 0x%08X\n", u.bits);

    u.f = 2.0f;
    printf("2.0f  is stored as 0x%08X\n", u.bits);

    u.f = -1.0f;
    printf("-1.0f is stored as 0x%08X\n", u.bits);

    u.f = 0.0f;
    printf("0.0f  is stored as 0x%08X\n", u.bits);

    /* Pull the three parts out of the last non-zero value. */
    u.f = -1.0f;
    printf("\nfor -1.0f:\n");
    printf("  sign bit  (1 bit)   = %u\n",  (u.bits >> 31) & 0x1u);
    printf("  exponent  (8 bits)  = %u\n",  (u.bits >> 23) & 0xFFu);
    printf("  fraction  (23 bits) = 0x%X\n", u.bits & 0x7FFFFFu);

    puts("");
    puts("A float is not a special kind of number. It is 32 bits split into");
    puts("three fields, and the union lets you look at those fields with the");
    puts("masks and shifts from file 02.");
    puts("");
    puts("This is useful on a microcontroller without a floating point unit.");
    puts("Checking 'is this value zero' or 'is the sign negative' becomes an");
    puts("integer test, which is far cheaper than a float comparison.");
}

/* ------------------------------------------------------------------ */
/* 5. Use three: one message, several shapes                           */
/* ------------------------------------------------------------------ */

/* This is the pattern you will want in P2. Several kinds of CAN message
 * travel on one bus. Each kind carries a different payload, but only one
 * kind arrives at a time - so they can share the memory. */

enum msg_type {
    MSG_TEMPERATURE,
    MSG_POSITION,
    MSG_STATUS
};

struct message {
    enum msg_type type;         /* which member below is the real one */
    union {
        struct { int16_t celsius_x10; }            temperature;
        struct { int16_t pan; int16_t tilt; }      position;
        struct { uint8_t flags; uint8_t errors; }  status;
    } payload;
};

static void print_message(const struct message *m)
{
    /* The tag decides how to read the payload. Reading the wrong member
     * would not crash - it would quietly give you nonsense. */
    switch (m->type) {
    case MSG_TEMPERATURE:
        printf("  temperature: %.1f C\n", m->payload.temperature.celsius_x10 / 10.0);
        break;
    case MSG_POSITION:
        printf("  position:    pan %d, tilt %d\n",
               m->payload.position.pan, m->payload.position.tilt);
        break;
    case MSG_STATUS:
        printf("  status:      flags 0x%02X, errors 0x%02X\n",
               m->payload.status.flags, m->payload.status.errors);
        break;
    }
}

static void tagged_union(void)
{
    struct message a = { .type = MSG_TEMPERATURE };
    struct message b = { .type = MSG_POSITION };
    struct message c = { .type = MSG_STATUS };

    section("5. one message, several shapes");

    a.payload.temperature.celsius_x10 = 235;
    b.payload.position.pan  = -120;
    b.payload.position.tilt = 45;
    c.payload.status.flags  = 0x03;
    c.payload.status.errors = 0x00;

    print_message(&a);
    print_message(&b);
    print_message(&c);

    printf("\nsizeof(struct message) = %zu bytes\n", sizeof(struct message));
    printf("the payload union alone = %zu bytes (the largest case)\n",
           sizeof(a.payload));

    puts("");
    puts("Without the union you would put all three payloads in the struct");
    puts("side by side, and two of them would always be dead weight.");
    puts("");
    puts("The 'type' field is called a TAG, and the whole pattern is a");
    puts("tagged union. The tag is not optional. A union cannot tell you");
    puts("which member is valid - if you lose the tag, the bytes become");
    puts("unreadable, because nothing in them says what they mean.");
}

/* ------------------------------------------------------------------ */
/* 6. Where you will meet unions in vendor code                        */
/* ------------------------------------------------------------------ */

/* A peripheral register, reachable as a whole word or field by field. */
union control_register {
    uint32_t word;
    struct {
        uint32_t enable    : 1;    /* ': 1' means one bit wide */
        uint32_t mode      : 2;
        uint32_t reserved  : 5;
        uint32_t prescaler : 8;
        uint32_t unused    : 16;
    } bits;
};

static void vendor_style(void)
{
    union control_register reg;

    section("6. the vendor header style");

    reg.word = 0;
    reg.bits.enable    = 1;
    reg.bits.mode      = 2;
    reg.bits.prescaler = 16;

    printf("built field by field, the whole word is 0x%08X\n", reg.word);
    printf("read back: enable=%u mode=%u prescaler=%u\n",
           reg.bits.enable, reg.bits.mode, reg.bits.prescaler);

    puts("");
    puts("You write single fields by name, then hand the whole word to the");
    puts("hardware in one store. ST and Nordic headers are full of this.");
    puts("");
    puts("The catch, and it is the same catch as file 07: the standard does");
    puts("not say which end the compiler starts packing bit fields from. So");
    puts("this is fine for talking to a chip with a known compiler, and a");
    puts("bad idea for a message format shared between two different chips.");
    puts("For anything that leaves the board, use masks and shifts.");
}

int main(void)
{
    side_by_side();
    overwriting();
    two_views();
    inside_a_float();
    tagged_union();
    vendor_style();

    printf("\n");
    return 0;
}
