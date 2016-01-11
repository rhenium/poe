#include "sandbox.h"
#include <systemd/sd-bus.h>
#include <systemd/sd-login.h>

static const char *dbus_name = "org.freedesktop.systemd1";
static const char *dbus_path = "/org/freedesktop/systemd1";
static const char *systemd_manager = "org.freedesktop.systemd1.Manager";

void
poe_init_systemd(pid_t pid)
{
    char *unit = NULL;
    asprintf(&unit, "poe-sandbox-%d.scope", pid);

    sd_bus *connection;
    sd_bus_open_system(&connection);

    sd_bus_message *message = NULL;
    sd_bus_message_new_method_call(connection, &message, dbus_name, dbus_path, systemd_manager, "StartTransientUnit");

    sd_bus_message_append(message, "ss", unit, "fail");
    sd_bus_message_open_container(message, 'a', "(sv)");
    sd_bus_message_append(message, "(sv)", "PIDs", "au", 1, pid);
    sd_bus_message_append(message, "(sv)", "Description", "s", unit);
    sd_bus_message_append(message, "(sv)", "MemoryLimit", "t", POE_MEMORY_LIMIT);
    sd_bus_message_append(message, "(sv)", "TasksMax", "t", POE_TASKS_LIMIT);
    sd_bus_message_append(message, "(sv)", "DevicePolicy", "s", "strict");
    sd_bus_message_append(message, "(sv)", "CPUAccounting", "b", 1);
    sd_bus_message_append(message, "(sv)", "BlockIOAccounting", "b", 1);
    sd_bus_message_close_container(message);
    sd_bus_message_append(message, "a(sa(sv))", 0);

    sd_bus_error error = SD_BUS_ERROR_NULL;
    int rc = sd_bus_call(connection, message, 0, &error, NULL);
    if (rc < 0) ERROR("sd_bus_call() failed: %s", sd_bus_error_is_set(&error) ? error.message : strerror(-rc));

    sd_bus_message_unref(message);

    // unit が設定できたか？
    while (true) {
        char *gunit;
        sd_pid_get_unit(pid, &gunit);
        bool cmp = strcmp(unit, gunit);
        free(gunit);
        if (cmp) break;
    }
}
/*
void
poe_stop_unit(sd_bus *con, const char *unit)
{
    sd_bus_error error = SD_BUS_ERROR_NULL;
    int rc = sd_bus_call_method(connection, systemd_bus_name, systemd_path_name, manager_interface,
                                "StopUnit", &error, NULL, "ss", unit_name, "fail");
    if (rc < 0) {
        if (sd_bus_error_is_set(&error)) {
            // NoSuchUnit errors are expected as the contained processes can die at any point.
            if (strcmp(error.name, "org.freedesktop.systemd1.NoSuchUnit"))
                errx(EXIT_FAILURE, "%s", error.message);
            sd_bus_error_free(&error);
        } else
            errx(EXIT_FAILURE, "%s", strerror(-rc));
    }
}*/
