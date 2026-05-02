#include "ring_buffer.h"

#include <string.h>

static bool rb_is_valid(const RB_HandleTypeDef *hRb)
{
    return ((hRb != NULL) && (hRb->pBuffer != NULL) && (hRb->Size > 0u));
}

void rb_init(RB_HandleTypeDef *hRb, uint8_t *storage, size_t capacity)
{
    if ((hRb == NULL) || (storage == NULL) || (capacity == 0u))
    {
        return;
    }

    hRb->pBuffer = storage;
    hRb->Size = capacity;
    hRb->WriteIdx = 0u;
    hRb->ReadIdx = 0u;
    hRb->Count = 0u;

    (void)memset(storage, 0, capacity);
}

void rb_reset(RB_HandleTypeDef *hRb)
{
    if (!rb_is_valid(hRb))
    {
        return;
    }

    hRb->WriteIdx = 0u;
    hRb->ReadIdx = 0u;
    hRb->Count = 0u;
}

bool rb_is_empty(const RB_HandleTypeDef *hRb)
{
    if (!rb_is_valid(hRb))
    {
        return true;
    }

    return (hRb->Count == 0u);
}

bool rb_is_full(const RB_HandleTypeDef *hRb)
{
    if (!rb_is_valid(hRb))
    {
        return false;
    }

    return (hRb->Count == hRb->Size);
}

size_t rb_capacity(const RB_HandleTypeDef *hRb)
{
    if (!rb_is_valid(hRb))
    {
        return 0u;
    }

    return hRb->Size;
}

size_t rb_count(const RB_HandleTypeDef *hRb)
{
    if (!rb_is_valid(hRb))
    {
        return 0u;
    }

    return hRb->Count;
}

size_t rb_free_space(const RB_HandleTypeDef *hRb)
{
    if (!rb_is_valid(hRb))
    {
        return 0u;
    }

    return (hRb->Size - hRb->Count);
}

bool rb_push_byte(RB_HandleTypeDef *hRb, uint8_t value)
{
    if (!rb_is_valid(hRb) || rb_is_full(hRb))
    {
        return false;
    }

    hRb->pBuffer[hRb->WriteIdx] = value;
    hRb->WriteIdx = (hRb->WriteIdx + 1u) % hRb->Size;
    hRb->Count++;

    return true;
}

size_t rb_push(RB_HandleTypeDef *hRb, const uint8_t *data, size_t length)
{
    size_t written = 0u;

    if (!rb_is_valid(hRb) || (data == NULL))
    {
        return 0u;
    }

    while ((written < length) && rb_push_byte(hRb, data[written]))
    {
        written++;
    }

    return written;
}

bool rb_pop_byte(RB_HandleTypeDef *hRb, uint8_t *value)
{
    if (!rb_is_valid(hRb) || (value == NULL) || rb_is_empty(hRb))
    {
        return false;
    }

    *value = hRb->pBuffer[hRb->ReadIdx];
    hRb->ReadIdx = (hRb->ReadIdx + 1u) % hRb->Size;
    hRb->Count--;

    return true;
}

size_t rb_pop(RB_HandleTypeDef *hRb, uint8_t *data, size_t length)
{
    size_t read = 0u;

    if (!rb_is_valid(hRb) || (data == NULL))
    {
        return 0u;
    }

    while ((read < length) && rb_pop_byte(hRb, &data[read]))
    {
        read++;
    }

    return read;
}

size_t rb_preview(RB_HandleTypeDef *hRb, uint8_t *data, size_t length)
{
    size_t read = 0u;

    if (!rb_is_valid(hRb) || (data == NULL))
    {
        return 0u;
    }

    while ((read < length) && rb_peek(hRb, read, &data[read]))
    {
        read++;
    }

    return read;
}

bool rb_peek(const RB_HandleTypeDef *hRb, size_t index, uint8_t *value)
{
    size_t pos;

    if (!rb_is_valid(hRb) || (value == NULL) || (index >= hRb->Count))
    {
        return false;
    }

    pos = (hRb->ReadIdx + index) % hRb->Size;
    *value = hRb->pBuffer[pos];
    return true;
}

bool rb_contains_byte(const RB_HandleTypeDef *hRb, uint8_t value)
{
    size_t i;
    uint8_t current;

    if (!rb_is_valid(hRb))
    {
        return false;
    }

    for (i = 0u; i < hRb->Count; ++i)
    {
        if (rb_peek(hRb, i, &current) && (current == value))
        {
            return true;
        }
    }

    return false;
}

size_t rb_discard(RB_HandleTypeDef *hRb, size_t length)
{
    size_t discarded = 0u;
    uint8_t ignored;

    if (!rb_is_valid(hRb))
    {
        return 0u;
    }

    while ((discarded < length) && rb_pop_byte(hRb, &ignored))
    {
        discarded++;
    }

    return discarded;
}
