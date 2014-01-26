#include <pebble.h>

#include "pebble_worldmap.h"

/* Globals */

// Window object
Window *g_window_settings;

// Selected option on settings screen
int g_selected_option = 0;

// Option 1 or 2 is being edited
int g_edit_option = 0;

// Externs
extern int g_draw_sunrise;
extern int g_home_pos[];
extern int home_latitude;
extern int home_longitude;
extern const float LATITUDE_TABLE[];

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

    graphics_draw_text(
            ctx,
            "Push <back> to exit",
            fonts_get_system_font(FONT_KEY_GOTHIC_14),
            rect,
            GTextOverflowModeWordWrap,
            GTextAlignmentLeft,
            NULL);
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

void init_settings() {
    // The "settings" window
    g_window_settings = window_create();

    // Attach our desired button functionality
    window_set_click_config_provider(g_window_settings, (ClickConfigProvider) settings_click_config_provider);

    layer_set_update_proc(
            window_get_root_layer(g_window_settings),
            &settings_layer_update_callback);
}

void show_settings_window() {
    g_selected_option = 0;
    window_stack_push(g_window_settings, 1);
}
