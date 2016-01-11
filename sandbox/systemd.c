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

#define test(val) if((val) == -1) ERROR("sd_bus function failed")
    test(sd_bus_open_system(&connection));

    sd_bus_message *message = NULL;
    test(sd_bus_message_new_method_call(connection, &message, sdbus_name, sdbus_path, sdbus_manager, "StartTransientUnit"));

    test(sd_bus_message_append(message, "ss", unit, "fail"));
    test(sd_bus_message_open_container(message, 'a', "(sv)"));
    test(sd_bus_message_append(message, "(sv)", "PIDs",                 "au", 1, pid));
    test(sd_bus_message_append(message, "(sv)", "Description",          "s", unit));
    test(sd_bus_message_append(message, "(sv)", "MemoryLimit",          "t", POE_MEMORY_LIMIT));
    test(sd_bus_message_append(message, "(sv)", "TasksMax",             "t", POE_TASKS_LIMIT));
    test(sd_bus_message_append(message, "(sv)", "CPUShares",            "t", 512ULL));
    test(sd_bus_message_append(message, "(sv)", "BlockIOWeight",        "t", 10ULL));
    test(sd_bus_message_append(message, "(sv)", "DevicePolicy",         "s", "strict"));
    test(sd_bus_message_close_container(message));
    test(sd_bus_message_append(message, "a(sa(sv))", 0));

    sd_bus_error error = SD_BUS_ERROR_NULL;
    int rc = sd_bus_call(connection, message, 0, &error, NULL);
    if (rc < 0) ERROR("sd_bus_call() failed: %s", sd_bus_error_is_set(&error) ? error.message : strerror(-rc));

    sd_bus_message_unref(message);

    // unit が設定できたか？
    while (true) {
        char *gunit;
        test(sd_pid_get_unit(pid, &gunit));
        bool cmp = strcmp(unit, gunit);
        free(gunit);
        if (cmp) break;
    }
#undef test
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
}
