#include <pebble.h>
#include "main.h"

static Window *s_main_window;
static Layer *window_layer;
static TextLayer *s_currency_layer, *s_time_layer, *s_high_layer, *s_bid_layer,*s_low_layer;
static BitmapLayer *s_icon_layer;
static Layer *s_canvas_layer;
//static *secret_key;
static GBitmap *s_icon_bitmap = NULL;
static AppSync s_sync;
static uint8_t s_sync_buffer[254];
static int s_high, s_bid, s_low, s_vwap, border, border2;
GColor color_setting(), CUSTOM_COLOR;
static char str_high[1000], str_low[1000], str_bid[1000];

//CRAY
// Persistent storage key
#define SETTINGS_KEY 1

// Define our settings struct
typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor ForegroundColor;
  bool SecondTick;
  bool Animations;
} ClaySettings;

// An instance of the struct
static ClaySettings settings;

// Save the settings to persistent storage
static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// AppMessage receive handler
static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Assign the values to our struct
  Tuple *bg_color_t = dict_find(iter, MESSAGE_KEY_BackgroundColor);
  if (bg_color_t) {
    settings.BackgroundColor = GColorFromHEX(bg_color_t->value->int32);
  }
  // ...
  prv_save_settings();
}



//cray

enum ZaifKey {
CURRENCY_KEY = 0x0,
HIGH_KEY     = 0x1,
LOW_KEY      = 0x2,
VWAP_KEY     = 0x3,
BID_KEY      = 0x4,
};

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  // Custom drawing happens here!
	// Set the line color

	GRect rect_bounds = GRect(2, 25, 140, 100);
	#ifdef PBL_COLOR
	graphics_context_set_fill_color(ctx, GColorCeleste);
	graphics_fill_rect(ctx, rect_bounds, 0, GCornersAll);
	#elif PBL_BW
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, rect_bounds, 0, GCornersAll);
	graphics_draw_rect(ctx, rect_bounds);
  #endif

	rect_bounds = GRect(2, 25 + border, 140, 100-border);
	#ifdef PBL_COLOR
	graphics_context_set_fill_color(ctx, GColorMelon);
	graphics_fill_rect(ctx, rect_bounds, 0, GCornersAll);
	#elif PBL_BW
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, rect_bounds, 0, GCornersAll);
	graphics_draw_rect(ctx, rect_bounds);
  #endif
		
	// Set the line color
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_stroke_width(ctx, 5);
	GPoint start = GPoint(0, 25 + border2);
	GPoint end = GPoint(144, 25 + border2);
	// Draw a line
	graphics_draw_line(ctx, start, end);
	
	rect_bounds = GRect(2, 144, 140, 28);
	#ifdef PBL_COLOR
	graphics_context_set_fill_color(ctx, GColorPastelYellow);
	graphics_fill_rect(ctx, rect_bounds, 5, GCornersAll);
	#elif PBL_BW
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, rect_bounds, 5, GCornersAll);
  #endif

}

//時間を表示する
static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);	

  //バッファを作っておく
  static char buffer[] = "00 000 00:00";
  
  //バッファの中に時間と分を書き出す
  if(clock_is_24h_style() == true){
    strftime(buffer, sizeof("00 000 00:00"), "%d %b %H:%M", tick_time);
  } else {
    strftime(buffer, sizeof("00 000 00:00"), "%d %b %I:%M", tick_time);
  }
  //テキストレイヤに時間を表示する
  text_layer_set_text(s_time_layer, buffer);
	
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}


//JSから通信があったら取り込む
static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {

	switch (key) {		
    	case CURRENCY_KEY:
      text_layer_set_text(s_currency_layer, new_tuple->value->cstring);
      break;

			case HIGH_KEY:
		  s_high = (unsigned int)new_tuple->value->uint32;
			snprintf(str_high, 1000 ,"HIGH %d", s_high);
			text_layer_set_text(s_high_layer, str_high);
      break;

			case LOW_KEY:
      s_low = (unsigned int)new_tuple->value->uint32;
			snprintf(str_low, 1000 , "LOW %d", s_low);
			text_layer_set_text(s_low_layer, str_low);
      break;
		
			case VWAP_KEY:
      s_vwap = (unsigned int)new_tuple->value->uint32;
			border2 =  100*(s_high- s_vwap)/(s_high- s_low);

      break;
		
			case BID_KEY:
      s_bid = (unsigned int)new_tuple->value->uint32;
			text_layer_destroy(s_bid_layer);

		border =  100*(s_high- s_bid)/(s_high- s_low);
	
if (border < 24){
  s_bid_layer = text_layer_create(GRect(0,border + 22, 138, 30));
	}else{
  s_bid_layer = text_layer_create(GRect(0, border - 2, 138, 30));	
};
	#ifdef PBL_COLOR
  text_layer_set_text_color(s_bid_layer, COLOR_FALLBACK(GColorBlack, GColorBlack));
	#elif PBL_BW
  text_layer_set_text_color(s_bid_layer, COLOR_FALLBACK(GColorBlack, GColorBlack));
  #endif
	text_layer_set_background_color(s_bid_layer, GColorClear);
  text_layer_set_font(s_bid_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_bid_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_bid_layer));

			snprintf(str_bid, 1000 , "BID %d", s_bid);
			text_layer_set_text(s_bid_layer, str_bid);
      break;
	}

}

GColor color_setting(uint color){
#ifdef PBL_COLOR
	switch(color){
		case 0x1:
			return GColorWhite;
		case 0x2:
			return GColorBlack;
		case 0x3:
			return GColorVividCerulean;
		case 0x4:
			return GColorScreaminGreen;
		case 0x5:
			return GColorFashionMagenta;
		case 0x6:
			return GColorChromeYellow;
		default:
			return GColorWhite;			
	}
#elif PBL_BW
	switch(color){
		case 0x1:
			return GColorWhite;
		case 0x2:
			return GColorBlack;
		default:
			return GColorWhite;			
	}
#endif
}

static void request_weather(void) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (!iter) {
    // Error creating outbound message
    return;
  }

  int value = 1;
  dict_write_int(iter, 1, &value, sizeof(int), true);
  dict_write_end(iter);
  app_message_outbox_send();
}

static void window_load(Window *window) {
window_layer = window_get_root_layer(window);
	
GRect bounds = layer_get_bounds(window_get_root_layer(window));

// Create canvas layer
s_canvas_layer = layer_create(bounds);
	// Assign the custom drawing procedure
layer_set_update_proc(s_canvas_layer, canvas_update_proc);

// Add to Window
layer_add_child(window_get_root_layer(window), s_canvas_layer);
	// Redraw this as soon as possible
layer_mark_dirty(s_canvas_layer);
	
  s_time_layer = text_layer_create(GRect(0, 140, 144, 28));
	#ifdef PBL_COLOR
  text_layer_set_text_color(s_time_layer, COLOR_FALLBACK(GColorBlack, GColorBlack));
  #elif PBL_BW
  text_layer_set_text_color(s_time_layer, COLOR_FALLBACK(GColorBlack, GColorBlack));
  #endif
	text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	
  s_currency_layer = text_layer_create(GRect(0, -5, 140, 30));
	#ifdef PBL_COLOR
  text_layer_set_text_color(s_currency_layer, COLOR_FALLBACK(GColorWhite, GColorWhite));
	#elif PBL_BW
  text_layer_set_text_color(s_currency_layer, COLOR_FALLBACK(GColorWhite, GColorWhite));
  #endif
	text_layer_set_background_color(s_currency_layer, GColorClear);
  text_layer_set_font(s_currency_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_currency_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_currency_layer));


  s_high_layer = text_layer_create(GRect(0, 8, 140, 30));
	#ifdef PBL_COLOR
  text_layer_set_text_color(s_high_layer, COLOR_FALLBACK(GColorWhite, GColorWhite));
	#elif PBL_BW
  text_layer_set_text_color(s_high_layer, COLOR_FALLBACK(GColorWhite, GColorWhite));
  #endif
	text_layer_set_background_color(s_high_layer, GColorClear);
  text_layer_set_font(s_high_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_high_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_high_layer));

  s_low_layer = text_layer_create(GRect(0, 123, 140, 30));
	#ifdef PBL_COLOR
  text_layer_set_text_color(s_low_layer, COLOR_FALLBACK(GColorWhite, GColorWhite));
	#elif PBL_BW
  text_layer_set_text_color(s_low_layer, COLOR_FALLBACK(GColorWhite, GColorWhite));
  #endif
	text_layer_set_background_color(s_low_layer, GColorClear);
  text_layer_set_font(s_low_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_low_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_low_layer));

  Tuplet initial_values[] = {
    TupletCString(CURRENCY_KEY, "BTC/JPY"),
    TupletInteger(HIGH_KEY, (uint32_t) 0),
    TupletInteger(BID_KEY, (uint32_t) 0),
    TupletInteger(LOW_KEY, (uint32_t) 0),
    TupletInteger(VWAP_KEY, (uint32_t) 0),
  };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer), 
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL
  );

  request_weather();
  update_time();
}

static void window_unload(Window *window) {
  if (s_icon_bitmap) {
    gbitmap_destroy(s_icon_bitmap);
  }
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_currency_layer);
  text_layer_destroy(s_high_layer);
  text_layer_destroy(s_bid_layer);
  text_layer_destroy(s_low_layer);
  bitmap_layer_destroy(s_icon_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  update_time();  
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Read color preferences
  Tuple *bg_color_t = dict_find(iter, MESSAGE_KEY_BackgroundColor);
  if(bg_color_t) {
    GColor bg_color = GColorFromHEX(bg_color_t->value->int32);
  }

  Tuple *fg_color_t = dict_find(iter, MESSAGE_KEY_ForegroundColor);
  if(fg_color_t) {
    GColor fg_color = GColorFromHEX(fg_color_t->value->int32);
  }

  // Read boolean preferences
  Tuple *second_tick_t = dict_find(iter, MESSAGE_KEY_SecondTick);
  if(second_tick_t) {
    bool second_ticks = second_tick_t->value->int32 == 1;
  }

  Tuple *animations_t = dict_find(iter, MESSAGE_KEY_Animations);
  if(animations_t) {
    bool animations = animations_t->value->int32 == 1;
  }

}

static void init(void) {
	app_message_outbox_size_maximum();
  s_main_window = window_create();
  window_set_background_color(s_main_window, COLOR_FALLBACK(GColorBlack, GColorBlack));

#ifdef PBL_SDK_2
  window_set_fullscreen(s_main_window, true);
#endif

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_main_window, true);

// Open AppMessage connection
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(254, 254);

//	accel_tap_service_subscribe(tap_handler);

	//時計にウィンドウを表示させて、アニメーションをさせる
  window_stack_push(s_main_window,true);
  
  //タイマーサービスを登録する
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
	
}

static void deinit(void) {
  window_destroy(s_main_window);

  app_sync_deinit(&s_sync);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
