package main

import (
	"fmt"
	"os"
	"io"
	"strconv"
	"syscall"
)

const (
	POE_TEMPORARY_BASE = "/tmp/poe"
)

func PlaygroundCreate(systemdir, envdir string) (string, error) {
	basedir := POE_TEMPORARY_BASE + "/" + strconv.Itoa(os.Getpid())
	if _, err := os.Stat(POE_TEMPORARY_BASE); err == nil {
		if err := os.RemoveAll(basedir); err != nil {
			return "", err
		}
	}
	if err := os.MkdirAll(basedir, 0755); err != nil {
		return "", err
	}

	if err := syscall.Mount("none", basedir, "tmpfs", syscall.MS_NOSUID, "size=32m"); err != nil {
		return "", err
	}
	workdir := basedir + "/work"
	if err := os.Mkdir(workdir, 0755); err != nil {
		return "", err
	}
	upperdir := basedir + "/upper"
	if err := os.Mkdir(upperdir, 0755); err != nil {
		return "", err
	}
	mergeddir := basedir + "/merged"
	if err := os.Mkdir(mergeddir, 0755); err != nil {
		return "", err
	}

	opts := fmt.Sprintf("lowerdir=%s:%s,upperdir=%s,workdir=%s", envdir, systemdir, upperdir, workdir)
	if err := syscall.Mount("none", mergeddir, "overlay", syscall.MS_NOSUID, opts); err != nil {
		return "", err
	}

	return mergeddir, nil
}

func PlaygroundCopy(rootdir string, file string) (string, error) {
	in, err := os.Open(file)
	if err != nil {
		return "", err
	}
	defer in.Close()
	out, err := os.Create(rootdir + "/tmp/prog") // TODO
	if err != nil {
		return "", err
	}
	defer out.Close()
	if _, err := io.Copy(out, in); err != nil {
		return "", err
	}
	return "/tmp/prog", out.Sync()
}

func PlaygroundDestroy() error {
	pid := os.Getpid()
	if pid == 1 {
		return fmt.Errorf("not parent process?")
	}

	basedir := POE_TEMPORARY_BASE + "/" + strconv.Itoa(os.Getpid())
	if _, err := os.Stat(basedir); err != nil {
		return nil
	}

	mergeddir := basedir + "/merged"
	syscall.Unmount(mergeddir, syscall.MNT_DETACH) // ignore error
	syscall.Unmount(basedir, syscall.MNT_DETACH)   // ignore error

	os.RemoveAll(basedir)                          // ignore error

	return nil
}
