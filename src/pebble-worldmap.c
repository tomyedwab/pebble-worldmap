#include <pebble.h>
#include <time.h>

#include "worldmap_image.h"
#include "angle_tables.h"
#include "config.h"

// Can be used to distinguish between multiple timers in your app
#define TIMER_ID_REFRESH 1

#define PERSIST_KEY_SHOW_HOME   1
#define PERSIST_KEY_LATITUDE    2
#define PERSIST_KEY_LONGITUDE   3


// Reverse-engineered internals of the GBitmap struct
#define ROW_SIZE(width) (width>>3)
void init_bitmap(GBitmap *bmp, int width, int height, void *data) {
  // width must be a multiple of 32?
  bmp->row_size_bytes = width >> 3; // 8 pixels per byte
  bmp->info_flags = 0;
  bmp->bounds.origin.x = 0;
  bmp->bounds.origin.y = 0;
  bmp->bounds.size.w = width;
  bmp->bounds.size.h = height;
  bmp->addr = data;
}

/*
 * Globals
 */

// Window objects
Window *g_window;
Window *g_window_settings;

// Bitmap we're drawing into
GBitmap g_bmp;

// Pixel data for the bitmap
char g_bmpdata[ROW_SIZE(224)*168];

// Flag that indicates the window is fully visible
int g_loaded = 0;

// Flag that signals a need to regenerate the overlay
int g_needs_refresh = 0;

// Latitude and longitude of "home"
int home_latitude;
int home_longitude;
// TODO: Time zone

// Enable drawing of sunrise/sunset times
int g_draw_sunrise = 0;

// Transformed latitude/longitude
int g_home_pos[] = {0, 0};

// The time-of-day offset (0-215)
int g_time_offset = 0;

// The time-of-year offset (0-365)
int g_year_offset = 0;

// Stored time information
int g_hour = 0;
int g_minute = 0;

// Last stored scroll position
int g_last_offset = 0;

// Selected option on settings screen
int g_selected_option = 0;

// Option 1 or 2 is being edited
int g_edit_option = 0;


void handle_timer(void *data);


// Calculate the longitude cos/sin
void calc_theta(int x_offset, float *cos_theta, float *sin_theta) {
    int offset;
    offset = x_offset % 216;
    if (offset > 108) offset = 216 - offset;
    *cos_theta = THETA_TABLE[offset];
    offset = (x_offset + 54) % 216;
    if (offset > 108) offset = 216 - offset;
    *sin_theta = THETA_TABLE[offset];
}

// Calculate the latitude cos/sin
void calc_phi(int y, float *cos_phi, float *sin_phi) {
    *cos_phi = PHI_COS_TABLE[y];
    *sin_phi = PHI_SIN_TABLE[y];
}

// Calculate the dot product of the position on the earth and the direction to the sun:
// <cos(phi)*cos(theta)*cos(alpha) + sin(phi)*sin(alpha), cos(phi)*sin(theta)> * <cos_year, sin_year>
float calc_dp(float cos_phi, float sin_phi, float cos_theta, float sin_theta, float cos_year, float sin_year) {
    return (cos_phi * cos_theta * COS_ALPHA + sin_phi * SIN_ALPHA) * cos_year
        + cos_phi * sin_theta * sin_year;
}

// Main rendering function for our only layer
void layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    (void)ctx;
    int x, y;
    GRect destination;

    // Calculate rotation around the sun
    float cos_year = YEAR_TABLE[g_year_offset % 365];
    float sin_year = YEAR_TABLE[(g_year_offset + 91) % 365];

    // Use the full screen
    destination.origin.x = 0;
    destination.origin.y = 0;
    destination.size.w = 216;
    destination.size.h = 168;

    if (g_needs_refresh) {
        // Render the image in WORLD_MAP_IMAGE into the bmpdata bitmap
        memset(g_bmpdata, 0, ROW_SIZE(224)*168);
        for (x = 0; x < 216; x++) {
            // Calculate the Earth's daily rotation
            int x_offset = x + g_time_offset + (g_year_offset * 59 / 100);
            float cos_theta, sin_theta;
            calc_theta(x_offset, &cos_theta, &sin_theta);

            for (y = 0; y < 168; y++) {
                // Get the input map's pixel value
                int addr = x + y * 216;
                char in_val = WORLD_MAP_IMAGE[addr/8] & (1<<(addr%8));
                float cos_phi, sin_phi, dp;

                calc_phi(y, &cos_phi, &sin_phi);
                dp = calc_dp(cos_phi, sin_phi, cos_theta, sin_theta, cos_year, sin_year);

                // If the dot product is negative, the sun is up.
                // If the dot product is positive, it's nighttime.
                int val = 0;
                if (in_val != 0) {
                    // Land masses!
                    char stipple = ((x % 2) == 0 && (y % 2) == 0) ? 0 : 1;
                    if (dp > 0) val = 1;
                    else val = stipple;
                } else {
                    // Water
                    char stipple = (((x + y) % 2) == 0) ? 1 : 0;
                    if (dp > 0) val = stipple;
                    else val = 0;
                }

                // If necessary, set the appropriate bit in the output bitmap
                if (val == 0) {
                    g_bmpdata[y*ROW_SIZE(224) + x/8] |= (1 << (x%8));
                }
            }
        }
    }

    // Render the map
    graphics_draw_bitmap_in_rect(ctx, &g_bmp, destination);

    if (g_draw_sunrise) {
        // Text to show the time for the next sunrise/sunset
        char g_sunrise[32];

        // Calculate sunrise/sunset time
        float last_dp = 0;
        int sunrise_x = -1, sunset_x = -1;

        // Calculate the latitude
        float cos_phi, sin_phi;
        calc_phi(g_home_pos[1], &cos_phi, &sin_phi);

        // Traverse from home coordinates going East until we hit a boundary
        for (x = 0; x < 216; x++) {
            // Calculate the Earth's daily rotation
            int x_offset = g_home_pos[0] + x + g_time_offset + (g_year_offset * 59 / 100);
            float cos_theta, sin_theta, dp;

            calc_theta(x_offset, &cos_theta, &sin_theta);
            dp = calc_dp(cos_phi, sin_phi, cos_theta, sin_theta, cos_year, sin_year);

            if (last_dp < 0 && dp > 0) {
                // Sunset!
                sunset_x = x;
            } else if (last_dp > 0 && dp < 0) {
                // Sunrise!
                sunrise_x = x;
            }

            // No boundary; keep going
            last_dp = dp;
        }

        if (sunrise_x >= 0 && sunset_x >= 0) {
            // Calculate the time of the crossing
            struct tm time;
            int minutes;
            char time_buf[6];

            // Sunrise
            minutes = g_minute + (sunrise_x * 1440) / 216;
            time.tm_hour = (g_hour + (minutes / 60)) % 24;
            time.tm_min = minutes % 60;
            if (clock_is_24h_style()) {
                strftime(time_buf, 20, "%H:%M", &time);
            } else {
                strftime(time_buf, 20, "%I:%M%p", &time);
            }

            strcpy(g_sunrise, "rise ");
            strcat(g_sunrise, time_buf);

            // Sunset
            // Sunset times seem to be about 25 minutes off from official tables
            minutes = g_minute + (sunset_x * 1440) / 216 + 25;
            time.tm_hour = (g_hour + (minutes / 60)) % 24;
            time.tm_min = minutes % 60;
            if (clock_is_24h_style()) {
                strftime(time_buf, 20, "%H:%M", &time);
            } else {
                strftime(time_buf, 20, "%I:%M%p", &time);
            }

            strcat(g_sunrise, " set ");
            strcat(g_sunrise, time_buf);
        } else {
            // If this happens to you, I feel sorry for you
            strcpy(g_sunrise, "No sunrise/sunset");
        }

        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, GRect(g_home_pos[0]-2, g_home_pos[1]-2, 5, 5), 0, GCornerNone);
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_rect(ctx, GRect(g_home_pos[0]-1, g_home_pos[1]-1, 3, 3), 0, GCornerNone);

        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_rect(ctx, GRect(0, 168-16, 144, 16), 0, GCornerNone);

        graphics_context_set_text_color(ctx, GColorBlack);
        graphics_draw_text(ctx,
                g_sunrise,
                fonts_get_system_font(FONT_KEY_FONT_FALLBACK),
                GRect(5, 168-16, 144-5, 16),
                GTextOverflowModeWordWrap,
                GTextAlignmentLeft,
                NULL);
    }

    // Unset the "needs refresh" flag
    g_needs_refresh = 0;
}


// Render the settings dialog
void settings_layer_update_callback(Layer *me, GContext* ctx) {
    char pos_str[12];
    GRect rect = {
        .origin = {
            .x = 0,
            .y = 0
        },
        .size = {
            .w = 144,
            .h = 20
        }
    };
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(0, 0, 216, 168), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, GColorBlack);

    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(
            ctx,
            "Settings",
            fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
            rect,
            GTextOverflowModeWordWrap,
            GTextAlignmentLeft,
            NULL);

    rect.origin.y += 20;
    rect.origin.x += 5;

    graphics_draw_text(
            ctx,
            "Show home",
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
            rect,
            GTextOverflowModeWordWrap,
            GTextAlignmentLeft,
            NULL);

    rect.origin.y += 16;

    if (g_selected_option == 0) {
        graphics_fill_rect(ctx, rect, 0, GCornerNone);
        graphics_context_set_text_color(ctx, GColorWhite);
    }
    graphics_draw_text(
            ctx,
            g_draw_sunrise ? "Enabled" : "Disabled",
            fonts_get_system_font(FONT_KEY_GOTHIC_14),
            rect,
            GTextOverflowModeWordWrap,
            GTextAlignmentLeft,
            NULL);
    graphics_context_set_text_color(ctx, GColorBlack);

    rect.origin.y += 16;

    graphics_draw_text(
            ctx,
            "Latitude",
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
            rect,
            GTextOverflowModeWordWrap,
            GTextAlignmentLeft,
            NULL);

    rect.origin.y += 16;

    if (g_selected_option == 1) {
        graphics_fill_rect(ctx, rect, 0, GCornerNone);
        graphics_context_set_text_color(ctx, GColorWhite);
    }
    snprintf(pos_str, 12, (g_selected_option == 1 && g_edit_option) ? "> %d <" : "%d", home_latitude);
    graphics_draw_text(
            ctx,
            pos_str,
            fonts_get_system_font(FONT_KEY_GOTHIC_14),
            rect,
            GTextOverflowModeWordWrap,
            GTextAlignmentLeft,
            NULL);
    graphics_context_set_text_color(ctx, GColorBlack);

    rect.origin.y += 16;

    graphics_draw_text(
            ctx,
            "Longitude",
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
            rect,
            GTextOverflowModeWordWrap,
            GTextAlignmentLeft,
            NULL);

    rect.origin.y += 16;

    if (g_selected_option == 2) {
        graphics_fill_rect(ctx, rect, 0, GCornerNone);
        graphics_context_set_text_color(ctx, GColorWhite);
    }
    snprintf(pos_str, 12, (g_selected_option == 2 && g_edit_option) ? "> %d <" : "%d", home_longitude);
    graphics_draw_text(
            ctx,
            pos_str,
            fonts_get_system_font(FONT_KEY_GOTHIC_14),
            rect,
            GTextOverflowModeWordWrap,
            GTextAlignmentLeft,
            NULL);
    graphics_context_set_text_color(ctx, GColorBlack);

    rect.origin.y += 16;
}


// Animate the layer position to a given X offset
void animate_layer(int destination) {
    static GRect from_rect = {
        .origin = {
            .x = 0,
            .y = 0
        },
        .size = {
            .w = 216,
            .h = 168
        }
    };
    static GRect to_rect = {
        .origin = {
            .x = 0,
            .y = 0
        },
        .size = {
            .w = 216,
            .h = 168
        }
    };
    from_rect.origin.x = g_last_offset;
    to_rect.origin.x = destination;
    PropertyAnimation *prop_anim = property_animation_create_layer_frame(
            window_get_root_layer(g_window), &from_rect, &to_rect);
    animation_set_duration((Animation*)prop_anim, 1000);
    animation_schedule((Animation*)prop_anim);

    g_last_offset = destination;
}


// Handle click on the "up" button (scroll left)
void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    (void)recognizer;
    (void)window;

    animate_layer(0);
}


// Handle click on the "down" button (scroll right)
void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    (void)recognizer;
    (void)window;

    animate_layer(-72);
}


// Handle click on the "select" button (displays settings dialog)
void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    g_selected_option = 0;
    window_stack_push(g_window_settings, 1);
}


// Register our input handlers
void click_config_provider(void *context) {
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, (ClickHandler) up_single_click_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, (ClickHandler) down_single_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) select_single_click_handler);
}


void update_home_pos() {
    int y;
    g_home_pos[0] = (home_longitude + 180) * 0.6; // 0-216
    g_home_pos[1] = 0;
    for (y = 0; y < 168; y++) {
        if (LATITUDE_TABLE[y] < home_latitude) {
            g_home_pos[1] = y;
            break;
        }
    }
}


// Handle click on the "up" button (previous setting or increment)
void setting_up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    (void)recognizer;
    (void)window;
    if (g_edit_option) {
        if (g_selected_option == 1) {
            if (home_latitude < 90) {
                home_latitude += 1;
                persist_write_int(PERSIST_KEY_LATITUDE, home_latitude);
            }
        } else if (g_selected_option == 2) {
            if (home_longitude < 180) {
                home_longitude += 1;
                persist_write_int(PERSIST_KEY_LONGITUDE, home_longitude);
                app_timer_register(500, handle_timer, (void *)TIMER_ID_REFRESH);
            }
        }
        update_home_pos();
    } else {
        if (g_selected_option > 0) {
            g_selected_option--;
        }
    }
    layer_mark_dirty(window_get_root_layer(g_window_settings));
}


// Handle click on the "down" button (next setting or decrement)
void setting_down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    (void)recognizer;
    (void)window;
    if (g_edit_option) {
        if (g_selected_option == 1) {
            if (home_latitude > -90) {
                home_latitude -= 1;
                persist_write_int(PERSIST_KEY_LATITUDE, home_latitude);
            }
        } else if (g_selected_option == 2) {
            if (home_longitude > -180) {
                home_longitude -= 1;
                persist_write_int(PERSIST_KEY_LONGITUDE, home_longitude);
                app_timer_register(500, handle_timer, (void *)TIMER_ID_REFRESH);
            }
        }
        update_home_pos();
    } else {
        if (g_selected_option < 2) {
            g_selected_option++;
        }
    }
    layer_mark_dirty(window_get_root_layer(g_window_settings));
}


// Handle click on the "select" button
void setting_select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    if (g_selected_option == 0) {
        // Toggle "draw home" setting
        g_draw_sunrise = 1 - g_draw_sunrise;
        persist_write_int(PERSIST_KEY_SHOW_HOME, g_draw_sunrise);
    } else {
        g_edit_option = 1 - g_edit_option;
    }
    layer_mark_dirty(window_get_root_layer(g_window_settings));
}

void settings_click_config_provider(void *context) {
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, (ClickHandler) setting_up_single_click_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, (ClickHandler) setting_down_single_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) setting_select_single_click_handler);
}


// Initialization routine
void handle_init() {
    int y;

    // Create fullscreen window
    g_window = window_create();
    window_set_fullscreen(g_window, 1);
    window_stack_push(g_window, true /* Animated */);

    // Attach our desired button functionality
    window_set_click_config_provider(g_window, (ClickConfigProvider) click_config_provider);

    // Init the layer for the map display
    layer_set_update_proc(window_get_root_layer(g_window),
            &layer_update_callback);

    // Initialize the bitmap structure
    init_bitmap(&g_bmp, 224, 168, g_bmpdata);

    // Render just the map while slide-in animation is happening,
    for (y = 0; y < 168; y++) {
        memcpy(&g_bmpdata[y*ROW_SIZE(224)], &WORLD_MAP_IMAGE[y*27], ROW_SIZE(224));
    }
    for (y = 0; y < 168*ROW_SIZE(224); y++) {
        g_bmpdata[y] ^= 0xFF;
    }

    // Read settings
    g_draw_sunrise = persist_read_int(PERSIST_KEY_SHOW_HOME);
    home_latitude = persist_read_int(PERSIST_KEY_LATITUDE);
    home_longitude = persist_read_int(PERSIST_KEY_LONGITUDE);

    // Calculate "home" position
    update_home_pos();
 
    // The "settings" window
    g_window_settings = window_create();

    // Attach our desired button functionality
    window_set_click_config_provider(g_window_settings, (ClickConfigProvider) settings_click_config_provider);

    layer_set_update_proc(
            window_get_root_layer(g_window_settings),
            &settings_layer_update_callback);

    // After half a second start rendering the sunlight map, as it is slow
    app_timer_register(500, handle_timer, (void *)TIMER_ID_REFRESH);
}


void handle_deinit() {
    window_destroy(g_window);
}


// Helper to update internal state based on the current time & date
void update_time(struct tm *time) {
    // Don't do anything until we're fully visible on-screen
    if (g_loaded) {
        int utc_hour;
        int tz_offset = home_longitude / 15;

        // Time-of-year offset (0-365)
        g_year_offset = time->tm_yday % 365;

        // Time-of-day offset (0-216)
        utc_hour = time->tm_hour - tz_offset + 12;
        g_time_offset = (((time->tm_min + utc_hour * 60) * 3) / 20) % 216;

        g_hour = time->tm_hour;
        g_minute = time->tm_min;

        // Trigger a refresh
        g_needs_refresh = 1;
        layer_mark_dirty(window_get_root_layer(g_window));
    }
}


// Handle timer callbacks
void handle_timer(void *data) {
    int cookie = (int)data;
    if (cookie == TIMER_ID_REFRESH) {
        // Indicate that we can start expensive rendering
        g_loaded = 1;

        // Indicate we need to rendered overlay
        g_needs_refresh = 1;

        // Update the time-based state
        time_t rawtime;
        time(&rawtime);
        struct tm *tick_time = localtime(&rawtime);
        update_time(tick_time);
    }
}


// Handle clock ticks
void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
    update_time(tick_time);
}


// Main entry point for the app
int main(void) {
    handle_init();
    tick_timer_service_subscribe(HOUR_UNIT, handle_tick);
    app_event_loop();
    handle_deinit();
}
