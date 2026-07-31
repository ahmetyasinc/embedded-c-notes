/*
 * 03 - volatile
 *
 * The compiler optimises by removing work it can prove is pointless.
 * Its proof rests only on what it can see in your code. A hardware
 * register changes for reasons outside your code, so the proof is wrong,
 * and correct code gets deleted.
 *
 * 'volatile' is how you tell the compiler: this location changes for
 * reasons you cannot see. Read it every time. Never skip a write.
 *
 * Build both ways and compare:
 *   gcc -Wall -O0 -o 03-volatile 03-volatile.c
 *   gcc -Wall -O2 -o 03-volatile 03-volatile.c                 <- HANGS
 *   gcc -Wall -O2 -DUSE_VOLATILE -o 03-volatile 03-volatile.c
 *
 * See the machine code the compiler actually produced:
 *   gcc -O2 -S -o - 03-volatile.c
 */

#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* 1. The flag an interrupt changes behind your back                   */
/* ------------------------------------------------------------------ */

/* -DUSE_VOLATILE on the command line defines this name, so one source
 * file builds both versions of the experiment. */
#ifdef USE_VOLATILE
static volatile int flag = 0;
#else
static int flag = 0;
#endif

/* A signal handler is the PC's version of an interrupt handler: the
 * operating system calls it, your code never does. That is precisely why
 * the compiler cannot see it coming. */
static void on_alarm(int sig)
{
    (void)sig;      /* accept the parameter, then say "I am ignoring it" */
    flag = 1;
}

static void waiting_for_a_flag(void)
{
    printf("=== 1. waiting for a flag ===\n");

#ifdef USE_VOLATILE
    printf("flag IS volatile\n");
#else
    printf("flag is NOT volatile\n");
#endif

    signal(SIGALRM, on_alarm);   /* call on_alarm when the alarm fires */
    alarm(1);                    /* fire it one second from now        */

    printf("waiting...\n");
    fflush(stdout);              /* push the text out before we spin   */

    while (flag == 0) {
        /* spin */
    }

    printf("loop exited, flag = %d\n\n", flag);
}

/* Without volatile, under -O2, the compiler reasons:
 *
 *     nothing inside this loop assigns to flag,
 *     therefore flag cannot change,
 *     therefore reading it again is wasted work.
 *
 * It loads flag once into a register and, seeing zero, emits a jump to
 * itself. The alarm still fires and on_alarm still runs - but main is no
 * longer looking at memory, so it never finds out.
 *
 * Every step of that reasoning is valid C. The standard lets a program
 * with no volatile and no synchronisation assume no outside changes.
 * The bug is not in the compiler; it is in the missing keyword. */

/* ------------------------------------------------------------------ */
/* 2. The mirror image: writes that get deleted                        */
/* ------------------------------------------------------------------ */

static uint32_t          plain_reg    = 0;
static volatile uint32_t volatile_reg = 0;

static void writes_disappear(void)
{
    printf("=== 2. writes that disappear ===\n");

    /* Three writes to the same place. Under -O2 the compiler sees that
     * the first two values are never read, and keeps only the last. */
    plain_reg = 0x01;
    plain_reg = 0x02;
    plain_reg = 0x03;

    /* Three writes to a volatile location. All three survive, in order. */
    volatile_reg = 0x01;
    volatile_reg = 0x02;
    volatile_reg = 0x03;

    printf("plain_reg    = 0x%02X\n", plain_reg);
    printf("volatile_reg = 0x%02X\n", volatile_reg);
    puts("(the final values match - the difference is invisible here,");
    puts(" and visible only in the assembly)\n");

    /* On a PC the collapse is harmless: nothing observes the middle
     * values. On hardware it is a real bug. Writing 0x01 to a control
     * register may be what arms the peripheral and 0x02 what triggers
     * it. Delete the first two writes and the device never starts.
     *
     * This is the half of volatile that people forget. Not only "re-read
     * every time", but also "every write I wrote must actually reach the
     * bus, in the order I wrote it". */
}

/* ------------------------------------------------------------------ */
/* 3. Where volatile goes in a declaration                             */
/* ------------------------------------------------------------------ */

static void declarations(void)
{
    printf("=== 3. reading the declarations ===\n");

    puts("  volatile uint32_t *p;    p points to volatile data");
    puts("                           (the DATA changes - a register)");
    puts("");
    puts("  uint32_t * volatile p;   the POINTER itself is volatile");
    puts("                           (the data is ordinary - rare)");
    puts("");
    puts("  volatile uint32_t * volatile p;   both");
    puts("");
    puts("Read them right to left, the same way you read const.");
    puts("Almost every line in a vendor header is the first form.\n");

    /* And the idiom that ties files 2 and 3 together:
     *
     *     #define GPIOD_ODR (*(volatile uint32_t *)0x40020C14)
     *
     * from the inside out:
     *   0x40020C14              just a number
     *   (volatile uint32_t *)   a cast: treat that number as the address
     *                           of a volatile 32-bit value
     *   *                       dereference: the thing living there
     *   ( ... )                 outer parens, so that GPIOD_ODR = 5
     *                           expands and groups correctly
     *
     * With that one line, SET_BIT and CLEAR_BIT from file 2 drive real
     * pins with no changes at all. */
}

/* ------------------------------------------------------------------ */
/* 4. What volatile does NOT give you                                  */
/* ------------------------------------------------------------------ */

static void what_volatile_is_not(void)
{
    printf("=== 4. what volatile is not ===\n");

    puts("NOT atomic. volatile guarantees the access happens, not that it");
    puts("happens in one indivisible step. A 32-bit variable on an 8-bit");
    puts("MCU is read as four separate byte loads; an interrupt landing");
    puts("between them hands you half the old value and half the new.");
    puts("");
    puts("Even on 32-bit hardware, count++ is three operations - load, add,");
    puts("store - and volatile does nothing about the gap between them. For");
    puts("that you disable interrupts around it, or use an atomic type.");
    puts("");
    puts("NOT thread synchronisation. It orders accesses to the one");
    puts("variable, places no barrier on the accesses around it, and says");
    puts("nothing about what other cores can see. Java and C# gave the word");
    puts("a stronger meaning, which is where the confusion comes from.");
    puts("");
    puts("Rule of thumb: reach for volatile when a value changes outside");
    puts("the visible flow of your program - a hardware register, a flag an");
    puts("ISR writes, a variable a signal handler touches. Reach for");
    puts("something stronger the moment more than one bit of the update has");
    puts("to be seen together.");
    puts("");
    puts("The strictly correct type for a flag shared with a signal handler");
    puts("is  volatile sig_atomic_t  - an integer the platform promises to");
    puts("read and write in a single step.");
}

int main(void)
{
    waiting_for_a_flag();
    writes_disappear();
    declarations();
    what_volatile_is_not();

    return 0;
}
