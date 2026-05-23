#include "message_keys.auto.h"
#include <pebble.h>
#include <stdint.h>
#include <stdio.h>

static Window *s_window;
static TextLayer *s_text_layer;
static StatusBarLayer *s_status_bar;

static const uint32_t s_inbox_size = 256;
static const uint32_t s_outbox_size = 64;

static int s_nozzle_temp = 0, s_nozzle_target = 0, s_bed_temp = 0, s_bed_target = 0,
           s_print_progress = 0, s_print_time_left = 0;
static char s_print_state[16] = "standby";

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Select");
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Up");
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Down");
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  // GRect bounds = layer_get_bounds(window_layer);

  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_bar, GColorBlack, GColorWhite);
  status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeNone);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  // int content_y = STATUS_BAR_LAYER_HEIGHT;
  // int content_h = bounds.size.h - STATUS_BAR_LAYER_HEIGHT;
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
}

static void prv_inbox_received_callback(DictionaryIterator *iter, void *context) {
  Tuple *t;

  t = dict_find(iter, MESSAGE_KEY_NozzleTemp);
  if (t) {
    s_nozzle_temp = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_NozzleTarget);
  if (t) {
    s_nozzle_target = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_BedTemp);
  if (t) {
    s_bed_temp = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_BedTarget);
  if (t) {
    s_bed_target = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_PrintProgress);
  if (t) {
    s_print_progress = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_PrintTimeLeft);
  if (t) {
    s_print_time_left = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_PrintState);
  if (t) {
    snprintf(s_print_state, sizeof(s_print_state), "%s", t->value->cstring);
  }
}

static void prv_inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_click_config_provider(s_window, prv_click_config_provider);

  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = prv_window_load,
                                           .unload = prv_window_unload,
                                       });
  const bool animated = true;
  window_stack_push(s_window, animated);

  app_message_register_inbox_received(prv_inbox_received_callback);
  app_message_register_inbox_dropped(prv_inbox_dropped_callback);
  app_message_open(s_inbox_size, s_outbox_size);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
