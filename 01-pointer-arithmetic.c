/*
 * 01 - Pointers and pointer arithmetic
 *
 * A pointer is an ordinary variable. What makes it special is only the
 * meaning of the number it stores: that number is an address.
 *
 * Build:  gcc -Wall -o 01-pointer-arithmetic 01-pointer-arithmetic.c
 * Run:    ./01-pointer-arithmetic
 */

#include <stdio.h>

static void section(const char *title)
{
    printf("\n=== %s ===\n", title);
}

/* ------------------------------------------------------------------ */
/* 1. What a pointer holds                                             */
/* ------------------------------------------------------------------ */
static void basics(void)
{
    int x = 5;
    int *p = &x;      /* here '*' is part of the TYPE: p points to an int */

    section("1. basics");

    printf("x    = %d          value inside x's box\n", x);
    printf("&x   = %p   address of that box\n", (void *)&x);
    printf("p    = %p   p holds that same address\n", (void *)p);
    printf("*p   = %d          follow p, read what is there\n", *p);
    printf("&p   = %p   p is a variable too, it has its own address\n",
           (void *)&p);

    /* '&p' is not stored anywhere. The compiler knows where p lives and
     * computes the address on demand. That is why there is no infinite
     * chain of pointers to pointers to pointers. */

    *p = 42;          /* here '*' is an OPERATION: write through the pointer */
    printf("after *p = 42, x = %d   (x changed without naming x)\n", x);
}

/* ------------------------------------------------------------------ */
/* 2. Arithmetic scales by the pointed-to type                         */
/* ------------------------------------------------------------------ */
static void arithmetic(void)
{
    int    ints[5]    = {10, 20, 30, 40, 50};
    char   chars[5]   = {'a', 'b', 'c', 'd', 'e'};
    double doubles[5] = {1.5, 2.5, 3.5, 4.5, 5.5};

    section("2. pointer arithmetic");

    printf("sizeof(int) = %zu, sizeof(char) = %zu, sizeof(double) = %zu\n\n",
           sizeof(int), sizeof(char), sizeof(double));

    int *pi = ints;   /* an array name already decays to the address of [0] */
    for (int i = 0; i < 5; i++) {
        printf("int    [%d]  value %-4d  address %p\n", i, *pi, (void *)pi);
        pi++;         /* moves by sizeof(int) bytes, not by one byte */
    }
    printf("\n");

    char *pc = chars;
    for (int i = 0; i < 5; i++) {
        printf("char   [%d]  value %-4c  address %p\n", i, *pc, (void *)pc);
        pc++;
    }
    printf("\n");

    double *pd = doubles;
    for (int i = 0; i < 5; i++) {
        printf("double [%d]  value %-4.1f  address %p\n", i, *pd, (void *)pd);
        pd++;
    }

    /* p++ means "advance to the next element", never "advance one byte".
     * That is the whole reason a pointer carries a type. */

    printf("\n&ints[4] - &ints[0] = %ld elements (not bytes)\n",
           (long)(&ints[4] - &ints[0]));
}

/* ------------------------------------------------------------------ */
/* 3. Pointer to pointer                                               */
/* ------------------------------------------------------------------ */
static void pointer_to_pointer(void)
{
    int   x  = 7;
    int  *p  = &x;    /* p  holds the address of x */
    int **pp = &p;    /* pp holds the address of p */

    section("3. pointer to pointer");

    printf("x   = %d\n", x);
    printf("p   = %p    *p  = %d\n", (void *)p, *p);
    printf("pp  = %p    *pp = %p    **pp = %d\n",
           (void *)pp, (void *)*pp, **pp);

    **pp = 99;
    printf("after **pp = 99, x = %d\n", x);
}

/* ------------------------------------------------------------------ */
/* 4. Pass by value vs pass by pointer                                 */
/* ------------------------------------------------------------------ */

/* Broken: receives copies, swaps the copies, copies die on return. */
static void swap_by_value(int a, int b)
{
    int temp = a;
    a = b;
    b = temp;
    printf("  inside swap_by_value:   a = %d, b = %d  (copies)\n", a, b);
}

/* Working: receives addresses, so it reaches the caller's boxes. */
static void swap_by_pointer(int *a, int *b)
{
    int temp = *a;    /* temp is int, not int* - we are saving a VALUE */
    *a = *b;
    *b = temp;
    printf("  inside swap_by_pointer: *a = %d, *b = %d\n", *a, *b);
}

static void passing(void)
{
    int x = 5;
    int y = 10;

    section("4. pass by value vs pass by pointer");

    printf("before:                 x = %d, y = %d\n", x, y);
    swap_by_value(x, y);
    printf("after swap_by_value:    x = %d, y = %d  <- unchanged\n", x, y);

    swap_by_pointer(&x, &y);
    printf("after swap_by_pointer:  x = %d, y = %d  <- swapped\n", x, y);

    /* C always passes by value. swap_by_pointer works because the value
     * being copied is an address, and a copy of an address still refers
     * to the original box. */
}

/* ------------------------------------------------------------------ */
/* 5. Returning more than one result                                   */
/* ------------------------------------------------------------------ */

/* A function returns one value. Extra results travel back through
 * pointers supplied by the caller. */
static void min_max(const int *data, int count, int *out_min, int *out_max)
{
    *out_min = data[0];
    *out_max = data[0];

    for (int i = 1; i < count; i++) {
        if (data[i] < *out_min) *out_min = data[i];
        if (data[i] > *out_max) *out_max = data[i];
    }
}

static void multiple_results(void)
{
    int values[6] = {14, 3, 27, 8, 31, 5};
    int lo, hi;

    section("5. two results from one call");

    min_max(values, 6, &lo, &hi);
    printf("min = %d, max = %d\n", lo, hi);
}

/* ------------------------------------------------------------------ */
/* 6. Arrays are passed as pointers                                    */
/* ------------------------------------------------------------------ */

/* 'int arr[]' in a parameter list is just another spelling of 'int *arr'.
 * The array is never copied - only its address travels. That is why the
 * length has to be passed separately: sizeof() inside here would report
 * the size of a pointer, not of the array. */
static void fill(int arr[], int count, int value)
{
    for (int i = 0; i < count; i++)
        arr[i] = value;
}

static void arrays_decay(void)
{
    int buffer[4] = {0, 0, 0, 0};

    section("6. arrays decay to pointers");

    printf("sizeof(buffer) in main = %zu bytes (the whole array)\n",
           sizeof(buffer));

    fill(buffer, 4, 7);
    printf("after fill: %d %d %d %d\n",
           buffer[0], buffer[1], buffer[2], buffer[3]);

    /* Passing 1000 ints by copy would move 4000 bytes on every call.
     * Passing the address moves 8. On a microcontroller with kilobytes
     * of RAM this is not an optimisation, it is the only option. */
}

/* ------------------------------------------------------------------ */
/* 7. Writing to a fixed address - the embedded case                   */
/* ------------------------------------------------------------------ */
static void fixed_address(void)
{
    section("7. writing to a fixed address");

    puts("On a hosted OS every process gets its own virtual address space,");
    puts("so an arbitrary address is not yours to write to. These two lines");
    puts("would abort this program with SIGSEGV:");
    puts("");
    puts("    int *p = (int *)0x1000;");
    puts("    *p = 42;");
    puts("");
    puts("On an STM32 there is no virtual memory. Peripherals ARE addresses,");
    puts("so the same construct is how a GPIO pin gets driven:");
    puts("");
    puts("    #define GPIOD_ODR (*(volatile uint32_t *)0x40020C14)");
    puts("    GPIOD_ODR = (1 << 12);");
    puts("");
    puts("'volatile' stops the compiler from optimising the write away.");
}

int main(void)
{
    basics();
    arithmetic();
    pointer_to_pointer();
    passing();
    multiple_results();
    arrays_decay();
    fixed_address();

    printf("\n");
    return 0;
}
