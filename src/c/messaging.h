#pragma once

#include <pebble.h>

typedef void (*MessagingUpdateCallback)(void);

void messaging_init(MessagingUpdateCallback on_update);

int messaging_get_nozzle_temp(void);
int messaging_get_nozzle_target(void);
int messaging_get_bed_temp(void);
int messaging_get_bed_target(void);
int messaging_get_print_progress(void);
int messaging_get_print_time_left(void);
const char *messaging_get_print_state(void);
const char *messaging_get_printer_name(void);
int messaging_get_printer_index(void);
int messaging_get_printer_count(void);
void messaging_select_next_printer(void);
