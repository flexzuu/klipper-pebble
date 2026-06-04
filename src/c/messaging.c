#include "messaging.h"

#include "message_keys.auto.h"
#include <pebble.h>
#include <stdio.h>

static const uint32_t s_inbox_size = 1024;
static const uint32_t s_outbox_size = 64;

static int s_nozzle_temp = 0, s_nozzle_target = 0;
static int s_bed_temp = 0, s_bed_target = 0;
static int s_print_progress = 0, s_print_time_left = 0;
static int s_printer_index = 0, s_printer_count = 0;
static char s_print_state[32] = "standby";
static char s_printer_name[32] = "No printer";

static MessagingUpdateCallback s_on_update = NULL;

int messaging_get_nozzle_temp(void) {
  return s_nozzle_temp;
}
int messaging_get_nozzle_target(void) {
  return s_nozzle_target;
}
int messaging_get_bed_temp(void) {
  return s_bed_temp;
}
int messaging_get_bed_target(void) {
  return s_bed_target;
}
int messaging_get_print_progress(void) {
  return s_print_progress;
}
int messaging_get_print_time_left(void) {
  return s_print_time_left;
}
const char *messaging_get_print_state(void) {
  return s_print_state;
}
const char *messaging_get_printer_name(void) {
  return s_printer_name;
}
int messaging_get_printer_index(void) {
  return s_printer_index;
}
int messaging_get_printer_count(void) {
  return s_printer_count;
}

void messaging_select_next_printer(void) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox begin failed: %d", (int)result);
    return;
  }

  dict_write_int(iter, MESSAGE_KEY_SelectPrinter, &(int){1}, sizeof(int), true);
  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed: %d", (int)result);
  }
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

  t = dict_find(iter, MESSAGE_KEY_PrinterName);
  if (t) {
    snprintf(s_printer_name, sizeof(s_printer_name), "%s", t->value->cstring);
  }

  t = dict_find(iter, MESSAGE_KEY_PrinterIndex);
  if (t) {
    s_printer_index = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_PrinterCount);
  if (t) {
    s_printer_count = t->value->int32;
  }

  if (s_on_update) {
    s_on_update();
  }
}

static void prv_inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

void messaging_init(MessagingUpdateCallback on_update) {
  s_on_update = on_update;
  app_message_register_inbox_received(prv_inbox_received_callback);
  app_message_register_inbox_dropped(prv_inbox_dropped_callback);
  app_message_open(s_inbox_size, s_outbox_size);
}
