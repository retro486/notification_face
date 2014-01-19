#include <pebble.h>

Window *my_window;
Layer *window_layer;
TextLayer *notification_layer;
TextLayer *clock_layer;
TextLayer *date_layer;
InverterLayer *invert_layer;

static char notification_buffer[255]; // long notification
static char clock_buffer[] = "00:00 XX"; // 11:26 am
static char date_buffer[] = "XXX, XXX 00"; // Wed, May 16

void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	// outgoing message failed
}

void in_received_handler(DictionaryIterator *received, void *context) {
  	// incoming message received
  	vibes_short_pulse();
  	light_enable_interaction();
	Tuple *notification = dict_find(received, 1);
	strncpy(notification_buffer, notification->value->cstring, sizeof(notification_buffer));
	text_layer_set_text(notification_layer, notification_buffer);
}


void in_dropped_handler(AppMessageResult reason, void *context) {
  // incoming message dropped
}

void line_layer_update_callback(Layer *me, GContext *ctx) {
	GRect bounds = layer_get_bounds(me);
	uint32_t pad = 5;
	uint32_t half_screen = bounds.size.h / 2;
	uint32_t quarter_screen = half_screen / 2;
	
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_draw_line(ctx, GPoint(pad, half_screen + pad), GPoint(bounds.size.w - pad, half_screen + pad));
}

void handle_minute_tick(struct tm* now, TimeUnits units_changed) {
	strftime(date_buffer, sizeof(date_buffer), "%a, %b %e", now);
	strftime(clock_buffer, sizeof(clock_buffer), "%I:%M %p", now);
	text_layer_set_text(date_layer, date_buffer);
	text_layer_set_text(clock_layer, clock_buffer);
}

void handle_init(void) {
	for(uint32_t i=0; i<sizeof(notification_buffer); i++) {
		notification_buffer[i] = '\0';
	}
	
	time_t n = time(NULL);
	struct tm* now = localtime(&n);
	strftime(date_buffer, sizeof(date_buffer), "%a, %b %e", now);
	strftime(clock_buffer, sizeof(clock_buffer), "%I:%M %p", now);
	
  	my_window = window_create();
 	window_stack_push(my_window, true /* Animated */);
	window_layer = window_get_root_layer(my_window);
	
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
	
	const uint32_t inbound_size = 64;
   	const uint32_t outbound_size = 64;
	app_message_open(inbound_size, outbound_size);
	
	GRect bounds = layer_get_bounds(window_layer);
	static const int pad = 5;
	
	// 5px padding all around, notifications take up the top half of the screen
	uint32_t half_screen = bounds.size.h / 2;
	uint32_t quarter_screen = half_screen / 2;
	
	notification_layer = text_layer_create((GRect) { { pad, pad }, { bounds.size.w - pad, half_screen - pad + 10 } });
	text_layer_set_font(notification_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(notification_layer));
	
	// followed by smallish date
	date_layer = text_layer_create((GRect) { {pad, half_screen + 10}, { bounds.size.w - pad, quarter_screen - pad} });
	text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	layer_add_child(window_layer, text_layer_get_layer(date_layer));
	text_layer_set_text(date_layer, date_buffer);
	
	// then largish time
	clock_layer = text_layer_create((GRect) { {pad, half_screen + quarter_screen}, {bounds.size.w - pad, quarter_screen - pad} });
	text_layer_set_font(clock_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(clock_layer));
	text_layer_set_text(clock_layer, clock_buffer);

	// setup the layer for the line separator
	Layer *line_layer = layer_create(bounds);
	layer_set_update_proc(line_layer, line_layer_update_callback);
	layer_add_child(window_layer, line_layer);
	
	// register callback to update date and time every minute
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
	
	invert_layer = inverter_layer_create(bounds);
	layer_add_child(window_layer, (Layer *)invert_layer);
}

void handle_deinit(void) {
	text_layer_destroy(notification_layer);
	text_layer_destroy(clock_layer);
	text_layer_destroy(date_layer);
	layer_destroy(window_layer);
	inverter_layer_destroy(invert_layer);
	window_destroy(my_window);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}