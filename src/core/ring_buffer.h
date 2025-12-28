#ifndef NETPULSE_RING_BUFFER_H
#define NETPULSE_RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Generic ring buffer for fixed-size elements.
 * Thread-safe for single producer, single consumer.
 */

typedef struct {
    void *data;           // Buffer storage
    size_t elem_size;     // Size of each element
    size_t capacity;      // Maximum number of elements
    size_t count;         // Current number of elements
    size_t head;          // Write position (next push)
    size_t tail;          // Read position (oldest element)
} ring_buffer_t;

// Initialize a ring buffer. Returns 0 on success, -1 on error.
int ring_buffer_init(ring_buffer_t *rb, size_t elem_size, size_t capacity);

// Free ring buffer resources
void ring_buffer_free(ring_buffer_t *rb);

// Push an element to the buffer. Overwrites oldest if full.
void ring_buffer_push(ring_buffer_t *rb, const void *elem);

// Get element at index (0 = oldest, count-1 = newest)
// Returns pointer to element or NULL if index out of bounds
void *ring_buffer_get(ring_buffer_t *rb, size_t index);

// Get the newest element (most recently pushed)
// Returns NULL if buffer is empty
void *ring_buffer_newest(ring_buffer_t *rb);

// Get the oldest element
// Returns NULL if buffer is empty
void *ring_buffer_oldest(ring_buffer_t *rb);

// Current number of elements in buffer
static inline size_t ring_buffer_count(const ring_buffer_t *rb) {
    return rb->count;
}

// Check if buffer is empty
static inline bool ring_buffer_empty(const ring_buffer_t *rb) {
    return rb->count == 0;
}

// Check if buffer is full
static inline bool ring_buffer_full(const ring_buffer_t *rb) {
    return rb->count == rb->capacity;
}

// Clear all elements
void ring_buffer_clear(ring_buffer_t *rb);

// Copy all elements to a contiguous array (oldest first)
// dest must have room for at least ring_buffer_count() elements
// Returns number of elements copied
size_t ring_buffer_copy_to_array(ring_buffer_t *rb, void *dest);

#endif // NETPULSE_RING_BUFFER_H
