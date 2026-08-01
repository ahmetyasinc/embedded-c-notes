/*
 * 10 - function pointers
 *
 * File 01 stored the address of an int in a variable. A function has an
 * address too, and it can be stored the same way.
 *
 * That one fact is what lets a program decide at RUN TIME which code to
 * run - without an if, without a switch, and without knowing at compile
 * time what the answer will be. It is how interrupt vectors, callbacks
 * and state machines are all built.
 *
 * Build:  gcc -Wall -o 10-function-pointers 10-function-pointers.c
 * Run:    ./10-function-pointers
 */

#include <stdio.h>
#include <stdint.h>

static void section(const char *title)
{
    printf("\n=== %s ===\n", title);
}

/* ------------------------------------------------------------------ */
/* 1. A function has an address                                        */
/* ------------------------------------------------------------------ */

static int add(int a, int b)      { return a + b; }
static int subtract(int a, int b) { return a - b; }
static int multiply(int a, int b) { return a * b; }

static void functions_have_addresses(void)
{
    section("1. a function is an address too");

    printf("add      lives at %p\n", (void *)(uintptr_t)add);
    printf("subtract lives at %p\n", (void *)(uintptr_t)subtract);
    printf("multiply lives at %p\n", (void *)(uintptr_t)multiply);

    puts("");
    puts("Those are addresses in .text - the code region from file 09.");
    puts("");
    puts("And just like an array name, a function name used on its own");
    puts("decays into its address. 'add' and '&add' mean the same thing.");
}

/* ------------------------------------------------------------------ */
/* 2. Declaring a variable that holds one                              */
/* ------------------------------------------------------------------ */

static void the_declaration(void)
{
    /* Read it from the inside out:
     *   (*operation)      operation is a pointer
     *   (*operation)(int, int)   ...to a function taking two ints
     *   int (*operation)(int,int) ...that returns int
     */
    int (*operation)(int, int);

    section("2. the declaration");

    operation = add;
    printf("operation = add;       operation(10, 3) = %d\n", operation(10, 3));

    operation = subtract;
    printf("operation = subtract;  operation(10, 3) = %d\n", operation(10, 3));

    operation = multiply;
    printf("operation = multiply;  operation(10, 3) = %d\n", operation(10, 3));

    puts("");
    puts("One variable, three different pieces of code. The call site never");
    puts("changed - only what the variable pointed at.");
    puts("");
    puts("The parentheses around *operation are not decoration:");
    puts("");
    puts("  int (*f)(int, int);   a POINTER to a function returning int");
    puts("  int  *f(int, int);    a FUNCTION returning a pointer to int");
    puts("");
    puts("Two completely different things. Without the parentheses the");
    puts("'*' binds to the return type instead of to the name.");
}

/* ------------------------------------------------------------------ */
/* 3. typedef, so it stays readable                                    */
/* ------------------------------------------------------------------ */

/* The syntax gets ugly fast once these appear in parameter lists and
 * arrays. A typedef gives the shape a name and hides the mess. */
typedef int (*binary_op)(int, int);

/* A function that TAKES a function. The parameter is the interesting
 * part: apply() has no idea what op does, and does not need to. */
static void apply(const char *name, binary_op op, int a, int b)
{
    printf("  %-9s(%d, %d) = %d\n", name, a, b, op(a, b));
}

static void with_typedef(void)
{
    section("3. typedef and callbacks");

    apply("add",      add,      10, 3);
    apply("subtract", subtract, 10, 3);
    apply("multiply", multiply, 10, 3);

    puts("");
    puts("A function passed into another function like this is a CALLBACK.");
    puts("apply() supplies the structure - the printing, the arguments - and");
    puts("the caller supplies the behaviour.");
    puts("");
    puts("This is how qsort works in the standard library: it knows how to");
    puts("sort, you tell it how to compare. And it is how every driver");
    puts("callback in ST's HAL works - the library owns the interrupt, you");
    puts("own what happens when it fires.");
}

/* ------------------------------------------------------------------ */
/* 4. A table of functions instead of a switch                         */
/* ------------------------------------------------------------------ */

/* Message handlers for a CAN bus, one per message type. */
static void on_temperature(const uint8_t *data)
{
    int16_t value = (int16_t)((data[0] << 8) | data[1]);
    printf("  temperature: %.1f C\n", value / 10.0);
}

static void on_position(const uint8_t *data)
{
    int16_t pan  = (int16_t)((data[0] << 8) | data[1]);
    int16_t tilt = (int16_t)((data[2] << 8) | data[3]);
    printf("  position:    pan %d, tilt %d\n", pan, tilt);
}

static void on_status(const uint8_t *data)
{
    printf("  status:      flags 0x%02X\n", data[0]);
}

typedef void (*msg_handler)(const uint8_t *);

/* The array itself is const, so it lands in .rodata - flash, not RAM.
 * That is file 05 and file 09 paying off together. */
static const msg_handler handlers[] = {
    on_temperature,     /* index 0 */
    on_position,        /* index 1 */
    on_status           /* index 2 */
};

#define HANDLER_COUNT (sizeof(handlers) / sizeof(handlers[0]))

static void dispatch(uint8_t type, const uint8_t *data)
{
    /* The bounds check is not optional. An index from the outside world
     * that runs past the end of this table would call whatever bytes
     * happen to sit after it. */
    if (type >= HANDLER_COUNT || handlers[type] == NULL) {
        printf("  unknown message type %u - ignored\n", type);
        return;
    }
    handlers[type](data);
}

static void dispatch_table(void)
{
    uint8_t temp_msg[2] = {0x00, 0xEB};              /* 235 -> 23.5 C */
    uint8_t pos_msg[4]  = {0xFF, 0x88, 0x00, 0x2D};  /* -120, 45      */
    uint8_t stat_msg[1] = {0x03};

    section("4. a table instead of a switch");

    dispatch(0, temp_msg);
    dispatch(1, pos_msg);
    dispatch(2, stat_msg);
    dispatch(9, stat_msg);

    puts("");
    puts("The alternative is a switch with one case per type. The table");
    puts("wins in three ways:");
    puts("");
    puts("  Adding a message type is one line in the array, not a new case");
    puts("  buried in a growing function.");
    puts("");
    puts("  The lookup takes the same time for every type. A switch may");
    puts("  compile into a chain of comparisons, so the last case costs more");
    puts("  than the first - which matters inside an interrupt handler.");
    puts("");
    puts("  The table is data, so it can live in flash and can be changed");
    puts("  at run time if you ever need to.");
}

/* ------------------------------------------------------------------ */
/* 5. A state machine                                                  */
/* ------------------------------------------------------------------ */

/* The same idea, with the table indexed by the current state. This is
 * the shape your gimbal controller will end up having. */

enum state { STATE_IDLE, STATE_HOMING, STATE_TRACKING, STATE_COUNT };

static enum state on_idle(void)
{
    puts("  idle:     waiting for a target");
    return STATE_HOMING;
}

static enum state on_homing(void)
{
    puts("  homing:   moving to the reference position");
    return STATE_TRACKING;
}

static enum state on_tracking(void)
{
    puts("  tracking: following the target");
    return STATE_IDLE;
}

typedef enum state (*state_fn)(void);

static const state_fn state_table[STATE_COUNT] = {
    on_idle,
    on_homing,
    on_tracking
};

static void state_machine(void)
{
    enum state current = STATE_IDLE;

    section("5. a state machine");

    for (int step = 0; step < 5; step++) {
        current = state_table[current]();
    }

    puts("");
    puts("Look at the loop body. One line, and it never mentions a single");
    puts("state by name:");
    puts("");
    puts("      current = state_table[current]();");
    puts("");
    puts("Each state function does its work and returns the next state.");
    puts("Adding a state means adding a function and one table entry - the");
    puts("loop stays exactly as it is.");
}

/* ------------------------------------------------------------------ */
/* 6. The vector table, and the ways this bites                        */
/* ------------------------------------------------------------------ */

static void the_real_thing(void)
{
    section("6. the interrupt vector table");

    puts("Everything above was practice for one real case.");
    puts("");
    puts("At the very start of a Cortex-M's flash sits an array of function");
    puts("pointers called the vector table. Entry 0 is the initial stack");
    puts("pointer, entry 1 is Reset_Handler (file 09), and after that comes");
    puts("one entry per interrupt source - timers, UART, CAN, EXTI.");
    puts("");
    puts("When the CAN peripheral receives a frame, no code decides to call");
    puts("your handler. The HARDWARE reads the pointer at the CAN slot and");
    puts("jumps there. Writing 'void CAN1_RX0_IRQHandler(void)' with that");
    puts("exact name is how your function gets its address into that slot.");
    puts("");
    puts("Two things to be careful about.");
    puts("");
    puts("A NULL function pointer is not like a NULL data pointer. Calling");
    puts("one jumps to address 0 and executes whatever is there. On a PC you");
    puts("get a segfault. On a Cortex-M you get a HardFault, or worse, the");
    puts("processor runs the bytes it finds. Check before you call - the");
    puts("dispatch() above does exactly that.");
    puts("");
    puts("And the compiler cannot see through an indirect call. It cannot");
    puts("inline it, and it cannot know what the target touches, so it makes");
    puts("pessimistic assumptions around it. A table lookup plus an indirect");
    puts("branch is still only a few cycles - but in the innermost loop of a");
    puts("control law, a direct call may be the better trade.");
}

int main(void)
{
    functions_have_addresses();
    the_declaration();
    with_typedef();
    dispatch_table();
    state_machine();
    the_real_thing();

    printf("\n");
    return 0;
}
