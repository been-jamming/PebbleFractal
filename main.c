#include <pebble.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#define SHIFT 24

struct fixed_struct{
  int32_t value;
  unsigned short int shift;
};

typedef struct fixed_struct fixed;

void set_value(fixed *num, int value){
  num->value = value<<(num->shift);
}

void set_shift(fixed *num, int shift){
  num->value = num->value>>(num->shift - shift);
  num->shift = shift;
}

fixed transform_fixed(fixed num1, fixed num2){
  fixed num3;
  num3.shift = num2.shift;
  num3.value = num1.value>>(num1.shift - num2.shift);
  return num3;
}  

fixed transform_integer(int num1, fixed num2){
  fixed num3;
  num3.shift = num2.shift;
  num3.value = num1<<num2.shift;
  return num3;
}

fixed multiply_fixed(fixed num1, fixed num2){
  fixed num3;
  num3.shift = num1.shift/2 + num2.shift/2;
  num3.value = (num1.value>>(num1.shift/2)) * (num2.value>>(num2.shift/2));
  return num3;
}

fixed divide_fixed(fixed num1, fixed num2){
  fixed num3;
  num3.shift = num1.shift-num2.shift;
  num3.value = num1.value/num2.value;
  return num3;
}

float divide_fixed_float(fixed num1, fixed num2){
  return ((float) num1.value)/((float) num2.value)*(1<<(num1.shift - num2.shift));
}

fixed add_float(fixed num1, float num2){
  fixed num3;
  num3.shift = num1.shift;
  num3.value = (int) (num1.value + num2*(1<<num1.shift));
  return num3;
}

fixed subtract_float(fixed num1, float num2){
  fixed num3;
  num3.shift = num1.shift;
  num3.value = (int) (num1.value - num2*(1<<num1.shift));
  return num3;
}

fixed add_integer(fixed num1, int num2){
  fixed num3;
  num3.shift = num1.shift;
  num3.value = num1.value + num2*(1<<num1.shift);
  return num3;
}

fixed add_fixed(fixed num1, fixed num2){
  fixed num3;
  num3.shift = num1.shift;
  num3.value = num1.value + (num2.value<<(num1.shift - num2.shift));
  return num3;
}

fixed subtract_fixed(fixed num1, fixed num2){
  fixed num3;
  num3.shift = num1.shift;
  num3.value = num1.value - (num2.value<<(num1.shift - num2.shift));
  return num3;
}

void increment_fixed(fixed *num1, float num2){
  num1->value = (int) (num1->value + num2*(1<<num1->shift));
}

float fixed_to_float(fixed num1){
  return ((float) num1.value)/(1<<num1.shift);
}

static Window *main_window;
static TextLayer *number;
static SimpleMenuLayer *menu;
GRect bounds;
static SimpleMenuSection sections[1];
static SimpleMenuItem menuitems[2];
static fixed value = {.value = 0, .shift = SHIFT};
char text[6];

static fixed imaginary = {.value = 0, .shift = SHIFT};
static fixed real = {.value = 0, .shift = SHIFT};

static short int state = -1;
static unsigned short int textlayer_active = 1;
static bool julia;
static bool rendered = 0;

static float zoom = 1;
static fixed x_offset = {.value = 0, .shift = SHIFT};
static fixed y_offset = {.value = 0, .shift = SHIFT};

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

static void change(fixed num){
  float_to_string(text, fixed_to_float(num));
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
    increment_fixed(&value, -0.01);
    change(value);
  } else {
    if(move_state == 0){
      zoom /= 2;
    } else if(move_state == 1){
      increment_fixed(&y_offset, -0.2/zoom);
    } else {
      increment_fixed(&x_offset, -0.2/zoom);
    }
    rendered = 0;
    layer_mark_dirty(window_layer);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context){
  if(state != 2 && state != 1){
    increment_fixed(&value, 0.01);
    change(value);
  } else {
    if(move_state == 0){
      zoom *= 2;
    } else if(move_state == 1){
      increment_fixed(&y_offset, 0.2/zoom);
    } else {
      increment_fixed(&x_offset, 0.2/zoom);
    }
    rendered = 0;
    layer_mark_dirty(window_layer);
  }
}

static void down_click_handler_held(ClickRecognizerRef recognizer, void *context){
  if(state != 2 && state != -1){
    increment_fixed(&value, -0.01);
    change(value);
  } else {
    if(move_state == 0){
      zoom /= 2;
    } else if(move_state == 1){
      increment_fixed(&y_offset, -0.2/zoom);
    } else {
      increment_fixed(&x_offset, -0.2/zoom);
    }
    rendered = 0;
    layer_mark_dirty(window_layer);
  }
}

static void up_click_handler_held(ClickRecognizerRef recognizer, void *context){
  if(state != 2 && state != -1){
    increment_fixed(&value, 0.01);
    change(value);
  } else {
    if(move_state == 0){
      zoom *= 2;
    } else if(move_state == 1){
      increment_fixed(&y_offset, 0.2/zoom);
    } else {
      increment_fixed(&x_offset, 0.2/zoom);
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
    value.value = 0;
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

static void get_point(int x, int y, fixed *x_out, fixed *y_out){
  fixed new_x = {.value = x<<16, .shift = 16};
  fixed new_y = {.value = y<<16, .shift = 16};
  fixed temp = subtract_float(new_x, 71.5);
  temp.value = (int32_t) (temp.value/38.5/zoom);
  *x_out = add_fixed(x_offset, temp);
  temp = subtract_float(new_y, 84.5);
  temp.value = (int32_t) (temp.value/-38.5/zoom);
  *y_out = add_fixed(y_offset, temp);
}

static GColor julia_color(int x, int y){
  fixed current_imaginary = {.value = 0, .shift = SHIFT};
  fixed current_real = {.value = 0, .shift = SHIFT};
  static short int red = 0;
  static short int green = 0;
  static short int blue = 0;
  static short int iteration;
  get_point(x, y, &current_real, &current_imaginary);
  static fixed temp;
  iteration = -1;
  fixed dissquared;
  for(int i = 0; i < QUALITY; i++){
    temp = add_fixed(subtract_fixed(multiply_fixed(current_real, current_real), multiply_fixed(current_imaginary, current_imaginary)), real);
    current_imaginary = add_fixed(multiply_fixed(current_real, current_imaginary), imaginary);
    current_imaginary.value = current_imaginary.value<<1;
    current_real = temp;
    dissquared = add_fixed(multiply_fixed(current_real, current_real), multiply_fixed(current_imaginary, current_imaginary));
    if((dissquared.value>>dissquared.shift) > 4){
      iteration = i;
      break;
    }
  }
  float third = ((float) QUALITY)/3;
  if(iteration == -1){
    red = 0;
    green = 0;
    blue = 0;
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
  fixed current_imaginary = {.value = 0, .shift = SHIFT};
  fixed current_real = {.value = 0, .shift = SHIFT};
  static short int red = 0;
  static short int green = 0;
  static short int blue = 0;
  static short int iteration;
  get_point(x, y, &real, &imaginary);
  static fixed temp;
  iteration = -1;
  fixed dissquared;
  imaginary.value = imaginary.value>>1;
  for(int i = 0; i < QUALITY; i++){
    temp = add_fixed(subtract_fixed(multiply_fixed(current_real, current_real), multiply_fixed(current_imaginary, current_imaginary)), real);
    current_imaginary = add_fixed(multiply_fixed(current_real, current_imaginary), imaginary);
    current_imaginary.value = current_imaginary.value<<1;
    current_real = temp;
    dissquared = add_fixed(multiply_fixed(current_real, current_real), multiply_fixed(current_imaginary, current_imaginary));
    if((dissquared.value>>dissquared.shift) > 4){
      iteration = i;
      break;
    }
  }
  float third = ((float) QUALITY)/3;
  if(iteration == -1){
    red = 0;
    green = 0;
    blue = 0;
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
