/*
 * 07 - endianness
 *
 * A byte has one obvious order. A 32-bit number does not. It occupies
 * four bytes in memory, and the machine has to decide which end goes
 * first. Two machines can hold the same number and disagree about the
 * arrangement of the bytes.
 *
 * This never matters while the value stays inside your program. It
 * matters the moment those bytes leave it - over CAN, over UART, into
 * a file, into flash.
 *
 * Build:  gcc -Wall -o 07-endianness 07-endianness.c
 * Run:    ./07-endianness
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

static void section(const char *title)
{
    printf("\n=== %s ===\n", title);
}

/* ------------------------------------------------------------------ */
/* 1. Looking at the bytes of one number                               */
/* ------------------------------------------------------------------ */

static void dump(const char *label, const uint8_t *bytes, size_t n)
{
    printf("%-22s", label);
    for (size_t i = 0; i < n; i++) {
        printf(" %02X", bytes[i]);
    }
    printf("\n");
}

static void look_at_the_bytes(void)
{
    uint32_t value = 0x12345678;

    section("1. one number, four bytes");

    printf("the value:             0x%08X\n", value);

    /* Take the address of the number and read it as plain bytes. */
    dump("in memory, first->last", (const uint8_t *)&value, sizeof(value));

    puts("");
    puts("You wrote 12 34 56 78. Memory holds 78 56 34 12.");
    puts("The smallest byte is stored FIRST. That is called little-endian.");
    puts("");
    puts("The other arrangement, 12 34 56 78, is big-endian. The name comes");
    puts("from which END of the number comes first: the little end or the");
    puts("big end.");
}

/* ------------------------------------------------------------------ */
/* 2. Asking the machine at run time                                   */
/* ------------------------------------------------------------------ */

/* A union puts every member at the SAME address. Write through one
 * member, read through another, and you are looking at the same bytes
 * with different eyes. (File 08 is about unions.) */
static int is_little_endian(void)
{
    union {
        uint32_t word;
        uint8_t  bytes[4];
    } probe;

    probe.word = 1;

    /* If the 1 landed in the first byte, the small end came first. */
    return probe.bytes[0] == 1;
}

static void which_machine(void)
{
    section("2. asking the machine");

    printf("this machine is: %s\n",
           is_little_endian() ? "little-endian" : "big-endian");

    puts("");
    puts("x86 is little-endian. ARM Cortex-M is little-endian too, in every");
    puts("configuration you will meet. So your laptop and your STM32 agree.");
    puts("");
    puts("But protocols usually do not. TCP/IP, CANopen and most fieldbus");
    puts("standards send the big end first. That arrangement is so common");
    puts("in protocols that it has a second name: network byte order.");
}

/* ------------------------------------------------------------------ */
/* 3. The wrong way to send a number                                   */
/* ------------------------------------------------------------------ */

static void the_wrong_way(void)
{
    uint32_t sensor = 0x12345678;
    uint8_t  wire[4];
    uint32_t received;

    section("3. the wrong way");

    /* Copy the raw bytes of the variable into the buffer. */
    memcpy(wire, &sensor, sizeof(sensor));
    dump("bytes we transmit", wire, 4);

    /* The receiver copies them back into a uint32_t. */
    memcpy(&received, wire, sizeof(received));
    printf("receiver reads:        0x%08X\n", received);

    puts("");
    puts("It worked - because both sides of this test are the same machine.");
    puts("");
    puts("Now imagine the receiver is big-endian, or the sender is a sensor");
    puts("whose datasheet says big-endian. The same four bytes are then read");
    puts("in the opposite order and 0x12345678 arrives as 0x78563412.");
    puts("");
    puts("Nothing warns you. The bytes are all correct. Only the agreement");
    puts("about their order is missing.");
}

/* ------------------------------------------------------------------ */
/* 4. The right way: shifting                                          */
/* ------------------------------------------------------------------ */

/* The key idea, and it is easy to miss:
 *
 *   memcpy and pointer casts work on BYTES, so they expose the machine's
 *   arrangement.
 *
 *   >> and << work on the VALUE, not on memory. (value >> 24) is the top
 *   byte of the number on every machine ever built.
 *
 * So code written with shifts produces the same bytes everywhere. It does
 * not need to know what machine it is running on. */

static void write_be32(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)(value >> 24);   /* big end first */
    buf[1] = (uint8_t)(value >> 16);
    buf[2] = (uint8_t)(value >> 8);
    buf[3] = (uint8_t)(value);
}

static uint32_t read_be32(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8)  |
           ((uint32_t)buf[3]);
}

static void write_le32(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)(value);         /* little end first */
    buf[1] = (uint8_t)(value >> 8);
    buf[2] = (uint8_t)(value >> 16);
    buf[3] = (uint8_t)(value >> 24);
}

static void the_right_way(void)
{
    uint32_t sensor = 0x12345678;
    uint8_t  buf[4];

    section("4. the right way");

    write_be32(buf, sensor);
    dump("big-endian on wire", buf, 4);
    printf("read back:             0x%08X\n", read_be32(buf));

    write_le32(buf, sensor);
    dump("little-endian on wire", buf, 4);

    puts("");
    puts("Look at the first line: 12 34 56 78, even though this machine");
    puts("stores the number as 78 56 34 12. The shifts ignored the machine");
    puts("completely.");
    puts("");
    puts("That is the rule worth keeping. Do not ask what your processor");
    puts("does. Decide what the WIRE format is, and build it with shifts.");
    puts("Then the code is correct on both nodes without a single #ifdef.");
    puts("");
    puts("Note the casts in read_be32. buf[0] is a uint8_t and would be");
    puts("promoted to a 32-bit signed int; shifting it left by 24 can reach");
    puts("the sign bit and that is undefined behaviour. Casting to uint32_t");
    puts("first makes it safe.");
}

/* ------------------------------------------------------------------ */
/* 5. The two-bug line                                                 */
/* ------------------------------------------------------------------ */

static void the_two_bug_line(void)
{
    section("5. one line, two bugs");

    puts("You will be tempted to write this in your CAN receive handler:");
    puts("");
    puts("    uint32_t value = *(uint32_t *)&rx_buffer[1];");
    puts("");
    puts("It contains two separate bugs.");
    puts("");
    puts("First, endianness. The bytes are interpreted with the machine's");
    puts("own order, not with the protocol's. If the frame is big-endian");
    puts("the number comes out reversed.");
    puts("");
    puts("Second, alignment - the problem from file 06. rx_buffer[1] is at");
    puts("an odd address. A 32-bit load from an odd address is slow on x86");
    puts("and a HardFault on a Cortex-M0.");
    puts("");
    puts("    uint32_t value = read_be32(&rx_buffer[1]);");
    puts("");
    puts("The version above has neither problem. It reads four separate");
    puts("bytes, so alignment never comes up, and the order is written down");
    puts("in the function instead of inherited from the hardware.");
}

/* ------------------------------------------------------------------ */
/* 6. 16-bit values, and where else this appears                       */
/* ------------------------------------------------------------------ */

static void write_be16(uint8_t *buf, uint16_t value)
{
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value);
}

static uint16_t read_be16(const uint8_t *buf)
{
    return (uint16_t)(((uint16_t)buf[0] << 8) | (uint16_t)buf[1]);
}

static void sixteen_bit(void)
{
    uint8_t  buf[2];
    uint16_t angle = 0xABCD;

    section("6. the same thing, 16 bits");

    write_be16(buf, angle);
    dump("angle 0xABCD, big-end", buf, 2);
    printf("read back:             0x%04X\n", read_be16(buf));

    puts("");
    puts("This is the shape you will use most. An IMU angle, an ADC reading,");
    puts("a servo position - almost everything you put on the CAN bus in P2");
    puts("is a 16-bit value that has to be split into two bytes and put back");
    puts("together on the other node.");
    puts("");
    puts("Two related places where byte order bites:");
    puts("");
    puts("  Sensor datasheets. An MPU-6050 returns its readings big-endian.");
    puts("  Read the two registers and combine them yourself - the datasheet");
    puts("  tells you which one is the high byte.");
    puts("");
    puts("  Bit fields in a struct. Which end the compiler starts packing");
    puts("  from is not defined by the standard. That is why protocol code");
    puts("  uses masks and shifts, not bit fields.");
}

int main(void)
{
    look_at_the_bytes();
    which_machine();
    the_wrong_way();
    the_right_way();
    the_two_bug_line();
    sixteen_bit();

    printf("\n");
    return 0;
}
