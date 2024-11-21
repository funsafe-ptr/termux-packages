#include <stdio.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <glvnd/libeglabi.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct ANativeWindow* TermuxNativeWindow_create(Display* dpy, Window win);
void TermuxNativeWindow_init(void);

// Both OVERRIDE and NO_OVERRIDE are macros like N(required, return type, name, args definition in parentheses, args in parentheses).
// OVERRIDE is for functions we override, NO_OVERRIDE for functions we dlopen from Android implementation and use as is.
#define EGL_FUNCS(OVERRIDE, NO_OVERRIDE) \
	/* EGL_VERSION_1_0 */ \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, ChooseConfig, (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config), (dpy, attrib_list, configs, config_size, num_config)) \
	OVERRIDE(EGL_TRUE, EGLBoolean, CopyBuffers, (EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target), (dpy, surface, target)) \
	NO_OVERRIDE(EGL_TRUE, EGLContext, CreateContext, (EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list), (dpy, config, share_context, attrib_list)) \
	NO_OVERRIDE(EGL_TRUE, EGLSurface, CreatePbufferSurface, (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list), (dpy, config, attrib_list)) \
	OVERRIDE(EGL_TRUE, EGLSurface, CreatePixmapSurface, (EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list), (dpy, config, pixmap, attrib_list)) \
	OVERRIDE(EGL_TRUE, EGLSurface, CreateWindowSurface, (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list), (dpy, config, win, attrib_list)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, DestroyContext, (EGLDisplay dpy, EGLContext ctx), (dpy, ctx)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, DestroySurface, (EGLDisplay dpy, EGLSurface surface), (dpy, surface)) \
	OVERRIDE(EGL_TRUE, EGLBoolean, GetConfigAttrib, (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value), (dpy, config, attribute, value)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, GetConfigs, (EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config), (dpy, configs, config_size, num_config)) \
	NO_OVERRIDE(EGL_TRUE, EGLDisplay, GetCurrentDisplay, (void), ()) \
	NO_OVERRIDE(EGL_TRUE, EGLSurface, GetCurrentSurface, (EGLint readdraw), (readdraw)) \
	OVERRIDE(EGL_TRUE, EGLDisplay, GetDisplay, (EGLNativeDisplayType display_id), (display_id)) \
	NO_OVERRIDE(EGL_TRUE, EGLint, GetError, (void), ()) \
	OVERRIDE(EGL_TRUE, __eglMustCastToProperFunctionPointerType, GetProcAddress, (const char *procname), (procname)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, Initialize, (EGLDisplay dpy, EGLint *major, EGLint *minor), (dpy, major, minor)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, MakeCurrent, (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx), (dpy, draw, read, ctx)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, QueryContext, (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value), (dpy, ctx, attribute, value)) \
	OVERRIDE(EGL_TRUE, const char *, QueryString, (EGLDisplay dpy, EGLint name), (dpy, name)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, QuerySurface, (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value), (dpy, surface, attribute, value)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, SwapBuffers, (EGLDisplay dpy, EGLSurface surface), (dpy, surface)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, Terminate, (EGLDisplay dpy), (dpy)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, WaitGL, (void), ()) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, WaitNative, (EGLint engine), (engine)) \
	/* EGL_VERSION_1_0 */ \
	/* EGL_VERSION_1_1 */ \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, BindTexImage, (EGLDisplay dpy, EGLSurface surface, EGLint buffer), (dpy, surface, buffer)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, ReleaseTexImage, (EGLDisplay dpy, EGLSurface surface, EGLint buffer), (dpy, surface, buffer)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, SurfaceAttrib, (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value), (dpy, surface, attribute, value)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, SwapInterval, (EGLDisplay dpy, EGLint interval), (dpy, interval)) \
	/* EGL_VERSION_1_1 */ \
	/* EGL_VERSION_1_2 */ \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, BindAPI, (EGLenum api), (api)) \
	NO_OVERRIDE(EGL_TRUE, EGLenum, QueryAPI, (void), ()) \
	NO_OVERRIDE(EGL_TRUE, EGLSurface, CreatePbufferFromClientBuffer, (EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list), (dpy, buftype, buffer, config, attrib_list)) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, ReleaseThread, (void), ()) \
	NO_OVERRIDE(EGL_TRUE, EGLBoolean, WaitClient, (void), ()) \
	/* EGL_VERSION_1_2 */ \
	/* EGL_VERSION_1_4 */ \
	NO_OVERRIDE(EGL_TRUE, EGLContext, GetCurrentContext, (void), ()) \
	/* EGL_VERSION_1_4 */ \
	/* EGL_VERSION_1_5 */ \
	NO_OVERRIDE(EGL_FALSE, EGLSync, CreateSync, (EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list), (dpy, type, attrib_list)) \
	NO_OVERRIDE(EGL_FALSE, EGLBoolean, DestroySync, (EGLDisplay dpy, EGLSync sync), (dpy, sync)) \
	NO_OVERRIDE(EGL_FALSE, EGLint, ClientWaitSync, (EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout), (dpy, sync, flags, timeout)) \
	NO_OVERRIDE(EGL_FALSE, EGLBoolean, GetSyncAttrib, (EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib *value), (dpy, sync, attribute, value)) \
	NO_OVERRIDE(EGL_FALSE, EGLImage, CreateImage, (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLAttrib *attrib_list), (dpy, ctx, target, buffer, attrib_list)) \
	NO_OVERRIDE(EGL_FALSE, EGLBoolean, DestroyImage, (EGLDisplay dpy, EGLImage image), (dpy, image)) \
	OVERRIDE(EGL_FALSE, EGLDisplay, GetPlatformDisplay, (EGLenum platform, void *native_display, const EGLAttrib *attrib_list), (platform, native_display, attrib_list)) \
	NO_OVERRIDE(EGL_FALSE, EGLSurface, CreatePlatformWindowSurface, (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLAttrib *attrib_list), (dpy, config, native_window, attrib_list)) \
	NO_OVERRIDE(EGL_FALSE, EGLSurface, CreatePlatformPixmapSurface, (EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list), (dpy, config, native_pixmap, attrib_list)) \
	NO_OVERRIDE(EGL_FALSE, EGLBoolean, WaitSync, (EGLDisplay dpy, EGLSync sync, EGLint flags), (dpy, sync, flags)) \
	/* EGL_VERSION_1_5 */ \
	/* EGL_KHR_debug */ \
	NO_OVERRIDE(EGL_FALSE, EGLint, DebugMessageControlKHR, (void* callback, const EGLAttrib *attrib_list), (callback, attrib_list)) \
	NO_OVERRIDE(EGL_FALSE, EGLBoolean, QueryDebugKHR, (EGLint attribute, EGLAttrib *value),  (attribute, value)) \
	NO_OVERRIDE(EGL_FALSE, EGLint, LabelObjectKHR, (EGLDisplay display, EGLenum objectType, EGLObjectKHR object, EGLLabelKHR label), (display, objectType, object, label)) \
	/* EGL_KHR_debug */ \
	/* EGL_KHR_display_reference */ \
	NO_OVERRIDE(EGL_FALSE, EGLBoolean, QueryDisplayAttribKHR, (EGLDisplay dpy, EGLint name, EGLAttrib *value), (dpy, name, value)) \
	/* EGL_KHR_display_reference */ \
	/* EGL_EXT_device_base */ \
	NO_OVERRIDE(EGL_FALSE, EGLBoolean, QueryDeviceAttribEXT, (EGLDeviceEXT device, EGLint attribute, EGLAttrib *value), (device, attribute, value)) \
	NO_OVERRIDE(EGL_FALSE, const char *, QueryDeviceStringEXT, (EGLDeviceEXT device, EGLint name), (device, name)) \
	NO_OVERRIDE(EGL_FALSE, EGLBoolean, QueryDevicesEXT, (EGLint max_devices, EGLDeviceEXT *devices, EGLint *num_devices), (max_devices, devices, num_devices)) \
	NO_OVERRIDE(EGL_FALSE, EGLBoolean, QueryDisplayAttribEXT, (EGLDisplay dpy, EGLint attribute, EGLAttrib *value), (dpy, attribute, value)) \
	/* EGL_EXT_device_base */

static struct {
#define FUNC(required, ret, name, argsdef, args) ret (*egl ## name) argsdef;
	EGL_FUNCS(FUNC, FUNC)
#undef FUNC
} android;
static void* GLESv2 = NULL;
static const char* extensions = NULL;
static Display* display = NULL; // There is no reason to have multiple Display pointers, usually program has only one;

static const __EGLapiExports *apiExports = NULL;

#define WRAP(required, ret, name, argsdef, args) \
	EGLAPI ret termuxEGL_ ## name argsdef { \
		apiExports->threadInit();	\
		ret r = android.egl ## name args;	\
		EGLint err = android.eglGetError(); \
		apiExports->setEGLError(err);	\
		if (err != EGL_SUCCESS) dprintf(2, "Invoking " #name " ended with error 0x%X\n", err);	\
		return r; \
	}
#define NOWRAP(required, ret, name, argsdef, args)
EGL_FUNCS(NOWRAP, WRAP)
#undef WRAP
#undef NOWRAP

EGLAPI EGLBoolean termuxEGL_CopyBuffers(EGLDisplay __unused dpy, EGLSurface __unused surface, EGLNativePixmapType __unused target) {
	// eglCopyBuffers is not implemented on Android
	apiExports->threadInit();
	apiExports->setEGLError(EGL_BAD_NATIVE_PIXMAP);
	return EGL_FALSE;
}

EGLAPI EGLSurface termuxEGL_CreatePixmapSurface(EGLDisplay __unused dpy, EGLSurface __unused surface, EGLNativePixmapType __unused target) {
	// eglCreatePixmapSurface is not implemented on Android
	apiExports->threadInit();
	apiExports->setEGLError(EGL_BAD_ALLOC);
	return EGL_NO_SURFACE;
}

EGLAPI EGLSurface termuxEGL_CreateWindowSurface (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list) {
	apiExports->threadInit();
	EGLSurface r = android.eglCreateWindowSurface(dpy, config, (EGLNativeWindowType) TermuxNativeWindow_create(display, win), attrib_list);
	apiExports->setEGLError(android.eglGetError());
	return r;
}

EGLAPI EGLBoolean termuxEGL_GetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value) {
	apiExports->threadInit();
	if (attribute == EGL_NATIVE_VISUAL_ID) {
		int count = 0;
		XVisualInfo t = { .depth = 24 }, *info = XGetVisualInfo (display, VisualDepthMask, &t, &count);

		if (count) {
			*value = info->visualid;
			apiExports->setEGLError(EGL_SUCCESS);
			return EGL_TRUE;
		}

		apiExports->setEGLError(EGL_BAD_ATTRIBUTE);
		return EGL_FALSE;
	}
	EGLBoolean r = android.eglGetConfigAttrib(dpy, config, attribute, value);
	EGLint err = android.eglGetError();
	apiExports->setEGLError(err);
	if (err != 0x3000) dprintf(2, "Invoking GetConfigAttrib ended with error 0x%X\n", err);
	return r;
}

EGLAPI const char * termuxEGL_QueryString(EGLDisplay dpy, EGLint name) {
	apiExports->threadInit();
	if (dpy == EGL_NO_DISPLAY && name == EGL_EXTENSIONS) {
		apiExports->setEGLError(EGL_SUCCESS);
		return extensions;
	}
	const char *r = android.eglQueryString(dpy, name);
	apiExports->setEGLError(android.eglGetError());
	return r;
}

EGLAPI EGLDisplay termuxEGL_GetPlatformDisplay(EGLenum platform, void *nativeDisplay, const EGLAttrib *attrib_list) {
	apiExports->threadInit();
	display = nativeDisplay; // Usually program has only one connection to X display, no reason to have many of them.
	EGLDisplay r = android.eglGetDisplay(EGL_DEFAULT_DISPLAY);
	apiExports->setEGLError(android.eglGetError());
	return r;
}

EGLAPI EGLDisplay termuxEGL_GetDisplay (EGLNativeDisplayType display_id) {
	apiExports->threadInit();
	dprintf(2, "termuxEGL_GetDisplay is not implemented by design. Report bug if you see this message\n");
	apiExports->setEGLError(EGL_BAD_ALLOC);
	return EGL_NO_DISPLAY;
}

EGLAPI void * termuxEGL_GetProcAddress(const char *procName) {
	if (!strncmp("egl", procName, 3)) {
		if (0) {}
#define FUNC(required, ret, name, argsdef, args) else if (!strcmp("egl" #name, procName)) return termuxEGL_ ## name;
		EGL_FUNCS(FUNC, FUNC)
#undef FUNC
		dprintf(2, "failed to locate %s symbol using GetProcAddress since it is not implemented\n", procName);
		return NULL;
	}
	if (!strncmp("gl", procName, 2)) {
		void* f = dlsym(GLESv2, procName);
		// if (!f)
			// dprintf(2, "failed to locate %s symbol due to linker error: %s\n", procName, dlerror());
		return f;
	}

	dprintf(2, "failed to locate %s symbol because it does not start with `gl` or `egl`\n", procName);
	return NULL;
}

EGLAPI EGLBoolean termuxEGL_GetSupportsAPI(EGLenum api) {
	return api == EGL_OPENGL_ES_API ? EGL_TRUE : EGL_FALSE;
}

EGLAPI const char * termuxEGL_GetVendorString(int name) {
	if (name == __EGL_VENDOR_STRING_PLATFORM_EXTENSIONS)
		return "EGL_KHR_platform_android EGL_KHR_platform_x11 EGL_EXT_platform_x11 EGL_EXT_platform_xcb";
	return NULL;
}

EGLAPI void *termuxEGL_GetDispatchAddress(const char *procName) {
	dprintf(2, "%s is not implemented yet, call with arg \"%s\" is not effective\n", __FUNCTION__, procName); // TODO: implement me
	return NULL;
}

EGLAPI void termuxEGL_SetDispatchIndex(const char * __unused procName, int __unused index) {
	// Not necessary to be implemented
}

EGLAPI EGLBoolean __egl_Main(uint32_t version, const __EGLapiExports *exports, __EGLvendorInfo __unused *vendor, __EGLapiImports *imports) {
	if (EGL_VENDOR_ABI_GET_MAJOR_VERSION(version) != EGL_VENDOR_ABI_MAJOR_VERSION) {
		dprintf(2, "GLVND abi version mismatch");
		return EGL_FALSE;
	}

	if (apiExports != NULL)
		return EGL_TRUE; // Already initialized.

	void* EGL = dlopen("/system/lib64/libEGL.so", RTLD_NOW | RTLD_GLOBAL);
	if (!EGL) {
		char* error;
		while ((error = dlerror()))
			dprintf(2, "EGL linking error: %s\n", error);
		return EGL_FALSE;
	}

	GLESv2 = dlopen("/system/lib64/libGLESv2.so", RTLD_NOW);
	if (!GLESv2) {
		char* error;
		while ((error = dlerror()))
			dprintf(2, "GLESv2 linking error: %s\n", error);
		return EGL_FALSE;
	}

#define FUNC(required, ret, name, argsdef, args) \
		android.egl ## name = dlsym(EGL, "egl" #name); \
		if (!android.egl ## name && required) { \
			dprintf(2, "EGL symbol resolution error: %s\n", dlerror()); \
			return EGL_FALSE; \
		}
EGL_FUNCS(FUNC, FUNC)
#undef FUNC

	asprintf((char**) &extensions, "%s %s", termuxEGL_GetVendorString(__EGL_VENDOR_STRING_PLATFORM_EXTENSIONS), android.eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS));

	TermuxNativeWindow_init();

	apiExports = exports;

	imports->getPlatformDisplay = termuxEGL_GetPlatformDisplay;
	imports->getSupportsAPI = termuxEGL_GetSupportsAPI;
	imports->getVendorString = termuxEGL_GetVendorString;
	imports->getProcAddress = termuxEGL_GetProcAddress;
	imports->getDispatchAddress = termuxEGL_GetDispatchAddress;
	imports->setDispatchIndex = termuxEGL_SetDispatchIndex;

	return EGL_TRUE;
}