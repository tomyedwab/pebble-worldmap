// Can be used to distinguish between multiple timers in your app
#define TIMER_ID_REFRESH 1

#define PERSIST_KEY_SHOW_HOME   1
#define PERSIST_KEY_LATITUDE    2
#define PERSIST_KEY_LONGITUDE   3

// pebble_worldmap.c
void handle_timer(void *data);

// settings.c
void init_settings();
void show_settings_window();
void update_home_pos();
