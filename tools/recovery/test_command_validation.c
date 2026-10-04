#include "command_validation.h"
#include <assert.h>
#include <stdio.h>
static void put_float(CanFrame *f, unsigned offset, float value) { memcpy(f->data + offset, &value, 4); }
int main(void) {
    CanFrame f = {.id = 3, .length = 8};
    assert(command_frame_valid(&f, 3));
    for (unsigned family = 0; family < 4; ++family) {
        f.id = 3 + 256 * family;
        for (unsigned length = 0; length <= 64; ++length) {
            f.length = length;
            assert(command_frame_valid(&f, 3) == (length == 8 || (family == 2 && length == 4)));
        }
    }
    f.length = 8;
    f.id = 0x403; assert(!command_frame_valid(&f, 3));
    f.id = 0x803; assert(!command_frame_valid(&f, 3));
    f.id = 4; assert(!command_frame_valid(&f, 3));
    for (unsigned family = 1; family < 4; ++family) {
        f.id = 3 + 256 * family;
        put_float(&f, 0, NAN); assert(!command_frame_valid(&f, 3));
        put_float(&f, 0, INFINITY); assert(!command_frame_valid(&f, 3));
        put_float(&f, 0, -INFINITY); assert(!command_frame_valid(&f, 3));
        put_float(&f, 0, .5f); assert(command_frame_valid(&f, 3));
    }
    f.id = 0x103; put_float(&f, 4, NAN); assert(!command_frame_valid(&f, 3));
    put_float(&f, 4, 20); assert(command_frame_valid(&f, 3));
    for (unsigned code = 0xFB; code <= 0xFE; ++code) {
        memset(f.data, 255, 8); f.data[7] = code;
        assert(command_frame_valid(&f, 3));
        f.length = 7; assert(!command_frame_valid(&f, 3)); f.length = 8;
    }
    for (unsigned dlc = 0; dlc < 16; ++dlc) {
        assert(command_receive_length(0, dlc << 16) == (dlc <= 8 ? dlc : 0));
        assert(command_receive_length(1U << 29, dlc << 16) == 0);
        assert(command_receive_length(1U << 30, dlc << 16) == 0);
    }
    assert(command_scaled_velocity_valid(20, 7));
    assert(command_scaled_velocity_valid(-20, 7));
    assert(!command_scaled_velocity_valid(FLT_MAX, 7));
    assert(!command_scaled_velocity_valid(1, NAN));
    assert(!command_scaled_velocity_valid(1, 0));
    puts("command validation: ID, DLC, RTR/XTD, specials, NaN/Inf and scaled overflow passed");
}
