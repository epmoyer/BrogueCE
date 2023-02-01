/**
 * Author: Eric Moyer
 *
 * Description:
 * Emulate a virtual mouse position using the RG351V analog stick.
 *
 * Compile:
 * gcc rg351v_stickmouse.c -o rg351v_stickmouse
 *
 */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "platform.h"
#include <SDL_image.h>
#include "stickmouse.h"
#include <linux/joystick.h>


#define boolean                 char
#define false                   0
#define true                    1

int STK_joystick;
int STK_window_width;
int STK_window_height;
float STK_mouse_x;
float STK_mouse_y;
float STK_velocity_scale;
boolean STK_mouse_position_was_reported;
Sint32 STK_mouse_x_int;
Sint32 STK_mouse_y_int;
Sint32 STK_mouse_x_int_last_reported;
Sint32 STK_mouse_y_int_last_reported;
long STK_ticks_previous;

/**
 * Current state of an axis.
 */
struct axis_state {
    short x, y;
};

struct axis_state STK_axes[3] = {0};

#define STK_JS_AXIS 0
#define STK_BUTTON_INDEX_MOUSEBUTTON_LEFT 0
#define STK_FPS 60
#define STK_JS_RANGE_MAX 32767
#define STK_SCREEN_TRANSIT_SECONDS_MIN 2.0


/**
 * Reads a joystick event from the joystick device.
 *
 * Returns 0 on success. Otherwise -1 is returned.
 */
int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    /* Error, could not read full event. */
    return -1;
}

/**
 * Returns the number of axes on the controller or 0 if an error occurs.
 */
size_t get_axis_count(int fd)
{
    __u8 axes;

    if (ioctl(fd, JSIOCGAXES, &axes) == -1)
        return 0;

    return axes;
}

/**
 * Returns the number of buttons on the controller or 0 if an error occurs.
 */
size_t get_button_count(int fd)
{
    __u8 buttons;
    if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
        return 0;

    return buttons;
}


/**
 * Keeps track of the current axis state.
 *
 * NOTE: This function assumes that axes are numbered starting from 0, and that
 * the X axis is an even number, and the Y axis is an odd number. However, this
 * is usually a safe assumption.
 *
 * Returns the axis that the event indicated.
 */
size_t get_axis_state(struct js_event *event, struct axis_state axes[3])
{
    size_t axis = event->number / 2;

    if (axis < 3)
    {
        if (event->number % 2 == 0)
            axes[axis].x = event->value;
        else
            axes[axis].y = event->value;
    }

    return axis;
}

int STK_init(int window_width, int window_height){
    STK_window_width = window_width;
    STK_window_height = window_height;
    STK_mouse_x = 0.0;
    STK_mouse_y = 0.0;
    STK_mouse_position_was_reported = false;
    STK_ticks_previous = SDL_GetTicks();
    // STK_joystick_x = 0;
    // STK_joystick_y = 0;

    fprintf(stderr, "STK_init(%d, %d)\n", window_width, window_height);

    // Set velocity scale such that, when multiplied by stick value, the mouse can cross the
    // width of the screen in STK_SCREEN_TRANSIT_SECONDS_MIN seconds if held at its maximum
    // deflection
    STK_velocity_scale = 
        (float)STK_window_width 
        / (float)STK_JS_RANGE_MAX 
        / (float)STK_FPS
        / STK_SCREEN_TRANSIT_SECONDS_MIN;

    STK_joystick = open("/dev/input/js0", O_RDONLY | O_NONBLOCK);

    if (STK_joystick == -1)
        perror("Could not open joystick");
    return 0;
}

void stickmouse_close(){
    close(STK_joystick);
}

boolean STK_PollEvent(STK_Event *stk_event) {
    int status;
    struct js_event event;
    size_t axis;
    boolean done;
    long ticks_current;
    long ticks_elapsed;

    done = false;
    while(!done){
        status = read_event(STK_joystick, &event);
        if (status == 0) {
            switch (event.type)
            {
                case JS_EVENT_BUTTON:
                    // printf("Button %u %s\n", event.number, event.value ? "pressed" : "released");
                    if(event.number == STK_BUTTON_INDEX_MOUSEBUTTON_LEFT){
                        stk_event->type = event.value ? STK_MOUSEBUTTONDOWN : STK_MOUSEBUTTONUP;
                        stk_event->button.button = STK_MOUSEBUTTON_LEFT;
                        stk_event->button.x = STK_mouse_x_int_last_reported;
                        stk_event->button.y = STK_mouse_y_int_last_reported;
                        return true;
                    }
                    break;
                case JS_EVENT_AXIS:
                    axis = get_axis_state(&event, STK_axes);
                    // if (axis == 0) {
                    //     STK_joystick_x = STK_axes[0].x;
                    //     STK_joystick_y = STK_axes[0].y;
                    //     printf("Axis %zu at (%6d, %6d)\n", axis, STK_axes[axis].x, STK_axes[axis].y);
                    // }
                    break;
                default:
                    /* Ignore init events. */
                    break;
            }
            // printf("event.type: %d\n", event.type);
            
            fflush(stdout);
        }
        else {
            // No more events
            done = true;
            // printf("No event\n");
        }
    }

    // Now we know the current position of the stick, so move the virtual mouse.
    ticks_current = SDL_GetTicks();
    ticks_elapsed = ticks_current - STK_ticks_previous;
    tick_scale = (float)ticks_elapsed / ((float)1000 / (float)STK_FPS);
    STK_ticks_previous = ticks_current;

    STK_mouse_x -= (float)STK_axes[STK_JS_AXIS].x * STK_velocity_scale * tick_scale;
    if(STK_mouse_x >= (float)STK_window_width){
        STK_mouse_x = (float)STK_window_width - 1;
    } else if (STK_mouse_x < 0){
        STK_mouse_x = 0;
    }

    STK_mouse_y -= (float)STK_axes[STK_JS_AXIS].y * STK_velocity_scale * tick_scale;
    if(STK_mouse_y >= (float)STK_window_height){
        STK_mouse_y = (float)STK_window_height - 1;
    } else if (STK_mouse_y < 0){
        STK_mouse_y = 0;
    }

    STK_mouse_x_int = (Sint32)STK_mouse_x;
    STK_mouse_y_int = (Sint32)STK_mouse_y;
    if ( 
        !STK_mouse_position_was_reported 
        || (STK_mouse_x_int != STK_mouse_x_int_last_reported) 
        || (STK_mouse_y_int != STK_mouse_y_int_last_reported) ) {
        
        stk_event->type = STK_MOUSEMOTION;
        stk_event->motion.x = STK_mouse_x_int;
        stk_event->motion.y = STK_mouse_y_int;

        STK_mouse_x_int_last_reported = STK_mouse_x_int;
        STK_mouse_y_int_last_reported = STK_mouse_y_int;
        STK_mouse_position_was_reported = true;

        return true;
    }
    return false;
}

// int main(int argc, char *argv[])
// {
//     float old_x;
//     float old_y;
//     STK_Event event;

//     STK_init(640, 480);

//     while (1) {
//         usleep(1000000 / STK_FPS);
        
//         // old_x = STK_mouse_x;
//         // old_y = STK_mouse_y;
//         // stickmouse_update();
//         // if((old_x != STK_mouse_x) || (old_y != STK_mouse_y)){
//         //     printf("stickmouse: (%5.1f, %5.1f)\n", STK_mouse_x, STK_mouse_y);
//         // }
//         if(STK_PollEvent(&event)){
//             switch(event.type){
//                 case STK_MOUSEMOTION:
//                     printf("STK_MOUSEMOTION: (%d, %d)\n", event.motion.x, event.motion.y);
//                     break;
//                 case STK_MOUSEBUTTONDOWN:
//                     printf("STK_MOUSEBUTTONDOWN\n");
//                     break;
//                 case STK_MOUSEBUTTONUP:
//                     printf("STK_MOUSEBUTTONUP\n");
//                     break;
//             }
//         }
//     }
    
//     stickmouse_close();

//     return 0;
// }
