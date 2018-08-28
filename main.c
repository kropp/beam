#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>


static char *const APPLICATION_NAME = "beam";

static const int BEAM_SIZE = 600;

static const int ALPHA_LUT_SIZE = 15;
static const unsigned long alpha_lut[] = { 0, 0x14, 0x27, 0x38, 0x47, 0x56, 0x63, 0x6E, 0x79, 0x82, 0x89, 0x8F, 0x94, 0x97, 0x99 };

void mouse_pointer_show(Display *display, Window window);
void mouse_pointer_hide(Display *display, Window window);

int main(int argc, char* argv[]) {
    char *display_name = getenv("DISPLAY");
    if (!display_name) {
        fprintf(stderr, "%s: cannot connect to X server '%s'\n", argv[0], display_name);
        exit(1);
    }

    Display *display = XOpenDisplay(display_name);
    int screen = DefaultScreen(display);
    Screen *screen_info = ScreenOfDisplay(display, screen);
    unsigned int screen_width = (unsigned int) screen_info->width;
    unsigned int screen_height = (unsigned int) screen_info->height;
    XFree(screen_info);

    XVisualInfo vinfo;
    XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor, &vinfo);

    XSetWindowAttributes attrs = {
        .override_redirect = 1,
        .colormap = XCreateColormap(display, DefaultRootWindow(display), vinfo.visual, AllocNone),
        .border_pixel = 0,
        .background_pixel = 0
    };

    Window window = XCreateWindow(display, XRootWindow(display, screen), 0, 0, screen_width, screen_height, 0, vinfo.depth, CopyFromParent, vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attrs);
    XMapWindow(display, window);
    XStoreName(display, window, APPLICATION_NAME);

    XClassHint *class = XAllocClassHint();
    class->res_name = APPLICATION_NAME;
    class->res_class = APPLICATION_NAME;
    XSetClassHint(display, window, class);
    XFree(class);

    // Keep the window on top
    XEvent e;
    memset(&e, 0, sizeof(e));
    e.xclient.type = ClientMessage;
    e.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", False);
    e.xclient.display = display;
    e.xclient.window = window;
    e.xclient.format = 32;
    e.xclient.data.l[0] = 1;
    e.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_STAYS_ON_TOP", False);
    XSendEvent(display, XRootWindow(display, screen), False, SubstructureRedirectMask, &e);

    XRaiseWindow(display, window);
    XFlush(display);

    XGCValues values = { .graphics_exposures = False };
    unsigned long value_mask = 0;

    GC gc = XCreateGC(display, window, value_mask, &values);
    XSelectInput(display, window, PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask);
    XEvent event;

    int alpha = 0;
    int direction = 0;
    int x = 0;
    int y = 0;

    while(True) {
        if (direction == 0 || XPending(display)) {
            XNextEvent(display, &event);
            switch(event.type) {
                case ButtonPress:
                    direction = 1;
                    mouse_pointer_hide(display, window);
                    break;
                case ButtonRelease:
                    direction = -1;
                    mouse_pointer_show(display, window);
                    break;
                case MotionNotify:
                    //printf("x %d y %d\n", event.xmotion.x, event.xmotion.y);
                    x = event.xmotion.x;
                    y = event.xmotion.y;
                    break;
                default:
                    break;
            }
        } else {
            usleep(1000000/60);
        }

        if (alpha == 0 && direction == -1) {
            direction = 0;
        }
        if (alpha == ALPHA_LUT_SIZE-1 && direction == 1) {
            direction = 0;
        }
        alpha += direction;

        if (alpha != 0) {
            XSetForeground(display, gc, 0x00000000ul | (alpha_lut[alpha] << 24));
            XFillRectangle(display, window, gc, 0, 0, screen_width, screen_height);
            XSetForeground(display, gc, 0x00000000ul);
            XFillArc(display, window, gc, x - BEAM_SIZE / 2, y - BEAM_SIZE / 2, BEAM_SIZE, BEAM_SIZE, 0, 360 * 64);
        } else {
            XClearWindow(display, window);
        }
        XSync(display, False);
    }
    XFreeGC(display, gc);
    XCloseDisplay(display);
}

void mouse_pointer_show(Display *display, Window window) {
    XUndefineCursor(display, window);
}

void mouse_pointer_hide(Display *display, Window window) {
    static char empty[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    Pixmap bitmap = XCreateBitmapFromData(display, window, empty, 8, 8);

    XColor black = { .red = 0, .green = 0, .blue = 0 };
    Cursor cursor = XCreatePixmapCursor(display, bitmap, bitmap, &black, &black, 0, 0);

    XDefineCursor(display, window, cursor);

    XFreeCursor(display, cursor);
    XFreePixmap(display, bitmap);
}
