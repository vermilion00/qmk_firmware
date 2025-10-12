#define TRIGGER_HEIGHT {{ 1.0, 2.0 }, \
                        { 3.0, 0.5 }};

#define RAPID_TRIGGER  {{ 0.5, 0.5 }, \
                        { 0.2, 2.0 }};

#define ROWS 2
#define COLUMNS 2

#define RESOLUTION 0.1

#define HYSTERESIS 7

//How are the sensors powered? Is every sensor powered all the time?
//Do they need to be powered by a pin before being read?
#define CONSTANT_POWER
