#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "worldmap_image.h"
#include "angle_tables.h"

// Unfortunately the watch doesn't store time zone information
// Enter the current time zone (ignoring daylight savings)
#define TZ_OFFSET -8 // PST

// Can be used to distinguish between multiple timers in your app
#define TIMER_ID_REFRESH 1

#define MY_UUID { 0xB0, 0x17, 0x45, 0xA9, 0xE2, 0x09, 0x44, 0x1C, 0xAB, 0xF2, 0x8F, 0xF2, 0xE5, 0x1A, 0xED, 0xD2 }
PBL_APP_INFO(MY_UUID,
             "World Map", "Tom Yedwab",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_STANDARD_APP);


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

// Window object
Window g_window;

// Layer to draw on
Layer g_layer;

// Bitmap we're drawing into
GBitmap g_bmp;

// Pixel data for the bitmap
char g_bmpdata[ROW_SIZE(224)*168];

// Flag that indicates the window is fully visible
int g_loaded = 0;

// Flag that signals a need to regenerate the overlay
int g_needs_refresh = 0;

// The time-of-day offset (0-215)
int g_time_offset = 0;

// The time-of-year offset (0-365)
int g_year_offset = 0;


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
            float cos_theta = THETA_TABLE[x_offset % 216];
            float sin_theta = THETA_TABLE[(x_offset + 54) % 216];

            for (y = 0; y < 168; y++) {
                // Get the input map's pixel value
                int addr = x + y * 216;
                char in_val = WORLD_MAP_IMAGE[addr/8] & (1<<(addr%8));

                // Calculate the latitude
                float cos_phi = PHI_TABLE[y];
                float sin_phi = (y <= 84) ? PHI_TABLE[84 - y] : -1 * PHI_TABLE[168 - (y - 84)];

                // Calculate the dot product of the position on the earth and the direction to the sun:
                // <cos(phi)*cos(theta)*cos(alpha) + sin(phi)*sin(alpha), cos(phi)*sin(theta)> * <cos_year, sin_year>
                float dp = (cos_phi * cos_theta * COS_ALPHA + sin_phi * SIN_ALPHA) * cos_year
                    + cos_phi * sin_theta * sin_year;

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

    // Unset the "needs refresh" flag
    g_needs_refresh = 0;
}


// Animate the layer position to a given X offset
void animate_layer(int destination) {
    static PropertyAnimation prop_ani;
    static GRect prop_rect = {
        .origin = {
            .x = 0,
            .y = 0
        },
        .size = {
            .w = 216,
            .h = 168
        }
    };
    prop_rect.origin.x = destination;
    property_animation_init_layer_frame(&prop_ani, &g_layer, NULL, &prop_rect);
    prop_ani.animation.duration_ms = 1000; // 1 second animation
    animation_schedule(&prop_ani.animation);
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


// Handle click on the "select" button (currently unused)
void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    (void)recognizer;
    (void)window;
}


// Handle long press on the "select" button (currently unused)
void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
}


// Register our input handlers
void click_config_provider(ClickConfig **config, Window *window) {
  (void)window;

  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;

  config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}


// Initialization routine
void handle_init(AppContextRef ctx) {
    (void)ctx;
    int y;

    // Create fullscreen window
    window_init(&g_window, "WorldMap");
    g_window.is_fullscreen = 1;
    window_stack_push(&g_window, true /* Animated */);

    // Attach our desired button functionality
    window_set_click_config_provider(&g_window, (ClickConfigProvider) click_config_provider);

    // Init the layer for the map display
    layer_init(&g_layer, g_window.layer.frame);
    g_layer.update_proc = &layer_update_callback;
    layer_add_child(&g_window.layer, &g_layer);

    // Initialize the bitmap structure
    init_bitmap(&g_bmp, 224, 168, g_bmpdata);

    // Render just the map while slide-in animation is happening,
    for (y = 0; y < 168; y++) {
        memcpy(&g_bmpdata[y*ROW_SIZE(224)], &WORLD_MAP_IMAGE[y*27], ROW_SIZE(224));
    }
    for (y = 0; y < 168*ROW_SIZE(224); y++) {
        g_bmpdata[y] ^= 0xFF;
    }

    // After half a second start rendering the sunlight map, as it is slow
    app_timer_send_event(ctx, 500 /* milliseconds */, TIMER_ID_REFRESH);
}


// Helper to update internal state based on the current time & date
void update_time(PblTm *time) {
    // Don't do anything until we're fully visible on-screen
    if (g_loaded) {
        int utc_hour;

        // Time-of-year offset (0-365)
        g_year_offset = time->tm_yday % 365;

        // Time-of-day offset (0-216)
        utc_hour = time->tm_hour - TZ_OFFSET + 12;
        g_time_offset = (((time->tm_min + utc_hour * 60) * 3) / 20) % 216;

        // Trigger a refresh
        g_needs_refresh = 1;
        layer_mark_dirty(&g_layer);
    }
}


// Handle timer callbacks
void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
    if (cookie == TIMER_ID_REFRESH) {
        // Indicate that we can start expensive rendering
        g_loaded = 1;

        // Indicate we need to rendered overlay
        g_needs_refresh = 1;

        // Update the time-based state
        PblTm time;
        get_time(&time);
        update_time(&time);
    }
}


// Handle clock ticks
void handle_tick(AppContextRef ctx, PebbleTickEvent *event) {
    update_time(event->tick_time);
}


// Main entry point for the app
void pbl_main(void *params) {
    PebbleAppHandlers handlers = {
        .init_handler = &handle_init,
        .timer_handler = &handle_timer,
        .tick_info = {
            .tick_handler = &handle_tick,
            .tick_units = HOUR_UNIT
        },
    };
    app_event_loop(params, &handlers);
}
