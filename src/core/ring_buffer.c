#include "core/ring_buffer.h"
#include <stdlib.h>
#include <string.h>

int ring_buffer_init(ring_buffer_t *rb, size_t elem_size, size_t capacity) {
    if (rb == NULL || elem_size == 0 || capacity == 0) {
        return -1;
    }

    rb->data = calloc(capacity, elem_size);
    if (rb->data == NULL) {
        return -1;
    }

    rb->elem_size = elem_size;
    rb->capacity = capacity;
    rb->count = 0;
    rb->head = 0;
    rb->tail = 0;

    return 0;
}

void ring_buffer_free(ring_buffer_t *rb) {
    if (rb != NULL && rb->data != NULL) {
        free(rb->data);
        rb->data = NULL;
        rb->capacity = 0;
        rb->count = 0;
    }
}

void ring_buffer_push(ring_buffer_t *rb, const void *elem) {
    if (rb == NULL || elem == NULL) {
        return;
    }

    // Copy element to head position
    char *dest = (char *)rb->data + (rb->head * rb->elem_size);
    memcpy(dest, elem, rb->elem_size);

    // Advance head
    rb->head = (rb->head + 1) % rb->capacity;

    if (rb->count < rb->capacity) {
        rb->count++;
    } else {
        // Buffer was full, advance tail (overwrite oldest)
        rb->tail = (rb->tail + 1) % rb->capacity;
    }
}

void *ring_buffer_get(ring_buffer_t *rb, size_t index) {
    if (rb == NULL || index >= rb->count) {
        return NULL;
    }

    // index 0 = oldest (tail), index count-1 = newest
    size_t actual_index = (rb->tail + index) % rb->capacity;
    return (char *)rb->data + (actual_index * rb->elem_size);
}

void *ring_buffer_newest(ring_buffer_t *rb) {
    if (rb == NULL || rb->count == 0) {
        return NULL;
    }

    // Head points to next write position, so newest is one before
    size_t newest_index = (rb->head + rb->capacity - 1) % rb->capacity;
    return (char *)rb->data + (newest_index * rb->elem_size);
}

void *ring_buffer_oldest(ring_buffer_t *rb) {
    if (rb == NULL || rb->count == 0) {
        return NULL;
    }

    return (char *)rb->data + (rb->tail * rb->elem_size);
}

void ring_buffer_clear(ring_buffer_t *rb) {
    if (rb != NULL) {
        rb->count = 0;
        rb->head = 0;
        rb->tail = 0;
    }
}

size_t ring_buffer_copy_to_array(ring_buffer_t *rb, void *dest) {
    if (rb == NULL || dest == NULL || rb->count == 0) {
        return 0;
    }

    char *dest_ptr = (char *)dest;

    for (size_t i = 0; i < rb->count; i++) {
        void *src = ring_buffer_get(rb, i);
        memcpy(dest_ptr, src, rb->elem_size);
        dest_ptr += rb->elem_size;
    }

    return rb->count;
}
