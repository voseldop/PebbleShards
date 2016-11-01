#include <pebble.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ARROW_MARGIN (5)
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

const int inbox_size = 128;
const int outbox_size = 128;
  
static Window *s_main_window;
static Layer *s_canvas_layer;
static GRect bounds;
static int radius = 0;
static GFont font;
static GFont weather_font;

static int seconds;
static int minutes;
static int hours;
static int s_battery_level;
static char s_buffer[8];
static char weather_update = 1;
static bool bluetooth_connected = 0;
static int size = 0;
static uint8_t * background = NULL;


// Store incoming information
static char temperature_buffer[8];
static char conditions_buffer[32];
static char weather_layer_buffer[32]="";

struct CodePair 
{
   short code;
   char symbol;
};

static struct CodePair icons[] = {{800, 'N'}, {801, 'C'}, {802, 'S'}, {803, 'S'}, {804, 'S'},
                                 {600, 'a'}, {601, 'a'}, {602, 'a'}, {611, 'a'}, {612, 'a'}, {615, 'a'}, {616, 'a'}, {620, 'a'}, {621, 'a'}, {622, 'a'},
                                 {500, 'L'}, {501, 'L'}, {502, 'L'}, {503, 'L'}, {504, 'L'}, {511, 'b'}, {520, 'b'}, {521, 'b'}, {522, 'b'}, {531, 'b'},
                                 {200, 'V'}, {201, 'V'}, {202, 'V'}, {210, 'V'}, {211, 'V'}, {212, 'V'}, {221, 'V'}, {230, 'V'}, {231, 'V'}, {232, 'V'},
                                 {701, '!'}, {711, '!'}, {721, '!'}, {731, '!'}, {741, '!'}, {751, '!'}, {761, '!'}, {762, '!'}, {771, '!'}, {781, '!'}
                                 };

static int min(int i, int j) {
  return (i<j) ? i : j;
}
static void bluetooth_callback(bool connected) {
  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
  bluetooth_connected = connected;
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
}

static void update_time(struct tm * tick_time) {
  // Get a tm structure
   
  time_t temp = time(NULL);
  if (tick_time==NULL)
    tick_time = localtime(&temp);
  
  if (tick_time==NULL)
    return;
  
  seconds = tick_time->tm_sec;  
  minutes = tick_time->tm_min;
  hours = tick_time->tm_hour;
  
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);  
  layer_mark_dirty(s_canvas_layer);
}

static void update_weather() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Update weather");
  // Begin dictionary
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  // Add a key-value pair
  dict_write_uint8(iter, 0, 0);

  // Send the message!
  app_message_outbox_send();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    weather_update = 1;
  }
  
  if (!bluetooth_connected)
    return;
  
  if (weather_update==1 || strlen(weather_layer_buffer)==0 || (weather_update && tick_time->tm_sec % 15 == 0))
  {
    update_weather();
    weather_update = 2;
  }
}

static void drawArrow(int radius, int angle, int angle_margin, GContext *ctx)
{
  int32_t second_angle = TRIG_MAX_ANGLE * (angle + angle_margin) / 360;  
  int32_t second0_angle = TRIG_MAX_ANGLE * (angle- angle_margin) / 360;
  
  int cx = bounds.size.w/2;
  int cy = bounds.size.h/2;
  int d0x = (sin_lookup(second_angle) * radius / TRIG_MAX_RATIO);
  int d0y = (-cos_lookup(second_angle) * radius / TRIG_MAX_RATIO);
  
  int dx = (sin_lookup(second0_angle) * radius / TRIG_MAX_RATIO);
  int dy = (-cos_lookup(second0_angle) * radius / TRIG_MAX_RATIO);  
  
  GPath * arrowPath = NULL;
  GPathInfo arrowPathInfo = {
  .num_points = 4,
  .points = (GPoint []) {{cx+dx, cy+dy}, {cx+3*dx, cy+3*dy}, {cx+3*d0x, cy+3*d0y}, {cx+d0x, cy+d0y}}
  };
  
  arrowPath = gpath_create(&arrowPathInfo);    
  gpath_draw_filled(ctx, arrowPath);  
  gpath_destroy(arrowPath);
}

static void drawBackground(Layer *layer, GContext *ctx)
{
  if (size == 0)
  {
      graphics_context_set_fill_color(ctx, settings.BackgroundColor);  
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
      graphics_context_set_stroke_width(ctx, 1);
      graphics_context_set_fill_color(ctx, settings.BackgroundColor);
      graphics_context_set_stroke_color(ctx, GColorRed); 
      graphics_fill_circle(ctx, GPoint(bounds.size.w/2, bounds.size.h/2), radius*2/3);
      graphics_draw_circle(ctx, GPoint(bounds.size.w/2, bounds.size.h/2), radius*2/3);
      graphics_context_set_text_color(ctx, GColorWhite);
      
      GBitmap *fb = graphics_capture_frame_buffer(ctx);
      uint8_t * data = gbitmap_get_data(fb);
      size = PBL_IF_ROUND_ELSE(0,  gbitmap_get_bytes_per_row(fb)*bounds.size.h);

      if (size > 0)
      {
        background = malloc(size);
        memcpy(background, data, size);   
      }
      graphics_release_frame_buffer(ctx, fb);
  }
  else
  {
    GBitmap *fb = graphics_capture_frame_buffer(ctx);
    uint8_t * data = gbitmap_get_data(fb);
    memcpy(data, background, size);

    graphics_release_frame_buffer(ctx, fb);
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {  
  // Draw circle + background!   
  drawBackground(layer, ctx);
  // Draw battery bar
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_stroke_color(ctx, GColorGreen); 
  graphics_draw_arc(ctx, GRect(bounds.size.w/2 - radius*2/3,  bounds.size.h/2 - radius*2/3,  radius*4/3,  radius*4/3), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(s_battery_level*36/10));
  
  // Draw arrows
  if (settings.SecondTick)
  {
    graphics_context_set_fill_color(ctx, GColorRed);
    graphics_context_set_stroke_color(ctx, GColorRed); 
    graphics_context_set_stroke_width(ctx, 1);
    drawArrow(radius*2/3 + ARROW_MARGIN, seconds*6, 2, ctx);
  }
  graphics_context_set_fill_color(ctx, settings.ForegroundColor);
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor); 
  graphics_context_set_stroke_width(ctx, 1);
  drawArrow(radius*2/3 + 2*ARROW_MARGIN, minutes*6, 3, ctx);
  graphics_context_set_stroke_width(ctx, 1);
  drawArrow(radius*2/3 + 3*ARROW_MARGIN, (hours%12)*30, 4, ctx); 
 
  // Draw time in text
  GRect time_bounds = GRect(bounds.size.w/2 - radius*2/3,  bounds.size.h/2 - radius/2 - 5,  radius*4/3,  radius/2);

  graphics_draw_text(ctx, s_buffer, font, time_bounds, GTextOverflowModeWordWrap, 
                                            GTextAlignmentCenter, NULL);
  
  // Draw weather text
  GRect weather_bounds = GRect(bounds.size.w/2 - radius*2/3,  bounds.size.h/2 + 15,  radius*4/3,  radius/2 - 15);
  graphics_draw_text(ctx, weather_layer_buffer, font, weather_bounds, GTextOverflowModeWordWrap, 
                                            GTextAlignmentCenter, NULL);
  
  GRect weather_icon_bounds =  GRect(bounds.size.w/2 - radius*2/3,  bounds.size.h/2 - 15,  radius*4/3,  radius/2);
  graphics_draw_text(ctx, conditions_buffer, weather_font, weather_icon_bounds, GTextOverflowModeWordWrap, 
                                            GTextAlignmentCenter, NULL); 
}

static void main_window_load(Window *window) {
  app_log(APP_LOG_LEVEL_INFO, __FILE_NAME__, __LINE__, "main window load");
  
  font = fonts_load_custom_font(
                          resource_get_handle(RESOURCE_ID_FONT_WATCHFACE_OPENSANS_20));
  weather_font = fonts_load_custom_font(
                          resource_get_handle(RESOURCE_ID_FONT_WEATHER_24));
  
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  bounds = layer_get_bounds(window_layer);
  
  radius = min(bounds.size.w, bounds.size.h)/2;
    
  // Create canvas layer
  s_canvas_layer = layer_create(bounds);
  // Assign the custom drawing procedure
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);  
  layer_add_child(window_layer, s_canvas_layer);  
  
  // Make sure the time is displayed from the start
  update_time(NULL);
}

static void main_window_unload(Window *window) {
  app_log(APP_LOG_LEVEL_INFO, __FILE_NAME__, __LINE__, "main window unload");
  
  fonts_unload_custom_font(font);
  fonts_unload_custom_font(weather_font);
  layer_destroy(s_canvas_layer);
}

// Save the settings to persistent storage
static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static char get_icon(int code) {
  for (unsigned int i = 0; i<sizeof(icons)/sizeof(icons[0]); i++)
  {
    if (code == icons[i].code)
      return icons[i].symbol;
  }
  return ' ';
}
  
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
  
  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°C", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%c", get_icon((int)conditions_tuple->value->int32));
  }
  
  // Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s", temperature_buffer);
  weather_update = 0;
  
  // Assign the values to our struct
  Tuple *bg_color_t = dict_find(iterator, MESSAGE_KEY_BACKGROUND_COLOR);
  if (bg_color_t) {
     settings.BackgroundColor = GColorFromHEX(bg_color_t->value->int32);
  }
  Tuple *fg_color_t = dict_find(iterator, MESSAGE_KEY_FOREGROUND_COLOR);
  if (fg_color_t) {
    settings.ForegroundColor = GColorFromHEX(fg_color_t->value->int32);
  }
  
  Tuple *seconds_tick = dict_find(iterator, MESSAGE_KEY_SECONDS_TICK);
  if (seconds_tick) {
    if (settings.SecondTick != (seconds_tick->value->int32 == 1) )
    {
      tick_timer_service_unsubscribe();
      tick_timer_service_subscribe(seconds_tick->value->int32 == 1 ? SECOND_UNIT: MINUTE_UNIT, tick_handler);
    }
    settings.SecondTick = seconds_tick->value->int32 == 1;      
  }
  // ...
  prv_save_settings();
  layer_mark_dirty(s_canvas_layer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  // Register with TickTimerService
  
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  app_message_open(inbox_size, outbox_size);
  update_weather();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
  
  tick_timer_service_subscribe(settings.SecondTick ? SECOND_UNIT: MINUTE_UNIT, tick_handler);
  
  battery_state_service_subscribe(battery_callback);
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);  
  battery_callback(battery_state_service_peek());
  connection_service_subscribe((ConnectionHandlers) {
  .pebble_app_connection_handler = bluetooth_callback
});
}

static void deinit() {
  
  if (background!=NULL)
    free(background);
  // Destroy Window
  connection_service_unsubscribe();
  app_message_deregister_callbacks();
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);  
}

int main(void) {
  settings.BackgroundColor = GColorBlack;
  settings.ForegroundColor = GColorWhite;
  settings.SecondTick = true;
  init();
  app_event_loop();
  deinit();
}
