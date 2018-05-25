#include <pebble.h>
#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_SHAREPRICE 2
#define MESSAGE_KEY_JSReady 3
#define KEY_GIMMEDATA 4
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_left_layer;
static TextLayer *s_date_right_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_bluetooth_layer;
static TextLayer *s_debug_layer;
static TextLayer *s_share_layer;
static TextLayer *s_step_layer;
// static TextLayer *s_hometime_layer;
static TextLayer *s_tz_layer;
int first_run = 1;
static bool s_js_ready;

static void update_date(struct tm *tt) {
  static char bufferl[] = "xxx xx";
  static char bufferr[] = "xxx xxxx";
  static char tzbuffer[] = "ABCD";
  
  strftime(bufferl, sizeof(bufferl), "%a %d", tt);
  strftime(bufferr, sizeof(bufferr), "%b %Y", tt);
  strftime(tzbuffer, sizeof(tzbuffer), "%Z", tt);
  text_layer_set_text(s_date_left_layer, bufferl);
  text_layer_set_text(s_date_right_layer, bufferr);
  text_layer_set_text(s_tz_layer, tzbuffer);
}

int store_int (char *buff, int percentage) {
  int t = percentage;
  int s = 0;
  if (percentage == 0) {
    buff[0] = '0';
    return (1);
  }
  for (s = 0 ; t > 0 ; s++)
    t /= 10;
  for (t = s ; t > 0 ; ) {
    buff[--t] = (percentage % 10) + '0';
    percentage /= 10;
  }
  return (s);
}

static void step_up() {
  static char steps[] = "No steps";
  
#if defined(PBL_HEALTH)
  // Use the step count metric
  HealthMetric metric = HealthMetricStepCount;

  // Create timestamps for midnight (the start time) and now (the end time)
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  // Check step data is available
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, 
                                                                    start, end);
  if (mask & HealthServiceAccessibilityMaskAvailable)
  {
    snprintf(steps, sizeof(steps), "%d", (int)health_service_sum_today(metric));
  }
#endif
  
  text_layer_set_text(s_step_layer, steps);
}

bool comm_is_js_ready() {
  return s_js_ready;
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  // struct tm *home_time = gmtime(&temp);
  
  // if (tick_time->tm_isdst)
  // {
  //  home_time->tm_hour = (home_time->tm_hour + 1) % 24;
  // }

  if (first_run || (tick_time->tm_min == 0))
  {
    first_run = 0;
    update_date(tick_time);
  }
  // Create a long-lived buffer
  static char buffer2[] = "00:00 ";
  // static char htbuffer[] = "\n00";
  
  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer2, sizeof("00:00"), "%H:%M ", tick_time);
    // strftime(htbuffer, sizeof(htbuffer), "\n%H", home_time);
  } else {
    // Use 12 hour format
    strftime(buffer2, sizeof("00:00"), "%I:%M ", tick_time);
    // strftime(htbuffer, sizeof(htbuffer), "\n%I", home_time);
  }
  if (tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    AppMessageResult result = app_message_outbox_begin(&iter);
    if (result == APP_MSG_OK) {
      int value = 0;
      dict_write_int(iter, KEY_GIMMEDATA, &value, sizeof(int), true);
      APP_LOG(APP_LOG_LEVEL_INFO, "Sent request");

      // Send the message!
      app_message_outbox_send();
    } else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
    }
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer2);
  // text_layer_set_text(s_hometime_layer, htbuffer);
  if (first_run || (tick_time->tm_min % 5 == 0))
      {
        step_up();
      }
}
static void debug_text(char *text) {
  text_layer_set_text(s_debug_layer, text);
}
static void share_price(char *text) {
  if (strlen(text) > 0)
    text_layer_set_text(s_share_layer, text);
}

static void step_count(char * text) {
  text_layer_set_text(s_step_layer, text);
}

static void update_bluetooth(bool connected) {
  
  if (connected) {
    if (app_comm_get_sniff_interval() == SNIFF_INTERVAL_NORMAL ) {
      text_layer_set_text(s_bluetooth_layer, "b");
    } else {
      text_layer_set_text(s_bluetooth_layer, "B");
    }
//    debug_text("");
  } else {
    vibes_double_pulse();
    text_layer_set_text(s_bluetooth_layer, ".");
    vibes_long_pulse();
    debug_text("Connection Lost");
  }
}

static void update_battery(BatteryChargeState b) {
  static char buffer[5] = "---%";
  int pos = store_int (buffer, b.charge_percent);
  buffer[pos++] = (b.is_charging ? 'C' : (b.is_plugged ? 'c' : '%'));
  buffer[pos] = 0;
  text_layer_set_text(s_battery_layer, buffer);
  if (b.is_charging){
    debug_text("Charging");
  } else if (b.is_plugged) {
    debug_text("Charged");
  } else if (b.charge_percent < 30) {
    debug_text("Low Battery");
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  // Create time TextLayer
  s_date_left_layer = text_layer_create(GRect(0, 0, 72, 32));
  s_date_right_layer = text_layer_create(GRect(72, 0, 72, 32));
  s_time_layer = text_layer_create(GRect(0, 55, 144, 50));
  s_battery_layer = text_layer_create(GRect(108, 144, 36,32));
  s_bluetooth_layer = text_layer_create(GRect(0, 144, 36, 32));
  s_debug_layer = text_layer_create(GRect(0, 104, 144, 44));
  s_step_layer = text_layer_create(GRect(0, 32, 72, 32));
  s_share_layer = text_layer_create(GRect(72, 32, 72, 32));
  // s_hometime_layer = text_layer_create(GRect(0, 64, 16, 50));
  s_tz_layer = text_layer_create(GRect(36, 144, 72, 32));
  
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_background_color(s_date_left_layer, GColorBlack);
  text_layer_set_background_color(s_date_right_layer, GColorBlack);
  text_layer_set_background_color(s_debug_layer, GColorBlack);
  text_layer_set_background_color(s_step_layer, GColorBlack);
  text_layer_set_background_color(s_share_layer, GColorBlack);
  text_layer_set_background_color(s_battery_layer, GColorBlack);
  text_layer_set_background_color(s_bluetooth_layer, GColorBlack);
  // text_layer_set_background_color(s_hometime_layer, GColorBlack);
  text_layer_set_background_color(s_tz_layer, GColorBlack);
  text_layer_set_text_color(s_tz_layer, GColorWhite);
  text_layer_set_text_color(s_time_layer, GColorYellow);
  text_layer_set_text_color(s_debug_layer, GColorWhite);
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_text_color(s_date_left_layer, GColorInchworm);
  text_layer_set_text_color(s_date_right_layer, GColorWhite);
  text_layer_set_text_color(s_debug_layer, GColorWhite);
  text_layer_set_text_color(s_step_layer, GColorCeleste);
  text_layer_set_text_color(s_share_layer, GColorWhite);
  text_layer_set_text_color(s_bluetooth_layer, GColorWhite);
  // text_layer_set_text_color(s_hometime_layer, GColorWhite);

  // Make sure the time is displayed from the start
  update_time();
  update_battery(battery_state_service_peek());
  update_bluetooth(bluetooth_connection_service_peek());
  // debug_text("Hello");

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_date_left_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_date_left_layer, GTextAlignmentLeft);
  text_layer_set_font(s_date_right_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_date_right_layer, GTextAlignmentRight);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  text_layer_set_font(s_bluetooth_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_bluetooth_layer, GTextAlignmentLeft);
  text_layer_set_font(s_debug_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_debug_layer, GTextAlignmentCenter);
  text_layer_set_font(s_step_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_step_layer, GTextAlignmentLeft);
  text_layer_set_font(s_share_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_share_layer, GTextAlignmentRight);
  // text_layer_set_font(s_hometime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  // text_layer_set_text_alignment(s_hometime_layer, GTextAlignmentCenter);
  text_layer_set_font(s_tz_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_tz_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_tz_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_left_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_right_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_debug_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_bluetooth_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_step_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_share_layer));
  // layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_hometime_layer));
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_left_layer);
  text_layer_destroy(s_date_right_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_bluetooth_layer);
  text_layer_destroy(s_debug_layer);
  text_layer_destroy(s_step_layer);
  text_layer_destroy(s_share_layer);
  // text_layer_destroy(s_hometime_layer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
  debug_text("Message dropped");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! %d", (int)reason);
  Tuple *t = dict_read_first(iterator);
  APP_LOG(APP_LOG_LEVEL_INFO, "message :%s:%d:", t->value->cstring, (int)t->key);
  debug_text("Outbox send failed");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
   APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *ready = dict_find(iterator, MESSAGE_KEY_JSReady);
  if (ready) {
    s_js_ready = true;
  }
  Tuple *t = dict_read_first(iterator);
  static char buffer[32], temp[8], cond[24], shareprice[20];
  APP_LOG(APP_LOG_LEVEL_INFO, "Inbox Callback Pinged");
  Tuple *item = dict_find(iterator, KEY_SHAREPRICE);
  if (item) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Share Price %s", item->value->cstring);
    snprintf(shareprice, 12, "S%s", item->value->cstring);
    share_price(shareprice);
  }

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temp, 8, "%d C", (int)t->value->int32);
      break;
    case KEY_CONDITIONS:
      snprintf(cond, 24, "%s", t->value->cstring);
      break;
    case KEY_SHAREPRICE:
      APP_LOG(APP_LOG_LEVEL_INFO, "SP: %s", t->value->cstring);
      snprintf(shareprice, 12, "%s", t->value->cstring);
      share_price(shareprice);
      break;
    case MESSAGE_KEY_JSReady:
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d %d not recognized!", (int)t->key, (int) KEY_SHAREPRICE);
      break;
    }
    // Look for next item
    t = dict_read_next(iterator);
  }
  snprintf(buffer, sizeof (buffer), "%s, %s", temp, cond);
  debug_text (buffer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  APP_LOG(APP_LOG_LEVEL_INFO, "Max in %d, Max out %d", (int)app_message_inbox_size_maximum(), (int)app_message_outbox_size_maximum());
  //app_message_open(2048, 2048);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe	(update_battery);
  bluetooth_connection_service_subscribe(update_bluetooth);
  
// Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
}

static void deinit() {
    // Destroy Window
    window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}