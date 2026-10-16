#ifndef SP01_RGB_TEST_H
#define SP01_RGB_TEST_H

#include "sp01_app.h"

typedef struct {
    app_u8 phase;
    app_u32 phase_elapsed_ms;
} sp01_rgb_test_t;

void sp01_rgb_test_init(sp01_rgb_test_t *test);
sp01_outputs_t sp01_rgb_test_tick(sp01_rgb_test_t *test);

#endif
