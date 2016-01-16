package main

import (
	"fmt"
	sdbus "github.com/coreos/go-systemd/dbus"
	"github.com/godbus/dbus"
)

var (
	conn *sdbus.Conn
	unit string
)

func InitializeSystemdBus() error {
	var err error
	conn, err = sdbus.New()
	return err
}

func SetupScope(pid uint32) error {
	unit = fmt.Sprintf("poe-sandbox-%d.scope", pid)
	props := []sdbus.Property{
		{"PIDs", dbus.MakeVariant([]uint32{pid})},
		{"Description", dbus.MakeVariant(fmt.Sprintf("poe sandbox (pid: %d)", pid))},
		{"MemoryLimit", dbus.MakeVariant(uint64(1024 * 1024 * 128))},
		{"TasksMax", dbus.MakeVariant(uint64(16))},
		{"CPUShares", dbus.MakeVariant(uint64(512))},
		{"BlockIOWeight", dbus.MakeVariant(uint64(10))},
		{"DevicePolicy", dbus.MakeVariant("strict")},
	}

	if _, err := conn.StartTransientUnit(unit, "fail", props, nil); err != nil {
		return err
	} else {
		return waitScopeToBeCreated(pid, unit)
	}
}

func StopScope() error {
	if unit == "" {
		return nil
	}
	conn.StopUnit(unit, "fail", nil)
	return nil // TODO: ignore no such (may be exited early)
}

func waitScopeToBeCreated(pid uint32, unit string) error {
	for {
		// TODO: sd_pid_get_unit
		return nil
	}
}
