#include "sandbox.h"

static const char *sdbus_name = "org.freedesktop.systemd1";
static const char *sdbus_path = "/org/freedesktop/systemd1";
static const char *sdbus_manager = "org.freedesktop.systemd1.Manager";

static char *unit = NULL;
static sd_bus *connection = NULL;

void
poe_init_systemd(pid_t pid)
{
    NONNEGATIVE(asprintf(&unit, "poe-sandbox-%d.scope", pid));

    NONNEGATIVE(sd_bus_open_system(&connection));

    sd_bus_message *message = NULL;
    NONNEGATIVE(sd_bus_message_new_method_call(connection, &message, sdbus_name, sdbus_path, sdbus_manager, "StartTransientUnit"));

    NONNEGATIVE(sd_bus_message_append(message, "ss", unit, "fail"));
    NONNEGATIVE(sd_bus_message_open_container(message, 'a', "(sv)"));
    NONNEGATIVE(sd_bus_message_append(message, "(sv)", "PIDs",                 "au", 1, pid));
    NONNEGATIVE(sd_bus_message_append(message, "(sv)", "Description",          "s", unit));
    NONNEGATIVE(sd_bus_message_append(message, "(sv)", "MemoryLimit",          "t", POE_MEMORY_LIMIT));
    NONNEGATIVE(sd_bus_message_append(message, "(sv)", "TasksMax",             "t", POE_TASKS_LIMIT));
    NONNEGATIVE(sd_bus_message_append(message, "(sv)", "CPUShares",            "t", 512ULL));
    NONNEGATIVE(sd_bus_message_append(message, "(sv)", "BlockIOWeight",        "t", 10ULL));
    NONNEGATIVE(sd_bus_message_append(message, "(sv)", "DevicePolicy",         "s", "strict"));
    NONNEGATIVE(sd_bus_message_close_container(message));
    NONNEGATIVE(sd_bus_message_append(message, "a(sa(sv))", 0));

    sd_bus_error error = SD_BUS_ERROR_NULL;
    int rc = sd_bus_call(connection, message, 0, &error, NULL);
    if (rc < 0) ERROR("sd_bus_call() failed: %s", sd_bus_error_is_set(&error) ? error.message : strerror(-rc));

    sd_bus_message_unref(message);

    // unit が設定できたか？
    while (true) {
        char *gunit;
        NONNEGATIVE(sd_pid_get_unit(pid, &gunit));
        bool cmp = strcmp(unit, gunit);
        free(gunit);
        if (!cmp) break;
    }
}

void
poe_exit_systemd(void)
{
    if (!unit || !connection) return;

    sd_bus_message *message = NULL;
    sd_bus_message_new_method_call(connection, &message, sdbus_name, sdbus_path, sdbus_manager, "StopUnit");
    sd_bus_message_append(message, "ss", unit, "fail");

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_call(connection, message, 0, &error, NULL); // ignore error
    sd_bus_message_unref(message);

    sd_bus_unref(connection);
    free(unit);
}
