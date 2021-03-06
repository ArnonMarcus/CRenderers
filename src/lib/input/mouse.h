#pragma once

#include "lib/core/types.h"
#include "lib/globals/inputs.h"

void initMouse() {
    mouse_moved = false;
    mouse_is_captured = false;
    mouse_double_clicked = false;
    mouse_wheel_scrolled= false;

    mouse_wheel_scroll_amount = 0;
    mouse_pos.x = 0;
    mouse_pos.y = 0;
    mouse_pos_raw_diff.x = 0;
    mouse_pos_raw_diff.y = 0;

    middle_mouse_button.is_pressed = false;
    middle_mouse_button.is_released = false;
    middle_mouse_button.up_pos.x = 0;
    middle_mouse_button.down_pos.x = 0;

    right_mouse_button.is_pressed = false;
    right_mouse_button.is_released = false;
    right_mouse_button.up_pos.x = 0;
    right_mouse_button.down_pos.x = 0;

    left_mouse_button.is_pressed = false;
    left_mouse_button.is_released = false;
    left_mouse_button.up_pos.x = 0;
    left_mouse_button.down_pos.x = 0;
}

void setMouseButtonDown(MouseButton *mouse_button, i32 x, i32 y) {
    mouse_button->is_pressed = true;
    mouse_button->is_released = false;

    mouse_button->down_pos.x = x;
    mouse_button->down_pos.y = y;
}

void setMouseButtonUp(MouseButton *mouse_button, i32 x, i32 y) {
    mouse_button->is_released = true;
    mouse_button->is_pressed = false;

    mouse_button->up_pos.x = x;
    mouse_button->up_pos.y = y;
}

void setMouseWheel(f32 amount) {
    mouse_wheel_scroll_amount += amount * 100;
    mouse_wheel_scrolled = true;
}

void setMousePosition(i32 x, i32 y) {
    mouse_pos.x = x;
    mouse_pos.y = y;
}

void setMouseMovement(i32 x, i32 y) {
    mouse_movement.x = x - mouse_pos.x;
    mouse_movement.y = y - mouse_pos.y;
    mouse_moved = true;
}

void setMouseRawMovement(i32 x, i32 y) {
    mouse_pos_raw_diff.x += x;
    mouse_pos_raw_diff.y += y;
    mouse_moved = true;
}