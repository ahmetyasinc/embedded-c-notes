/*
 * 09 - the memory map
 *
 * Files 04 and 05 showed pieces of this. Here it is as one picture.
 *
 * Your program is not one lump of memory. The linker sorts everything you
 * declare into a handful of regions, and each region has a different cost,
 * a different lifetime, and on a microcontroller a different chip.
 *
 * The whole reason this matters: on a PC there is one big pool and you can
 * ignore all of it. On an STM32F4 you have 1 MB of flash and 192 KB of RAM,
 * they are different hardware, and the difference between a variable that
 * costs flash and one that costs RAM is decided by how you declared it.
 *
 * Build:  gcc -Wall -o 09-memory-map 09-memory-map.c
 * Run:    ./09-memory-map
 * Look:   size 09-memory-map
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>     /* malloc, free */

static void section(const char *title)
{
    printf("\n=== %s ===\n", title);
}

/* ------------------------------------------------------------------ */
/* One variable of every kind                                          */
/* ------------------------------------------------------------------ */

static const char message[] = "a constant string";  /* .rodata - flash   */
static int   initialised   = 42;                    /* .data   - both    */
static int   zeroed;                                /* .bss    - RAM     */
static char  buffer[1024];                          /* .bss    - RAM     */

/* ------------------------------------------------------------------ */
/* 1. The regions, by address                                          */
/* ------------------------------------------------------------------ */

static void the_regions(void)
{
    int    local = 1;                    /* stack */
    void  *heap  = malloc(64);           /* heap  */

    section("1. one address from each region");

    printf("  code (.text)     %p   the function you are running\n",
           (void *)(uintptr_t)the_regions);
    printf("  .rodata          %p   %s\n", (void *)message, message);
    printf("  .data            %p   value %d\n",
           (void *)&initialised, initialised);
    printf("  .bss             %p   value %d\n", (void *)&zeroed, zeroed);
    printf("  .bss (buffer)    %p   1024 bytes\n", (void *)buffer);
    printf("  heap             %p   from malloc()\n", heap);
    printf("  stack            %p   a local variable\n", (void *)&local);

    free(heap);

    puts("");
    puts("Read the addresses from top to bottom. Code and constants sit low.");
    puts("Then the variables. The heap comes after them. The stack is far");
    puts("away at the top, with a huge empty gap in between.");
    puts("");
    puts("That gap is deliberate. The heap grows upward and the stack grows");
    puts("downward, so they grow toward each other through the free space.");
}

/* ------------------------------------------------------------------ */
/* 2. What each region costs                                           */
/* ------------------------------------------------------------------ */

static void what_it_costs(void)
{
    section("2. what each region costs on an MCU");

    puts("  .text     your code.              FLASH only.");
    puts("  .rodata   const data.             FLASH only.");
    puts("  .data     globals WITH a value.   FLASH and RAM.");
    puts("  .bss      globals with no value.  RAM only.");
    puts("  stack     locals, call frames.    RAM only.");
    puts("  heap      malloc.                 RAM only.");
    puts("");
    puts(".data is the expensive one, and it surprises people. The initial");
    puts("values have to be stored somewhere while the power is off, so they");
    puts("live in flash - and they must also exist in RAM so the program can");
    puts("change them. You pay twice.");
    puts("");
    puts(".bss costs RAM but no flash. The image only records 'clear N bytes");
    puts("at this address'. A 4 KB buffer of zeros adds 4 KB of RAM and");
    puts("almost nothing to your firmware file.");
    puts("");
    puts("Run 'size 09-memory-map' and you get exactly three numbers:");
    puts("text, data, bss. Flash used = text + data. RAM used = data + bss,");
    puts("plus whatever the stack and heap take at run time.");
}

/* ------------------------------------------------------------------ */
/* 3. What happens before main()                                       */
/* ------------------------------------------------------------------ */

static void before_main(void)
{
    section("3. what runs before main()");

    printf("initialised = %d   (this value came from flash)\n", initialised);
    printf("zeroed      = %d   (nothing wrote this - the startup code did)\n",
           zeroed);

    puts("");
    puts("On a PC the operating system prepares all this. On a microcontroller");
    puts("there is no operating system, so a small piece of assembly runs");
    puts("first. It is usually called Reset_Handler, and it does three things:");
    puts("");
    puts("  1. copy the .data section from flash into RAM");
    puts("  2. fill the whole .bss section with zeros");
    puts("  3. call main()");
    puts("");
    puts("That is the entire answer to 'why is an uninitialised global zero");
    puts("but an uninitialised local is garbage'. Step 2 covers the globals.");
    puts("Nothing does step 2 for your stack.");
    puts("");
    puts("It also explains a classic bug: code that runs BEFORE step 1 sees");
    puts("uninitialised globals. If you put something clever in a constructor");
    puts("or in early startup, it can read values that have not arrived yet.");
}

/* ------------------------------------------------------------------ */
/* 4. Which way the stack grows                                        */
/* ------------------------------------------------------------------ */

static void descend(int depth)
{
    int marker = depth;     /* a fresh local at every level */

    printf("  depth %d   local variable at %p\n", depth, (void *)&marker);

    if (depth < 4) {
        descend(depth + 1);
    }
}

static void stack_direction(void)
{
    section("4. which way the stack grows");

    descend(1);

    puts("");
    puts("Each call gets its own copy of 'marker', and the addresses go");
    puts("DOWN. Every function call pushes a new frame below the last one:");
    puts("the return address, the saved registers, and the locals.");
    puts("");
    puts("So deep call chains and big locals both eat the same resource.");
    puts("A recursive function on an MCU is dangerous for this reason - the");
    puts("depth is often decided by input data, and nothing checks it.");
}

/* ------------------------------------------------------------------ */
/* 5. Stack overflow without an operating system                       */
/* ------------------------------------------------------------------ */

static void stack_overflow(void)
{
    section("5. running out of stack");

    puts("On your laptop the memory manager notices when the stack passes");
    puts("its limit, and the program dies with a segmentation fault. Ugly,");
    puts("but honest - you know immediately.");
    puts("");
    puts("A Cortex-M0 has no memory manager. The stack simply keeps growing");
    puts("downward into whatever is below it, which is your .bss. Your");
    puts("variables get overwritten by call frames. Nothing faults. The");
    puts("program keeps running with quietly corrupted data.");
    puts("");
    puts("This is the hardest class of bug in embedded work, because the");
    puts("symptom appears far away from the cause. Common defences:");
    puts("");
    puts("  - fill the stack region with a known pattern at startup, then");
    puts("    check later how far down it was overwritten");
    puts("  - avoid recursion");
    puts("  - keep large arrays out of locals: make them static, or let the");
    puts("    caller pass a buffer in");
    puts("  - on Cortex-M with an MPU, set a guard region below the stack");
    puts("");
    puts("A 1 KB array as a local costs 1 KB of stack for the whole call.");
    puts("The same array declared static costs 1 KB of .bss - the same RAM,");
    puts("but it is accounted for at link time, where 'size' can show it to");
    puts("you before the board ever runs.");
}

/* ------------------------------------------------------------------ */
/* 6. Why malloc is rare in firmware                                   */
/* ------------------------------------------------------------------ */

static void about_malloc(void)
{
    void *a = malloc(32);
    void *b = malloc(32);

    section("6. the heap, and why firmware avoids it");

    printf("  malloc(32) -> %p\n", a);
    printf("  malloc(32) -> %p\n", b);

    free(a);
    free(b);

    puts("");
    puts("malloc works fine on an MCU. It is avoided anyway, for reasons");
    puts("that have nothing to do with correctness:");
    puts("");
    puts("  Fragmentation. Allocate and free different sizes for long enough");
    puts("  and the free space becomes many small holes. A request for 200");
    puts("  bytes fails while 2 KB is free, and you cannot defragment.");
    puts("");
    puts("  Timing. malloc walks a list. How long it takes depends on the");
    puts("  state of the heap. In a control loop that must answer within a");
    puts("  fixed number of microseconds, 'it depends' is not acceptable.");
    puts("");
    puts("  No recovery. When malloc returns NULL on a server you log it and");
    puts("  retry. On a gimbal controller there is nothing sensible to do.");
    puts("");
    puts("So firmware usually decides all its memory at compile time: static");
    puts("buffers and fixed-size pools. Then 'size' tells you the whole story");
    puts("before you flash the board, and the answer never changes at run");
    puts("time. That predictability is worth more than the flexibility.");
}

int main(void)
{
    the_regions();
    what_it_costs();
    before_main();
    stack_direction();
    stack_overflow();
    about_malloc();

    printf("\n");
    return 0;
}
