#include "cards.h"
#include "messaging.h"
#include <pebble.h>

static Window *s_window;

static void prv_init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_click_config_provider(s_window, cards_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = cards_window_load,
                                           .unload = cards_window_unload,
                                       });
  window_stack_push(s_window, true);
  messaging_init(cards_update);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
