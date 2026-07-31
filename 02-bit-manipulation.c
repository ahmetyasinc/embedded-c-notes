/*
 * 02 - Bit manipulation
 *
 * A register is one box holding many independent switches. Every operation
 * here exists to change SOME switches while leaving the others exactly as
 * they were. That constraint is the whole subject.
 *
 * Build:  gcc -Wall -o 02-bit-manipulation 02-bit-manipulation.c
 * Run:    ./02-bit-manipulation
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/* The four operations                                                 */
/* ------------------------------------------------------------------ */

/* OR: a 0 in the mask passes the original bit through, a 1 forces it on. */
#define SET_BIT(reg, n)      ((reg) |= (1u << (n)))

/* AND with an inverted mask: a 1 preserves, the single 0 destroys. */
#define CLEAR_BIT(reg, n)    ((reg) &= ~(1u << (n)))

/* XOR: comparing against 0 returns the bit, against 1 returns its opposite. */
#define TOGGLE_BIT(reg, n)   ((reg) ^= (1u << (n)))

/* Every parameter is parenthesised because a macro is text replacement.
 * Without them, TEST(x, 3) == 0 would expand to x & (1u << 3) == 0, and
 * '==' binds tighter than '&', so C would read it as x & ((1u<<3) == 0),
 * which is x & 0, which is always zero. */

/* Reading needs no macro. This only inspects, so a copy is enough and the
 * caller keeps the simple call syntax - no '&' at the call site. As a
 * function it also gets type checking, which a macro can never offer. */
static bool test_bit(uint32_t reg, int n)
{
    /* reg & mask yields the mask value (8, 16, ...), not 1. Comparing
     * against zero squeezes any non-zero result down to a clean true. */
    return (reg & (1u << n)) != 0;
}

/* ------------------------------------------------------------------ */
/* Printing helpers                                                    */
/* ------------------------------------------------------------------ */

static void print_bits8(uint8_t value)
{
    for (int i = 7; i >= 0; i--) {
        putchar(((value >> i) & 1u) ? '1' : '0');
    }
}

static void show(const char *label, uint8_t value)
{
    printf("%-26s ", label);
    print_bits8(value);
    printf("   (0x%02X)\n", value);
}

static void section(const char *title)
{
    printf("\n=== %s ===\n", title);
}

/* ------------------------------------------------------------------ */
/* 1. Set, clear, toggle                                               */
/* ------------------------------------------------------------------ */
static void set_clear_toggle(void)
{
    uint8_t reg = 0b00000101;   /* bits 0 and 2 already on */

    section("1. set / clear / toggle");

    show("start", reg);

    SET_BIT(reg, 4);
    show("after SET_BIT(4)", reg);

    CLEAR_BIT(reg, 2);
    show("after CLEAR_BIT(2)", reg);

    TOGGLE_BIT(reg, 7);
    show("after TOGGLE_BIT(7)", reg);

    TOGGLE_BIT(reg, 7);
    show("after TOGGLE_BIT(7) again", reg);

    /* Toggling twice returns to the starting value. XOR is its own
     * inverse - the same operation undoes itself. */
}

/* ------------------------------------------------------------------ */
/* 2. Testing a bit                                                    */
/* ------------------------------------------------------------------ */
static void testing(void)
{
    uint8_t reg = 0b00010101;

    section("2. testing");

    show("register", reg);

    for (int n = 0; n < 8; n++) {
        printf("bit %d is %s\n", n, test_bit(reg, n) ? "set" : "clear");
    }

    /* This is the shape of nearly every driver's wait loop:
     *
     *     while (!test_bit(UART_STATUS, TX_EMPTY))
     *         ;
     *     UART_DATA = byte;
     *
     * You are not reading a number, you are asking the hardware a yes/no
     * question and refusing to move on until the answer changes. */
}

/* ------------------------------------------------------------------ */
/* 3. Why '=' is not 'set'                                             */
/* ------------------------------------------------------------------ */
static void the_assignment_trap(void)
{
    uint8_t leds = 0b00010000;   /* the green LED, bit 4, is already on */

    section("3. the assignment trap");

    show("green LED on", leds);

    leds = (1u << 5);            /* "turn on the orange LED" */
    show("leds = (1u << 5)", leds);

    /* Green went dark. '=' does not set a bit, it replaces all eight of
     * them with a number whose bit 5 happens to be 1. The hardware did
     * exactly what it was told. */

    leds = 0b00010000;
    SET_BIT(leds, 5);
    show("SET_BIT(leds, 5)", leds);

    /* Read the old value, change one bit, write it back. The three steps
     * are why this is called read-modify-write, and why it is not free:
     * on real hardware an interrupt firing between the read and the write
     * can lose a change made by the interrupt handler. Cortex-M offers
     * bit-banding and separate set/reset registers (BSRR) precisely to
     * make the operation atomic. */
}

/* ------------------------------------------------------------------ */
/* 4. Writing a field of several bits                                  */
/* ------------------------------------------------------------------ */

/* (1u << width) - 1 produces 'width' ones stacked at the bottom:
 * 1u << 2 is 0b100, minus one is 0b011. Shifting that up by pos slides
 * the block of ones to where the field actually lives. */
#define FIELD_MASK(pos, width)   ((uint8_t)((((1u << (width)) - 1u)) << (pos)))

#define FIELD_WRITE(reg, pos, width, value)                       \
    ((reg) = (uint8_t)(((reg) & ~FIELD_MASK(pos, width)) |        \
                       (((value) << (pos)) & FIELD_MASK(pos, width))))

static void field_write(void)
{
    /* Imagine a control register: bits 4-5 hold a 2-bit mode, and the
     * surrounding bits are unrelated flags that must survive. */
    uint8_t ctrl = 0b10000011;

    section("4. multi-bit field");

    show("start (flags set)", ctrl);
    show("field mask, bits 4-5", FIELD_MASK(4, 2));

    FIELD_WRITE(ctrl, 4, 2, 0b10);
    show("mode = 2", ctrl);

    FIELD_WRITE(ctrl, 4, 2, 0b01);
    show("mode = 1", ctrl);

    FIELD_WRITE(ctrl, 4, 2, 0b00);
    show("mode = 0", ctrl);

    /* Two steps, in order: AND with the inverted mask to clear the field,
     * then OR the new value in. Skipping the clear is the classic bug -
     * OR alone can only add ones, so mode 2 could never go back to 0.
     *
     * The second mask, on the value side, is a guard: if the caller passes
     * 0b111 into a 2-bit field, it gets truncated instead of corrupting
     * the neighbouring bit. */
}

/* ------------------------------------------------------------------ */
/* 5. Counting set bits                                                */
/* ------------------------------------------------------------------ */

/* The obvious way: look at all eight positions. */
static int count_bits_naive(uint8_t value)
{
    int count = 0;

    for (int i = 0; i < 8; i++) {
        if ((value >> i) & 1u) {
            count++;
        }
    }
    return count;
}

/* Kernighan's way: v & (v - 1) removes the lowest set bit, so the loop
 * runs once per set bit rather than once per position.
 *
 * Subtracting 1 flips the lowest 1 to 0 and turns everything below it
 * into ones:   0b01011000 - 1 = 0b01010111
 * ANDing them keeps only what was above that bit:  0b01010000 */
static int count_bits_kernighan(uint8_t value)
{
    int count = 0;

    while (value) {
        value &= (uint8_t)(value - 1u);
        count++;
    }
    return count;
}

static void counting(void)
{
    uint8_t samples[4] = {0b00000000, 0b00000001, 0b01011000, 0b11111111};

    section("5. counting set bits");

    for (int i = 0; i < 4; i++) {
        print_bits8(samples[i]);
        printf("   naive %d   kernighan %d\n",
               count_bits_naive(samples[i]),
               count_bits_kernighan(samples[i]));
    }

    /* Both are correct; they differ in how many iterations they need.
     * The name for this count is the population count, and most CPUs have
     * a single instruction for it - __builtin_popcount() on GCC. */
}

/* ------------------------------------------------------------------ */
/* 6. What this looks like on real hardware                            */
/* ------------------------------------------------------------------ */
static void on_real_hardware(void)
{
    section("6. on real hardware");

    puts("Port D on an STM32F4 has one 32-bit output register at 0x40020C14.");
    puts("Its low 16 bits are the 16 pins of that port; the Discovery board");
    puts("wires LEDs to bits 12 through 15. So the macros above become:");
    puts("");
    puts("    #define GPIOD_ODR (*(volatile uint32_t *)0x40020C14)");
    puts("");
    puts("    SET_BIT(GPIOD_ODR, 12);      /* green on, others untouched  */");
    puts("    CLEAR_BIT(GPIOD_ODR, 12);    /* green off                   */");
    puts("    TOGGLE_BIT(GPIOD_ODR, 13);   /* orange blinks               */");
    puts("");
    puts("Nothing new is needed - the same three lines drive real pins. The");
    puts("only additions are the cast, which says 'treat this address as a");
    puts("32-bit register', and 'volatile', which stops the compiler from");
    puts("optimising away a write whose effect it cannot see. That keyword");
    puts("is the next file.");
}

int main(void)
{
    set_clear_toggle();
    testing();
    the_assignment_trap();
    field_write();
    counting();
    on_real_hardware();

    printf("\n");
    return 0;
}
