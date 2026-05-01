#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t *pBuffer;
    size_t Size;
    size_t WriteIdx;
    size_t ReadIdx;
    size_t Count;
} RB_HandleTypeDef;

void rb_init(RB_HandleTypeDef *hRb, uint8_t *storage, size_t capacity);
void rb_reset(RB_HandleTypeDef *hRb);

bool rb_is_empty(const RB_HandleTypeDef *hRb);
bool rb_is_full(const RB_HandleTypeDef *hRb);
size_t rb_capacity(const RB_HandleTypeDef *hRb);
size_t rb_count(const RB_HandleTypeDef *hRb);
size_t rb_free_space(const RB_HandleTypeDef *hRb);

bool rb_push_byte(RB_HandleTypeDef *hRb, uint8_t value);
size_t rb_push(RB_HandleTypeDef *hRb, const uint8_t *data, size_t length);

bool rb_pop_byte(RB_HandleTypeDef *hRb, uint8_t *value);
size_t rb_pop(RB_HandleTypeDef *hRb, uint8_t *data, size_t length);

bool rb_peek(const RB_HandleTypeDef *hRb, size_t index, uint8_t *value);
bool rb_contains_byte(const RB_HandleTypeDef *hRb, uint8_t value);
size_t rb_discard(RB_HandleTypeDef *hRb, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* RING_BUFFER_H_ */
