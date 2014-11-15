/* Written by Artem Falcon <lomka@gero.in> */

#ifndef _KERNEL_MODE
#error "They only support kernel mode ACPI. Sigh :("
#endif

#include <stdlib.h>

#include <ACPI.h>
#include "acpi_fujitsu_common.h"
#include <driver_settings.h>

//#define TRACE_FJL 1
#ifdef TRACE_FJL
#       define TRACE(x...) dprintf("acpi_fujitsu_laptop: " x)
#else
#       define TRACE(x...) ;
#endif

#define ACPI_FJL_MODULE_NAME "drivers/power/acpi_fujitsu_laptop/driver_v1"
#define ACPI_FJL_DEVICE_MODULE_NAME "drivers/power/acpi_fujitsu_laptop/device_v1"

/* Base Namespace devices are published to */
#define ACPI_FJL_BASENAME "power/acpi_fujitsu_laptop/%d"

// name of pnp generator of path ids
#define ACPI_FJL_PATHID_GENERATOR "acpi_fujitsu_laptop/path_id"

static device_manager_info *sDeviceManager;

typedef struct acpi_ns_device_info {
	device_node *node;
	acpi_device_module_info *acpi;
	acpi_device acpi_cookie;
} acpi_fjl_device_info;

typedef struct {
	int backlight_level;
} acpi_fujitsu_settings;

/* bypass device/driver separation */
struct {
	acpi_module_info *acpi;
	struct {
		char *path;
		char *dev_name;
		char *what;

		const char *hid_dev_name;
		//const char *htk_dev_name;
	} full_query;
	unsigned int max_brightness;
	status_t (*set_lcd_level)(int);
} acpi_fujitsu;

static const char *fjl_get_full_query(const char *, char *const);
static status_t fjl_get_settings(acpi_fujitsu_settings *);
static int fjl_hw_get_max_brightness(void);
static int fjl_hw_get_lcd_level(void);

static status_t fjl_set_lcd_level_dumb(int);
static status_t fjl_hw_set_lcd_level(int);

//	#pragma mark -
// device module API


static status_t
acpi_fjl_init_device(void *_cookie, void **cookie)
{
	device_node *node = (device_node *)_cookie;
	acpi_fjl_device_info *device;
	device_node *parent;

	device = (acpi_fjl_device_info *)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;

	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);
	sDeviceManager->put_node(parent);

	*cookie = device;
	return B_OK;
}


static void
acpi_fjl_uninit_device(void *_cookie)
{
	acpi_fjl_device_info *device = (acpi_fjl_device_info *)_cookie;
	free(device);
}


static status_t
acpi_fjl_open(void *_cookie, const char *path, int flags, void** cookie)
{
	acpi_fjl_device_info *device = (acpi_fjl_device_info *)_cookie;
	*cookie = device;
	return B_OK;
}


static status_t
acpi_fjl_read(void* _cookie, off_t position, void *buf, size_t* num_bytes)
{
	return B_ERROR;
}


static status_t
acpi_fjl_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	return B_ERROR;
}


static status_t
acpi_fjl_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	//acpi_fjl_device_info* device = (acpi_fjl_device_info*)_cookie;

	switch (op) {
		case GET_BACKLIGHT_LIMIT: {
			return user_memcpy(arg, &acpi_fujitsu.max_brightness, sizeof(unsigned int));
		}
		case GET_BACKLIGHT_LEVEL: {
			int level = fjl_hw_get_lcd_level();
			return user_memcpy(arg, &level, sizeof(int));
		}
		case SET_BACKLIGHT_LEVEL: {
			return acpi_fujitsu.set_lcd_level(*(int*)arg);
		}
	}

	return B_DEV_INVALID_IOCTL;
}


static status_t
acpi_fjl_close (void* cookie)
{
	return B_OK;
}


static status_t
acpi_fjl_free (void* cookie)
{
	return B_OK;
}


//	#pragma mark -
// driver module API


static float
acpi_fjl_support(device_node *parent)
{
	const char *bus;
	uint32 device_type;
	const char *path;

	// make sure parent is really the ACPI bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "acpi"))
		return 0.0f;

	// check whether it's really a device
	if (sDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM, &device_type, false) != B_OK
		|| device_type != ACPI_TYPE_DEVICE) {
		return 0.0f;
	}

	// check whether it's our device
	if (sDeviceManager->get_attr_string(parent, /*ACPI_DEVICE_HID_ITEM*/ ACPI_DEVICE_PATH_ITEM
		, &path, false) == B_OK
			// parsing of string-valued HID is broken on Haiku, at least with my laptop :(
			/*|| strcmp(hid, "FUJ02B1")*/) {
		//TRACE("ACPI path: %s\n", path);

		if ((acpi_fujitsu.full_query.hid_dev_name = strstr(path, "FJEX"))) { // backlight control
			dprintf("acpi_fujitsu_laptop: Found supported device: %s\n",
				acpi_fujitsu.full_query.hid_dev_name);

			acpi_fujitsu.full_query.path = (char *) path;
			return 0.8f;
		}
		/*else
			acpi_fujitsu.full_query.htk_dev_name = strstr(path,
				"FEXT"*/ /* aka FUJ02E3, hotkeys device */ /*);*/
	}

	return 0.0f;
}


static status_t
acpi_fjl_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "Fujitsu-Siemens Laptop ACPI driver" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, ACPI_FJL_MODULE_NAME, attrs, NULL, NULL);
}


static status_t
acpi_fjl_init_driver(device_node *node, void **_driverCookie)
{
	char *prefix;
	acpi_handle handle;
	acpi_fujitsu_settings settings;

	*_driverCookie = node;
	TRACE("init_driver()\n");

	/* path prefix, required cause of lack of cookie?! */
	prefix = strcat(acpi_fujitsu.full_query.path, ".ABCD");

	acpi_fujitsu.full_query.dev_name = strstr(prefix, acpi_fujitsu.full_query.hid_dev_name);
	acpi_fujitsu.full_query.what = strstr(acpi_fujitsu.full_query.dev_name, "ABCD");

	acpi_fujitsu.full_query.path = prefix;

	if (fjl_hw_get_max_brightness() < 0)
		return B_ERROR;

	if (acpi_fujitsu.acpi->get_handle(NULL, fjl_get_full_query(acpi_fujitsu.full_query.hid_dev_name,
		"SBLL"), &handle) != B_OK) {
		acpi_fujitsu.set_lcd_level = fjl_set_lcd_level_dumb;
		TRACE("SBLL wasn't found\n");
	}
	else {
		acpi_fujitsu.set_lcd_level = fjl_hw_set_lcd_level;

		memset(&settings, 0, sizeof(acpi_fujitsu_settings));
		/* load driver settings and set them */
		if (fjl_get_settings(&settings) == B_OK) {
			if (settings.backlight_level < 0 ||
				settings.backlight_level >= acpi_fujitsu.max_brightness) {
				TRACE("backlight_level value is out the range supported by device\n");
			} else
				acpi_fujitsu.set_lcd_level(settings.backlight_level);
		}
	}

	/*fjl_hw_get_lcd_level();*/
	//TRACE("HTK: %s\n", acpi_fujitsu.full_query.htk_dev_name);

	return B_OK;
}


static void
acpi_fjl_uninit_driver(void *driverCookie)
{
	//TRACE("uninit_driver()\n");
}


static status_t
acpi_fjl_register_child_devices(void *_cookie)
{
	device_node *node = _cookie;
	int path_id;
	char name[128];

	TRACE("register_child_devices()\n");

	path_id = sDeviceManager->create_id(ACPI_FJL_PATHID_GENERATOR);
	if (path_id < 0) {
		TRACE("register_child_devices(): couldn't create a path_id\n");
		return B_ERROR;
	}

	snprintf(name, sizeof(name), ACPI_FJL_BASENAME, path_id);

	return sDeviceManager->publish_device(node, name, ACPI_FJL_DEVICE_MODULE_NAME);
}


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{ B_ACPI_MODULE_NAME, (module_info **)&acpi_fujitsu.acpi },
	{}
};


driver_module_info acpi_fjl_driver_module = {
	{
		ACPI_FJL_MODULE_NAME,
		0,
		NULL
	},

	acpi_fjl_support,
	acpi_fjl_register_device,
	acpi_fjl_init_driver,
	acpi_fjl_uninit_driver,
	acpi_fjl_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info acpi_fjl_device_module = {
	{
		ACPI_FJL_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	acpi_fjl_init_device,
	acpi_fjl_uninit_device,
	NULL,

	acpi_fjl_open,
	acpi_fjl_close,
	acpi_fjl_free,
	acpi_fjl_read,
	acpi_fjl_write,
	NULL,
	acpi_fjl_control,

	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&acpi_fjl_driver_module,
	(module_info *)&acpi_fjl_device_module,
	NULL
};

/*
void c------------------------------() {}
*/

static const char *fjl_get_full_query(const char *dev, char *const what)
{
	// broken, panics on query from userland, won't fix
	//strncpy(acpi_fujitsu.full_query.dev_name, dev, strlen(dev));
	strcpy(acpi_fujitsu.full_query.what, what);

	return acpi_fujitsu.full_query.path;
}
static status_t fjl_get_settings(acpi_fujitsu_settings *settings)
{
	void *handle;

	if ((handle = load_driver_settings("acpi_fujitsu_laptop"))) {
		const char *str = get_driver_parameter(handle, "backlight_level", NULL, NULL);
		if (str)
			settings->backlight_level = atoi(str);

		unload_driver_settings(handle);
		return B_OK;
	}

	return B_ERROR;
}

static int fjl_hw_get_max_brightness(void)
{
	acpi_object_type arg0;
	acpi_data arg_data;

	arg_data.pointer = &arg0;
	arg_data.length = sizeof(acpi_object_type);

	if (acpi_fujitsu.acpi->evaluate_method(NULL,
		fjl_get_full_query(acpi_fujitsu.full_query.hid_dev_name, "RBLL"), NULL, &arg_data) != B_OK
		|| arg0.object_type != ACPI_TYPE_INTEGER)
		return -1;

	TRACE("fjl_hw_get_max_brightness() = %d\n",
		(int) arg0.data.integer);

	return acpi_fujitsu.max_brightness = arg0.data.integer;
}
static int fjl_hw_get_lcd_level(void)
{
	unsigned int level;
	acpi_object_type arg0;
	acpi_data arg_data;

	//bool brightness_changed;

	arg_data.pointer = &arg0;
	arg_data.length = sizeof(acpi_object_type);

	if (acpi_fujitsu.acpi->evaluate_method(NULL,
		fjl_get_full_query(acpi_fujitsu.full_query.hid_dev_name, "GBLL"), NULL, &arg_data) != B_OK
		|| arg0.object_type != ACPI_TYPE_INTEGER)
		return -1;

	level = arg0.data.integer & 0x0fffffff;
	//brightness_changed = (arg0.data.integer & 0x80000000) ? true : false;

	TRACE("fjl_hw_get_lcd_level()"/* { brightness_changed = %d }*/" = %d\n",
		/*brightness_changed,*/ level);

	return level;
}

static status_t fjl_set_lcd_level_dumb(int level)
{
	return B_ERROR;
}
static status_t fjl_hw_set_lcd_level(int level)
{
	acpi_object_type arg0;
	acpi_objects arg_list;

	arg0.object_type = ACPI_TYPE_INTEGER;
	arg0.data.integer = level;
	arg_list.count = 1;
	arg_list.pointer = &arg0;

	TRACE("fjl_hw_set_lcd_level(level = %d"/*, path = %s*/")\n", level
		/*,acpi_fujitsu.full_path*/);

	return acpi_fujitsu.acpi->evaluate_method(NULL,
		fjl_get_full_query(acpi_fujitsu.full_query.hid_dev_name, "SBLL"), &arg_list, NULL);
}
