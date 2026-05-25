#include "drawing.h"
#include "message_keys.auto.h"
#include <pebble.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define NUM_CARDS   3
#define CARD_BED    0
#define CARD_NOZZLE 1
#define CARD_PRINT  2

#define BG_BED    GColorOxfordBlue
#define BG_NOZZLE GColorBulgarianRose
#define BG_PRINT  GColorArmyGreen

#define LABEL_TEXT_BED_TEMP "BED TEMP"
#define LABEL_TEXT_NOZZLE   "NOZZLE"
#define LABEL_TEXT_PRINT    "PRINT"

#define VALUE_TEXT_DONE  "Done"
#define VALUE_TEXT_ERROR "Error"
#define VALUE_TEXT_IDLE  "Idle"

#define SUBTEXT_TEXT_HEATING    "Heating.."
#define SUBTEXT_TEXT_AT_TARGET  "At target"
#define SUBTEXT_TEXT_HEATER_OFF "Heater off"
#define SUBTEXT_TEXT_COMPLETE   "Print complete"
#define SUBTEXT_TEXT_PAUSED     "Paused"
#define SUBTEXT_TEXT_ERROR      "Check printer"
#define SUBTEXT_TEXT_READY      "Ready"

#define ANIM_FRAME_MS 80 // 12.5 fps (1 / 12.5) * 1000

static Window *s_window;
static TextLayer *s_label_layer;
static TextLayer *s_value_layer;
static TextLayer *s_subtext_layer;
static StatusBarLayer *s_status_bar;
static Layer *s_canvas_layer;

// static int s_current_card = CARD_BED;
static int s_current_card = CARD_NOZZLE;

static char s_label_buf[32];
static char s_value_buf[32];
static char s_subtext_buf[32];

static AppTimer *s_anim_timer = NULL;
static int s_anim_frame = 0;

static int s_icon_area_h = 0;

static const uint32_t s_inbox_size = 256;
static const uint32_t s_outbox_size = 64;

static int s_nozzle_temp = 0, s_nozzle_target = 0, s_bed_temp = 0, s_bed_target = 0,
           s_print_progress = 0, s_print_time_left = 0;
static char s_print_state[16] = "standby";

static void prv_draw_card_icon(GContext *ctx, int card, GRect bounds) {
  switch (card) {
  case CARD_BED:
    drawing_draw_bed(ctx, bounds, s_anim_frame, s_bed_target);
    break;
  case CARD_NOZZLE:
    drawing_draw_nozzle(ctx, bounds, s_anim_frame, s_nozzle_target);
    break;
  case CARD_PRINT:
    drawing_draw_print(ctx, bounds, s_anim_frame, s_print_progress, s_print_state);
    break;
  }
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_card >= NUM_CARDS - 1) {
    return;
  }
  s_current_card += 1;
  layer_mark_dirty(s_canvas_layer);
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_card <= 0) {
    return;
  }
  s_current_card -= 1;
  layer_mark_dirty(s_canvas_layer);
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void prv_format_time_remaining(int seconds, char *buf, int buf_size) {
  if (seconds <= 0) {
    snprintf(buf, buf_size, "-- : --");
    return;
  }
  int h = seconds / 3600;
  int m = (seconds % 3600) / 60;
  if (h > 0) {
    snprintf(buf, buf_size, "%dh %02dm left", h, m);
  } else {
    snprintf(buf, buf_size, "%dm left", m);
  }
}

static void prv_set_heater_content(char *label_text, int temp, int target) {
  snprintf(s_label_buf, sizeof(s_label_buf), "%s", label_text);
  snprintf(s_value_buf, sizeof(s_value_buf), "%d\u00B0 / %d\u00B0", temp, target);
  if (target > 0 && temp < target) {
    snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_HEATING);
  } else if (target > 0) {
    snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_AT_TARGET);
  } else {
    snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_HEATER_OFF);
  }
}

static void prv_update_card_text() {
  switch (s_current_card) {
  case CARD_BED:
    prv_set_heater_content(LABEL_TEXT_BED_TEMP, s_bed_temp, s_bed_target);
    break;
  case CARD_NOZZLE:
    prv_set_heater_content(LABEL_TEXT_NOZZLE, s_nozzle_temp, s_nozzle_target);
    break;
  case CARD_PRINT:
    snprintf(s_label_buf, sizeof(s_label_buf), LABEL_TEXT_PRINT);
    if (strcmp(s_print_state, "printing") == 0) {
      snprintf(s_value_buf, sizeof(s_value_buf), "%d%%", s_print_progress);
      prv_format_time_remaining(s_print_time_left, s_subtext_buf, sizeof(s_subtext_buf));
    } else if (strcmp(s_print_state, "complete") == 0) {
      snprintf(s_value_buf, sizeof(s_value_buf), VALUE_TEXT_DONE);
      snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_COMPLETE);
    } else if (strcmp(s_print_state, "paused") == 0) {
      snprintf(s_value_buf, sizeof(s_value_buf), "%d%%", s_print_progress);
      snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_PAUSED);
    } else if (strcmp(s_print_state, "error") == 0) {
      snprintf(s_value_buf, sizeof(s_value_buf), VALUE_TEXT_ERROR);
      snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_ERROR);
    } else {
      snprintf(s_value_buf, sizeof(s_value_buf), VALUE_TEXT_IDLE);
      snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_ERROR);
    }
    break;
  }

  text_layer_set_text(s_label_layer, s_label_buf);
  text_layer_set_text(s_value_layer, s_value_buf);
  text_layer_set_text(s_subtext_layer, s_subtext_buf);
  layer_mark_dirty(s_canvas_layer);
}

static void prv_canvas_update_proc(Layer *layer, GContext *context) {
  GRect bounds = layer_get_bounds(layer);
  bounds.size.h = s_icon_area_h;
  prv_draw_card_icon(context, s_current_card, bounds);
  prv_update_card_text();
}

static void prv_anim_timer_callback(void *context) {
  s_anim_frame++;
  layer_mark_dirty(s_canvas_layer);
  s_anim_timer = app_timer_register(ANIM_FRAME_MS, prv_anim_timer_callback, NULL);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_bar, GColorBlack, GColorWhite);
  status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeNone);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  int content_y = STATUS_BAR_LAYER_HEIGHT;
  int content_h = bounds.size.h - STATUS_BAR_LAYER_HEIGHT;

  s_canvas_layer = layer_create(GRect(0, content_y, bounds.size.w, content_h));
  layer_set_update_proc(s_canvas_layer, prv_canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  int icon_h = (content_h * 55) / 100;
  s_icon_area_h = icon_h;

  // Label
  int label_y = content_y + icon_h;
  s_label_layer = text_layer_create(GRect(0, label_y, bounds.size.w, 22));
  text_layer_set_background_color(s_label_layer, GColorClear);
  text_layer_set_text_color(s_label_layer, GColorLightGray);
  text_layer_set_text_alignment(s_label_layer, GTextAlignmentCenter);
  text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_label_layer));

  // Value — large, prominent
  int value_y = label_y + 20;
  s_value_layer = text_layer_create(GRect(0, value_y, bounds.size.w, 36));
  text_layer_set_background_color(s_value_layer, GColorClear);
  text_layer_set_text_color(s_value_layer, GColorWhite);
  text_layer_set_text_alignment(s_value_layer, GTextAlignmentCenter);
  text_layer_set_font(s_value_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_value_layer));

  // Sub-text
  int sub_y = value_y + 34;
  s_subtext_layer = text_layer_create(GRect(0, sub_y, bounds.size.w, 22));
  text_layer_set_background_color(s_subtext_layer, GColorClear);
  text_layer_set_text_color(s_subtext_layer, GColorDarkGray);
  text_layer_set_text_alignment(s_subtext_layer, GTextAlignmentCenter);
  text_layer_set_font(s_subtext_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_subtext_layer));

  prv_update_card_text();
  s_anim_timer = app_timer_register(ANIM_FRAME_MS, prv_anim_timer_callback, NULL);
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_label_layer);
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

  prv_update_card_text();
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
