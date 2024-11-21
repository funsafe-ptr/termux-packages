/*
 * Copyright (c) 2024 Twaik Yont <twaikyont@gmail.com>
 *
 * Licensed under the GNU GENERAL PUBLIC LICENSE, Version 3 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// ReSharper disable CppParameterMayBeConst

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/socket.h>

#include <xcb/xcb.h>
#include <xcb/present.h>
#include <X11/Xlib-xcb.h>

#include <glvnd/libeglabi.h>

#include <android/hardware_buffer.h>
#include <utils/Errors.h>
#include <system/window.h>

#define ANW_TRACE(args, ...)
#ifndef ANW_TRACE
#define ANW_TRACE(args, ...) dprintf(2, "%s:%d: %s(" args ")\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);
#endif

#define container_of(ptr, type, member) ({	const typeof( ((type *)0)->member ) *__mptr = (ptr); (type *)( (char *)__mptr - offsetof(type,member) );})
int sync_wait(int fd, int timeout);

static const char* getQueryOpString(int query) {
    switch(query) {
        #define QUERY(name) case NATIVE_WINDOW_ ## name: return #name
        QUERY(WIDTH);
        QUERY(HEIGHT);
        QUERY(FORMAT);
        QUERY(MIN_UNDEQUEUED_BUFFERS);
        QUERY(QUEUES_TO_WINDOW_COMPOSER);
        QUERY(CONCRETE_TYPE);
        QUERY(DEFAULT_WIDTH);
        QUERY(DEFAULT_HEIGHT);
        QUERY(TRANSFORM_HINT);
        QUERY(CONSUMER_RUNNING_BEHIND);
        QUERY(CONSUMER_USAGE_BITS);
        QUERY(STICKY_TRANSFORM);
        QUERY(DEFAULT_DATASPACE);
        QUERY(BUFFER_AGE);
        QUERY(LAST_DEQUEUE_DURATION);
        QUERY(LAST_QUEUE_DURATION);
        QUERY(LAYER_COUNT);
        QUERY(IS_VALID);
        QUERY(FRAME_TIMESTAMPS_SUPPORTS_PRESENT);
        QUERY(CONSUMER_IS_PROTECTED);
        QUERY(DATASPACE);
        QUERY(MAX_BUFFER_COUNT);
        #undef QUERY
        default: return NULL;
    }
}

static const char* getPerformOpString(int op) {
    switch(op) {
        #define OP(name) case NATIVE_WINDOW_ ## name: return #name;
        OP(SET_USAGE);
        OP(CONNECT);
        OP(DISCONNECT);
        OP(SET_CROP);
        OP(SET_BUFFER_COUNT);
        OP(SET_BUFFERS_GEOMETRY);
        OP(SET_BUFFERS_TRANSFORM);
        OP(SET_BUFFERS_TIMESTAMP);
        OP(SET_BUFFERS_DIMENSIONS);
        OP(SET_BUFFERS_FORMAT);
        OP(SET_SCALING_MODE);
        OP(LOCK);
        OP(UNLOCK_AND_POST);
        OP(API_CONNECT);
        OP(API_DISCONNECT);
        OP(SET_BUFFERS_USER_DIMENSIONS);
        OP(SET_POST_TRANSFORM_CROP);
        OP(SET_BUFFERS_STICKY_TRANSFORM);
        OP(SET_SIDEBAND_STREAM);
        OP(SET_BUFFERS_DATASPACE);
        OP(SET_SURFACE_DAMAGE);
        OP(SET_SHARED_BUFFER_MODE);
        OP(SET_AUTO_REFRESH);
        OP(GET_REFRESH_CYCLE_DURATION);
        OP(GET_NEXT_FRAME_ID);
        OP(ENABLE_FRAME_TIMESTAMPS);
        OP(GET_COMPOSITOR_TIMING);
        OP(GET_FRAME_TIMESTAMPS);
        OP(GET_WIDE_COLOR_SUPPORT);
        OP(GET_HDR_SUPPORT)
        OP(SET_USAGE64);
        OP(GET_CONSUMER_USAGE64);
        OP(SET_BUFFERS_SMPTE2086_METADATA);
        OP(SET_BUFFERS_CTA861_3_METADATA);
        OP(SET_BUFFERS_HDR10_PLUS_METADATA);
        OP(SET_AUTO_PREROTATION);
        OP(GET_LAST_DEQUEUE_START);
        OP(SET_DEQUEUE_TIMEOUT);
        OP(GET_LAST_DEQUEUE_DURATION);
        OP(GET_LAST_QUEUE_DURATION);
        OP(SET_FRAME_RATE);
        OP(SET_CANCEL_INTERCEPTOR);
        OP(SET_DEQUEUE_INTERCEPTOR);
        OP(SET_PERFORM_INTERCEPTOR);
        OP(SET_QUEUE_INTERCEPTOR);
        OP(ALLOCATE_BUFFERS);
        OP(GET_LAST_QUEUED_BUFFER);
        OP(SET_QUERY_INTERCEPTOR);
        OP(SET_FRAME_TIMELINE_INFO);
        OP(GET_LAST_QUEUED_BUFFER2);
        OP(SET_BUFFERS_ADDITIONAL_OPTIONS);
        default: return NULL;
    }
}

static size_t ANativeWindowBufferOffset = 0;
#define to_ANativeWindowBuffer(addr) ((struct ANativeWindowBuffer*) ((void*) addr + ANativeWindowBufferOffset))
#define from_ANativeWindowBuffer(addr) ((AHardwareBuffer*) ((void*) addr - ANativeWindowBufferOffset))

struct TermuxNativeWindow {
    int refcount;
    xcb_connection_t *conn;
    xcb_special_event_t *special_event;
    xcb_gcontext_t gc;
    xcb_window_t win;
    int32_t width, height, usage;
    bool allocation_needed;
    uint8_t swapInterval;
    struct{
        AHardwareBuffer* self;
        uint32_t pixmap, serial;
        uint8_t busy;
    } buffers[2], *front, *back;
    struct ANativeWindow base;
    void* reserved[16]; // We do not know if vendor added anything to the end of struct or not. Let's reserve some space.
};

EGLAPI void TermuxNativeWindow_incRef(android_native_base_t* base) {
    struct ANativeWindow *awin = container_of(base, struct ANativeWindow, common);
    struct TermuxNativeWindow* win = container_of(awin, struct TermuxNativeWindow, base);
    ANW_TRACE("base %p awin %p win %p", base, awin, win);
    __sync_fetch_and_add(&win->refcount,1);
}

EGLAPI void TermuxNativeWindow_decRef(android_native_base_t* base) {
    struct ANativeWindow *awin = container_of(base, struct ANativeWindow, common);
    struct TermuxNativeWindow* win = container_of(awin, struct TermuxNativeWindow, base);
    ANW_TRACE("base %p awin %p win %p", base, awin, win);
    if (__sync_fetch_and_sub(&win->refcount, 1) == 1) {
        ANW_TRACE("destroying win %p", base, awin, win);
        free(win);
    }
}

EGLAPI int TermuxNativeWindow_setSwapInterval(struct ANativeWindow *window, int interval) {
    ANW_TRACE("win %p interval %d", window, interval);
    struct TermuxNativeWindow* win = container_of(window, struct TermuxNativeWindow, base);
    if (!window || !win)
        return BAD_VALUE;

    win->swapInterval = interval<0 ? 0 : interval>1 ? 1 : interval;

    return NO_ERROR;
}

EGLAPI int TermuxNativeWindow_dequeueBuffer_DEPRECATED(struct ANativeWindow *window, struct ANativeWindowBuffer **buffer) {
    ANW_TRACE("win %p buffer %p", window, buffer);
    struct TermuxNativeWindow* win = container_of(window, struct TermuxNativeWindow, base);
    if (!window || !win)
        return BAD_VALUE;
    return win->base.dequeueBuffer(window, buffer, NULL);
}

EGLAPI int TermuxNativeWindow_lockBuffer_DEPRECATED(struct ANativeWindow *window, struct ANativeWindowBuffer *buffer) {
    ANW_TRACE("win %p buffer %p", window, buffer);
    return NO_ERROR;
}

EGLAPI int TermuxNativeWindow_queueBuffer_DEPRECATED(struct ANativeWindow *window, struct ANativeWindowBuffer *buffer) {
    ANW_TRACE("win %p buffer %p", window, buffer);
    struct TermuxNativeWindow* win = container_of(window, struct TermuxNativeWindow, base);
    if (!window || !win)
        return BAD_VALUE;
    return win->base.queueBuffer(window, buffer, -1);
}

EGLAPI int TermuxNativeWindow_query(const struct ANativeWindow *window, int what, int *value) {
    struct TermuxNativeWindow* win = container_of(window, struct TermuxNativeWindow, base);
    if (!window || !win)
        return BAD_VALUE;

    const char* q = getQueryOpString(what);
    ANW_TRACE("win %p what %s value %p", window, q, value);
    if (!value)
        return BAD_VALUE;
    switch(what) {
        case NATIVE_WINDOW_FORMAT:
            *value = 5 /* PIXEL_FORMAT_BGRA_8888 */;
            //*value = 1 /* PIXEL_FORMAT_RGBA_8888 */;
            return NO_ERROR;
        case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
            *value = 2;
            return NO_ERROR;
        case NATIVE_WINDOW_WIDTH:
        case NATIVE_WINDOW_DEFAULT_WIDTH:
            *value = win->width;
            return NO_ERROR;
        case NATIVE_WINDOW_HEIGHT:
        case NATIVE_WINDOW_DEFAULT_HEIGHT:
            *value = win->height;
            return NO_ERROR;
        case NATIVE_WINDOW_TRANSFORM_HINT:
            *value = 0;
            return NO_ERROR;
        case NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND:
        case NATIVE_WINDOW_IS_VALID:
            *value = 1;
            return NO_ERROR;
        case NATIVE_WINDOW_CONSUMER_USAGE_BITS:
            *value = win->usage;
            return NO_ERROR;
        default:
            return INVALID_OPERATION;
    }
}

EGLAPI int TermuxNativeWindow_perform(struct ANativeWindow *window, int operation, ...) {
    va_list args;
    va_start(args, operation);
    const char *op = getPerformOpString(operation);
    if (!op) {
        ANW_TRACE("win %p unknown op %d", window, operation);
        return BAD_VALUE;
    }

    switch(operation) {
        case NATIVE_WINDOW_SET_USAGE: {
            uint64_t usage = va_arg(args, uint32_t);
            ANW_TRACE("win %p %s %lu (no op)", window, op, usage);
            return NO_ERROR;
        }
        case NATIVE_WINDOW_CONNECT:
        case NATIVE_WINDOW_DISCONNECT: {
            ANW_TRACE("win %p %s (no op)", window, op);
            return NO_ERROR;
        }
        case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM: {
            int format = va_arg(args, int);
            ANW_TRACE("win %p %s %d (no op)", window, op, format);
            return NO_ERROR;
        }
        case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP: {
            long long timestamp = va_arg(args, long long);
            ANW_TRACE("win %p %s %lld (no op)", window, op, timestamp);
            return NO_ERROR;
        }
        case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS: {
            int w = va_arg(args, unsigned int), h = va_arg(args, unsigned int);
            ANW_TRACE("win %p %s %d %d (no op)", window, op, w, h);
            return NO_ERROR;
        }
        case NATIVE_WINDOW_SET_BUFFERS_FORMAT: {
            int format = va_arg(args, int);
            ANW_TRACE("win %p %s %d (no op)", window, op, format);
            return NO_ERROR;
        }
        case NATIVE_WINDOW_SET_SCALING_MODE: {
            int mode = va_arg(args, int);
            ANW_TRACE("win %p %s %d (no op)", window, op, mode);
            return NO_ERROR;
        }
        case NATIVE_WINDOW_API_CONNECT: {
            int api = va_arg(args, int);
            ANW_TRACE("win %p %s %d (no op)", window, op, api);
            if (api != NATIVE_WINDOW_API_EGL && api != NATIVE_WINDOW_API_CPU
                    && api != NATIVE_WINDOW_API_MEDIA && api != NATIVE_WINDOW_API_CAMERA) {
                dprintf(2, "%s:%d: perform(API_CONNECT): invalid api: %d\n", __FILE__, __LINE__, api);
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
        case NATIVE_WINDOW_API_DISCONNECT:
            ANW_TRACE("win %p %s (no op)", window, op);
            return NO_ERROR;
        case NATIVE_WINDOW_SET_BUFFERS_DATASPACE: {
            int dataspace = va_arg(args, int);
            ANW_TRACE("win %p %s %d (no op)", window, op, dataspace);
            return NO_ERROR;
        }
        case NATIVE_WINDOW_SET_USAGE64: {
            uint64_t usage = va_arg(args, uint64_t);
            ANW_TRACE("win %p %s %lld (no op)", window, op, usage);
            return NO_ERROR;
        }

        default:
            break;
    }
    ANW_TRACE("win %p unimplemented operation %d (no op)", window, operation);
    return INVALID_OPERATION;
}

EGLAPI int TermuxNativeWindow_cancelBuffer_DEPRECATED(struct ANativeWindow *window, struct ANativeWindowBuffer *buffer) {
    ANW_TRACE("win %p buffer %p", window, buffer);
    struct TermuxNativeWindow* win = container_of(window, struct TermuxNativeWindow, base);
    if (!window || !win)
        return BAD_VALUE;
    return win->base.cancelBuffer(window, buffer, -1);
}

EGLAPI int TermuxNativeWindow_dequeueBuffer(struct ANativeWindow *window, struct ANativeWindowBuffer **buffer, int *fenceFd) {
    ANW_TRACE("win %p buffer %p fenceFd %p", window, buffer, fenceFd);
    struct TermuxNativeWindow* win = container_of(window, struct TermuxNativeWindow, base);
    if (!window || !win || !buffer)
        return BAD_VALUE;

    if (win->allocation_needed) {
        AHardwareBuffer_Desc desc = { .width = win->width, .height = win->height, .format = 5, .layers = 1, .usage = win->usage }, desc1 = {0};
        int error = NO_ERROR;

        // Release existing buffers if any
        for (int i=0; i<sizeof(win->buffers)/sizeof(win->buffers[0]); i++) {
            if (!win->buffers[i].self)
                continue;

            AHardwareBuffer_release(win->buffers[i].self);
            xcb_free_pixmap_checked(win->conn, win->buffers[i].pixmap);
            memset(&win->buffers[i], 0, sizeof(win->buffers[i]));
        }

        // TODO: probably we should implement copying data from old frontbuffer to the new one to avoid glitches when user resizes window. Not so sure it will help though.

        // Allocate new buffers
        for (int i=0; i<sizeof(win->buffers)/sizeof(win->buffers[0]); i++) {
            int fds[] = { -1, -1 };
            uint8_t buf = 0;

            if ((error = AHardwareBuffer_allocate(&desc, &win->buffers[i].self)) != NO_ERROR) {
                dprintf(2, "can not allocate buffer width %d height %d format %d usage %lu, error %d\n",
                        desc.width, desc.height, desc.format, desc.usage, error);
                return NO_MEMORY;
            }

            if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
                return NO_MEMORY;

            win->buffers[i].pixmap = xcb_generate_id(win->conn);
            AHardwareBuffer_describe(win->buffers[i].self, &desc1);
            xcb_void_cookie_t cookie = xcb_dri3_pixmap_from_buffers_checked(win->conn, win->buffers[i].pixmap, win->win, 1, win->width, win->height, desc1.stride*4, 0, 0, 0, 0, 0, 0, 0, 24, 32, 1255, &fds[1]);
            xcb_flush(win->conn);

            read(fds[0], &buf, 1);
            AHardwareBuffer_sendHandleToUnixSocket(win->buffers[i].self, fds[0]);
            xcb_generic_error_t *err = xcb_request_check(win->conn, cookie);
            if (err) {
                free(err);
                win->buffers[i].pixmap = 0;
                AHardwareBuffer_release(win->buffers[i].self);
                win->buffers[i].self = NULL;
                dprintf(2, "xcb error while doing xcb_dri3_pixmap_from_buffers\n");
                return NO_MEMORY;
            }
        }

        win->front = &win->buffers[0];
        win->back = &win->buffers[1];
        win->allocation_needed = false;
    }

    *buffer = to_ANativeWindowBuffer(win->back->self);
    if (fenceFd)
        *fenceFd = -1;
    return NO_ERROR;
}

EGLAPI void TermuxNativeWindow_handleEvents(struct TermuxNativeWindow* win, xcb_generic_event_t* event) {
    xcb_present_generic_event_t* present = (xcb_present_generic_event_t*) event;
    xcb_present_complete_notify_event_t* complete = (xcb_present_complete_notify_event_t*) event;
    xcb_present_configure_notify_event_t *config = (xcb_present_configure_notify_event_t*) event;
    switch(present->evtype) {
        case XCB_PRESENT_EVENT_COMPLETE_NOTIFY: {
            if (complete->kind != XCB_PRESENT_COMPLETE_KIND_PIXMAP)
                break;

            for (int i=0; i<sizeof(win->buffers)/sizeof(win->buffers[0]); i++) {
                if (win->buffers[i].serial == complete->serial) {
                    win->buffers[i].busy = false;
                    break;
                }
            }
            break;
        }
        case XCB_PRESENT_EVENT_CONFIGURE_NOTIFY:
            win->width = config->pixmap_width;
            win->height = config->pixmap_height;
            win->allocation_needed = true;
            break;
        default:
            dprintf(2, "got some weird present event %d\n", present->evtype);
        xcb_flush(win->conn);
    }
}

EGLAPI int TermuxNativeWindow_queueBuffer(struct ANativeWindow *window, struct ANativeWindowBuffer *buffer, int fenceFd) {
    ANW_TRACE("win %p buffer %p fenceFd %d", window, buffer, fenceFd);
    static int serial = 1;
    struct TermuxNativeWindow* win = container_of(window, struct TermuxNativeWindow, base);
    if (!window || !win || !buffer)
        return BAD_VALUE;

    if (fenceFd >= 0) {
        sync_wait(fenceFd, 500);
        close(fenceFd);
    }

    xcb_generic_event_t* event;

    xcb_flush(win->conn);
    while((event = xcb_poll_for_special_event(win->conn, win->special_event)))
        TermuxNativeWindow_handleEvents(win, event);

    if (!win->swapInterval && win->front->busy)
        return NO_ERROR;

    while (win->front->busy) {
        event = xcb_wait_for_special_event(win->conn, win->special_event);
        if (!event) {
            dprintf(2, "failed to obtain special event\n");
            return DEAD_OBJECT;
        }

        TermuxNativeWindow_handleEvents(win, event);
    }

    win->back->serial = serial++;
    win->back->busy = true;
    xcb_void_cookie_t cookie = xcb_present_pixmap_checked(win->conn, win->win, win->back->pixmap, win->back->serial, 0, 0, 0, 0, 0, 0, 0, XCB_PRESENT_OPTION_NONE, 0, 0, 0, 0, NULL);
    xcb_discard_reply(win->conn, cookie.sequence);
    xcb_flush(win->conn);

    void* temp = win->front;
    win->front = win->back;
    win->back = temp;

    return NO_ERROR;
}

EGLAPI int TermuxNativeWindow_cancelBuffer(struct ANativeWindow *window, struct ANativeWindowBuffer *buffer, int fenceFd) {
    ANW_TRACE("win %p buffer %p fenceFd %d", window, buffer, fenceFd);
    // We do not explicitly mark or register dequeued buffers so no action required here.
    return NO_ERROR;
}

EGLAPI struct ANativeWindow* TermuxNativeWindow_create(Display* dpy, Window win) {
    // ReSharper disable once CppDFAMemoryLeak
    struct TermuxNativeWindow* new = calloc(1, sizeof(struct TermuxNativeWindow));
    new->base.common.magic = ANDROID_NATIVE_WINDOW_MAGIC;
    new->base.common.version = sizeof(struct ANativeWindow);
    memset(new->base.common.reserved, 0, sizeof(new->base.common.reserved));
    *(int*) &new->base.minSwapInterval = 0;
    *(int*) &new->base.maxSwapInterval = 1;
    *(float*) &new->base.xdpi = 160;
    *(float*) &new->base.ydpi = 160;
    new->base.common.incRef = TermuxNativeWindow_incRef;
    new->base.common.decRef = TermuxNativeWindow_decRef;

    new->base.setSwapInterval = TermuxNativeWindow_setSwapInterval;
    new->base.dequeueBuffer_DEPRECATED = TermuxNativeWindow_dequeueBuffer_DEPRECATED;
    new->base.lockBuffer_DEPRECATED = TermuxNativeWindow_lockBuffer_DEPRECATED;
    new->base.queueBuffer_DEPRECATED = TermuxNativeWindow_queueBuffer_DEPRECATED;
    new->base.query = TermuxNativeWindow_query;
    new->base.perform = TermuxNativeWindow_perform;
    new->base.cancelBuffer_DEPRECATED = TermuxNativeWindow_cancelBuffer_DEPRECATED;
    new->base.dequeueBuffer = TermuxNativeWindow_dequeueBuffer;
    new->base.queueBuffer = TermuxNativeWindow_queueBuffer;
    new->base.cancelBuffer = TermuxNativeWindow_cancelBuffer;

    new->conn = XGetXCBConnection(dpy);
    new->win = win;
    xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(new->conn, xcb_get_geometry(new->conn, new->win), NULL);
    new->width = geom->width;
    new->height = geom->height;
    new->usage = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER | AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;
    new->allocation_needed = true;
    __sync_fetch_and_add(&new->refcount,1);

    xcb_setup_t const *const setup = xcb_get_setup(new->conn);
    xcb_screen_t *const screen = xcb_setup_roots_iterator(setup).data;

    new->gc = xcb_generate_id(new->conn);
    xcb_create_gc(new->conn, new->gc, win, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, (const uint32_t[]) {screen->black_pixel, 0});

    uint32_t eid = xcb_generate_id(new->conn);
    new->special_event = xcb_register_for_special_xge(new->conn, &xcb_present_id, eid, NULL);
    xcb_present_select_input(new->conn, eid, new->win, XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY | XCB_PRESENT_EVENT_MASK_CONFIGURE_NOTIFY);

    return &new->base;
}

EGLAPI void TermuxNativeWindow_init(void) {
    size_t (*AHardwareBuffer_to_ANativeWindowBuffer)(void*) = dlsym(NULL, "_ZN7android38AHardwareBuffer_to_ANativeWindowBufferEP15AHardwareBuffer");
    size_t (*GraphicBuffer_getNativeBuffer)(void*) = dlsym(NULL, "_ZNK7android13GraphicBuffer15getNativeBufferEv");
    if (AHardwareBuffer_to_ANativeWindowBuffer)
        ANativeWindowBufferOffset = AHardwareBuffer_to_ANativeWindowBuffer((void*) 0x1) - 0x1; // To avoid error `getNativeBuffer() called on NULL GraphicBuffer`
    else if (GraphicBuffer_getNativeBuffer)
        ANativeWindowBufferOffset = GraphicBuffer_getNativeBuffer((void*) 0x1) - 0x1;
}
