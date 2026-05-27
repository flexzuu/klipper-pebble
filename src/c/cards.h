#pragma once

#include <pebble.h>

void cards_window_load(Window *window);
void cards_window_unload(Window *window);
void cards_update(void);
void cards_click_config_provider(void *context);
