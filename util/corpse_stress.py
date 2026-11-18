#!/usr/bin/env python3
"""Corpse and death stress test for JadeHack.

Creates and kills every ordinary monster type many times in wizard mode
(through mondied() and xkilled()), to catch crashes in make_corpse() and the
other death handling -- for example a special case that leaves no corpse and
then dereferences the null object, as the glue golem once did.

How it works: src/allmain.c contains a function, compiled only when
CORPSE_STRESS_TEST is defined, that runs when the playground contains a file
named "corpsetest".  It logs each monster to ct.log *before* killing it, and
writes DONE as the last line when everything survived.  This script builds a
throwaway copy of your build tree with that macro defined, runs the game in
wizard mode on a pseudo-terminal, keeps dismissing --More-- prompts, and
reports the monster that was being killed if the game dies.

Usage:
    util/corpse_stress.py BUILD_DIR [--keys k,H,M,L,.] [--timeout SECONDS]
                                    [--keep]

BUILD_DIR must be a configured tree that has already been built from the
current sources and data files (for example the ~/jadehack-play/build tree
after a sync and a full make); it is copied, never modified.  Python 3 and a
Unix pseudo-terminal are needed.  --keys are the character selection keys for
the menu (default: Knight, human, male, lawful).  Exit status: 0 if every
monster survived and the panic log stayed empty, 1 otherwise.
"""
import argparse
import fcntl
import os
import pty
import select
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import termios
import time

DATA = ("nhdat", "dungeon", "quest.dat", "data", "rumors", "oracles", "options")


def build(tree):
    cfg = os.path.join(tree, "include", "config.h")
    with open(cfg) as f:
        text = f.read()
    marker = "#define CONFIG_H"
    if marker not in text:
        sys.exit("cannot find %r in include/config.h" % marker)
    if "CORPSE_STRESS_TEST" not in text:
        text = text.replace(marker, marker + "\n#define CORPSE_STRESS_TEST", 1)
        with open(cfg, "w") as f:
            f.write(text)
    r = subprocess.run(["make", "-C", os.path.join(tree, "src")],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       universal_newlines=True)
    if r.returncode != 0:
        print(r.stdout[-3000:])
        sys.exit("build failed")


def playground(tree, top):
    play = os.path.join(top, "play")
    os.makedirs(os.path.join(play, "save"))
    os.makedirs(os.path.join(play, "dumplog"))
    shutil.copy(os.path.join(tree, "src", "jadehack"), play)
    for name in DATA:
        shutil.copy(os.path.join(tree, "dat", name), play)
    for name in ("record", "logfile", "xlogfile", "livelog", "perm",
                 "paniclog", "corpsetest"):
        open(os.path.join(play, name), "w").close()
    return play


def read_log(play):
    try:
        with open(os.path.join(play, "ct.log")) as f:
            return f.read().strip().split("\n")
    except IOError:
        return [""]


def run(play, home, keys, timeout):
    env = dict(os.environ, TERM="xterm", HOME=home, JADEHACKDIR=play,
               LINES="24", COLUMNS="80")
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(play)
        os.execve(os.path.join(play, "jadehack"),
                  [os.path.join(play, "jadehack"), "-D", "-uwizard"], env)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 80, 0, 0))

    def pump(t):
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.02)
            if r:
                try:
                    if not os.read(fd, 65536):
                        return
                except OSError:
                    return

    pump(1.5)
    for k in keys:
        os.write(fd, k.encode())
        pump(0.8)
    start = time.time()
    status = None
    ticks = 0
    while time.time() - start < timeout:
        try:
            os.write(fd, b" ")      # dismiss --More-- and the like
        except OSError:
            pass
        pump(0.03)
        ticks += 1
        if ticks % 40 == 0:
            r, st = os.waitpid(pid, os.WNOHANG)
            if r:
                status = st
                break
            if read_log(play)[-1] == "DONE":
                break
    else:
        status = "timeout"
    try:
        os.kill(pid, signal.SIGKILL)
        os.waitpid(pid, 0)
    except OSError:
        pass
    return status


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("build_dir")
    ap.add_argument("--keys", default="k,H,M,L,.")
    ap.add_argument("--timeout", type=int, default=1800)
    ap.add_argument("--keep", action="store_true",
                    help="keep the temporary tree and playground")
    args = ap.parse_args()

    top = tempfile.mkdtemp(prefix="corpse-stress-")
    try:
        tree = os.path.join(top, "build")
        shutil.copytree(args.build_dir, tree, symlinks=True)
        print("building with CORPSE_STRESS_TEST ...")
        build(tree)
        play = playground(tree, top)
        home = os.path.join(top, "home")
        os.makedirs(home)
        print("running (every monster, killed repeatedly) ...")
        status = run(play, home, args.keys.split(","), args.timeout)
        last = read_log(play)[-1]
        with open(os.path.join(play, "paniclog")) as f:
            panic = f.read().strip()
        ok = True
        if status == "timeout":
            print("TIMED OUT; last monster logged: %s" % last)
            ok = False
        elif status is not None:
            if os.WIFSIGNALED(status):
                why = "signal %d" % os.WTERMSIG(status)
            else:
                why = "exit status %d" % os.WEXITSTATUS(status)
            print("GAME DIED (%s) while handling: %s" % (why, last))
            ok = False
        else:
            print("every monster survived (%d log lines)"
                  % len(read_log(play)))
        if panic:
            print("panic log:\n" + panic)
            ok = False
        return 0 if ok else 1
    finally:
        if args.keep:
            print("kept: " + top)
        else:
            shutil.rmtree(top, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
