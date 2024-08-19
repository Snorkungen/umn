/* This is an arena allocator used for the umn parser ... */

#ifndef UMN_ARENA_H
#define UMN_ARENA_H

#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>

struct UMN_Arena
{
    void *head; /* place to allocate continue allocation  */
    void *prev; /* most recent allocation */

    void *start; /* the backing buffer of the arena */
    void *end;   /* where the allocation buffer has its end */

    /* would be fun the arena could track the average size of allocation ?? */
};

/* Initialize an arena based on a given size */
struct UMN_Arena *
umn_arena_init(size_t arena_size)
{
    /* allocate enough room for the arena data and  */

    struct UMN_Arena *arena = malloc(sizeof(struct UMN_Arena) + arena_size);

    if (arena == NULL)
    {
        return arena;
    }
    /* set the fields of the arena */
    arena->start = arena; /* pointer to itself why not ?? */
    arena->end = (void *)arena + sizeof(struct UMN_Arena) + arena_size;

    arena->head = (void *)arena + sizeof(struct UMN_Arena);
    arena->prev = arena->head;

    return arena;
}

void umn_arena_delete(struct UMN_Arena *arena)
{
    /* this could be a macro ...*/
    free(arena);
}

/* reset the area and do stuff i guess */
void umn_arena_reset(struct UMN_Arena *arena)
{
    if (arena == NULL)
    {
        return;
    }
    arena->head = arena->start + sizeof(struct UMN_Arena);
    arena->prev = arena->head;

    /* this might be doing something dumb ??*/
    /* set the memory to zero */
    memset(arena->head, 0, arena->end - arena->head);
};

/* if it was the most recent allocation the free the data otherwise do nothing */
void umn_arena_free(struct UMN_Arena *arena, void *allocation)
{
    if (arena == NULL)
    {
        return;
    }

    if (allocation != arena->prev)
    {
        return; /* there is nothing to do */
    }

    /* NOTE the allocation size header is being left */

    arena->head = arena->prev - sizeof(size_t);
    arena->prev = arena->head;

    /* this might be doing something dumb ??*/
    /* set the memory to zero */
    memset(arena->head, 0, arena->end - arena->head);
};

/* allocate an amount of bytes from the arena */
void *umn_arena_alloc(struct UMN_Arena *arena, size_t alloc_size)
{
    if (arena == NULL)
    {
        return NULL;
    }

    /* what is memory alignment ?? */

    /* check that allocation fits into arena */
    if ((arena->head + alloc_size + sizeof(size_t)) >= arena->end)
    {
        /* do something to cope with need for more memory */

        /* lazy call realloc to see if it would be possible to extend the allocation */
        void *ptr = realloc(arena->start, (arena->end - arena->start) + (alloc_size * 2)); /* arbitrary increase the allocation size with random amount because why not */
        if (ptr != arena->start)
        {
            fputs("umn_arena_alloc: not enough memory to cope with the allocation \n", stderr);
            free(ptr);
            return NULL;
        }
    }

    /* do the actual memory allocation */
    void *allocation = arena->head;

    arena->prev = arena->head + sizeof(size_t);
    arena->head = arena->head + alloc_size + sizeof(size_t);

    *(size_t *)(allocation) = alloc_size;

    return allocation + sizeof(size_t);
}

#define umn_arena_allocation_size(allocation) *(size_t *)((size_t *)allocation - 1)

/* extend the current allocation if possible */
void *umn_arena_realloc(struct UMN_Arena *arena, void *allocation, size_t alloc_size)
{
    if (arena == NULL)
    {
        return NULL;
    }

    /* check if this is the most recent allocation */
    if (arena->prev == allocation)
    {
        /* WARNING the following relies on the fact that alloc does not touch the actual bytes */
        arena->head = arena->prev - sizeof(size_t); /* just move the head back so that the program then moves the head forwar x bytes*/
        return umn_arena_alloc(arena, alloc_size);
    }
    else
    {
        size_t prev_size = umn_arena_allocation_size(allocation);
        umn_arena_free(arena, allocation); /* free the allocation because a new allocation is going to happen */

        void *new_allocation = umn_arena_alloc(arena, alloc_size);

        if (new_allocation == NULL)
        {
            return NULL;
        }

        memmove(new_allocation, allocation, prev_size);

        return new_allocation;
    }

    return NULL;
}

/* A fixed LIFO stack of same sized elements */
struct UMN_Stack
{
    void *data; /* backing memory allocation */

    int direction; /* 1 if stack grows upwards, -1 stack grows downward */
    size_t index;  /* stack pointer */
    size_t element_capacity;
    size_t element_size;
};

struct UMN_Stack *
umn_stack_init(struct UMN_Arena *arena, size_t element_capacity, size_t element_size)
{
    /* first allocate memory for the stack from the arena */
    /* I'm just lazy and  use an arena for all allocations so the arena can just be deleted at a later date */
    const size_t allocation_size = (element_capacity * element_size) + (sizeof(struct UMN_Stack));
    struct UMN_Stack *stack = umn_arena_alloc(arena, allocation_size);

    if (stack == NULL)
    {
        return NULL; /* the arena allocation failed */
    }

    /* set the stack member values */
    stack->data = (void *)stack + sizeof(struct UMN_Stack);

    stack->direction = 1;
    stack->element_capacity = element_capacity;
    stack->element_size = element_size;
    stack->index = 0;

    return stack;
}

struct UMN_Stack *
umn_stack_resize(struct UMN_Stack, struct UMN_Arena *arena, size_t new_element_capacity)
{
    assert(0), "TODO: implement this method";
}

/* @return 0 = success, -1 = fail */
int umn_stack_push(struct UMN_Stack *stack, void *src_element, struct UMN_Arena *arena)
{
    if (stack == NULL || src_element == NULL)
    {
        return -1; /* bad parameters */
    }

    /* check if there is room in the stack */
    if ((stack->index + 1) >= stack->element_capacity)
    {
        if (arena == NULL)
        {
            return -1; /* no arena given */
        }
        
        return -1; /* no room in stack */
    }

    /* memmove data into the stack */
    memmove(
        ((void *)stack->data + (stack->index * stack->element_size) * stack->direction), /* destination */
        src_element,                                                                     /* source */
        stack->element_size);

    /* increment index */
    stack->index += 1;

    return 0;
}

/* @return 0 = success, -1 = fail */
int umn_stack_pop(struct UMN_Stack *stack, void *dest_element)
{
    if (stack == NULL || dest_element == NULL)
    {
        return -1; /* bad parameters */
    }

    /* check that stack can pop */
    if (stack->index == 0)
    {
        return -1;
    }

    /* decrement index */
    stack->index -= 1;

    /* read from index position into element */
    memmove(
        dest_element,                                                                    /* destination */
        ((void *)stack->data + (stack->index * stack->element_size) * stack->direction), /* source */
        stack->element_size);

    return 0;
}

/* inplace reverse the direction that the stack goes in
    @return 0 = success, -1 = fail
*/
int umn_stack_reverse(struct UMN_Stack *stack)
{
    if (stack == NULL)
    {
        return -1;
    }

    stack->direction *= -1;
    /* capacity remains the same but the data is moved */
    int data_move_amount;
    if (stack->direction < 0)
    {
        /* move data to the right (upward) */
        memmove(
            /* the destination is data + (capacity - index + 1)*/
            stack->data + stack->element_size * (stack->element_capacity - (stack->index)),
            stack->data,
            stack->element_size * stack->index);

        stack->data = stack->data + (stack->element_size * (stack->element_capacity - 1));
    }
    else
    {
        /* move data to the left (down ward)*/
        stack->data = stack->data - (stack->element_size * (stack->element_capacity - 1));
        memmove(
            stack->data,
            stack->data + stack->element_size * (stack->element_capacity - (stack->index)),
            stack->element_size * (stack->index ));
    }

    return 0;
}

#endif
