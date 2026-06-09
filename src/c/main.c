#include <pebble.h>
#include <string.h>

#define SETTINGS_KEY 1
#define SETTINGS_VERSION 1
#define DEFAULT_STEP_GOAL 8000
#define DEFAULT_USER_AGE 30
#define DEFAULT_DEMO_STEPS 8600
#define DEFAULT_DEMO_HR 72
#define RING_RADIUS 28
#define RING_STROKE 5

typedef enum {
  TimeModeSystem = 0,
  TimeMode12h = 1,
  TimeMode24h = 2
} TimeMode;

typedef enum {
  DstModeNone = 0,
  DstModeNZ = 1,
  DstModeEU = 2,
  DstModeUS = 3,
  DstModeAUS = 4
} DstMode;

typedef enum {
  FooterUV = 0,
  FooterSun = 1,
  FooterWorld = 2,
  FooterBattery = 3,
  FooterWeather = 4,
  FooterTemp = 5,
  FooterRain = 6,
  FooterSteps = 7,
  FooterHeart = 8
} FooterMode;

typedef struct {
  uint8_t version;
  bool dark_mode;
  uint8_t time_mode;
  uint8_t temp_unit;
  int32_t step_goal;
  uint8_t user_age;
  char world_label[5];
  int32_t world_offset_minutes;
  uint8_t world_dst_mode;
  int32_t hr_color;
  int32_t steps_color;
  int32_t rain_color;
  uint8_t footer_left;
  uint8_t footer_center;
  uint8_t footer_right;
  bool demo_fallback;
  bool show_weather;
  bool show_health;
  bool show_battery_percent;
} Settings;

typedef struct {
  const char *label;
  const char *value;
  int percent;
  GColor accent;
} GaugeSpec;

static Window *s_main_window;
static Layer *s_canvas_layer;
static Settings s_settings;

static char s_time_text[8] = "--:--";
static char s_date_text[16] = "--- -- ---";
static char s_steps_text[10] = "8.6k";
static char s_hr_text[8] = "72";
static char s_rain_text[8] = "42%";
static char s_temp_text[8] = "18C";
static char s_uv_text[8] = "3.1";
static char s_sun_text[8] = "21:14";
static char s_world_text[16] = "NZ --:--";
static char s_battery_text[8] = "--%";

static int s_steps_percent = 100;
static int s_hr_percent = 38;
static int s_rain_percent = 42;
static int s_weather_code = 2;
static int s_sun_event_type = 1;
static int s_battery_percent = 0;
static int s_temp_c = 18;

static GFont s_font_time;
static GFont s_font_num;
static GFont s_font_val;
static GFont s_font_pix_lg;
static GFont s_font_pix_sm;

static GDrawCommandImage *s_wx_sun;
static GDrawCommandImage *s_wx_partly;
static GDrawCommandImage *s_wx_cloudy;
static GDrawCommandImage *s_wx_rain;
static GDrawCommandImage *s_wx_snow;
static GDrawCommandImage *s_wx_storm;
static GDrawCommandImage *s_wx_fog;
static GDrawCommandImage *s_sun_rise;
static GDrawCommandImage *s_sun_set;

static void request_weather(void);
static void request_settings(void);

static int clamp_int(int value, int min_value, int max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

static GColor color_from_setting(int32_t hex, GColor fallback) {
  return PBL_IF_COLOR_ELSE(GColorFromHEX(hex), fallback);
}

static GColor color_bg(void) {
  return PBL_IF_COLOR_ELSE(s_settings.dark_mode ? GColorFromHEX(0x081827)
                                                : GColorFromHEX(0xf8f0dc),
                           GColorWhite);
}

static GColor color_panel(void) {
  return PBL_IF_COLOR_ELSE(s_settings.dark_mode ? GColorBlack
                                                : GColorFromHEX(0x081827),
                           GColorBlack);
}

static GColor color_panel_stroke(void) {
  return PBL_IF_COLOR_ELSE(s_settings.dark_mode ? GColorFromHEX(0x66737d)
                                                : GColorFromHEX(0x4d5c62),
                           GColorBlack);
}

static GColor color_capsule(void) {
  return PBL_IF_COLOR_ELSE(s_settings.dark_mode ? GColorFromHEX(0x18202a)
                                                : GColorFromHEX(0xeceee6),
                           GColorWhite);
}

static GColor color_ring_bg(void) {
  return PBL_IF_COLOR_ELSE(s_settings.dark_mode ? GColorFromHEX(0x56616a)
                                                : GColorFromHEX(0x26333b),
                           GColorBlack);
}

static GColor color_muted(void) {
  return PBL_IF_COLOR_ELSE(s_settings.dark_mode ? GColorFromHEX(0xb7c0bd)
                                                : GColorFromHEX(0x455157),
                           GColorBlack);
}

static GColor color_text(void) {
  return PBL_IF_COLOR_ELSE(s_settings.dark_mode ? GColorFromHEX(0xf8f0dc)
                                                : GColorBlack,
                           GColorBlack);
}

static GColor color_line(void) {
  return PBL_IF_COLOR_ELSE(s_settings.dark_mode ? GColorFromHEX(0xb7c0bd)
                                                : GColorBlack,
                           GColorBlack);
}

static GColor color_hr(void) {
  return color_from_setting(s_settings.hr_color, GColorBlack);
}

static GColor color_steps(void) {
  return color_from_setting(s_settings.steps_color, GColorBlack);
}

static GColor color_rain(void) {
  return color_from_setting(s_settings.rain_color, GColorBlack);
}

static GColor color_sun(void) {
  return PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite);
}

static GColor color_temp(void) {
  return PBL_IF_COLOR_ELSE(GColorFromHEX(0xf04a00), GColorBlack);
}

static GColor color_battery(void) {
  return PBL_IF_COLOR_ELSE(GColorFromHEX(0x00b96d), GColorBlack);
}

static void default_settings(void) {
  memset(&s_settings, 0, sizeof(s_settings));
  s_settings.version = SETTINGS_VERSION;
  s_settings.dark_mode = false;
  s_settings.time_mode = TimeModeSystem;
  s_settings.temp_unit = 0;
  s_settings.step_goal = DEFAULT_STEP_GOAL;
  s_settings.user_age = DEFAULT_USER_AGE;
  snprintf(s_settings.world_label, sizeof(s_settings.world_label), "NZ");
  s_settings.world_offset_minutes = 12 * 60;
  s_settings.world_dst_mode = DstModeNZ;
  s_settings.hr_color = 0xff7393;
  s_settings.steps_color = 0x00b96d;
  s_settings.rain_color = 0x009bff;
  s_settings.footer_left = FooterUV;
  s_settings.footer_center = FooterSun;
  s_settings.footer_right = FooterWorld;
  s_settings.demo_fallback = true;
  s_settings.show_weather = true;
  s_settings.show_health = true;
  s_settings.show_battery_percent = true;
}

static void save_settings(void) {
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

static void load_settings(void) {
  Settings loaded;
  default_settings();
  if (persist_read_data(SETTINGS_KEY, &loaded, sizeof(loaded)) == (int)sizeof(loaded) &&
      loaded.version == SETTINGS_VERSION) {
    s_settings = loaded;
    s_settings.step_goal = clamp_int(s_settings.step_goal, 1000, 50000);
    s_settings.user_age = clamp_int(s_settings.user_age, 10, 100);
    s_settings.footer_left = clamp_int(s_settings.footer_left, FooterUV, FooterHeart);
    s_settings.footer_center = clamp_int(s_settings.footer_center, FooterUV, FooterHeart);
    s_settings.footer_right = clamp_int(s_settings.footer_right, FooterUV, FooterHeart);
  }
}

static bool use_24h_time(void) {
  if (s_settings.time_mode == TimeMode12h) {
    return false;
  }
  if (s_settings.time_mode == TimeMode24h) {
    return true;
  }
  return clock_is_24h_style();
}

static int day_of_week(int year, int month, int day) {
  static const int offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (month < 3) {
    year--;
  }
  return (year + year / 4 - year / 100 + year / 400 + offsets[month - 1] + day) % 7;
}

static int first_weekday_in_month(int year, int month, int weekday) {
  return 1 + ((weekday - day_of_week(year, month, 1) + 7) % 7);
}

static int nth_weekday_in_month(int year, int month, int weekday, int nth) {
  return first_weekday_in_month(year, month, weekday) + ((nth - 1) * 7);
}

static int last_weekday_in_month(int year, int month, int last_day, int weekday) {
  return last_day - ((day_of_week(year, month, last_day) - weekday + 7) % 7);
}

static bool is_dst_for_mode(int year, int month, int day, int mode) {
  if (mode == DstModeNZ) {
    if (month >= 10 || month <= 3) {
      return true;
    }
    if (month >= 5 && month <= 8) {
      return false;
    }
    if (month == 9) {
      return day >= last_weekday_in_month(year, 9, 30, 0);
    }
    return day < first_weekday_in_month(year, 4, 0);
  }

  if (mode == DstModeEU) {
    if (month > 3 && month < 10) {
      return true;
    }
    if (month == 3) {
      return day >= last_weekday_in_month(year, 3, 31, 0);
    }
    if (month == 10) {
      return day < last_weekday_in_month(year, 10, 31, 0);
    }
  }

  if (mode == DstModeUS) {
    if (month > 3 && month < 11) {
      return true;
    }
    if (month == 3) {
      return day >= nth_weekday_in_month(year, 3, 0, 2);
    }
    if (month == 11) {
      return day < first_weekday_in_month(year, 11, 0);
    }
  }

  if (mode == DstModeAUS) {
    if (month >= 11 || month <= 3) {
      return true;
    }
    if (month == 10) {
      return day >= first_weekday_in_month(year, 10, 0);
    }
    if (month == 4) {
      return day < first_weekday_in_month(year, 4, 0);
    }
  }

  return false;
}

static void draw_text(GContext *ctx, const char *text, GFont font, GRect box,
                      GTextAlignment alignment, GColor color) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, text, font, box, GTextOverflowModeTrailingEllipsis,
                     alignment, NULL);
}

static void format_steps(int32_t steps, char *buffer, size_t buffer_size) {
  if (steps < 0) {
    snprintf(buffer, buffer_size, "--");
  } else if (steps >= 100000) {
    snprintf(buffer, buffer_size, "%ldk", (long)(steps / 1000));
  } else if (steps >= 1000) {
    snprintf(buffer, buffer_size, "%ld.%ldk", (long)(steps / 1000),
             (long)((steps % 1000) / 100));
  } else {
    snprintf(buffer, buffer_size, "%ld", (long)steps);
  }
}

static int max_hr_for_gauge(void) {
  return clamp_int(220 - s_settings.user_age, 120, 210);
}

static void format_temp_text(void) {
  int temp = s_temp_c;
  char unit = 'C';
  if (s_settings.temp_unit == 1) {
    temp = ((s_temp_c * 9) / 5) + 32;
    unit = 'F';
  }
  snprintf(s_temp_text, sizeof(s_temp_text), "%d%c", temp, unit);
}

static void update_world_time(time_t now) {
  time_t standard_epoch = now + (s_settings.world_offset_minutes * 60);
  struct tm *standard_tm = gmtime(&standard_epoch);
  if (!standard_tm) {
    snprintf(s_world_text, sizeof(s_world_text), "%s --:--", s_settings.world_label);
    return;
  }

  struct tm standard_copy = *standard_tm;
  int dst_offset = is_dst_for_mode(standard_copy.tm_year + 1900,
                                   standard_copy.tm_mon + 1,
                                   standard_copy.tm_mday,
                                   s_settings.world_dst_mode) ? 60 : 0;
  time_t world_epoch = now + ((s_settings.world_offset_minutes + dst_offset) * 60);
  struct tm *world_tm = gmtime(&world_epoch);
  if (!world_tm) {
    snprintf(s_world_text, sizeof(s_world_text), "%s --:--", s_settings.world_label);
    return;
  }

  char time_buffer[8];
  strftime(time_buffer, sizeof(time_buffer), use_24h_time() ? "%H:%M" : "%I:%M",
           world_tm);
  if (!use_24h_time() && time_buffer[0] == '0') {
    memmove(time_buffer, time_buffer + 1, strlen(time_buffer));
  }

  snprintf(s_world_text, sizeof(s_world_text), "%s %s",
           s_settings.world_label, time_buffer);
}

static void update_time_text(void) {
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  if (!tick_time) {
    return;
  }

  strftime(s_time_text, sizeof(s_time_text), use_24h_time() ? "%H:%M" : "%I:%M",
           tick_time);
  if (!use_24h_time() && s_time_text[0] == '0') {
    memmove(s_time_text, s_time_text + 1, strlen(s_time_text));
  }

  strftime(s_date_text, sizeof(s_date_text), "%a %d %b", tick_time);
  for (int i = 0; s_date_text[i]; i++) {
    if (s_date_text[i] >= 'a' && s_date_text[i] <= 'z') {
      s_date_text[i] = (char)(s_date_text[i] - 32);
    }
  }
  update_world_time(now);
}

static void update_health_text(void) {
  if (!s_settings.show_health) {
    snprintf(s_steps_text, sizeof(s_steps_text), "--");
    snprintf(s_hr_text, sizeof(s_hr_text), "--");
    s_steps_percent = 0;
    s_hr_percent = 0;
    return;
  }

#if defined(PBL_HEALTH)
  time_t now = time(NULL);
  time_t start = time_start_of_today();

  HealthServiceAccessibilityMask steps_access =
      health_service_metric_accessible(HealthMetricStepCount, start, now);
  if (steps_access & HealthServiceAccessibilityMaskAvailable) {
    int32_t steps = (int32_t)health_service_sum_today(HealthMetricStepCount);
    if (steps > 0 || !s_settings.demo_fallback) {
      format_steps(steps, s_steps_text, sizeof(s_steps_text));
      s_steps_percent = clamp_int((steps * 100) / s_settings.step_goal, 0, 100);
    } else {
      format_steps(DEFAULT_DEMO_STEPS, s_steps_text, sizeof(s_steps_text));
      s_steps_percent = clamp_int((DEFAULT_DEMO_STEPS * 100) / s_settings.step_goal, 0, 100);
    }
  } else if (s_settings.demo_fallback) {
    format_steps(DEFAULT_DEMO_STEPS, s_steps_text, sizeof(s_steps_text));
    s_steps_percent = clamp_int((DEFAULT_DEMO_STEPS * 100) / s_settings.step_goal, 0, 100);
  } else {
    snprintf(s_steps_text, sizeof(s_steps_text), "--");
    s_steps_percent = 0;
  }

  HealthServiceAccessibilityMask hr_access =
      health_service_metric_aggregate_averaged_accessible(
          HealthMetricHeartRateBPM, now, now, HealthAggregationAvg,
          HealthServiceTimeScopeOnce);
  if (hr_access & HealthServiceAccessibilityMaskAvailable) {
    HealthValue bpm = health_service_peek_current_value(HealthMetricHeartRateBPM);
    int bpm_value = clamp_int((int)bpm, 0, 999);
    if (bpm > 999) {
      snprintf(s_hr_text, sizeof(s_hr_text), "999+");
      s_hr_percent = 100;
    } else if (bpm_value > 0 || !s_settings.demo_fallback) {
      snprintf(s_hr_text, sizeof(s_hr_text), "%d", bpm_value);
      s_hr_percent = clamp_int((bpm_value * 100) / max_hr_for_gauge(), 0, 100);
    } else {
      snprintf(s_hr_text, sizeof(s_hr_text), "%d", DEFAULT_DEMO_HR);
      s_hr_percent = clamp_int((DEFAULT_DEMO_HR * 100) / max_hr_for_gauge(), 0, 100);
    }
  } else if (s_settings.demo_fallback) {
    snprintf(s_hr_text, sizeof(s_hr_text), "%d", DEFAULT_DEMO_HR);
    s_hr_percent = clamp_int((DEFAULT_DEMO_HR * 100) / max_hr_for_gauge(), 0, 100);
  } else {
    snprintf(s_hr_text, sizeof(s_hr_text), "--");
    s_hr_percent = 0;
  }
#else
  if (s_settings.demo_fallback) {
    format_steps(DEFAULT_DEMO_STEPS, s_steps_text, sizeof(s_steps_text));
    snprintf(s_hr_text, sizeof(s_hr_text), "%d", DEFAULT_DEMO_HR);
    s_steps_percent = clamp_int((DEFAULT_DEMO_STEPS * 100) / s_settings.step_goal, 0, 100);
    s_hr_percent = clamp_int((DEFAULT_DEMO_HR * 100) / max_hr_for_gauge(), 0, 100);
  } else {
    snprintf(s_steps_text, sizeof(s_steps_text), "--");
    snprintf(s_hr_text, sizeof(s_hr_text), "--");
    s_steps_percent = 0;
    s_hr_percent = 0;
  }
#endif
}

static void mark_canvas_dirty(void) {
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void battery_callback(BatteryChargeState state) {
  s_battery_percent = clamp_int(state.charge_percent, 0, 100);
  snprintf(s_battery_text, sizeof(s_battery_text), "%d%%", state.charge_percent);
  mark_canvas_dirty();
}

static void update_dashboard(void) {
  s_settings.step_goal = clamp_int(s_settings.step_goal, 1000, 50000);
  format_temp_text();
  update_time_text();
  update_health_text();
  mark_canvas_dirty();
}

static void draw_gauge(GContext *ctx, GPoint center, int percent, GColor accent,
                       const char *label, const char *value) {
  GRect ring = GRect(center.x - RING_RADIUS, center.y - RING_RADIUS,
                    RING_RADIUS * 2, RING_RADIUS * 2);
  percent = clamp_int(percent, 0, 100);

  graphics_context_set_stroke_width(ctx, RING_STROKE);
  graphics_context_set_stroke_color(ctx, color_ring_bg());
  graphics_draw_arc(ctx, ring, GOvalScaleModeFitCircle, 0, TRIG_MAX_ANGLE - 1);

  if (percent > 0) {
    graphics_context_set_stroke_color(ctx, accent);
    graphics_draw_arc(ctx, ring, GOvalScaleModeFitCircle, 0,
                      (TRIG_MAX_ANGLE * percent) / 100);
  }
  graphics_context_set_stroke_width(ctx, 1);

  draw_text(ctx, label, s_font_pix_sm,
            GRect(center.x - 22, center.y - 16, 44, 10),
            GTextAlignmentCenter, accent);
  draw_text(ctx, value, s_font_val,
            GRect(center.x - 21, center.y - 1, 42, 18),
            GTextAlignmentCenter, color_text());
}

static void draw_battery_icon(GContext *ctx, GRect frame, int percent) {
  percent = clamp_int(percent, 0, 100);
  graphics_context_set_fill_color(ctx, color_capsule());
  graphics_fill_rect(ctx, frame, 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, color_text());
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(ctx, frame);
  graphics_fill_rect(ctx, GRect(frame.origin.x + frame.size.w,
                                frame.origin.y + 3, 2, frame.size.h - 6),
                     0, GCornerNone);

  int fill_width = ((frame.size.w - 4) * percent) / 100;
  if (fill_width > 0) {
    graphics_context_set_fill_color(ctx, color_battery());
    graphics_fill_rect(ctx, GRect(frame.origin.x + 2, frame.origin.y + 2,
                                  fill_width, frame.size.h - 4),
                       0, GCornerNone);
  }
}

static GDrawCommandImage *weather_image(int code) {
  if (code < 0) return s_wx_cloudy;
  if (code == 0) return s_wx_sun;
  if (code <= 2) return s_wx_partly;
  if (code == 3) return s_wx_cloudy;
  if (code <= 48) return s_wx_fog;
  if (code <= 67) return s_wx_rain;
  if (code <= 77) return s_wx_snow;
  if (code <= 82) return s_wx_rain;
  if (code <= 86) return s_wx_snow;
  if (code <= 99) return s_wx_storm;
  return s_wx_cloudy;
}

static void draw_weather_pdc(GContext *ctx, GPoint origin, int code) {
  GDrawCommandImage *img = weather_image(code);
  if (img) {
    gdraw_command_image_draw(ctx, img, origin);
  }
}

static void draw_sun_event_pdc(GContext *ctx, GPoint origin, int event_type) {
  GDrawCommandImage *img = (event_type == 1) ? s_sun_set : s_sun_rise;
  if (img) {
    gdraw_command_image_draw(ctx, img, origin);
  }
}

static void draw_time_display(GContext *ctx, GRect area) {
  const char *colon = strchr(s_time_text, ':');
  if (!colon) {
    draw_text(ctx, s_time_text, s_font_time, area, GTextAlignmentLeft, color_text());
    return;
  }

  char hh[6] = {0};
  char mm[6] = {0};
  size_t hlen = (size_t)(colon - s_time_text);
  if (hlen >= sizeof(hh)) {
    hlen = sizeof(hh) - 1;
  }
  memcpy(hh, s_time_text, hlen);
  strncpy(mm, colon + 1, sizeof(mm) - 1);

  GSize hsz = graphics_text_layout_get_content_size(
      hh, s_font_time, area, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);

  const int gap = 5;
  const int dot = 7;
  int colon_x = area.origin.x + hsz.w + gap;
  int min_x = colon_x + dot + gap;

  graphics_context_set_text_color(ctx, color_text());
  graphics_draw_text(ctx, hh, s_font_time, area,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  graphics_context_set_fill_color(ctx, color_text());
  graphics_fill_rect(ctx, GRect(colon_x, area.origin.y + 21, dot, dot), 1, GCornersAll);
  graphics_fill_rect(ctx, GRect(colon_x, area.origin.y + 34, dot, dot), 1, GCornersAll);

  GRect mbox = GRect(min_x, area.origin.y,
                     area.size.w - (min_x - area.origin.x), area.size.h);
  graphics_draw_text(ctx, mm, s_font_time, mbox,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static int footer_text_w(const char *s) {
  GSize sz = graphics_text_layout_get_content_size(
      s, s_font_pix_sm, GRect(0, 0, 200, 20), GTextOverflowModeFill,
      GTextAlignmentLeft);
  return sz.w;
}

static void draw_footer_pair(GContext *ctx, int col_cx, int y, const char *a,
                             GColor ca, const char *b, GColor cb) {
  const int gap = 4;
  int aw = footer_text_w(a);
  int bw = footer_text_w(b);
  int x = col_cx - (aw + gap + bw) / 2;
  draw_text(ctx, a, s_font_pix_sm, GRect(x, y, aw + 8, 12), GTextAlignmentLeft, ca);
  draw_text(ctx, b, s_font_pix_sm, GRect(x + aw + gap, y, bw + 8, 12),
            GTextAlignmentLeft, cb);
}

static void draw_footer_sun(GContext *ctx, int col_cx, int text_y, int icon_y) {
  const int icon_w = 22;
  const int gap = 3;
  int tw = footer_text_w(s_sun_text);
  int x = col_cx - (icon_w + gap + tw) / 2;
  draw_sun_event_pdc(ctx, GPoint(x, icon_y), s_sun_event_type);
  draw_text(ctx, s_sun_text, s_font_pix_sm, GRect(x + icon_w + gap, text_y, tw + 8, 12),
            GTextAlignmentLeft, GColorWhite);
}

static void draw_footer_cell(GContext *ctx, int x, int w, int mode) {
  int cx = x + (w / 2);
  const int text_y = 197;

  switch (mode) {
    case FooterSun:
      draw_footer_sun(ctx, cx, text_y, 191);
      break;
    case FooterWorld:
      draw_text(ctx, s_world_text, s_font_pix_sm, GRect(x, text_y, w, 12),
                GTextAlignmentCenter, GColorWhite);
      break;
    case FooterBattery:
      draw_footer_pair(ctx, cx, text_y, "BAT", color_battery(),
                       s_settings.show_battery_percent ? s_battery_text : "", GColorWhite);
      break;
    case FooterWeather:
      draw_footer_pair(ctx, cx, text_y, "WX", color_rain(), s_temp_text, GColorWhite);
      break;
    case FooterTemp:
      draw_footer_pair(ctx, cx, text_y, "TMP", color_temp(), s_temp_text, GColorWhite);
      break;
    case FooterRain:
      draw_footer_pair(ctx, cx, text_y, "RAIN", color_rain(), s_rain_text, GColorWhite);
      break;
    case FooterSteps:
      draw_footer_pair(ctx, cx, text_y, "STEP", color_steps(), s_steps_text, GColorWhite);
      break;
    case FooterHeart:
      draw_footer_pair(ctx, cx, text_y, "HR", color_hr(), s_hr_text, GColorWhite);
      break;
    case FooterUV:
    default:
      draw_footer_pair(ctx, cx, text_y, "UV", color_sun(), s_uv_text, GColorWhite);
      break;
  }
}

static void draw_telemetry_gauges(GContext *ctx) {
  GaugeSpec gauges[3];
  int count = 0;

  if (s_settings.show_health) {
    gauges[count++] = (GaugeSpec) {"HR", s_hr_text, s_hr_percent, color_hr()};
    gauges[count++] = (GaugeSpec) {"STEP", s_steps_text, s_steps_percent, color_steps()};
  }
  if (s_settings.show_weather) {
    gauges[count++] = (GaugeSpec) {"RAIN", s_rain_text, s_rain_percent, color_rain()};
  }

  if (count == 3) {
    draw_gauge(ctx, GPoint(40, 144), gauges[0].percent, gauges[0].accent,
               gauges[0].label, gauges[0].value);
    draw_gauge(ctx, GPoint(100, 144), gauges[1].percent, gauges[1].accent,
               gauges[1].label, gauges[1].value);
    draw_gauge(ctx, GPoint(160, 144), gauges[2].percent, gauges[2].accent,
               gauges[2].label, gauges[2].value);
  } else if (count == 2) {
    draw_gauge(ctx, GPoint(70, 144), gauges[0].percent, gauges[0].accent,
               gauges[0].label, gauges[0].value);
    draw_gauge(ctx, GPoint(130, 144), gauges[1].percent, gauges[1].accent,
               gauges[1].label, gauges[1].value);
  } else if (count == 1) {
    draw_gauge(ctx, GPoint(100, 144), gauges[0].percent, gauges[0].accent,
               gauges[0].label, gauges[0].value);
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int width = bounds.size.w;

  graphics_context_set_fill_color(ctx, color_bg());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  GRect capsule = GRect(7, 7, width - 14, 34);
  graphics_context_set_fill_color(ctx, color_capsule());
  graphics_fill_rect(ctx, capsule, 3, GCornersAll);
  graphics_context_set_stroke_color(ctx, color_panel_stroke());
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_round_rect(ctx, capsule, 3);

  draw_text(ctx, s_date_text, s_font_pix_lg,
            GRect(14, 12, 120, 20), GTextAlignmentLeft, color_muted());
  int battery_x = s_settings.show_battery_percent ? width - 58 : width - 37;
  draw_battery_icon(ctx, GRect(battery_x, 19, 22, 10), s_battery_percent);
  if (s_settings.show_battery_percent) {
    draw_text(ctx, s_battery_text, s_font_pix_sm,
              GRect(width - 34, 19, 30, 12), GTextAlignmentLeft, color_battery());
  }

  draw_time_display(ctx, GRect(7, 48, 138, 50));
  if (s_settings.show_weather) {
    draw_text(ctx, s_temp_text, s_font_num,
              GRect(width - 60, 48, 51, 24), GTextAlignmentRight, color_temp());
    draw_weather_pdc(ctx, GPoint(width - 42, 70), s_weather_code);
  }

  graphics_context_set_stroke_color(ctx, color_line());
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(10, 101), GPoint(width - 10, 101));

  draw_telemetry_gauges(ctx);

  GRect footer = GRect(10, 190, width - 20, 22);
  graphics_context_set_fill_color(ctx, color_panel());
  graphics_fill_rect(ctx, footer, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorFromHEX(0x233547), GColorWhite));
  graphics_fill_rect(ctx, GRect(66, 193, 2, 16), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(132, 193, 2, 16), 0, GCornerNone);

  draw_footer_cell(ctx, 10, 56, s_settings.footer_left);
  draw_footer_cell(ctx, 68, 64, s_settings.footer_center);
  draw_footer_cell(ctx, 134, 56, s_settings.footer_right);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_dashboard();

  if (s_settings.show_weather && tick_time->tm_min % 30 == 0) {
    request_weather();
  }
}

static bool apply_int_setting(DictionaryIterator *iterator, uint32_t key,
                              int32_t *target, int min_value, int max_value) {
  Tuple *tuple = dict_find(iterator, key);
  if (!tuple) {
    return false;
  }
  *target = clamp_int((int)tuple->value->int32, min_value, max_value);
  return true;
}

static bool apply_bool_setting(DictionaryIterator *iterator, uint32_t key, bool *target) {
  Tuple *tuple = dict_find(iterator, key);
  if (!tuple) {
    return false;
  }
  *target = tuple->value->int32 != 0;
  return true;
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  bool settings_changed = false;

  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *weather_code_tuple = dict_find(iterator, MESSAGE_KEY_WEATHER_CODE);
  Tuple *rain_tuple = dict_find(iterator, MESSAGE_KEY_RAIN_CHANCE);
  Tuple *uv_tuple = dict_find(iterator, MESSAGE_KEY_UV_INDEX);
  Tuple *sun_time_tuple = dict_find(iterator, MESSAGE_KEY_SUN_EVENT_TIME);
  Tuple *sun_type_tuple = dict_find(iterator, MESSAGE_KEY_SUN_EVENT_TYPE);

  if (temp_tuple) {
    s_temp_c = (int)temp_tuple->value->int32;
    format_temp_text();
  }
  if (weather_code_tuple) {
    s_weather_code = (int)weather_code_tuple->value->int32;
  }
  if (rain_tuple) {
    int rain = clamp_int((int)rain_tuple->value->int32, 0, 100);
    s_rain_percent = rain;
    snprintf(s_rain_text, sizeof(s_rain_text), "%d%%", rain);
  }
  if (uv_tuple) {
    int uv_scaled = clamp_int((int)uv_tuple->value->int32, 0, 999);
    snprintf(s_uv_text, sizeof(s_uv_text), "%d.%d", uv_scaled / 10, uv_scaled % 10);
  }
  if (sun_time_tuple) {
    snprintf(s_sun_text, sizeof(s_sun_text), "%s", sun_time_tuple->value->cstring);
  }
  if (sun_type_tuple) {
    s_sun_event_type = (int)sun_type_tuple->value->int32;
  }

  settings_changed |= apply_bool_setting(iterator, MESSAGE_KEY_DARK_MODE,
                                         &s_settings.dark_mode);
  Tuple *time_mode = dict_find(iterator, MESSAGE_KEY_TIME_MODE);
  if (time_mode) {
    s_settings.time_mode = clamp_int((int)time_mode->value->int32, TimeModeSystem, TimeMode24h);
    settings_changed = true;
  }
  Tuple *temp_unit = dict_find(iterator, MESSAGE_KEY_TEMP_UNIT);
  if (temp_unit) {
    s_settings.temp_unit = clamp_int((int)temp_unit->value->int32, 0, 1);
    settings_changed = true;
  }
  settings_changed |= apply_int_setting(iterator, MESSAGE_KEY_STEP_GOAL,
                                        &s_settings.step_goal, 1000, 50000);
  Tuple *age = dict_find(iterator, MESSAGE_KEY_USER_AGE);
  if (age) {
    s_settings.user_age = clamp_int((int)age->value->int32, 10, 100);
    settings_changed = true;
  }
  Tuple *world_label = dict_find(iterator, MESSAGE_KEY_WORLD_LABEL);
  if (world_label) {
    snprintf(s_settings.world_label, sizeof(s_settings.world_label), "%s",
             world_label->value->cstring);
    settings_changed = true;
  }
  settings_changed |= apply_int_setting(iterator, MESSAGE_KEY_WORLD_OFFSET_MIN,
                                        &s_settings.world_offset_minutes, -720, 840);
  Tuple *dst_mode = dict_find(iterator, MESSAGE_KEY_WORLD_DST_MODE);
  if (dst_mode) {
    s_settings.world_dst_mode = clamp_int((int)dst_mode->value->int32,
                                          DstModeNone, DstModeAUS);
    settings_changed = true;
  }
  settings_changed |= apply_int_setting(iterator, MESSAGE_KEY_HR_COLOR,
                                        &s_settings.hr_color, 0, 0xffffff);
  settings_changed |= apply_int_setting(iterator, MESSAGE_KEY_STEPS_COLOR,
                                        &s_settings.steps_color, 0, 0xffffff);
  settings_changed |= apply_int_setting(iterator, MESSAGE_KEY_RAIN_COLOR,
                                        &s_settings.rain_color, 0, 0xffffff);
  Tuple *footer_left = dict_find(iterator, MESSAGE_KEY_FOOTER_LEFT);
  if (footer_left) {
    s_settings.footer_left = clamp_int((int)footer_left->value->int32, FooterUV, FooterHeart);
    settings_changed = true;
  }
  Tuple *footer_center = dict_find(iterator, MESSAGE_KEY_FOOTER_CENTER);
  if (footer_center) {
    s_settings.footer_center = clamp_int((int)footer_center->value->int32, FooterUV, FooterHeart);
    settings_changed = true;
  }
  Tuple *footer_right = dict_find(iterator, MESSAGE_KEY_FOOTER_RIGHT);
  if (footer_right) {
    s_settings.footer_right = clamp_int((int)footer_right->value->int32, FooterUV, FooterHeart);
    settings_changed = true;
  }
  settings_changed |= apply_bool_setting(iterator, MESSAGE_KEY_DEMO_FALLBACK,
                                         &s_settings.demo_fallback);
  settings_changed |= apply_bool_setting(iterator, MESSAGE_KEY_SHOW_WEATHER,
                                         &s_settings.show_weather);
  settings_changed |= apply_bool_setting(iterator, MESSAGE_KEY_SHOW_HEALTH,
                                         &s_settings.show_health);
  settings_changed |= apply_bool_setting(iterator, MESSAGE_KEY_SHOW_BATTERY_PERCENT,
                                         &s_settings.show_battery_percent);

  if (settings_changed) {
    save_settings();
    window_set_background_color(s_main_window, color_bg());
    update_dashboard();
    if (s_settings.show_weather) {
      request_weather();
    }
    return;
  }

  mark_canvas_dirty();
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Inbox dropped: %d", reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason,
                                   void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox failed: %d", reason);
}

static void request_key(uint32_t key) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result == APP_MSG_OK && iter) {
    dict_write_uint8(iter, key, 1);
    app_message_outbox_send();
  }
}

static void request_weather(void) {
  if (s_settings.show_weather) {
    request_key(MESSAGE_KEY_REQUEST_WEATHER);
  }
}

static void request_settings(void) {
  request_key(MESSAGE_KEY_REQUEST_SETTINGS);
}

#if defined(PBL_HEALTH)
static void health_event_handler(HealthEventType event, void *context) {
  update_health_text();
  mark_canvas_dirty();
}
#endif

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_44));
  s_font_num = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_NUM_22));
  s_font_val = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_VAL_15));
  s_font_pix_lg = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PIX_16));
  s_font_pix_sm = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PIX_8));

  s_wx_sun = gdraw_command_image_create_with_resource(RESOURCE_ID_WX_SUN);
  s_wx_partly = gdraw_command_image_create_with_resource(RESOURCE_ID_WX_PARTLY);
  s_wx_cloudy = gdraw_command_image_create_with_resource(RESOURCE_ID_WX_CLOUDY);
  s_wx_rain = gdraw_command_image_create_with_resource(RESOURCE_ID_WX_RAIN);
  s_wx_snow = gdraw_command_image_create_with_resource(RESOURCE_ID_WX_SNOW);
  s_wx_storm = gdraw_command_image_create_with_resource(RESOURCE_ID_WX_STORM);
  s_wx_fog = gdraw_command_image_create_with_resource(RESOURCE_ID_WX_FOG);
  s_sun_rise = gdraw_command_image_create_with_resource(RESOURCE_ID_SUN_RISE);
  s_sun_set = gdraw_command_image_create_with_resource(RESOURCE_ID_SUN_SET);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  update_dashboard();
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  s_canvas_layer = NULL;

  fonts_unload_custom_font(s_font_time);
  fonts_unload_custom_font(s_font_num);
  fonts_unload_custom_font(s_font_val);
  fonts_unload_custom_font(s_font_pix_lg);
  fonts_unload_custom_font(s_font_pix_sm);

  gdraw_command_image_destroy(s_wx_sun);
  gdraw_command_image_destroy(s_wx_partly);
  gdraw_command_image_destroy(s_wx_cloudy);
  gdraw_command_image_destroy(s_wx_rain);
  gdraw_command_image_destroy(s_wx_snow);
  gdraw_command_image_destroy(s_wx_storm);
  gdraw_command_image_destroy(s_wx_fog);
  gdraw_command_image_destroy(s_sun_rise);
  gdraw_command_image_destroy(s_sun_set);
}

static void init(void) {
  load_settings();

  s_main_window = window_create();
  window_set_background_color(s_main_window, color_bg());
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_open(1024, 512);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
  battery_callback(battery_state_service_peek());
#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_event_handler, NULL);
#endif

  update_dashboard();
  request_settings();
  request_weather();
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
