#include <string.h>
#include <driver_settings.h>
#include <safemode_defs.h>

#define TRACE_ACPI 1
#ifdef TRACE_ACPI
extern "C" void _sPrintf(const char* format, ...);
#	define TRACE(x) _sPrintf("intel_extreme: ACPI: %s\n", x)
#else
#	define TRACE(x) ;
#endif

extern "C" status_t _kern_get_safemode_option(const char* parameter,
        char* buffer, size_t* _bufferSize);
static void start();

void intel_acpi_init()
{
	void *settings;
	bool acpiEnabled = true;

	char parameter[32];
	size_t parameterLength = sizeof(parameter);

	/* no syscall to replace this as yet */
	if ((settings = load_driver_settings("kernel"))) {
		// is values == { "0" }
		acpiEnabled = get_driver_boolean_parameter(settings, "acpi", true, true);

		unload_driver_settings(settings);
	}
	/* */
	if (acpiEnabled)
		if (_kern_get_safemode_option(B_SAFEMODE_DISABLE_ACPI, parameter,
			&parameterLength) == B_OK) {
			if (!strcasecmp(parameter, "enabled") || !strcasecmp(parameter, "on")
				|| !strcasecmp(parameter, "true") || !strcasecmp(parameter, "yes")
				|| !strcasecmp(parameter, "enable") || !strcmp(parameter, "1"))
				acpiEnabled = false;
		}

	if (acpiEnabled) {
		_sPrintf("intel_extreme: ACPI mode is enabled. Registering extensions.\n");
		start();
	}
}

static void start()
{
}