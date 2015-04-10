/*

   Bold Hour watch

   A digital watch with very large hour digits that take up the entire screen
   and smaller minute digits that fit in the null space of the hour digits.

   This watch's load/unload code is mostly taken from the big_time watchface which has to
   load/unload images as necessary. The same is true for bold-hour.

   Author: Jon Eisen <jon@joneisen.me>

 */

#include <pebble.h>

// -- build configuration --
#define WHITE_TEXT
  
// If this is defined, the face will use minutes and seconds instead of hours and minutes
// to make debugging faster.
//#define DEBUG_FAST

// -- macros --
#define UNINITTED -1

#ifdef WHITE_TEXT
  #define TEXT_COLOR GColorWhite
  #define BKGD_COLOR GColorBlack
#else
  #define TEXT_COLOR GColorBlack
  #define BKGD_COLOR GColorWhite
#endif

// -- Global assets --
Window *window;

TextLayer *minuteLayer;
BitmapLayer *hourLayer;
InverterLayer *btConnectionInverter;

GRect minuteFrame;
GBitmap *hourImage;

int loaded_hour;

// These are all of our images. Each is the entire screen in size.
const int IMAGE_RESOURCE_IDS[24] = {
  RESOURCE_ID_IMAGE_NUM_0, RESOURCE_ID_IMAGE_NUM_1, RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3, RESOURCE_ID_IMAGE_NUM_4, RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6, RESOURCE_ID_IMAGE_NUM_7, RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9, RESOURCE_ID_IMAGE_NUM_10, RESOURCE_ID_IMAGE_NUM_11,
  RESOURCE_ID_IMAGE_NUM_12, RESOURCE_ID_IMAGE_NUM_13, RESOURCE_ID_IMAGE_NUM_14,
  RESOURCE_ID_IMAGE_NUM_15, RESOURCE_ID_IMAGE_NUM_16, RESOURCE_ID_IMAGE_NUM_17,
  RESOURCE_ID_IMAGE_NUM_18, RESOURCE_ID_IMAGE_NUM_19, RESOURCE_ID_IMAGE_NUM_20,
  RESOURCE_ID_IMAGE_NUM_21, RESOURCE_ID_IMAGE_NUM_22, RESOURCE_ID_IMAGE_NUM_23
};


void load_digit_image(int digit_value) {
  if ((digit_value < 0) || (digit_value > 23)) {
    return;
  }

  if (!hourImage) {
    hourImage = gbitmap_create_with_resource(IMAGE_RESOURCE_IDS[digit_value]);
    bitmap_layer_set_bitmap(hourLayer, hourImage);
    loaded_hour = digit_value;
  }

}

void unload_digit_image() {

  if (hourImage) {
    gbitmap_destroy(hourImage);
    hourImage = NULL;
    loaded_hour = -1;
  }

}

void set_minute_layer_location(unsigned short horiz) {
  if (minuteFrame.origin.x != horiz) {
    minuteFrame.origin.x = horiz;
    layer_set_frame(text_layer_get_layer(minuteLayer), minuteFrame);
    layer_mark_dirty(text_layer_get_layer(minuteLayer));
  }
}

void display_time(struct tm * tick_time) {

#ifdef DEBUG_FAST
  //unsigned short hour = tick_time->tm_min % 24;
  unsigned short hour = tick_time->tm_sec % 24;
#else
  unsigned short hour = tick_time->tm_hour % 24;
#endif
  
  // Only do memory unload/load if necessary
  if (loaded_hour != hour) {
    unload_digit_image();
    load_digit_image(hour);
  }

  // Show minute
  static char text[] = "00";

#ifdef DEBUG_FAST
  strftime(text, sizeof(text), "%S", tick_time);
#else
  strftime(text, sizeof(text), "%M", tick_time);
#endif

  unsigned short n1s = (text[0]=='1') + (text[1]=='1');

  if (hour == 10 || (hour >= 12 && hour < 20)) {
    set_minute_layer_location(70 + 3*n1s);
  } else if (hour == 20 || hour >= 22) {
    set_minute_layer_location(88 + 3*n1s);
  } else if (hour == 21) {
    set_minute_layer_location(32 + 3*n1s);
  } else {
    set_minute_layer_location(53 + 3*n1s);
  }

  text_layer_set_text(minuteLayer, text);
}


void handle_minute_tick(struct tm * tick_time, TimeUnits units_changed) {
  display_time(tick_time);
}

void handle_bluetooth_connection(bool connected) {
  //show inverted when BT not connected
  layer_set_hidden(inverter_layer_get_layer(btConnectionInverter), connected);
  
  if (!connected) {
    vibes_double_pulse();
  }
}

void handle_init() {

  minuteFrame = GRect(53, 23, 40, 40);

  // Dynamic allocation of assets
  window = window_create();
  minuteLayer = text_layer_create(minuteFrame);
  hourLayer = bitmap_layer_create(GRect(0, 0, 144, 168));

  // Configure window
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, BKGD_COLOR);
  window_set_fullscreen(window, true);

  // Setup minute layer
  text_layer_set_text_color(minuteLayer, TEXT_COLOR);
  text_layer_set_background_color(minuteLayer, GColorClear);
  text_layer_set_font(minuteLayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MINUTE_38)));

  // Setup layers
  layer_add_child(bitmap_layer_get_layer(hourLayer), text_layer_get_layer(minuteLayer));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(hourLayer));

  // Avoids a blank screen on watch start.
  time_t tick_time = time(NULL);
  display_time(localtime(&tick_time));
  
   //create inverted layer to display BT status
  btConnectionInverter = inverter_layer_create(GRect(0, 0, 144, 168));
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(btConnectionInverter));
  
  //also subsctibe to bluetooth status
  bluetooth_connection_service_subscribe(handle_bluetooth_connection);
  
  //intialize to correct bluetooth status at init?
  handle_bluetooth_connection(bluetooth_connection_service_peek());
}

void handle_deinit() {
  unload_digit_image();
  bluetooth_connection_service_unsubscribe();
  
  text_layer_destroy(minuteLayer);
  bitmap_layer_destroy(hourLayer);
  inverter_layer_destroy(btConnectionInverter);
  window_destroy(window);
}

int main(void) {
  handle_init();
#ifdef DEBUG_FAST
	tick_timer_service_subscribe(SECOND_UNIT, handle_minute_tick);
#else
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
#endif
  app_event_loop();
  handle_deinit();
}
