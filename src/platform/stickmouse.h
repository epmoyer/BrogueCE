
// platform definitions
// typedef int                     int32_t;
// typedef unsigned int uint32_t;
// typedef unsigned char uint8_t;

// typedef uint32_t Uint32;
// typedef int32_t Sint32;
// typedef uint8_t Uint8;

#define STK_MOUSEBUTTON_LEFT     1

typedef enum
{
    /* Mouse events */
    STK_MOUSEMOTION    = 0x400, /**< Mouse moved */
    STK_MOUSEBUTTONDOWN,        /**< Mouse button pressed */
    STK_MOUSEBUTTONUP,          /**< Mouse button released */
} STK_EventType;


typedef struct STK_MouseMotionEvent
{
    Uint32 type;        /**< ::SDL_MOUSEMOTION */
    Sint32 x;           /**< X coordinate, relative to window */
    Sint32 y;           /**< Y coordinate, relative to window */
    // Uint32 timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    // Uint32 windowID;    /**< The window with mouse focus, if any */
    // Uint32 which;       /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
    // Uint32 state;       /**< The current button state */
    // Sint32 xrel;        /**< The relative motion in the X direction */
    // Sint32 yrel;        /**< The relative motion in the Y direction */
} STK_MouseMotionEvent;

typedef struct STK_MouseButtonEvent
{
    Uint32 type;        /**< ::SDL_MOUSEBUTTONDOWN or ::SDL_MOUSEBUTTONUP */
    Sint32 x;           /**< X coordinate, relative to window */
    Sint32 y;           /**< Y coordinate, relative to window */
    // Uint32 timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    // Uint32 windowID;    /**< The window with mouse focus, if any */
    // Uint32 which;       /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
    Uint8 button;       /**< The mouse button index */
    // Uint8 state;        /**< ::SDL_PRESSED or ::SDL_RELEASED */
    // Uint8 clicks;       /**< 1 for single-click, 2 for double-click, etc. */
    // Uint8 padding1;
} STK_MouseButtonEvent;

typedef union SDL_Event
{
    Uint32 type;                            /**< Event type, shared with all events */
    STK_MouseMotionEvent motion;            /**< Mouse motion event data */
    STK_MouseButtonEvent button;            /**< Mouse button event data */
} STK_Event;