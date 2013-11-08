//// #include "pebble_os.h"
//// #include "pebble_app.h"
//// #include "pebble_fonts.h"
//// #include "app_info.h"
#include <pebble.h>
#include "app_options.h"

//// #define MY_UUID { 0x77, 0x1C, 0xEE, 0xD5, 0x5E, 0xB9, 0x47, 0x36, 0x93, 0xC8, 0x2A, 0x0C, 0xE4, 0x0E, 0xFA, 0x0A }

//// PBL_APP_INFO(MY_UUID, APP_NAME, APP_AUTHOR,
////             APP_VER_MAJOR, APP_VER_MINOR,
////             RESOURCE_ID_IMAGE_MENU_ICON,
////             APP_INFO_WATCH_FACE
////	     );

Window *window;
static bool is_animating;
static ResHandle res_h[6];
static GFont fonts[6];   

#define IMAGE_WIDTH 80
#define IMAGE_HEIGHT 80
#define IMAGE_POS_NORMAL 0
#define IMAGE_POS_DRAW   1
#define IMAGE_POS_POINT  2
#define IMAGE_POS_AIM    3
#define IMAGE_POS_SHOOT  4
#define IMAGE_POS_LOWER  5
#define IMAGE_POS_DONE   6
	
static InverterLayer inverter;

typedef struct
{
	int image_index;
	uint32_t show_interval;
} animation_details;
 
#define IMAGE_COUNT 7
animation_details animation[IMAGE_COUNT] = 
{ 
	{	.image_index = RESOURCE_ID_IMAGE_MARVIN01,	.show_interval = 50	  },
	{	.image_index = RESOURCE_ID_IMAGE_MARVIN02,	.show_interval = 50	  },
	{	.image_index = RESOURCE_ID_IMAGE_MARVIN03,	.show_interval = 50	  },
	{	.image_index = RESOURCE_ID_IMAGE_MARVIN04,	.show_interval = 500  },
	{	.image_index = RESOURCE_ID_IMAGE_MARVIN04,	.show_interval = 1000 },
	{	.image_index = RESOURCE_ID_IMAGE_MARVIN03,	.show_interval = 50   },
	{	.image_index = RESOURCE_ID_IMAGE_MARVIN02,	.show_interval = 50   }
};


static BitmapLayer *me;
//// static AppContextRef appctx;

#define BOLT_ANIMATION_DURATION 1500
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
	
typedef struct
{
	GRect frame;
	uint32_t duration;
} animation_frame;

animation_frame send_frames[FRAME_COUNT];
static BitmapLayer *bolt;
static BitmapLayer *flag;
static BitmapLayer *planet;
static BitmapLayer *earth;
static PropertyAnimation send_animation[FRAME_COUNT];
static PropertyAnimation explode_animation;

#define SHRINK_FONT01 90
#define SHRINK_FONT02 91
#define SHRINK_FONT03 92
#define SHRINK_FONT04 93
#define SHRINK_FONT05 94
#define EXPAND_FONT01 95
#define EXPAND_FONT02 96
#define EXPAND_FONT03 97
#define EXPAND_FONT04 98
#define RESTORE_FONT  99

#define TIME_FRAME_PADDING 5
#define TIME_FRAME_Y 5 + TIME_FRAME_PADDING //61 (44 + 17) is the lowest possible point of the moving bolt animation and the padding for the top side
#define TIME_FRAME_WIDTH SCREEN_WIDTH - (IMAGE_WIDTH / 2) //the padding for the right/left side
#define TIME_FRAME_HEIGHT 30
static TextLayer *time_text;
static TextLayer *date_text;

void init_frames();
void setup_animation();
void setup_screen();
void setup_me(int current_position);
void setup_bolt(bool send, bool explode);
void setup_time();
void setup_date();
void setup_background();
void animate_bolt(bool send);
void animate_explode();
void animate_text();
void set_time(struct tm *t);
void set_date(struct tm *t);
static void handle_timer(void *data);

void clear_background()
{
//	layer_remove_from_parent(flag);
//	layer_remove_from_parent(planet->layer);
//	layer_remove_from_parent(earth->layer);
	bitmap_layer_destroy(flag);
	bitmap_layer_destroy(planet);
	bitmap_layer_destroy(earth);
}

void clear_me()
{
//	layer_remove_from_parent((Layer *) me.layer);
	bitmap_layer_destroy(me);
}

void clear_bolt()
{
//	layer_remove_from_parent((Layer *) bolt.layer);
	bitmap_layer_destroy(bolt);
}

void clear_time()
{
//	layer_remove_from_parent((Layer *) time_text);
}

void clear_date()
{
//	layer_remove_from_parent((Layer *) date_text);
}

void clear_screen()
{
	clear_time();
	clear_date();
	clear_me();
	clear_bolt();
	clear_background();
}

void animate_send()
{
	if(is_animating) return;
	else is_animating = true;

	app_timer_register(animation[IMAGE_POS_NORMAL].show_interval, handle_timer, (void *) IMAGE_POS_DRAW);
}

void send_animation_stopped(Animation *animation, void *data)
{
	(void) animation;
	(void) data;

	clear_bolt();
	setup_bolt(true, true);
	animate_explode();
}

void explode_animation_stopped(Animation *animation, void *data)
{
	(void)animation;
	(void)data;

	is_animating = false;

	clear_me();
	clear_bolt();
	setup_me(IMAGE_POS_NORMAL);
	animate_text();
}

void animate_explode()
{
	property_animation_init_layer_frame(&explode_animation, (Layer *) &bolt.layer, &LASTFRAME, &LASTFRAME);

	animation_set_duration(&explode_animation.animation, 1000); // animation[IMAGE_POS_RECEIVED].show_interval);
	animation_set_curve(&explode_animation.animation, AnimationCurveEaseInOut);

	animation_set_handlers(&explode_animation.animation,
						   (AnimationHandlers)
						   {
							   .stopped = (AnimationStoppedHandler)explode_animation_stopped
						   },
						   NULL);
	animation_schedule(&explode_animation.animation);
}

void animate_bolt(bool send)
{
	for(int x = 0; x < FRAME_COUNT - 1; x++)
	{
		animation_schedule(&send_animation[x].animation);
	}
}

void animate_text()
{
	is_animating = true;
	app_timer_register(100, &handle_timer, (void *) SHRINK_FONT01);
}

void init_frames()
{
	//send animation starts at IMAGE_POS_SHOOT
	//so total send animation is minus the IMAGE_POS_NORMAL, IMAGE_POS_DRAW and IMAGE_POS_POINT durations
	//118.0 is the total of the duration of the send animation_frame 
	double duration = (BOLT_ANIMATION_DURATION - animation[IMAGE_POS_NORMAL].show_interval - animation[IMAGE_POS_DRAW].show_interval - animation[IMAGE_POS_POINT].show_interval) / 118.0;

	//the constants below are the corresponding lengths of the frames
	//this is to ensure that the speed of the animation is consistent
	memcpy(send_frames,
			(animation_frame[FRAME_COUNT])
			{
				{ 	.frame = FRAME01,	.duration = 22 * duration	},
				{	.frame = FRAME02,	.duration = 22 * duration	},
				{	.frame = FRAME03,	.duration = 22 * duration	},
				{	.frame = FRAME04,	.duration = 22 * duration	},
				{	.frame = FRAME05,	.duration = 22 * duration	},
				{	.frame = FRAME06,	.duration = 22 * duration	},
				{	.frame = FRAME07,	.duration = 22 * duration	},
				{	.frame = FRAME08,	.duration = 22 * duration	},
				{	.frame = FRAME09,	.duration = 22 * duration	},
				{	.frame = FRAME10,	.duration = 22 * duration	},
				{	.frame = FRAME11,	.duration = 22 * duration	},
				{	.frame = FRAME12,	.duration = 22 * duration	}
			},
			sizeof send_frames);

}

void setup_inverter()
{
	inverter_layer_init(&inverter, GRect(0, 0, SCREEN_WIDTH, (INVERT_COLOUR ? SCREEN_HEIGHT : 0)));
	layer_add_child(&window.layer, &inverter.layer);
}

void setup_animation()
{

	int total_send_delay = 0;

	//since the receive animation is supposed to be the continuation of the send animation of the other watch, 
	//start the delay at BOLT_ANIMATION_DURATION 
	//but if the splash screen is showing, then the delay is not needed 

	for(int x = 0; x < FRAME_COUNT - 1; x++) //-1 because animate_bolt looks at the current frame and the next frame in the array
	{
		property_animation_init_layer_frame(&send_animation[x], (Layer *) &bolt.layer, &send_frames[x].frame, &send_frames[x + 1].frame);
		animation_set_duration(&send_animation[x].animation, send_frames[x].duration);
		animation_set_delay(&send_animation[x].animation, total_send_delay);
		total_send_delay += send_frames[x].duration;

		if(x == FRAME_COUNT - 2) //-2 because that is the last item when the condition to break is < X - 1
		{
			animation_set_handlers(&send_animation[x].animation,
								   (AnimationHandlers)
								   {
									   .stopped = (AnimationStoppedHandler)send_animation_stopped
								   },
								   NULL);
		}

		animation_set_curve(&send_animation[x].animation, AnimationCurveLinear);

	}
}

void setup_bolt(bool send, bool explode)
{
	if(explode) 
	{
		bitmap_layer_create
		bmp_init_container(RESOURCE_ID_IMAGE_EXPLODE, &bolt);
		layer_set_frame((Layer *) &bolt.layer, LASTFRAME);
	}
	else
	{
		bmp_init_container(RESOURCE_ID_IMAGE_BOLT, &bolt);
		layer_set_frame((Layer *) &bolt.layer, send_frames[0].frame);
	}

	bitmap_layer_set_compositing_mode(&bolt.layer, GCompOpAnd);
	layer_insert_below_sibling((Layer *) &bolt.layer, (Layer *) &inverter);
}

void setup_me(int current_position)
{
	if(current_position >= IMAGE_COUNT) current_position = IMAGE_POS_NORMAL;

	bmp_init_container(animation[current_position].image_index, &me);
	bitmap_layer_set_compositing_mode(&me.layer, GCompOpAnd);
	layer_set_frame((Layer *) &me.layer, GRect(-5, 56, IMAGE_WIDTH, IMAGE_HEIGHT));
	layer_insert_below_sibling((Layer *) &me.layer, (Layer *) &planet.layer); //&inverter);
}

void setup_time()
{
	text_layer_init(&time_text, GRect((IMAGE_WIDTH / 2), TIME_FRAME_Y, TIME_FRAME_WIDTH, TIME_FRAME_HEIGHT));
	
	text_layer_set_font(&time_text, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HANDSEAN_18)));
	text_layer_set_text_alignment(&time_text, GTextAlignmentCenter);
	layer_insert_below_sibling((Layer *) &time_text, (Layer *) &inverter);
}

void setup_date()
{
	text_layer_init(&date_text, GRect((IMAGE_WIDTH / 2), TIME_FRAME_Y + TIME_FRAME_HEIGHT, TIME_FRAME_WIDTH, TIME_FRAME_HEIGHT));
	
	text_layer_set_font(&date_text, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HANDSEAN_18)));
	text_layer_set_text_alignment(&date_text, GTextAlignmentCenter);
	layer_insert_below_sibling((Layer *) &date_text, (Layer *) &inverter);
}

void setup_background()
{
	bmp_init_container(RESOURCE_ID_IMAGE_FLAG, &flag);
	bmp_init_container(RESOURCE_ID_IMAGE_PLANET, &planet);
	bmp_init_container(RESOURCE_ID_IMAGE_EARTH, &earth);

	layer_set_frame((Layer *) &flag.layer, GRect(75, (SCREEN_HEIGHT - 90), 40, 60));
	layer_set_frame((Layer *) &planet.layer, GRect(0, (SCREEN_HEIGHT - 40), SCREEN_WIDTH, 40));
	layer_set_frame((Layer *) &earth.layer, GRect(4, 4, 32, 32));

	layer_insert_below_sibling((Layer *) &flag.layer, (Layer *) &inverter);
	layer_insert_below_sibling((Layer *) &planet.layer, (Layer *) &inverter);
	layer_insert_below_sibling((Layer *) &earth.layer, (Layer *) &inverter);
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

static void skip_splash()
{
	clear_screen();
	setup_fonts();
	setup_inverter();
	init_frames();	
	setup_animation();
	setup_me(IMAGE_POS_NORMAL);
	
	is_animating = false;
	clear_me();
	clear_bolt();
	setup_background();
	setup_time();
	setup_date();

////	struct tm *current;
////	localtime(time(NULL)); // (current);
	set_time(localtime(time(NULL)));
	set_date(localtime(time(NULL)));
	setup_me(IMAGE_POS_NORMAL);
	
	setup_animation();
}

static void handle_timer(void *data)
{
	if(is_animating == false) return;

	clear_me();
	int cookie = *data;
	setup_me(cookie);

	static uint32_t new_position;
	if(cookie == (uint32_t) IMAGE_POS_NORMAL) 
	{
		is_animating = false;
		new_position = IMAGE_POS_NORMAL;
		return;
	}
	else if(cookie == (uint32_t) IMAGE_POS_DRAW) 
	{
		new_position = cookie + 1;
	}
	else if(cookie == (uint32_t) IMAGE_POS_POINT) 
	{
		new_position = cookie + 1;
	}
	else if(cookie == (uint32_t) IMAGE_POS_AIM) 
	{
		new_position = cookie + 1;
	}
	else if(cookie == (uint32_t) IMAGE_POS_SHOOT) 
	{
		new_position = cookie + 1;
		clear_bolt();
		setup_bolt(true, false);
		animate_bolt(true);
	}
	else if(cookie == (uint32_t) IMAGE_POS_LOWER) 
	{
		new_position = cookie + 1;
	}
	else if(cookie == (uint32_t) IMAGE_POS_DONE) 
	{
		new_position = IMAGE_POS_NORMAL;
	}
	else if (cookie == (uint32_t) SHRINK_FONT01) 
	{
 		text_layer_set_font(&time_text, fonts[4]);
		text_layer_set_font(&date_text, fonts[4]);
 	    //// text_layer_set_text(&date_text, "SH01");	//// debugging
		app_timer_send_event(50, &handle_timer, (void *) SHRINK_FONT02);
		return;
	}
	else if (cookie == (uint32_t) SHRINK_FONT02) 
	{
 		text_layer_set_font(&time_text, fonts[3]);
		text_layer_set_font(&date_text, fonts[3]);
 	    //// text_layer_set_text(&date_text, "SH02");
 	    app_timer_send_event(ctx, 50, SHRINK_FONT03);
		return;
	}
	else if (cookie == (uint32_t) SHRINK_FONT03) 
	{
 		text_layer_set_font(&time_text, fonts[2]);
		text_layer_set_font(&date_text, fonts[2]);
 	    //// text_layer_set_text(&date_text, "SH03");
 	    app_timer_send_event(ctx, 50, SHRINK_FONT04);
		return;
	}
	else if (cookie == (uint32_t) SHRINK_FONT04) 
	{
 		text_layer_set_font(&time_text, fonts[1]);
		text_layer_set_font(&date_text, fonts[1]);
 	    //// text_layer_set_text(&date_text, "SH04");
 	    app_timer_send_event(ctx, 50, SHRINK_FONT05);
		return;
	}
	else if (cookie == (uint32_t) SHRINK_FONT05) 
	{
 		text_layer_set_font(&time_text, fonts[0]);
		text_layer_set_font(&date_text, fonts[0]);
 	    //// text_layer_set_text(&date_text, "SH05");
 	    app_timer_send_event(ctx, 1000, EXPAND_FONT01);
		return;
	}
	else if (cookie == (uint32_t) EXPAND_FONT01) 
	{
 		text_layer_set_font(&time_text, fonts[1]);
		text_layer_set_font(&date_text, fonts[1]);
 	    //// text_layer_set_text(&date_text, "EX01");
 	    app_timer_send_event(ctx, 50, EXPAND_FONT02);
		return;
	}
	else if (cookie == (uint32_t) EXPAND_FONT02) 
	{
 		text_layer_set_font(&time_text, fonts[2]);
		text_layer_set_font(&date_text, fonts[2]);
 	    //// text_layer_set_text(&date_text, "EX02");
 	    app_timer_send_event(ctx, 50, EXPAND_FONT03);
		return;
	}
	else if (cookie == (uint32_t) EXPAND_FONT03) 
	{
 		text_layer_set_font(&time_text, fonts[3]);
		text_layer_set_font(&date_text, fonts[3]);
 	    //// text_layer_set_text(&date_text, "EX03");
 	    app_timer_send_event(ctx, 50, EXPAND_FONT04);
		return;
	}
	else if (cookie == (uint32_t) EXPAND_FONT04) 
	{
 		text_layer_set_font(&time_text, fonts[4]);
		text_layer_set_font(&date_text, fonts[4]);
 	    //// text_layer_set_text(&date_text, "EX04");
 	    app_timer_send_event(ctx, 50, RESTORE_FONT);
		return;
	}
	else if (cookie == (uint32_t) RESTORE_FONT) 
	{
 		text_layer_set_font(&time_text, fonts[5]);
		text_layer_set_font(&date_text, fonts[5]);
 	    //// text_layer_set_text(&date_text, "REST");
		is_animating = false;
		return;
	}
	else 
	{
		is_animating = false;
		new_position = IMAGE_POS_NORMAL;
		return;
	}

	app_timer_send_event(ctx, animation[cookie].show_interval, new_position);
}

void set_time(struct tm *t)
{
	static char hourText[] = "04:44pm"; 	//this is the longest possible text based on the font used
	if(clock_is_24h_style())
		strftime(hourText, sizeof(hourText), "%H:%M", t);
	else
		strftime(hourText, sizeof(hourText), "%I:%M", t);
	if (hourText[0] == '0') { hourText[0] = ' '; }
	if (t->tm_hour < 12) strcat(hourText, "am"); else strcat(hourText, "pm");

	text_layer_set_text(&time_text, hourText);
}

void set_date(struct tm *t)
{
	static char dateText[] = "XXX 00/00"; 
    strftime(dateText, sizeof(dateText), "%a %m/%d", t);
	text_layer_set_text(&date_text, dateText);
}

static void handle_second_tick(struct tm *t, TimeUnits units_changed)
// void handle_second_tick(PebbleTickEvent *t)
{
//	(void)ctx;

	int seconds = t->tm_sec;
	int minutes = t->tm_min;
////	int hours = t->tick_time->tm_hour;

	if(seconds == 0)
	{
		set_time(t);	
		set_date(t);
	}

	bool sender = false;
////	if ((seconds % 10) == 0) sender = true;
	
	if(seconds == 54)    //// try to catch min change during shrink/expand animation
	{
		sender = true;
		int send_interval = 1;   //// Interval of animation - e.g. 5 min;
		sender = (minutes % send_interval == 0);
	}
    
	if(sender)
	{
		animate_send();
	}

	
}

void handle_init(void)
{
	window = window_create();
	tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
	
//	window_init(window, "Main");
//	#ifdef DEBUG
//		window_set_fullscreen(&window, true);
//		window_set_click_config_provider(&window, (ClickConfigProvider) config_provider);
//	#endif

//	window_stack_push(&window, true);

//	resource_init_current_app(&APP_RESOURCES);

	srand(time(NULL));
	is_animating = false;

//	appctx = ctx;
	skip_splash();
}

void handle_deinit(void) 
{
////	clear_screen();
}

int main(void)
{
  	handle_init();
	app_event_loop();
	handle_deinit();
}