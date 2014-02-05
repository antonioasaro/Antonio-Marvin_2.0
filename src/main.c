#include <pebble.h>
#include "app_options.h"


//// defines 
#define IMAGE_COUNT 7
#define IMAGE_WIDTH 80
#define IMAGE_HEIGHT 80
#define IMAGE_POS_NORMAL 0
#define IMAGE_POS_DRAW   1
#define IMAGE_POS_POINT  2
#define IMAGE_POS_AIM    3
#define IMAGE_POS_SHOOT  4
#define IMAGE_POS_LOWER  5
#define IMAGE_POS_DONE   6
	
#define FRAME_COUNT 12
#define FRAME01 GRect(62,  68,  17, 17)     // anchor
#define FRAME02 GRect(80,  65,  17, 17)
#define FRAME03 GRect(100, 63,  17, 17)
#define FRAME04 GRect(110, 80,  17, 17)
#define FRAME05 GRect(112, 100, 17, 17)
#define FRAME06 GRect(99,  108, 17, 17)
#define FRAME07 GRect(80,  110, 17, 17)
#define FRAME08 GRect(60,  107, 17, 17)
#define FRAME09 GRect(50,  95,  17, 17)
#define FRAME10 GRect(51,  70,  17, 17)  
#define FRAME11 GRect(75,  55,  17, 17) 
#define FRAME12 GRect(80,  28,  17, 17)     // anchor
#define LASTFRAME FRAME12
#define LASTINDEX 12-1
#define BOLT_ANIMATION_DURATION 1500
	
#define SHRINK_FONT01 90
#define SHRINK_FONT02 91
#define SHRINK_FONT03 92
#define SHRINK_FONT04 93
#define SHRINK_FONT05 94
#define EXPAND_FONT01 95
#define EXPAND_FONT02 96
#define EXPAND_FONT03 97
#define EXPAND_FONT04 98
#define NORMAL_FONT   99

#define TIME_FRAME_PADDING 5
#define TIME_FRAME_Y 5 + TIME_FRAME_PADDING //61 (44 + 17) is the lowest possible point of the moving bolt animation and the padding for the top side
#define TIME_FRAME_WIDTH SCREEN_WIDTH - (IMAGE_WIDTH / 2) //the padding for the right/left side
#define TIME_FRAME_HEIGHT 30	

// variables
Window *window;
Layer *window_layer;
GRect window_bounds;
static AppTimer *timer;

static bool is_animating;
static BitmapLayer *marvin;
static BitmapLayer *bolt;
static BitmapLayer *explosion;
static BitmapLayer *earth;
static BitmapLayer *flag;
static BitmapLayer *mars;
static GBitmap *marvin01_image;
static GBitmap *marvin02_image;
static GBitmap *marvin03_image;
static GBitmap *marvin04_image;
static GBitmap *bolt_image;
static GBitmap *explosion_image;
static GBitmap *earth_image;
static GBitmap *flag_image;
static GBitmap *mars_image;
static PropertyAnimation *bolt_animation[FRAME_COUNT];
static PropertyAnimation *explosion_animation;
static TextLayer *time_text;
static TextLayer *date_text;
static GFont fonts[6];   
static ResHandle res_h[6];

typedef struct
{
	GRect frame;
	uint32_t duration;
} bolt_frame;
bolt_frame bolt_frames[FRAME_COUNT];

typedef struct
{
	GBitmap *image;
	int duration;
}
marvin_frame;
marvin_frame marvin_frames[IMAGE_COUNT];


//// prototypes
static void handle_timer(void *data);
static void bolt_animation_started(Animation *animation, void *data);
static void bolt_animation_stopped(Animation *animation, bool finished, void *data);
static void explosion_animation_started(Animation *animation, void *data);
static void explosion_animation_stopped(Animation *animation, bool finished, void *data);


//// clear functions
void clear_gbitmap()
{
	gbitmap_destroy(marvin01_image);
	gbitmap_destroy(marvin02_image);
	gbitmap_destroy(marvin03_image);
	gbitmap_destroy(marvin04_image);
	gbitmap_destroy(bolt_image);
	gbitmap_destroy(explosion_image);
	gbitmap_destroy(earth_image);
	gbitmap_destroy(flag_image);
	gbitmap_destroy(mars_image); 
}

void clear_marvin()
{
	bitmap_layer_destroy(marvin);
}

void clear_time()
{
	text_layer_destroy(time_text);
}

void clear_date()
{
	text_layer_destroy(date_text);
}

void clear_bolt()
{
	bitmap_layer_destroy(bolt);
}

void clear_explosion()
{
	bitmap_layer_destroy(explosion);
}

void clear_background()
{
	bitmap_layer_destroy(earth);
	bitmap_layer_destroy(flag);
	bitmap_layer_destroy(mars);
}

void clear_animations() {
    for (int i=0; i<FRAME_COUNT; i++) property_animation_destroy(bolt_animation[i]);
    property_animation_destroy(explosion_animation);
}

void clear_fonts() {
    for (int i=0; i<6; i++) fonts_unload_custom_font(fonts[i]);
}

void clear_all()
{
	clear_fonts();
	clear_gbitmap();
	clear_marvin();
	clear_time();
	clear_date();
	clear_bolt();
	clear_explosion();
	clear_animations();
	clear_background();
}

//// setup functions
void setup_gbitmap()
{
	marvin01_image  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARVIN01);
	marvin02_image  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARVIN02);
	marvin03_image  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARVIN03);
	marvin04_image  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARVIN04);
	bolt_image      = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BOLT);
	explosion_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_EXPLOSION);
	earth_image     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_EARTH);
	flag_image      = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FLAG);
	mars_image      = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARS);
}

void setup_bolt()
{
	bolt = bitmap_layer_create(FRAME01);
	bitmap_layer_set_bitmap(bolt, bolt_image);
	bitmap_layer_set_compositing_mode(bolt, GCompOpAnd);
	layer_add_child(window_get_root_layer(window),  bitmap_layer_get_layer(bolt));
	layer_set_hidden(bitmap_layer_get_layer(bolt), true);	
}

void setup_explosion()
{
	explosion = bitmap_layer_create(LASTFRAME);
	bitmap_layer_set_bitmap(explosion, explosion_image);
	bitmap_layer_set_compositing_mode(explosion, GCompOpAnd);
	layer_add_child(window_get_root_layer(window),  bitmap_layer_get_layer(explosion));
	layer_set_hidden(bitmap_layer_get_layer(explosion), true);	
}

void setup_marvin()
{
	marvin = bitmap_layer_create(GRect(-5, 56, IMAGE_WIDTH, IMAGE_HEIGHT));
	bitmap_layer_set_bitmap(marvin, marvin01_image);
	bitmap_layer_set_compositing_mode(marvin, GCompOpAnd);
	layer_add_child(window_get_root_layer(window),  bitmap_layer_get_layer(marvin));
	
	marvin_frames[IMAGE_POS_NORMAL].duration = 50;
	marvin_frames[IMAGE_POS_DRAW].duration 	 = 50;
	marvin_frames[IMAGE_POS_POINT].duration  = 50;
	marvin_frames[IMAGE_POS_AIM].duration 	 = 500;
	marvin_frames[IMAGE_POS_SHOOT].duration  = 1000;
	marvin_frames[IMAGE_POS_LOWER].duration  = 50;
	marvin_frames[IMAGE_POS_DONE].duration 	 = 50;
	marvin_frames[IMAGE_POS_NORMAL].image 	 = marvin01_image; 
	marvin_frames[IMAGE_POS_DRAW].image 	 = marvin02_image; 
	marvin_frames[IMAGE_POS_POINT].image	 = marvin03_image; 
	marvin_frames[IMAGE_POS_AIM].image 	 	 = marvin04_image; 
	marvin_frames[IMAGE_POS_SHOOT].image 	 = marvin04_image; 
	marvin_frames[IMAGE_POS_LOWER].image 	 = marvin03_image; 
	marvin_frames[IMAGE_POS_DONE].image 	 = marvin02_image; 
}

void setup_time()
{
  	time_text = text_layer_create(GRect((IMAGE_WIDTH / 2), TIME_FRAME_Y, TIME_FRAME_WIDTH, TIME_FRAME_HEIGHT));
	text_layer_set_text_color(time_text, GColorBlack);
  	text_layer_set_background_color(time_text, GColorClear);
	text_layer_set_text_alignment(time_text, GTextAlignmentCenter);
	text_layer_set_font(time_text, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HANDSEAN_18)));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_text));
}

void setup_date()
{
  	date_text = text_layer_create(GRect((IMAGE_WIDTH / 2)-10, TIME_FRAME_Y + TIME_FRAME_HEIGHT, TIME_FRAME_WIDTH+20, TIME_FRAME_HEIGHT));
  	text_layer_set_text_color(date_text, GColorBlack);
  	text_layer_set_background_color(date_text, GColorClear);
	text_layer_set_text_alignment(date_text, GTextAlignmentCenter);
 	text_layer_set_font(date_text, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HANDSEAN_18)));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_text));
}

void setup_background()
{
	earth = bitmap_layer_create(GRect(4, 4, 32, 32));
	bitmap_layer_set_bitmap(earth, earth_image);
	layer_add_child(window_get_root_layer(window),  bitmap_layer_get_layer(earth));
	layer_set_hidden(bitmap_layer_get_layer(earth), true);	

	flag = bitmap_layer_create(GRect(75, (SCREEN_HEIGHT - 90), 40, 60));
	bitmap_layer_set_bitmap(flag, flag_image);
	layer_add_child(window_get_root_layer(window),  bitmap_layer_get_layer(flag));

	mars = bitmap_layer_create(GRect(0, (SCREEN_HEIGHT - 40), SCREEN_WIDTH, 40));
	bitmap_layer_set_bitmap(mars, mars_image);
	layer_add_child(window_get_root_layer(window),  bitmap_layer_get_layer(mars));
}

void setup_fonts()
{
	res_h[0] = resource_get_handle(RESOURCE_ID_FONT_HANDSEAN_8);
	res_h[1] = resource_get_handle(RESOURCE_ID_FONT_HANDSEAN_10);
	res_h[2] = resource_get_handle(RESOURCE_ID_FONT_HANDSEAN_12);
	res_h[3] = resource_get_handle(RESOURCE_ID_FONT_HANDSEAN_14);
	res_h[4] = resource_get_handle(RESOURCE_ID_FONT_HANDSEAN_16);
	res_h[5] = resource_get_handle(RESOURCE_ID_FONT_HANDSEAN_18);
    fonts[0] = fonts_load_custom_font(res_h[0]);
    fonts[1] = fonts_load_custom_font(res_h[1]);
    fonts[2] = fonts_load_custom_font(res_h[2]);
    fonts[3] = fonts_load_custom_font(res_h[3]);
    fonts[4] = fonts_load_custom_font(res_h[4]);
    fonts[5] = fonts_load_custom_font(res_h[5]);
}

void setup_bolt_frames()
{
	double duration = 10;
	bolt_frames[0].frame  = FRAME01; bolt_frames[0].duration  = 25 * duration;
	bolt_frames[1].frame  = FRAME02; bolt_frames[1].duration  = 25 * duration;
	bolt_frames[2].frame  = FRAME03; bolt_frames[2].duration  = 25 * duration;
	bolt_frames[3].frame  = FRAME04; bolt_frames[3].duration  = 25 * duration;
	bolt_frames[4].frame  = FRAME05; bolt_frames[4].duration  = 25 * duration;
	bolt_frames[5].frame  = FRAME06; bolt_frames[5].duration  = 25 * duration;
	bolt_frames[6].frame  = FRAME07; bolt_frames[6].duration  = 25 * duration;
	bolt_frames[7].frame  = FRAME08; bolt_frames[7].duration  = 25 * duration;
	bolt_frames[8].frame  = FRAME09; bolt_frames[8].duration  = 25 * duration;
	bolt_frames[9].frame  = FRAME10; bolt_frames[9].duration  = 25 * duration;
	bolt_frames[10].frame = FRAME11; bolt_frames[10].duration = 25 * duration;
	bolt_frames[11].frame = FRAME12; bolt_frames[11].duration = 25 * duration;
}

void setup_bolt_animation()
{
	int total_send_delay = 0;
	for(int x = 0; x < FRAME_COUNT - 1; x++) //-1 because animate_bolt looks at the current frame and the next frame in the array
	{
		bolt_animation[x] = property_animation_create_layer_frame(bitmap_layer_get_layer(bolt), &bolt_frames[x].frame, &bolt_frames[x + 1].frame);
		animation_set_duration((Animation*) bolt_animation[x], bolt_frames[x].duration);
		animation_set_delay((Animation*) bolt_animation[x], total_send_delay);
		animation_set_curve((Animation*) bolt_animation[x], AnimationCurveLinear);
		total_send_delay += bolt_frames[x].duration;

		if(x == FRAME_COUNT - 2) //-2 because that is the last item when the condition to break is < X - 1
		{
			animation_set_handlers((Animation*) bolt_animation[x], (AnimationHandlers) {
    			.started = (AnimationStartedHandler) bolt_animation_started,
    			.stopped = (AnimationStoppedHandler) bolt_animation_stopped,
            }, NULL /* callback data */);
		}

	}
}

void setup_explosion_animation()
{
	explosion_animation = property_animation_create_layer_frame(bitmap_layer_get_layer(explosion), NULL, &bolt_frames[LASTINDEX].frame);
	animation_set_duration((Animation*) explosion_animation, 1000);
	animation_set_delay((Animation*) explosion_animation, 0);
	animation_set_curve((Animation*) explosion_animation, AnimationCurveLinear);
	animation_set_handlers((Animation*) explosion_animation, (AnimationHandlers) {
 		.started = (AnimationStartedHandler) explosion_animation_started,
 		.stopped = (AnimationStoppedHandler) explosion_animation_stopped,
    }, NULL /* callback data */);
}

//// update functions
void update_time(struct tm *t)
{
	static char hourText[] = "04:44pm"; 	//this is the longest possible text based on the font used
	if(clock_is_24h_style())
		strftime(hourText, sizeof(hourText), "%H:%M", t);
	else
		strftime(hourText, sizeof(hourText), "%I:%M", t);
	if (hourText[0] == '0') { hourText[0] = ' '; }
	if (t->tm_hour < 12) strcat(hourText, "am"); else strcat(hourText, "pm");

	text_layer_set_text(time_text, hourText);
}

void update_date(struct tm *t)
{
	static char dateText[] = "XXX 00/00"; 
    strftime(dateText, sizeof(dateText), "%a %m/%d", t);
	if (dateText[4] == '0') { strcpy(&dateText[4], &dateText[5]); }
	text_layer_set_text(date_text, dateText);
}

void update_marvin(int current_position)
{
	bitmap_layer_set_bitmap(marvin, marvin_frames[current_position].image);
}

//// animate functions
void animate_marvin()
{
	is_animating = true;
	timer = app_timer_register(marvin_frames[IMAGE_POS_NORMAL].duration, handle_timer, (int *) IMAGE_POS_DRAW);
}

void animate_font()
{
	is_animating = true;
	timer = app_timer_register(100, &handle_timer, (int *) SHRINK_FONT01);
}

void animate_bolt()
{
	layer_set_hidden(bitmap_layer_get_layer(bolt), false);	
	for(int x = 0; x < FRAME_COUNT - 1; x++)
	{
		animation_schedule((Animation*) bolt_animation[x]);
	}
}

void animate_explosion()
{
	layer_set_hidden(bitmap_layer_get_layer(explosion), false);	
	animation_schedule((Animation*) explosion_animation);
}

//// started + stopped functions
static void bolt_animation_started(Animation *animation, void *data)
{
}

static void bolt_animation_stopped(Animation *animation, bool finished, void *data)
{
	layer_set_hidden(bitmap_layer_get_layer(bolt), true);	
	animate_explosion();
}

static void explosion_animation_started(Animation *animation, void *data)
{
}

static void explosion_animation_stopped(Animation *animation, bool finished, void *data)
{
	layer_set_hidden(bitmap_layer_get_layer(explosion), true);	
	animate_font();
}

//// handle functions
static void handle_timer(void *data)
{
	uint32_t cookie = (uint32_t) data;
	static uint32_t new_position;

	if(is_animating == false) return;
	if(cookie == (uint32_t) IMAGE_POS_NORMAL) 
	{
		is_animating = false;
		update_marvin(cookie);
		new_position = IMAGE_POS_NORMAL;
		return;
	}
	else if(cookie == (uint32_t) IMAGE_POS_DRAW) 
	{
		update_marvin(cookie);
		new_position = cookie + 1;
		timer = app_timer_register(marvin_frames[cookie].duration, &handle_timer, (void *) new_position);
		return;
	}
	else if(cookie == (uint32_t) IMAGE_POS_POINT) 
	{
		update_marvin(cookie);
		new_position = cookie + 1;
		timer = app_timer_register(marvin_frames[cookie].duration, &handle_timer, (void *) new_position);
		return;
	}
	else if(cookie == (uint32_t) IMAGE_POS_AIM) 
	{
		update_marvin(cookie);
		new_position = cookie + 1;
		timer = app_timer_register(marvin_frames[cookie].duration, &handle_timer, (void *) new_position);
		return;
	}
	else if(cookie == (uint32_t) IMAGE_POS_SHOOT) 
	{
		update_marvin(cookie);
		new_position = cookie + 1;
		animate_bolt();
		timer = app_timer_register(marvin_frames[cookie].duration, &handle_timer, (void *) new_position);
		return;
	}
	else if(cookie == (uint32_t) IMAGE_POS_LOWER) 
	{
		update_marvin(cookie);
		new_position = cookie + 1;
		timer = app_timer_register(marvin_frames[cookie].duration, &handle_timer, (void *) new_position);
		return;
	}
	else if(cookie == (uint32_t) IMAGE_POS_DONE) 
	{
		update_marvin(cookie);
		new_position = IMAGE_POS_NORMAL;
		timer = app_timer_register(marvin_frames[cookie].duration, &handle_timer, (void *) new_position);
		return;
	}
	else if (cookie == (uint32_t) SHRINK_FONT01) 
	{
 		text_layer_set_font(time_text, fonts[4]);
		text_layer_set_font(date_text, fonts[4]);
		timer = app_timer_register(50, handle_timer, (int *) SHRINK_FONT02);
		return;
	}
	else if (cookie == (uint32_t) SHRINK_FONT02) 
	{
 		text_layer_set_font(time_text, fonts[3]);
		text_layer_set_font(date_text, fonts[3]);
 	    timer = app_timer_register(50, handle_timer, (int *) SHRINK_FONT03);
		return;
	}
	else if (cookie == (uint32_t) SHRINK_FONT03) 
	{
 		text_layer_set_font(time_text, fonts[2]);
		text_layer_set_font(date_text, fonts[2]);
 	    timer = app_timer_register(50, handle_timer, (int *) SHRINK_FONT04);
		return;
	}
	else if (cookie == (uint32_t) SHRINK_FONT04) 
	{
 		text_layer_set_font(time_text, fonts[1]);
		text_layer_set_font(date_text, fonts[1]);
 	    timer = app_timer_register(50, handle_timer, (int *) SHRINK_FONT05);
		return;
	}
	else if (cookie == (uint32_t) SHRINK_FONT05) 
	{
 		text_layer_set_font(time_text, fonts[0]);
		text_layer_set_font(date_text, fonts[0]);
 	    timer = app_timer_register(1000, handle_timer, (int *) EXPAND_FONT01);
		return;
	}
	else if (cookie == (uint32_t) EXPAND_FONT01) 
	{
 		text_layer_set_font(time_text, fonts[1]);
		text_layer_set_font(date_text, fonts[1]);
 	    app_timer_register(50, handle_timer, (int *) EXPAND_FONT02);
		return;
	}
	else if (cookie == (uint32_t) EXPAND_FONT02) 
	{
 		text_layer_set_font(time_text, fonts[2]);
		text_layer_set_font(date_text, fonts[2]);
 	    timer = app_timer_register(50, handle_timer, (int *) EXPAND_FONT03);
		return;
	}
	else if (cookie == (uint32_t) EXPAND_FONT03) 
	{
 		text_layer_set_font(time_text, fonts[3]);
		text_layer_set_font(date_text, fonts[3]);
 	    timer = app_timer_register(50, handle_timer, (int *) EXPAND_FONT04);
		return;
	}
	else if (cookie == (uint32_t) EXPAND_FONT04) 
	{
 		text_layer_set_font(time_text, fonts[4]);
		text_layer_set_font(date_text, fonts[4]);
 	    timer = app_timer_register(50, handle_timer, (int *) NORMAL_FONT);
		return;
	}
	else if (cookie == (uint32_t) NORMAL_FONT) 
	{
 		text_layer_set_font(time_text, fonts[5]);
		text_layer_set_font(date_text, fonts[5]);
		is_animating = false;
		return;
	}
}

static void handle_second_tick(struct tm *t, TimeUnits units_changed)
{
	int seconds = t->tm_sec;
	int minutes = t->tm_min;

	if(seconds == 0)
	{
		update_time(t);	
		update_date(t);
	}

	bool show = false;
	if(seconds == 54)    //// try to catch min change during shrink/expand animation
	{
		show = true;
		int show_interval = 1;   //// Interval of animation - e.g. 5 min;
		show = (minutes % show_interval == 0);
	}
    
	if(show)
	{
		animate_marvin();
	}
}

void handle_init(void)
{
	srand(time(NULL));

	window = window_create();
	window_layer = window_get_root_layer(window);
	window_bounds = layer_get_frame(window_layer);
	window_stack_push(window, true /* Animated */);
    tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
	
	setup_time();
	setup_date();
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	update_time(t);
	update_date(t);

	setup_gbitmap();
	setup_fonts();
	setup_background();
	setup_marvin();
	setup_bolt();
	setup_explosion();
	setup_bolt_frames();
	setup_bolt_animation();
	setup_explosion_animation();
}

void handle_deinit(void) 
{
	window_destroy(window);	
	layer_destroy(window_layer);
	clear_all();
}

//// main function
int main(void)
{
  	handle_init();
	app_event_loop();
	handle_deinit();
}
