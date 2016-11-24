#include <pebble.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#define ARROW_MARGIN (5)
// Persistent storage key
#define SETTINGS_KEY 1

#define INBOX_SIZE  128
#define OUTBOX_SIZE 128

// Define our settings struct
typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor ForegroundColor;
  bool   SecondTick;
  bool   Animations;
  bool   MetricUnits;
  
  char   StoredWeatherTemperature[32];
  char   StoredWeatherConditions[32];
  char   StoredWeatherAPIKey[32];
  GColor Foreground2Color;
  int    WeatherProvider;
} ClaySettings;

enum WEATHER_PROVIDER
{
  OPENWEATHER = 0,
  WUNDERGROUND,
  YAHOO
};

enum WEATHER_STATUS 
{
  INVALID = 0,
  INPROGRESS,
  VALID
};

// An instance of the struct
static ClaySettings settings;

static Window *s_main_window;
static Layer *s_canvas_layer;
static GRect bounds;
static int radius = 0;
static GFont font;
static GFont weather_font;

static int temperature = INT_MIN;
static int conditions = INT_MIN;
static int seconds;
static int minutes;
static int hours;
static int s_battery_level;
static char s_buffer[8];
static enum WEATHER_STATUS weather_update = INVALID;
static bool bluetooth_connected = 0;
static int size = 0;
static uint8_t * background = NULL;

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
  hours   = tick_time->tm_hour;
  
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);  
  layer_mark_dirty(s_canvas_layer);
}

static void update_weather() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Update weather");
  // Begin dictionary
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  // Add a weather provider ID
  dict_write_uint8(iter, MESSAGE_KEY_WEATHER_PROVIDER, settings.WeatherProvider);
  if (strlen(settings.StoredWeatherAPIKey)) {
    dict_write_cstring(iter, MESSAGE_KEY_WEATHERAPI_KEY, settings.StoredWeatherAPIKey);
  }
  
  // Send the message!
  app_message_outbox_send();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
  // Get weather update every twice an hour
  if(tick_time->tm_min % 30 == 0) {
    weather_update = INVALID;
    APP_LOG(APP_LOG_LEVEL_INFO, "Weather is old");
  }
  
  if (!bluetooth_connected)
    return;
  
  if (weather_update==INVALID || (weather_update==INPROGRESS && tick_time->tm_sec % 30 == 0))
  {
    weather_update = INPROGRESS;
    update_weather();    
    APP_LOG(APP_LOG_LEVEL_INFO, "Updating weather");
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

static void drawGraduationMark(int radius, int radius1, int angle, GContext *ctx)
{
  int32_t second_angle = TRIG_MAX_ANGLE * angle / 360;  
    
  int cx = bounds.size.w/2;
  int cy = bounds.size.h/2;
  int d0x = (sin_lookup(second_angle) * radius / TRIG_MAX_RATIO);
  int d0y = (-cos_lookup(second_angle) * radius / TRIG_MAX_RATIO);
  
  int dx = (sin_lookup(second_angle) * radius1 / TRIG_MAX_RATIO);
  int dy = (-cos_lookup(second_angle) * radius1 / TRIG_MAX_RATIO);  
  
  graphics_draw_line(ctx, GPoint(cx+d0x, cy+d0y), GPoint(cx+dx, cy+dy));
  
}

static void drawBackground(Layer *layer, GContext *ctx)
{
  if (size == 0)
  {
      graphics_context_set_fill_color(ctx, settings.BackgroundColor);  
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
      graphics_context_set_stroke_width(ctx, 1);
      graphics_context_set_fill_color(ctx, settings.BackgroundColor);
      graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorRed, settings.ForegroundColor)); 
      graphics_fill_circle(ctx, GPoint(bounds.size.w/2, bounds.size.h/2), radius*2/3);
      graphics_draw_circle(ctx, GPoint(bounds.size.w/2, bounds.size.h/2), radius*2/3); 
          
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
  graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorGreen, settings.ForegroundColor)); 
  graphics_draw_arc(ctx, GRect(bounds.size.w/2 - radius*2/3,  bounds.size.h/2 - radius*2/3,  radius*4/3,  radius*4/3), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(s_battery_level*36/10));
  
  for (int i =0; i<360; i+=30)
  {     
     if (s_battery_level*36/10 < i)
     {
       graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorRed, settings.ForegroundColor)); 
     }
     drawGraduationMark(radius*2/3, radius*3/5, i, ctx);
  }
  
  // Draw arrows
  graphics_context_set_fill_color(ctx, settings.ForegroundColor);
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor); 
  graphics_context_set_text_color(ctx, settings.ForegroundColor);
  graphics_context_set_stroke_width(ctx, 1);
  drawArrow(radius*2/3 + 2*ARROW_MARGIN, minutes*6, 3, ctx);
  graphics_context_set_stroke_width(ctx, 1);
  drawArrow(radius*2/3 + 3*ARROW_MARGIN, (hours%12)*30, 4, ctx); 
  
  if (settings.SecondTick)
  {
    graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorRed, settings.ForegroundColor));
    graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorRed, settings.ForegroundColor)); 
    graphics_context_set_stroke_width(ctx, 1);
    drawArrow(radius*2/3 + ARROW_MARGIN, seconds*6, 2, ctx);
  }
 
  // Draw time in text
  graphics_context_set_text_color(ctx, settings.ForegroundColor);
  GRect time_bounds = GRect(bounds.size.w/2 - radius*2/3,  bounds.size.h/2 - radius/2 - 5,  radius*4/3,  radius/2);

  graphics_draw_text(ctx, s_buffer, font, time_bounds, GTextOverflowModeWordWrap, 
                                            GTextAlignmentCenter, NULL);  
  
  graphics_context_set_text_color(ctx, weather_update == VALID ? settings.ForegroundColor : settings.Foreground2Color);
  // Draw weather text
  GRect weather_bounds = GRect(bounds.size.w/2 - radius*2/3,  bounds.size.h/2 + 15,  radius*4/3,  radius/2 - 15);
  graphics_draw_text(ctx, settings.StoredWeatherTemperature, font, weather_bounds, GTextOverflowModeWordWrap, 
                                            GTextAlignmentCenter, NULL);
  
  GRect weather_icon_bounds =  GRect(bounds.size.w/2 - radius*2/3,  bounds.size.h/2 - 15,  radius*4/3,  radius/2);
  graphics_draw_text(ctx, settings.StoredWeatherConditions, weather_font, weather_icon_bounds, GTextOverflowModeWordWrap, 
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

static int toImperial(int val)
{
  return 9*val/5 + 32;
}

static void readWeather(DictionaryIterator *iterator)
{
   // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
  
  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Weather info received");
    temperature = (int)temp_tuple->value->int32;
    conditions = (int)conditions_tuple->value->int32;    
    weather_update = VALID;
  }
  
  // Print temperature in desired units
  if (temperature > INT_MIN)
  {
    if (settings.MetricUnits) {
      snprintf(settings.StoredWeatherTemperature, sizeof(settings.StoredWeatherTemperature), "%d°C", temperature);
    }
    else {
      snprintf(settings.StoredWeatherTemperature, sizeof(settings.StoredWeatherTemperature), "%d°F", toImperial(temperature));
    }
  }
  if (conditions > INT_MIN)
  {
    snprintf(settings.StoredWeatherConditions, sizeof(settings.StoredWeatherConditions), "%c", (char)conditions);
  }
}

static void readSettings(DictionaryIterator *iterator)
{
  // Check units before reading temperature
  Tuple *temperature_units = dict_find(iterator, MESSAGE_KEY_TEMP_FORMAT);
  if (temperature_units) {
    settings.MetricUnits = strcmp((char *)temperature_units->value->cstring, "1") == 0;    
  }    
  // Assign the values to our struct
  Tuple *bg_color_t = dict_find(iterator, MESSAGE_KEY_BACKGROUND_COLOR);
  if (bg_color_t) {
     settings.BackgroundColor = GColorFromHEX(bg_color_t->value->int32);
  }
  Tuple *fg_color_t = dict_find(iterator, MESSAGE_KEY_FOREGROUND_COLOR);
  if (fg_color_t) {
    settings.ForegroundColor = GColorFromHEX(fg_color_t->value->int32);
  }
  
  Tuple *bg2_color_t = dict_find(iterator, MESSAGE_KEY_FOREGROUND2_COLOR);
  if (bg2_color_t) {
    settings.Foreground2Color = GColorFromHEX(bg2_color_t->value->int32);
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
  
  Tuple *weather_provider = dict_find(iterator, MESSAGE_KEY_WEATHER_PROVIDER);
  if (weather_provider) {
    int provider = atoi(weather_provider->value->cstring);    
    if (provider != settings.WeatherProvider) {
      settings.WeatherProvider = provider;    
      weather_update = INPROGRESS;
      update_weather();
    }
  }
  
  Tuple *weather_key = dict_find(iterator, MESSAGE_KEY_WEATHERAPI_KEY);
  if (weather_key) {
    strncpy(settings.StoredWeatherAPIKey, weather_key->value->cstring, sizeof(settings.StoredWeatherAPIKey));
    APP_LOG(APP_LOG_LEVEL_INFO, "Weather API key received %s", weather_key->value->cstring);
  }
  
  
  // Request background invalidation background
  if (bg_color_t || fg_color_t || bg2_color_t)
  {
    size = 0;
    free(background);
  }
}
  
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
  
  readSettings(iterator);
  readWeather(iterator);
    
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  layer_mark_dirty(s_canvas_layer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! reason: %d", reason);
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
  
  app_message_open(INBOX_SIZE, OUTBOX_SIZE);
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));  
  tick_timer_service_subscribe(settings.SecondTick ? SECOND_UNIT: MINUTE_UNIT, tick_handler);
  
  battery_state_service_subscribe(battery_callback);
  bluetooth_connected = connection_service_peek_pebble_app_connection();
  
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
  settings.Foreground2Color = GColorFromHEX(0xAAAAAA);
  settings.SecondTick = true;
  settings.MetricUnits = true;
  memset(settings.StoredWeatherTemperature, 0, sizeof(settings.StoredWeatherTemperature));
  memset(settings.StoredWeatherConditions, 0, sizeof(settings.StoredWeatherConditions));
  init();
  app_event_loop();
  deinit();
}
