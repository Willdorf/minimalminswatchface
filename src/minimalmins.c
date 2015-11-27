#include <pebble.h>

#define KEY_BACKGROUND_COLOR 0

#define KEY_TEMPERATURE 1
#define KEY_CONDITIONS 2

static Window *window;
static Layer *s_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;

static GColor background_color;

static Layer *s_bluetooth_icon_layer;
static bool s_bluetooth_connected;

static uint8_t s_hour;
static uint8_t s_min;
static uint8_t s_sec;

static const GPathInfo BLUETOOTH_INFO = {
	.num_points = 9,
	.points = (GPoint []) {{3,26},{26,3},{14,15},{14,26},{20,20},{9,9},{14,14},{14,3},{20,9}}
};

GPath *bluetooth_path = NULL;

static void bluetooth_callback(bool connected) {
	
	//show icon if disconnected
	if (!connected) {
		//issue a vibrating alert
		vibes_double_pulse();
	}
	
	s_bluetooth_connected = connected;
	layer_mark_dirty(s_bluetooth_icon_layer);
}

static void bluetooth_update_proc(Layer *layer, GContext *ctx) {
	if (!s_bluetooth_connected) {
		graphics_context_set_stroke_width(ctx, 3);
		graphics_context_set_stroke_color(ctx, gcolor_legible_over(background_color));
		gpath_draw_outline(ctx, bluetooth_path);
	}
}

static void update_time(struct tm *tick_time) {
	s_hour = tick_time->tm_hour;
	s_min = tick_time->tm_min;
	s_sec = tick_time->tm_sec;

	static char buffer[] = "00";

	//update minutes
	strftime(buffer, sizeof("00"), "%M", tick_time);
	text_layer_set_text(s_time_layer, buffer);

	//update the date using localized format
	text_layer_set_text_color(s_date_layer, gcolor_legible_over(background_color));
	static char date_buffer[20];
	strftime(date_buffer, sizeof(date_buffer), "%x", tick_time);
	text_layer_set_text(s_date_layer, date_buffer);

	layer_mark_dirty(s_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time(tick_time);
}

static void draw_watchface(Layer *layer, GContext *ctx) {
	graphics_context_set_fill_color(ctx, gcolor_legible_over(background_color));

	//top dot
	graphics_fill_rect(ctx, GRect(70, 37, 3, 3), 0, 0);

	//right dot
	graphics_fill_rect(ctx, GRect(109, 82, 4, 4), 0, 0);

	//bottom dot
	graphics_fill_rect(ctx, GRect(70, 128, 3, 3), 0, 0);

	//left dot
	graphics_fill_rect(ctx, GRect(30, 82, 4, 4), 0, 0);


	//position the minutes
	uint8_t cur_hour = s_hour % 12;
	if (cur_hour == 0) {
		cur_hour = 12;
	}

	GRect f = layer_get_frame((Layer *) s_time_layer);
	int shift = 6;
	switch (cur_hour) {
		case 12:
			f.origin.x = 59;
			f.origin.y = 13 - shift;
			break;
		case 1:
			f.origin.x = 84;
			f.origin.y = 24 - shift;
			break;
		case 2:
			f.origin.x = 105;
			f.origin.y = 48 - shift;
			break;
		case 3:
			f.origin.x = 113;
			f.origin.y = 72 - shift;
			break;
		case 4:
			f.origin.x = 105;
			f.origin.y = 96 - shift;
			break;
		case 5:
			f.origin.x = 84;
			f.origin.y = 120 - shift;
			break;
		case 6:
			f.origin.x = 59;
			f.origin.y = 131 - shift;
			break;
		case 7:
			f.origin.x = 34;
			f.origin.y = 120 - shift;
			break;
		case 8:
			f.origin.x = 13;
			f.origin.y = 96 - shift;
			break;
		case 9:
			f.origin.x = 5;
			f.origin.y = 72 - shift;
			break;
		case 10:
			f.origin.x = 12;
			f.origin.y = 48 - shift;
			break;
		case 11:
			f.origin.x = 34;
			f.origin.y = 24 - shift;
			break;
		default:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "invalid hour %d", cur_hour);
	}
	layer_set_frame((Layer *) s_time_layer, f);
}

static void set_background_and_text_color(int color) {
	background_color = GColorFromHEX(color);
	window_set_background_color(window, background_color);
	text_layer_set_text_color(s_time_layer, gcolor_legible_over(background_color));
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "inbox received handler");
	Tuple *background_color_t = dict_find(iter, KEY_BACKGROUND_COLOR);

	Tuple *temp_t = dict_find(iter, KEY_TEMPERATURE);
	Tuple *conditions_t = dict_find(iter, KEY_CONDITIONS);

	//Store incoming information
	static char temperature_buffer[8];
	static char conditions_buffer[32];
	static char weather_layer_buffer[42];

	if (temp_t) {
		snprintf(temperature_buffer, sizeof(temperature_buffer), "%d\u00B0", (int) temp_t->value->int32);
	}

	if (conditions_t) {
		snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_t->value->cstring);
	}

	if (conditions_t && temp_t) {
		snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
		text_layer_set_text_color(s_weather_layer, gcolor_legible_over(background_color));
		text_layer_set_text(s_weather_layer, weather_layer_buffer);
	}


	if (background_color_t) {
		int bc = background_color_t->value->int32;
		if (bc == 0) { //quick fix so that black colour persists
			bc++;
		}

		persist_write_int(KEY_BACKGROUND_COLOR, bc);

		set_background_and_text_color(bc);

		APP_LOG(APP_LOG_LEVEL_DEBUG, "background color: %d", bc);
	}

	time_t start_time = time(NULL);
	update_time(localtime(&start_time));
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Message Dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_layer = layer_create(layer_get_bounds(window_get_root_layer(window)));
  layer_add_child(window_get_root_layer(window), s_layer);
  layer_set_update_proc(s_layer, draw_watchface);

  //text layer for displaying the minutes
  s_time_layer = text_layer_create(GRect(0,0,24,24));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  if (persist_read_int(KEY_BACKGROUND_COLOR)) {
	  set_background_and_text_color(persist_read_int(KEY_BACKGROUND_COLOR));
  } else {
	  background_color = GColorWhite;
  }
  s_weather_layer = text_layer_create(GRect(0,152, 144, 14));
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, gcolor_legible_over(background_color));
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentRight);
  text_layer_set_text(s_weather_layer, "Loading...");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));

  s_bluetooth_icon_layer = layer_create(GRect(0,0,30,30));
  layer_set_update_proc(s_bluetooth_icon_layer, bluetooth_update_proc);
  bluetooth_path = gpath_create(&BLUETOOTH_INFO);
  layer_add_child(window_get_root_layer(window), s_bluetooth_icon_layer);

  //show the correct state of the bluetooth connection from the start
#ifdef PBL_SDK_2
  bluetooth_callback(bluetooth_connection_service_peek());
#elif PBL_SDK_3
  bluetooth_callback(connection_service_peek_pebble_app_connection());
#endif

  s_date_layer = text_layer_create(GRect(0,0,144,14));
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(s_date_layer, GColorClear);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
}

static void window_unload(Window *window) {
	text_layer_destroy(s_time_layer);

	//destroy the main layer
	layer_destroy(s_layer);

	//destroy the date layer
	text_layer_destroy(s_date_layer);

	//destroy the weather layer
	text_layer_destroy(s_weather_layer);

	//destroy the bluetooth stuffs
	layer_destroy(s_bluetooth_icon_layer);
	gpath_destroy(bluetooth_path);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  //Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  //Register Callbacks
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  //open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  //Register for Bluetooth connections updates
#ifdef PBL_SDK_2
  bluetooth_connection_service_subscribe(bluetooth_callback);
#elif PBL_SDK_3
  connection_service_subscribe((ConnectionHandlers) {
		  .pebble_app_connection_handler = bluetooth_callback
		  });
#endif

  //display the time right away
  time_t start_time = time(NULL);
  update_time(localtime(&start_time));
}

static void deinit(void) {
  window_destroy(window);
  tick_timer_service_unsubscribe();
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
