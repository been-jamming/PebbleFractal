#include <pebble.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

static Window *main_window;
static TextLayer *number;
static SimpleMenuLayer *menu;
GRect bounds;
static SimpleMenuSection sections[1];
static SimpleMenuItem menuitems[2];
static double value = 0;
char text[6];

static double imaginary = 0;
static double real = 0;

static short int state = -1;
static unsigned short int textlayer_active = 1;
static bool julia;
static bool rendered = 0;

static float zoom = 1;
static float x_offset = 0;
static float y_offset = 0;

static unsigned short int move_state = 0;

int QUALITY = 20;

Layer *window_layer;

static void float_to_string(char *output, float flt){
  if(flt < 0){
    output[0] = '-';
    flt *= -1;
  } else {
    output[0] = ' ';
  }
  output[1] = (int) flt + '0';
  flt -= (int) flt;
  flt *= 10;
  output[2] = '.';
  output[3] = (int) flt + '0';
  flt -= (int) flt;
  flt *= 10;
  output[4] = (int) flt + '0';
  output[5] = 0;
}

static void change(float num){
  float_to_string(text, num);
  if(textlayer_active){
    text_layer_set_text(number, text);
  }
}

static bool byte_get_bit(uint8_t *byte, uint8_t bit) {
  return ((*byte) >> bit) & 1;
}

static void byte_set_bit(uint8_t *byte, uint8_t bit, uint8_t value) {
  *byte ^= (-value ^ *byte) & (1 << bit);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context){
  if(state != 2 && state != -1){
    value -= 0.01;
    change(value);
  } else {
    if(move_state == 0){
      zoom /= 2;
    } else if(move_state == 1){
      y_offset -= 0.2/zoom;
    } else {
      x_offset -= 0.2/zoom;
    }
    rendered = 0;
    layer_mark_dirty(window_layer);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context){
  if(state != 2 && state != 1){
    value += 0.01;
    change(value);
  } else {
    if(move_state == 0){
      zoom *= 2;
    } else if(move_state == 1){
      y_offset += 0.2/zoom;
    } else {
      x_offset += 0.2/zoom;
    }
    rendered = 0;
    layer_mark_dirty(window_layer);
  }
}

static void down_click_handler_held(ClickRecognizerRef recognizer, void *context){
  if(state != 2 && state != -1){
    value -= 0.01;
    change(value);
  } else {
    if(move_state == 0){
      zoom /= 2;
    } else if(move_state == 1){
      y_offset -= 0.2/zoom;
    } else {
      x_offset -= 0.2/zoom;
    }
    rendered = 0;
    layer_mark_dirty(window_layer);
  }
}

static void up_click_handler_held(ClickRecognizerRef recognizer, void *context){
  if(state != 2 && state != -1){
    value += 0.01;
    change(value);
  } else {
    if(move_state == 0){
      zoom *= 2;
    } else if(move_state == 1){
      y_offset += 0.2/zoom;
    } else {
      x_offset += 0.2/zoom;
    }
    rendered = 0;
    layer_mark_dirty(window_layer);
  }
}

void select_click_handler(ClickRecognizerRef recognizer, void *context){
  if(state == 0){
    real = value;
  } else if(state == 1){
    imaginary = value;
    textlayer_active = 0;
    text_layer_destroy(number);
  }
  
  if(state != 2){
    state = state+1;
  } else {
    move_state = (move_state+1)%3;
  }
  if(state == 0){    
    GRect bounds = layer_get_bounds(window_layer);
    number = text_layer_create(bounds);
    text_layer_set_background_color(number, GColorClear);
    text_layer_set_text_color(number, GColorBlack);
    text_layer_set_font(number, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(number, GTextAlignmentCenter);
  
    layer_add_child(window_layer, text_layer_get_layer(number));
    textlayer_active = 1;
    rendered = 0;
  }
  
  if(state != 2 && state != -1){
    value = 0;
    text_layer_set_text(number, "0.00");
  }
}


static void increase_quality(ClickRecognizerRef recognizer, void *context){
  if(move_state == 0){
    QUALITY *= 4;
    QUALITY /= 3;
    rendered = 0;
    layer_mark_dirty(window_layer);
  } else {
    int num_of_clicks = click_number_of_clicks_counted(recognizer);
    for(int i = 0; i < num_of_clicks; i++){
      up_click_handler(recognizer, context);
    }
  }
}

static void decrease_quality(ClickRecognizerRef recognizer, void *context){
  if(move_state == 0){
    QUALITY *= 3;
    QUALITY /= 4;
    rendered = 0;
    layer_mark_dirty(window_layer);
  } else {
    int num_of_clicks = click_number_of_clicks_counted(recognizer);
    for(int i = 0; i < num_of_clicks; i++){
      down_click_handler(recognizer, context);
    }
  }
}

static void click_config_provider(void *context) {
  // Subcribe to button click events here
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, down_click_handler_held);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, up_click_handler_held);
  window_multi_click_subscribe(BUTTON_ID_UP, 2, 0, 0, 1, increase_quality);
  window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 0, 0, 1, decrease_quality);

}

static void get_point(int x, int y, double *x_out, double *y_out){
  *x_out = (((float) x - 71.5))/38.5/zoom + x_offset;
  *y_out = (-((float) y - 84.5))/38.5/zoom + y_offset;
}

static GColor julia_color(int x, int y){
  double current_imaginary = 0;
  double current_real = 0;
  static short int red = 0;
  static short int green = 0;
  static short int blue = 0;
  static short int iteration;
  get_point(x, y, &current_real, &current_imaginary);
  static double temp = 0;
  iteration = -1;
  for(int i = 0; i < QUALITY; i++){
    temp = current_real*current_real - current_imaginary*current_imaginary + real;
    current_imaginary = 2*current_real*current_imaginary + imaginary;
    current_real = temp;

    if((current_real*current_real + current_imaginary*current_imaginary) > 4){
      iteration = i;
      break;
    }
  }
  float third = ((float) QUALITY)/3;
  if(iteration == -1){
    red = 0;
    green = 0;
    blue = 0;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "x: %d", x);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "y: %d", y);
  } else if(iteration < third){
    red = (int) 255*(1-((float) iteration)/third);
    green = 255;
    blue = 255;
  } else if(iteration < third*2){
    red = (int) 255*((float)(iteration-third))/third;
    green = (int) 255*(1-((float) iteration)/third);
    blue = 255;
  } else {
    red = 255;
    green = (int) 255*((float)(iteration-third*2))/third;
    blue = (int) 255*(1-((float) iteration)/third);
  }
  
  if(red > 255){
    red = 255;
  }
  if(green > 255){
    green = 255;
  }
  if(blue > 255){
    blue = 255;
  }
  
  return GColorFromRGB(red, green, blue);
}

static GColor mandelbrot_color(int x, int y){
  double current_imaginary = 0;
  double current_real = 0;
  static short int red = 0;
  static short int green = 0;
  static short int blue = 0;
  static short int iteration;
  get_point(x, y, &real, &imaginary);
  static double temp = 0;
  iteration = -1;
  for(int i = 0; i < QUALITY; i++){
    temp = current_real*current_real - current_imaginary*current_imaginary + real;
    current_imaginary = 2*current_real*current_imaginary + imaginary;
    current_real = temp;

    if((current_real*current_real + current_imaginary*current_imaginary) > 4){
      iteration = i;
      break;
    }
  }
  float third = ((float) QUALITY)/3;
  if(iteration == -1){
    red = 0;
    green = 0;
    blue = 0;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "x: %d", x);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "y: %d", y);
  } else if(iteration < third){
    red = (int) 255*(1-((float) iteration)/third);
    green = 255;
    blue = 255;
  } else if(iteration < third*2){
    red = (int) 255*((float)(iteration-third))/third;
    green = (int) 255*(1-((float) iteration)/third);
    blue = 255;
  } else {
    red = 255;
    green = (int) 255*((float)(iteration-third*2))/third;
    blue = (int) 255*(1-((float) iteration)/third);
  }
  
  if(red > 255){
    red = 255;
  }
  if(green > 255){
    green = 255;
  }
  if(blue > 255){
    blue = 255;
  }
  
  return GColorFromRGB(red, green, blue);
}

static void set_pixel_color(GBitmapDataRowInfo info, GPoint point, GColor color) {
#if defined(PBL_COLOR)
  memset(&info.data[point.x], color.argb, 1);
#elif defined(PBL_BW)
  uint8_t byte = point.x / 8;
  uint8_t bit = point.x % 8; 
  byte_set_bit(&info.data[byte], bit, gcolor_equal(color, GColorWhite) ? 1 : 0);
#endif
}

static void update_proc(Layer *layer, GContext *ctx){
  if(! rendered && state == 2){
    GBitmap *fb = graphics_capture_frame_buffer(ctx);
    GRect bounds = layer_get_bounds(layer);
    
    for(int y = 0; y < bounds.size.h; y++) {
      GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y);

      for(int x = info.min_x; x <= info.max_x; x++) {
        
        if(julia){
          set_pixel_color(info, GPoint(x, y), julia_color(x, y));
        } else {
          set_pixel_color(info, GPoint(x, y), mandelbrot_color(x, y));
        }
      }
    }
    graphics_release_frame_buffer(ctx, fb);
    rendered = 1;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Done rendering");
  } else if(state != 2){
    GBitmap *fb = graphics_capture_frame_buffer(ctx);
    GRect bounds = layer_get_bounds(layer);
    
    for(int y = 0; y < bounds.size.h; y++) {
      GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y);

      for(int x = info.min_x; x <= info.max_x; x++) {
        
        set_pixel_color(info, GPoint(x, y), GColorWhite);
      }
    }
    graphics_release_frame_buffer(ctx, fb);
  }
}

static void create_text(){
  number = text_layer_create(bounds);
  text_layer_set_background_color(number, GColorClear);
  text_layer_set_text_color(number, GColorBlack);
  text_layer_set_font(number, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(number, GTextAlignmentCenter);
  text_layer_set_text(number, "0.00");
  
  layer_add_child(window_layer, text_layer_get_layer(number));
}

static void setup(){
  window_set_click_config_provider(main_window, click_config_provider);
  layer_set_update_proc(window_layer, update_proc);
}

static void menu_select_callback(int index, void *ctx) {
  julia = 1-index;
  simple_menu_layer_destroy(menu);
  if(julia){
    create_text();
    state = 0;
  } else {
    state = 2;
  }
  setup();
}

static void main_window_load(Window *window){
  window_layer = window_get_root_layer(window);
  
  bounds = layer_get_bounds(window_layer);
  
  menuitems[0] = (SimpleMenuItem){.title="Julia set", .callback = menu_select_callback};
  menuitems[1] = (SimpleMenuItem){.title="Mandelbrot set", .callback = menu_select_callback};
  sections[0] = (SimpleMenuSection){.title="Select fractal type", .items = menuitems, .num_items = 2};
  
  menu = simple_menu_layer_create(bounds, window, sections, 1, NULL);
  layer_add_child(window_layer, (Layer *) menu);
}

static void main_window_unload(Window *window){
  
}

static void init(){
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {.load = main_window_load,.unload = main_window_unload});
  window_stack_push(main_window, true);
}

static void deinit(){
  window_destroy(main_window);
  if(state != 2){
    text_layer_destroy(number);
  }
  if(state == -1){
    simple_menu_layer_destroy(menu);
  }
}

int main(){
  init();
  app_event_loop();
  deinit();
}
