#!/usr/bin/env python3
"""Stress tests for JadeHack: monster death, item creation and item use.

Three tests run inside the game in wizard mode, each driven by code in
src/allmain.c that is compiled only when its macro is defined:

  corpse  (CORPSE_STRESS_TEST, trigger file "corpsetest", log ct.log)
          creates and kills every ordinary monster type repeatedly, through
          mondied() and xkilled(), to catch crashes in make_corpse() and the
          other death handling -- for example a special case that leaves no
          corpse and then dereferences the null object, as the glue golem did.

  items   (ITEM_STRESS_TEST, trigger file "itemtest", log it.log)
          creates every kind of object many times in many states (blessed,
          cursed, eroded, greased, identified, stacked, holding things), names
          and weighs it, puts it on the floor and into your inventory and
          throws it away; makes corpses, statues, figurines, eggs and tins of
          every monster; makes every artifact; and generates random objects of
          every class.

  use     (USE_STRESS_TEST, trigger file "usetest", log ut.log)
          uses every kind of object through the real commands: wears and
          wields it, zaps, engraves with and breaks wands, reads scrolls and
          spellbooks and casts the spells, quaffs and throws potions, applies
          tools, fires launchers with every matching ammunition, eats, and
          invokes artifacts; each in several states (blessed, cursed,
          confused, blind and hallucinating), then fights a monster while
          wearing or wielding it and lets timeouts and object timers run.
          Also eats the corpse, egg and tin of every monster, and wishes for
          every object and artifact by name in four forms (plain, blessed
          and enchanted, cursed, plural), reporting any wish that gives the
          wrong object as a MISMATCH.  The game answers its own prompts.

Each step is written to the log *before* it is done, so if the game dies the
last line names the monster, object or artifact responsible; the last line is
DONE when the whole test survived.  This script builds a throwaway copy of
your build tree with the macros defined, runs the game in wizard mode on a
pseudo-terminal while dismissing every --More--, and reports the culprit and
anything written to the panic log (the game's impossible() messages).

Usage:
    util/stress_test.py BUILD_DIR [--test corpse|items|use|all]
                                  [--keys k,H,M,L,.] [--timeout SECONDS]
                                  [--keep]

BUILD_DIR must be a configured tree that has already been built from the
current sources and data files (for example the ~/jadehack-play/build tree
after a sync and a full make); it is copied, never modified.  Python 3 and a
Unix pseudo-terminal are needed.  --keys are the character selection keys for
the menu (default: Knight, human, male, lawful).  Exit status: 0 if
everything survived and the panic log stayed empty, 1 otherwise.
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

TESTS = {
    "corpse": ("corpsetest", "ct.log"),
    "items": ("itemtest", "it.log"),
    "use": ("usetest", "ut.log"),
}
MACROS = ("CORPSE_STRESS_TEST", "ITEM_STRESS_TEST", "USE_STRESS_TEST")
DATA = ("nhdat", "dungeon", "quest.dat", "data", "rumors", "oracles", "options")


def build(tree):
    cfg = os.path.join(tree, "include", "config.h")
    with open(cfg) as f:
        text = f.read()
    marker = "#define CONFIG_H"
    if marker not in text:
        sys.exit("cannot find %r in include/config.h" % marker)
    for macro in MACROS:
        if macro not in text:
            text = text.replace(marker, marker + "\n#define " + macro, 1)
    with open(cfg, "w") as f:
        f.write(text)
    r = subprocess.run(["make", "-C", os.path.join(tree, "src")],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       universal_newlines=True)
    if r.returncode != 0:
        print(r.stdout[-3000:])
        sys.exit("build failed")


def playground(tree, top, name, trigger):
    play = os.path.join(top, "play-" + name)
    os.makedirs(os.path.join(play, "save"))
    os.makedirs(os.path.join(play, "dumplog"))
    shutil.copy(os.path.join(tree, "src", "jadehack"), play)
    for fname in DATA:
        shutil.copy(os.path.join(tree, "dat", fname), play)
    for fname in ("record", "logfile", "xlogfile", "livelog", "perm",
                  "paniclog", trigger):
        open(os.path.join(play, fname), "w").close()
    return play


def read_log(play, logname):
    try:
        with open(os.path.join(play, logname)) as f:
            return f.read().strip().split("\n")
    except IOError:
        return [""]


def run(play, home, keys, timeout, logname):
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
            if read_log(play, logname)[-1] == "DONE":
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
    ap.add_argument("--test", choices=("corpse", "items", "use", "all"),
                    default="all")
    ap.add_argument("--keys", default="k,H,M,L,.")
    ap.add_argument("--timeout", type=int, default=1800,
                    help="per test, in seconds")
    ap.add_argument("--keep", action="store_true",
                    help="keep the temporary tree and playgrounds")
    args = ap.parse_args()

    top = tempfile.mkdtemp(prefix="stress-test-")
    try:
        tree = os.path.join(top, "build")
        shutil.copytree(args.build_dir, tree, symlinks=True)
        print("building with the stress test macros ...")
        build(tree)
        names = ["corpse", "items", "use"] if args.test == "all" else [args.test]
        all_ok = True
        for name in names:
            trigger, logname = TESTS[name]
            play = playground(tree, top, name, trigger)
            home = os.path.join(top, "home-" + name)
            os.makedirs(home)
            print("running the %s test ..." % name)
            status = run(play, home, args.keys.split(","), args.timeout,
                         logname)
            lines = read_log(play, logname)
            last = lines[-1]
            with open(os.path.join(play, "paniclog")) as f:
                panic = f.read().strip()
            ok = True
            if status == "timeout":
                print("  TIMED OUT; last step logged: %s" % last)
                ok = False
            elif status is not None:
                if os.WIFSIGNALED(status):
                    why = "signal %d" % os.WTERMSIG(status)
                else:
                    why = "exit status %d" % os.WEXITSTATUS(status)
                print("  GAME DIED (%s) at: %s" % (why, last))
                ok = False
            else:
                print("  survived (%d log lines)" % len(lines))
            bad = [l for l in lines if "MISMATCH" in l]
            if bad:
                print("  wishes that gave the wrong object (%d, first 20):"
                      % len(bad))
                for line in bad[:20]:
                    print("    " + line)
                ok = False
            if panic:
                culprits = []
                for i, line in enumerate(lines):
                    if "panic log grew" in line and i > 0:
                        step = lines[i - 1]
                        if step not in culprits:
                            culprits.append(step)
                print("  panic log:")
                for line in panic.split("\n")[:20]:
                    print("    " + line)
                if culprits:
                    print("  steps that wrote to the panic log (first 10):")
                    for step in culprits[:10]:
                        print("    " + step)
                ok = False
            all_ok = all_ok and ok
        return 0 if all_ok else 1
    finally:
        if args.keep:
            print("kept: " + top)
        else:
            shutil.rmtree(top, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
