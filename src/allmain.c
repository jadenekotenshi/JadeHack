/*	SCCS Id: @(#)allmain.c	3.4	2003/04/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* various code that was replicated in *main.c */

#include "hack.h"

#ifndef NO_SIGNAL
#include <signal.h>
#endif

#ifdef POSITIONBAR
STATIC_DCL void NDECL(do_positionbar);
#endif

#ifdef OVL0

static long prev_dgl_extrainfo = 0;


#if defined(CORPSE_STRESS_TEST) || defined(ITEM_STRESS_TEST) || defined(USE_STRESS_TEST)
#include <sys/stat.h>
/* note in the stress test log when the panic log grew since the last call,
   so that an impossible() message can be tied to the step that caused it */
STATIC_OVL void
stress_note_panic(f)
FILE *f;
{
    static long last = 0;
    struct stat st;

    if (stat("paniclog", &st) == 0 && st.st_size != last) {
	fprintf(f, "  ^ panic log grew here\n");
	fflush(f);
	last = st.st_size;
    }
}

/* monsters the stress tests leave alone: uniques and the monsters with
   special roles, which have side effects when they are made or die */
STATIC_OVL boolean
stress_skip_pm(i)
int i;
{
    return (boolean) ((mons[i].geno & G_UNIQ) ||
	    mons[i].msound == MS_LEADER || mons[i].msound == MS_NEMESIS ||
	    mons[i].msound == MS_GUARDIAN ||
	    i == PM_ORACLE || i == PM_PRIEST || i == PM_ALIGNED_PRIEST ||
	    i == PM_HIGH_PRIEST || i == PM_SHOPKEEPER || i == PM_GUARD ||
	    i == PM_WATCHMAN || i == PM_WATCH_CAPTAIN);
}
#endif

#ifdef CORPSE_STRESS_TEST
/*
 * Corpse stress test (see util/stress_test.py; not compiled normally).
 * In wizard mode, if a file named "corpsetest" exists in the playground,
 * create and kill every ordinary monster type many times through
 * mondied() and xkilled(), logging the monster to "ct.log" first, so a
 * crash in death handling or make_corpse() points at the guilty monster.
 * The last line of ct.log is DONE when the whole run completed.
 */
#include <unistd.h>
#ifndef CT_MONDIED_REPS
#define CT_MONDIED_REPS 12
#endif
#ifndef CT_XKILLED_REPS
#define CT_XKILLED_REPS 2
#endif

STATIC_OVL void
corpse_stress_test()
{
    int i, k;
    FILE *f = fopen("ct.log", "w");
    struct monst *m, *nx;
    struct obj *o;

    if (!f) return;
    for (i = LOW_PM; i < NUMMONS; i++) {
	if (stress_skip_pm(i)) continue;
	for (k = 0; k < CT_MONDIED_REPS; k++) {
	    fprintf(f, "%d %s %d mondied\n", i, mons[i].mname, k);
	    fflush(f);
	    m = makemon(&mons[i], u.ux, u.uy, NO_MM_FLAGS);
	    if (!m) continue;
	    m->mrevived = (k % 5 == 0);	/* also try revived monsters */
	    mondied(m);
	    stress_note_panic(f);
	    while ((o = level.objects[u.ux][u.uy]) != 0) delobj(o);
	    for (m = fmon; m; m = m->nmon)	/* leftovers from groups */
		if (!DEADMONSTER(m) && m != u.usteed && !m->mtame) {
		    mongone(m);
		    break;
		}
	}
	for (k = 0; k < CT_XKILLED_REPS; k++) {
	    fprintf(f, "%d %s %d xkilled\n", i, mons[i].mname, k);
	    fflush(f);
	    m = makemon(&mons[i], u.ux, u.uy, NO_MM_FLAGS);
	    if (!m || (m->mx == u.ux && m->my == u.uy)) continue;
	    xkilled(m, 1);
	    stress_note_panic(f);
	    while ((o = level.objects[m->mx][m->my]) != 0) delobj(o);
	}
	for (m = fmon; m; m = nx) {
	    nx = m->nmon;
	    if (!DEADMONSTER(m) && !m->mtame) mongone(m);
	}
	dmonsfree();
    }
    fprintf(f, "DONE\n");
    fclose(f);
}
#endif /* CORPSE_STRESS_TEST */

#ifdef ITEM_STRESS_TEST
/*
 * Item stress test (see util/stress_test.py; not compiled normally).  In
 * wizard mode, if a file named "itemtest" exists in the playground, create
 * every kind of object many times in many states (blessed, cursed, eroded,
 * greased, identified, stacked, with contents), then name it, weigh it,
 * place it on the floor, pick it up and throw it away; make corpses,
 * statues, figurines, eggs and tins of every monster; make every artifact;
 * and generate random objects of every class.  Each step is logged to
 * "it.log" first, so a crash points at the guilty object; the last line is
 * DONE if the whole run survived.
 */
#ifndef CORPSE_STRESS_TEST
#include <unistd.h>
#endif
#ifndef IT_REPS
#define IT_REPS 16
#endif
#ifndef IT_RANDOM_REPS
#define IT_RANDOM_REPS 1500
#endif

/* name, weigh and handle an object in several ways, then dispose of it */
STATIC_OVL void
item_exercise(o, k)
struct obj *o;
int k;
{
    struct obj *inv, *c;
    int j;
    const char *nm;

    nm = xname(o);
    nm = doname(o);
    nm = distant_name(o, xname);
    nm = makeplural(xname(o));
    nm = makesingular(nm);
    nm = an(simple_typename(o->otyp));
    (void) weight(o);
    (void) obj_resists(o, 5, 50);
    (void) nm;

    if (Is_container(o) && o->otyp != BAG_OF_TRICKS) {
	for (j = 0; j < 3; j++) {
	    c = mkobj(RANDOM_CLASS, TRUE);
	    if (c && !Is_container(c)) (void) add_to_container(o, c);
	    else if (c) obfree(c, (struct obj *)0);
	}
	o->owt = weight(o);
	nm = doname(o);
	(void) nm;
    }
    if (k % 2) {
	place_object(o, u.ux, u.uy);
	stackobj(o);
	obj_extract_self(o);
    }
    if (k % 3 == 0) {
	inv = addinv(o);
	nm = doname(inv);
	(void) nm;
	freeinv(inv);
	o = inv;
    }
    if (Has_contents(o)) delete_contents(o);
    obfree(o, (struct obj *)0);
}

STATIC_OVL void
item_stress_test()
{
    FILE *f = fopen("it.log", "w");
    int otyp, k, i, cls;
    short at;
    struct obj *o;
    const char *an_;

    if (!f) return;

    /* every kind of object, many states */
    for (otyp = 1; otyp < NUM_OBJECTS; otyp++) {
	if (!OBJ_NAME(objects[otyp])) continue;
	for (k = 0; k < IT_REPS; k++) {
	    fprintf(f, "object %d %s %d\n", otyp, OBJ_NAME(objects[otyp]), k);
	    fflush(f);
	    o = mksobj(otyp, TRUE, (k & 1) ? TRUE : FALSE);
	    if (!o) continue;
	    switch (k % 4) {
		case 1: o->blessed = 1; o->cursed = 0; break;
		case 2: o->cursed = 1; o->blessed = 0; break;
		case 3: o->blessed = o->cursed = 0; break;
		default: break;
	    }
	    o->dknown = o->bknown = o->known = o->rknown = (k & 2) ? 1 : 0;
	    if (objects[otyp].oc_class == WEAPON_CLASS ||
		objects[otyp].oc_class == ARMOR_CLASS ||
		objects[otyp].oc_class == RING_CLASS)
		o->spe = (k % 7) - 2;
	    if (is_damageable(o)) {
		o->oeroded = k % 4;
		o->oeroded2 = (k / 2) % 4;
		o->oerodeproof = (k % 5 == 0);
	    }
	    o->greased = (k % 6 == 0);
	    if (objects[otyp].oc_merge) o->quan = 1 + k % 3;
	    if ((k % 4) == 3 && (objects[otyp].oc_class == WEAPON_CLASS ||
				 objects[otyp].oc_class == ARMOR_CLASS))
		(void) create_oprop(o, TRUE);
	    o->owt = weight(o);
	    item_exercise(o, k);
	    stress_note_panic(f);
	}
    }

    /* corpses, statues, figurines, eggs and tins of every monster */
    for (i = LOW_PM; i < NUMMONS; i++) {
	static const int mk[] = { CORPSE, STATUE, FIGURINE, EGG, TIN };
	int t;

	for (t = 0; t < 5; t++)
	    for (k = 0; k < 4; k++) {
		fprintf(f, "monster object %d %s kind %d %d\n", i,
			mons[i].mname, mk[t], k);
		fflush(f);
		o = mksobj(mk[t], FALSE, FALSE);
		if (!o) continue;
		o->corpsenm = i;
		if (mk[t] == TIN) o->spe = (k == 1) ? -1 : (k == 2) ? 1 : 0;
		o->dknown = o->known = (k & 1);
		o->quan = 1;
		o->owt = weight(o);
		item_exercise(o, k);
		stress_note_panic(f);
	    }
	fprintf(f, "make corpse %d %s\n", i, mons[i].mname);
	fflush(f);
	o = mkcorpstat(CORPSE, (struct monst *)0, &mons[i], u.ux, u.uy, TRUE);
	if (o) {
	    (void) doname(o);
	    delobj(o);
	}
    }

    /* every artifact */
    for (i = 1; i <= NROFARTIFACTS; i++) {
	an_ = artiname(i);
	if (!an_ || !*an_) continue;
	if (!artifact_name(an_, &at)) continue;
	for (k = 0; k < 4; k++) {
	    fprintf(f, "artifact %d %s %d\n", i, an_, k);
	    fflush(f);
	    o = mksobj((int) at, TRUE, FALSE);
	    if (!o) continue;
	    o = oname(o, an_);
	    if (k & 1) o->blessed = 1, o->cursed = 0;
	    o->dknown = o->bknown = o->known = (k >= 2);
	    o->owt = weight(o);
	    item_exercise(o, k);
	    stress_note_panic(f);
	}
    }

    /* random objects of every class */
    for (cls = RANDOM_CLASS; cls < MAXOCLASSES; cls++) {
	if (cls == ILLOBJ_CLASS) continue;
	for (k = 0; k < IT_RANDOM_REPS; k++) {
	    if (k % 25 == 0) {
		fprintf(f, "random class %d %d\n", cls, k);
		fflush(f);
	    }
	    o = mkobj(cls, (k & 1) ? TRUE : FALSE);
	    if (o) item_exercise(o, k);
	    stress_note_panic(f);
	}
    }

    fprintf(f, "DONE\n");
    fclose(f);
}
#endif /* ITEM_STRESS_TEST */

#ifdef USE_STRESS_TEST
#include <unistd.h>
/*
 * Use stress test (see util/stress_test.py; not compiled normally).  In
 * wizard mode, if a file named "usetest" exists in the playground, use every
 * kind of object the way a player would, through the real commands (wear,
 * put on, wield, zap, engrave, break, read, cast, quaff, apply, eat, fire,
 * throw, invoke), many times in different states (blessed, cursed, confused,
 * blind and hallucinating); fight a monster while wearing or wielding it;
 * let the hero's timeouts and the object timers run; eat the corpse, egg and
 * tin of every monster; and wish for every object and artifact by name.
 *
 * Prompts are answered by the game itself: win/tty/wintty.c, getline.c and
 * termcap.c honour stress_input (every key press is a safe answer, every
 * line is escaped and nothing waits), and getobj() and getdir() honour
 * stress_obj and stress_dir.  Each step is logged to "ut.log" first, so a
 * crash points at the guilty object; a line containing MISMATCH is a wish
 * that gave the wrong object; the last line is DONE if the whole run
 * survived.
 */
#ifndef UT_REPS
#define UT_REPS 9
#endif
#ifndef UT_RANDOM_REPS
#define UT_RANDOM_REPS 150
#endif

int stress_input = 0;		/* 1: answer every prompt; 2: say y once */
struct obj *stress_obj = (struct obj *) 0;	/* the next getobj() answer */
int stress_dir = 0;		/* getdir() answer: 1 self, 2 east, 3 up, 4 down */

static const int ust_dirs[] = { 2, 1, 3, 4 };

/* is the object still in the hero's inventory? */
STATIC_OVL boolean
ust_held(o)
struct obj *o;
{
    struct obj *i;

    for (i = invent; i; i = i->nobj) if (i == o) return TRUE;
    return FALSE;
}

STATIC_OVL void
ust_free(o)
struct obj *o;
{
    if (Has_contents(o)) delete_contents(o);
    obfree(o, (struct obj *) 0);
}

/* let delayed things finish: the delay after wearing something, eating and
   other occupations, a deferred level change */
STATIC_OVL void
ust_settle()
{
    int n = 0;

    unmul((char *) 0);
    while (occupation && n++ < 400)
	if (!(*occupation)()) occupation = 0;
    occupation = 0;
    multi = 0;
    unmul((char *) 0);
    if (u.utotype) deferred_goto();
    multi = 0;
}

/* run a command that asks for one object, giving it that object, and for a
   direction, giving it that direction */
STATIC_OVL void
ust_cmd(fn, o, dir)
int NDECL((*fn));
struct obj *o;
int dir;
{
    stress_obj = o;
    stress_dir = dir;
    (void) (*fn)();
    stress_obj = (struct obj *) 0;
    ust_settle();
}

/* a monster of a kind that depends on the step, next to the hero */
STATIC_OVL struct monst *
ust_monster(n)
int n;
{
    int i = LOW_PM + (n * 13 + 7) % (NUMMONS - LOW_PM), t = 0;

    while (stress_skip_pm(i) && t++ < NUMMONS)
	i = (i + 1 < NUMMONS) ? i + 1 : LOW_PM;
    return makemon(&mons[i], u.ux + 1, u.uy, NO_MM_FLAGS);
}

/* put the hero, the level and the inventory back as they were */
STATIC_OVL void
ust_reset(startlev)
d_level *startlev;
{
    struct obj *o, *nx;
    struct monst *m, *mn;
    int i;
    boolean swallowed = u.uswallow;

    ust_settle();
    stress_obj = (struct obj *) 0;
    u.ustuck = (struct monst *) 0;
    u.uswallow = 0;
    u.uswldtim = 0;
    if (Upolyd) rehumanize();
    if (Punished) unpunish();
    u.utrap = 0;
    if (u.ulycn >= LOW_PM) you_unwere(FALSE);
    for (o = invent; o; o = o->nobj)
	if (o->owornmask) remove_worn_item(o, TRUE);
    for (o = invent; o; o = nx) {
	nx = o->nobj;
	freeinv(o);
	ust_free(o);
    }
    for (o = fobj; o; o = nx) {
	nx = o->nobj;
	obj_extract_self(o);
	ust_free(o);
    }
    for (m = fmon; m; m = mn) {
	mn = m->nmon;
	if (!DEADMONSTER(m)) mongone(m);
    }
    dmonsfree();
    if (u.utotype) deferred_goto();
    if (!on_level(&u.uz, startlev)) goto_level(startlev, FALSE, FALSE, FALSE);
    else if (swallowed) docrt();
    del_engr_at(u.ux, u.uy);

    for (i = 1; i <= LAST_PROP; i++) u.uprops[i].intrinsic = 0L;
    for (i = 0; i < A_MAX; i++) {
	ABASE(i) = AMAX(i) = 16;
	ATEMP(i) = ATIME(i) = 0;
    }
    u.ulevel = 8;
    u.uexp = newuexp(7);
    u.uhpmax = u.uhp = 30000;
    u.uenmax = u.uen = 500;
    u.uhunger = 150;
    u.uluck = u.moreluck = 0;
    u.ugangr = 0;
    u.ublesscnt = 0;
    u.ualign.record = 20;
    for (i = 0; i < MAXSPELL; i++) spl_book[i].sp_id = NO_SPELL;
    flags.botl = 1;
}

/* the hero is wearing or wielding something: fight a monster both ways and
   let the timeouts run */
STATIC_OVL void
ust_exercise(n)
int n;
{
    struct monst *m = ust_monster(n);
    int i;

    find_ac();
    enlightenment(0);
    if (m && !DEADMONSTER(m)) {
	if (!m->mpeaceful) {
	    m->mux = u.ux;
	    m->muy = u.uy;
	    for (i = 0; i < 2 && !DEADMONSTER(m) && !u.uswallow; i++)
		(void) mattacku(m);
	}
	u.dx = 1;
	u.dy = 0;
	u.dz = 0;
	for (i = 0; i < 3 && !DEADMONSTER(m) && !u.uswallow &&
		    m->mx == u.ux + 1 && m->my == u.uy; i++)
	    (void) attack(m);
    }
    for (i = 0; i < 8; i++) nh_timeout();
    u.uhp = u.uhpmax;
    ust_settle();
}

/* let the object timers run: burning lamps, rotting corpses, eggs, figurines */
STATIC_OVL void
ust_timers()
{
    moves += 2000L;
    run_timers();
    ust_settle();
}

/* something to be read, zapped or quaffed at: a wielded sword, some worn
   armor and a few other things in the pack */
STATIC_OVL void
ust_gear(n)
int n;
{
    static const int worn[] = { ELVEN_MITHRIL_COAT, HELMET, LEATHER_GLOVES,
				LOW_BOOTS };
    struct obj *g;
    int i;

    g = addinv(mksobj(LONG_SWORD, TRUE, FALSE));
    ust_cmd(dowield, g, 0);
    for (i = 0; i < 4; i++) {
	g = addinv(mksobj(worn[i], TRUE, FALSE));
	ust_cmd(dowear, g, 0);
    }
    for (i = 0; i < 4; i++) {
	g = mkobj(RANDOM_CLASS, TRUE);
	if (g) (void) addinv(g);
    }
    (void) n;
}

/* the hero's condition for this repetition */
STATIC_OVL void
ust_condition(rep)
int rep;
{
    switch ((rep / 3) % 3) {
	case 1: make_confused(500L, FALSE); break;
	case 2:
	    make_blinded(500L, FALSE);
	    (void) make_hallucinated(500L, FALSE, 0L);
	    break;
	default: break;
    }
}

/* use one freshly made object; it is not in the inventory yet */
STATIC_OVL void
ust_item(o, n, rep)
struct obj *o;
int n, rep;
{
    int d, otyp = o->otyp, oclass = o->oclass, i, spell;
    struct obj *a;

    switch (rep % 3) {
	case 0: o->blessed = o->cursed = 0; break;
	case 1: o->blessed = 1; o->cursed = 0; break;
	default: o->cursed = 1; o->blessed = 0; break;
    }
    o->dknown = o->bknown = o->known = (rep & 1);
    if (oclass != ARMOR_CLASS && oclass != WEAPON_CLASS &&
	oclass != RING_CLASS && oclass != AMULET_CLASS)
	ust_gear(n);
    o = addinv(o);
    ust_condition(rep);
    (void) ust_monster(n + 1);

    if (o->oartifact)
	for (d = 0; d < 4 && ust_held(o); d++)
	    ust_cmd(doinvoke, o, ust_dirs[d]);

    switch (oclass) {
    case ARMOR_CLASS:
	ust_cmd(dowear, o, 0);
	if (ust_held(o) && o->owornmask) {
	    ust_exercise(n);
	    ust_cmd(dotakeoff, o, 0);
	}
	break;
    case RING_CLASS:
    case AMULET_CLASS:
	ust_cmd(doputon, o, 0);
	if (ust_held(o) && o->owornmask) {
	    ust_exercise(n);
	    ust_cmd(doremring, o, 0);
	}
	break;
    case WEAPON_CLASS:
	ust_cmd(dowield, o, 0);
	if (ust_held(o) && o->owornmask) ust_exercise(n);
	if (ust_held(o) && is_launcher(o) && o->owornmask) {
	    /* fire every kind of ammunition that goes with it */
	    for (i = 1; i < NUM_OBJECTS; i++) {
		if (objects[i].oc_class != WEAPON_CLASS ||
		    objects[i].oc_skill != -objects[otyp].oc_skill)
		    continue;
		a = mksobj(i, TRUE, FALSE);
		a->quan = 12;
		a->owt = weight(a);
		a = addinv(a);
		ust_cmd(dowieldquiver, a, 0);
		(void) ust_monster(n + i);
		stress_dir = 2;
		for (d = 0; d < 2; d++) {
		    (void) dofire();
		    ust_settle();
		}
	    }
	}
	for (d = 0; d < 4 && ust_held(o); d++)
	    ust_cmd(doapply, o, ust_dirs[d]);
	if (ust_held(o)) ust_cmd(dothrow, o, 2);
	break;
    case WAND_CLASS:
	for (d = 0; d < 4 && ust_held(o); d++) {
	    o->spe = 6;
	    ust_cmd(dozap, o, ust_dirs[d]);
	}
	if (ust_held(o)) ust_cmd(doengrave, o, 0);
	if (ust_held(o)) {		/* break it, answering yes */
	    o->spe = 3;
	    stress_input = 2;
	    ust_cmd(doapply, o, 0);
	    stress_input = 1;
	}
	break;
    case SCROLL_CLASS:
	ust_cmd(doread, o, 0);
	ust_exercise(n);
	break;
    case POTION_CLASS:
	ust_cmd(dodrink, o, 0);
	ust_exercise(n);
	a = mksobj(otyp, TRUE, FALSE);
	a->blessed = (rep % 3 == 1);
	a->cursed = (rep % 3 == 2);
	a = addinv(a);
	ust_cmd(dothrow, a, 2);
	break;
    case SPBOOK_CLASS:
	ust_cmd(doread, o, 0);
	spell = -1;
	for (i = 0; i < MAXSPELL; i++)
	    if (spellid(i) == otyp) spell = i;
	if (spell < 0) {
	    a = mksobj(otyp, FALSE, FALSE);
	    initialspell(a);
	    ust_free(a);
	    for (i = 0; i < MAXSPELL; i++)
		if (spellid(i) == otyp) spell = i;
	}
	if (spell >= 0 && otyp != SPE_BLANK_PAPER &&
	    otyp != SPE_BOOK_OF_THE_DEAD) {	/* nothing to cast */
	    for (d = 0; d < 5; d++) {
		u.uen = u.uenmax = 500;
		u.uhunger = 900;
		stress_dir = ust_dirs[d % 4];
		(void) spelleffects(spell, d == 4);
		ust_settle();
	    }
	    ust_exercise(n);
	}
	break;
    case FOOD_CLASS:
	if (rep & 1) o->age = moves - 700L;
	ust_cmd(doeat, o, 0);
	ust_exercise(n);
	break;
    default:		/* tools, gems, rocks, venom */
	if (oclass == TOOL_CLASS && (otyp == BLINDFOLD || otyp == TOWEL ||
				     otyp == LENSES)) {
	    ust_cmd(doputon, o, 0);
	    if (ust_held(o) && o->owornmask) {
		ust_exercise(n);
		ust_cmd(doremring, o, 0);
	    }
	}
	for (d = 0; d < 4 && ust_held(o); d++)
	    ust_cmd(doapply, o, ust_dirs[d]);
	if (ust_held(o) && oclass != TOOL_CLASS) ust_cmd(dothrow, o, 2);
	break;
    }
    ust_timers();
}

/* wish for every object and artifact by name, in four ways */
STATIC_OVL void
ust_wishes(f)
FILE *f;
{
    static const char *pre[] = { "", "blessed +2 ", "cursed -1 " };
    char buf[BUFSZ], orig[BUFSZ], name[BUFSZ];
    const char *cls, *an_;
    struct obj *o, nothing;
    short at;
    int otyp, v, i;

    nothing = zeroobj;		/* only its address matters */

    for (otyp = 1; otyp < NUM_OBJECTS; otyp++) {
	if (!OBJ_NAME(objects[otyp])) continue;
	switch (objects[otyp].oc_class) {
	    case COIN_CLASS: case ILLOBJ_CLASS: continue;
	    case WAND_CLASS: cls = "wand of "; break;
	    case RING_CLASS: cls = "ring of "; break;
	    case POTION_CLASS: cls = "potion of "; break;
	    case SCROLL_CLASS: cls = "scroll of "; break;
	    case SPBOOK_CLASS: cls = "spellbook of "; break;
	    default: cls = ""; break;
	}
	Sprintf(name, "%s%s", cls, OBJ_NAME(objects[otyp]));
	for (v = 0; v < 4; v++) {
	    /* names that are already plural (boots, gloves) have no plural */
	    if (v == 3) {
		const char *of = strstri(name, " of ");
		int ln = of ? (int) (of - name) : (int) strlen(name);

		if (name[ln - 1] == 's') continue;
	    }
	    if (v == 3) Sprintf(buf, "3 %s", makeplural(name));
	    else Sprintf(buf, "%s%s", pre[v], name);
	    Strcpy(orig, buf);		/* readobjnam alters its argument */
	    fprintf(f, "wish %d \"%s\"\n", otyp, buf);
	    fflush(f);
	    o = readobjnam(buf, &nothing, TRUE);
	    if (!o || o == &nothing || o == &zeroobj) {
		fprintf(f, "MISMATCH wish \"%s\" gave nothing\n", orig);
	    } else {
		if (o->otyp != otyp)
		    fprintf(f, "MISMATCH wish \"%s\" gave %s\n", orig,
			    OBJ_NAME(objects[o->otyp]) ?
				OBJ_NAME(objects[o->otyp]) : "?");
		ust_free(o);
	    }
	    stress_note_panic(f);
	}
    }
    for (i = 1; i <= NROFARTIFACTS; i++) {
	an_ = artiname(i);
	if (!an_ || !*an_ || !artifact_name(an_, &at)) continue;
	Sprintf(buf, "%s", an_);
	fprintf(f, "wish artifact %d \"%s\"\n", i, buf);
	fflush(f);
	o = readobjnam(buf, &nothing, TRUE);
	if (o == &zeroobj) {
	    /* an artifact that already exists is denied */
	} else if (!o || o == &nothing) {
	    fprintf(f, "MISMATCH wish \"%s\" gave nothing\n", an_);
	} else {
	    if (o->otyp != (int) at)
		fprintf(f, "MISMATCH wish \"%s\" gave %s\n", an_,
			OBJ_NAME(objects[o->otyp]) ?
			    OBJ_NAME(objects[o->otyp]) : "?");
	    ust_free(o);
	}
	stress_note_panic(f);
    }
}

STATIC_OVL void
use_stress_test()
{
    FILE *f = fopen("ut.log", "w");
    static const int classes[] = { ARMOR_CLASS, WEAPON_CLASS, RING_CLASS,
	AMULET_CLASS, WAND_CLASS, SCROLL_CLASS, POTION_CLASS, SPBOOK_CLASS,
	FOOD_CLASS, TOOL_CLASS, GEM_CLASS };
    static const int eggs[] = { CORPSE, EGG, TIN };
    d_level startlev;
    struct obj *o;
    int otyp, rep, i, k, t;

    if (!f) return;
    startlev = u.uz;
    stress_input = 1;
    urole.spelbase = -100;		/* so that casting always works */
    ust_reset(&startlev);

    ust_wishes(f);

    /* every kind of object, in several states */
    for (otyp = 1; otyp < NUM_OBJECTS; otyp++) {
	if (!OBJ_NAME(objects[otyp])) continue;
	switch (objects[otyp].oc_class) {
	    case COIN_CLASS: case ILLOBJ_CLASS: case BALL_CLASS:
	    case CHAIN_CLASS: continue;
	    default: break;
	}
	if (otyp == BOULDER || otyp == STATUE || otyp == CORPSE ||
	    otyp == EGG || otyp == TIN)
	    continue;			/* done below, for every monster */
	for (rep = 0; rep < UT_REPS; rep++) {
	    fprintf(f, "use %d %s %d\n", otyp, OBJ_NAME(objects[otyp]), rep);
	    fflush(f);
	    ust_reset(&startlev);
	    o = mksobj(otyp, TRUE, (rep & 1) ? TRUE : FALSE);
	    if (o) ust_item(o, otyp * UT_REPS + rep, rep);
	    stress_note_panic(f);
	}
    }

    /* random objects of the classes one can use */
    for (k = 0; k < (int) SIZE(classes); k++)
	for (rep = 0; rep < UT_RANDOM_REPS; rep++) {
	    fprintf(f, "use random class %d %d\n", classes[k], rep);
	    fflush(f);
	    ust_reset(&startlev);
	    o = mkobj(classes[k], TRUE);
	    if (o) ust_item(o, 100000 + k * UT_RANDOM_REPS + rep, rep);
	    stress_note_panic(f);
	}

    /* eat the corpse, egg and tin of every monster */
    for (i = LOW_PM; i < NUMMONS; i++)
	for (t = 0; t < 3; t++)
	    for (rep = 0; rep < 3; rep++) {
		fprintf(f, "eat %d %s kind %d %d\n", i, mons[i].mname,
			eggs[t], rep);
		fflush(f);
		ust_reset(&startlev);
		o = mksobj(eggs[t], FALSE, FALSE);
		if (!o) continue;
		o->corpsenm = i;
		o->quan = 1;
		if (eggs[t] == TIN) o->spe = (rep == 1) ? -1 : (rep == 2) ? 1 : 0;
		if (eggs[t] == CORPSE && rep) o->age = moves - 100L * rep;
		o->owt = weight(o);
		if (rep == 2) o->cursed = 1;
		o = addinv(o);
		ust_cmd(doeat, o, 0);
		for (k = 0; k < 10; k++) nh_timeout();
		u.uhp = u.uhpmax;
		ust_timers();
		stress_note_panic(f);
	    }

    ust_reset(&startlev);
    stress_input = 0;
    fprintf(f, "DONE\n");
    fclose(f);
}
#endif /* USE_STRESS_TEST */

void
moveloop()
{
#if defined(MICRO) || defined(WIN32)
    char ch;
    int abort_lev;
#endif
    int moveamt = 0, wtcap = 0, change = 0;
#if defined(CORPSE_STRESS_TEST) || defined(ITEM_STRESS_TEST) || defined(USE_STRESS_TEST)
    static int stress_ran = 0;
#endif
	 int regen_frequency;
    boolean didmove = FALSE, monscanmove = FALSE;
	 /* don't make it obvious when monsters will start speeding up */
	 int monclock;
	 int timeout_start = rnd(10000)+15000;
	 int clock_base = 70000L-timeout_start;
	 int past_clock;
#ifdef CORPSE_STRESS_TEST
    if (!stress_ran && wizard && access("corpsetest", 0) == 0) {
	stress_ran = 1;
	corpse_stress_test();
    }
#endif
#ifdef ITEM_STRESS_TEST
    if (!stress_ran && wizard && access("itemtest", 0) == 0) {
	stress_ran = 1;
	item_stress_test();
    }
#endif
#ifdef USE_STRESS_TEST
    if (!stress_ran && wizard && access("usetest", 0) == 0) {
	stress_ran = 1;
	use_stress_test();
    }
#endif

    flags.moonphase = phase_of_the_moon();
    if(flags.moonphase == FULL_MOON) {
	You("are lucky!  Full moon tonight.");
	change_luck(1);
    } else if(flags.moonphase == NEW_MOON) {
	pline("Be careful!  New moon tonight.");
    }
    flags.friday13 = friday_13th();
    if (flags.friday13) {
	pline("Watch out!  Bad things can happen on Friday the 13th.");
	change_luck(-1);
    }

    initrack();


    /* Note:  these initializers don't do anything except guarantee that
	    we're linked properly.
    */
    decl_init();
    monst_init();
    monstr_init();	/* monster strengths */
    objects_init();

#ifdef WIZARD
    if (wizard) add_debug_extended_commands();
#endif

    (void) encumber_msg(); /* in case they auto-picked up something */

    u.uz0.dlevel = u.uz.dlevel;
    youmonst.movement = NORMAL_SPEED;	/* give the hero some movement points */
#ifdef WHEREIS_FILE
	 touch_whereis();
#endif

    for(;;) {
	get_nh_event();
#ifdef POSITIONBAR
	do_positionbar();
#endif

	didmove = flags.move;
	if(didmove) {
	    /* actual time passed */
	    youmonst.movement -= NORMAL_SPEED;

	    do { /* hero can't move this turn loop */
		wtcap = encumber_msg();

		flags.mon_moving = TRUE;
		do {
		    monscanmove = movemon();
		    if (youmonst.movement > NORMAL_SPEED)
			break;	/* it's now your turn */
		} while (monscanmove);
		flags.mon_moving = FALSE;

		if (!monscanmove && youmonst.movement < NORMAL_SPEED) {
		    /* both you and the monsters are out of steam this round */
		    /* set up for a new turn */
		    struct monst *mtmp;
		    mcalcdistress();	/* adjust monsters' trap, blind, etc */

		    /* reallocate movement rations to monsters */
		    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
			mtmp->movement += mcalcmove(mtmp);

			 /* Vanilla generates a critter every 70-ish turns.
			  * The rate accelerates to every 50 or so below the Castle,
			  * and 'round every 25 turns once you've done the Invocation.
			  *
			  * We will push it even further.  Monsters post-Invocation
			  * will almost always appear on the stairs (if present), and 
			  * much more frequently; this, along with the extra intervene()
			  * calls, should certainly make it seem like you're wading back
			  * through the teeming hordes.
			  *
			  * Aside from that, a more general clock should be put on things;
			  * after about 30,000 turns, the frequency rate of appearance
			  * and (TODO) difficulty of monsters generated will slowly increase until
			  * it reaches the point it will be at as if you were post-Invocation.
			  *
			  * 80,000 turns should be adequate as a target mark for this effect;
			  * if you haven't ascended in 80,000 turns, you're intentionally
			  * fiddling around somewhere and will certainly be strong enough
			  * to handle anything that comes your way, so this won't be 
			  * dropping newbies off the edge of the planet.  -- DSR 12/2/07
			  */

			 monclock = 70;
			 if (u.uevent.udemigod) {
				 monclock = 10;
			 } else {
				 if (depth(&u.uz) > depth(&stronghold_level)) {
					 monclock = 50;
				 }
				 past_clock = moves - timeout_start;
				 if (past_clock > 0) {
					 monclock -= past_clock*60/clock_base;
				 }
			 }
			 /* make sure we don't fall off the bottom */
			 if (monclock < 10) { monclock = 10; }

			 /* TODO: adj difficulty in makemon */
		    if(!rn2(monclock)) {
				 if (u.uevent.udemigod && xupstair && rn2(10)) {
					(void) makemon((struct permonst *)0, xupstair, yupstair, MM_ADJACENTOK);
				 } else {
					(void) makemon((struct permonst *)0, 0, 0, NO_MM_FLAGS);
				 }
			 }

		    /* calculate how much time passed. */
#ifdef STEED
		    if (u.usteed && u.umoved) {
			/* your speed doesn't augment steed's speed */
			moveamt = mcalcmove(u.usteed);
		    } else
#endif
		    {
				moveamt = youmonst.data->mmove;

				if (Super_fast) {	/* Quicksilver: speed 24 */
					if (moveamt < 2 * NORMAL_SPEED)
						moveamt = 2 * NORMAL_SPEED;
				} else if (Very_fast) {	/* speed boots or potion */
					/* average movement is 1.67 times normal */
					moveamt += NORMAL_SPEED / 2;
					if (rn2(3) == 0) moveamt += NORMAL_SPEED / 2;
				} else if (Fast) {
					/* average movement is 1.33 times normal */
					if (rn2(3) != 0) moveamt += NORMAL_SPEED / 2;
				} else if (Slow) {
					/* average movement noticeably slower */
					if (rn2(3) != 0) moveamt -= NORMAL_SPEED / 2;
				}
				/* a topaz dragon's breath cuts even into fast heroes */
				if (Slow && (Fast || Very_fast || Super_fast))
					moveamt -= moveamt / 3;
			 }

		    switch (wtcap) {
			case UNENCUMBERED: break;
			case SLT_ENCUMBER: moveamt -= (moveamt / 4); break;
			case MOD_ENCUMBER: moveamt -= (moveamt / 2); break;
			case HVY_ENCUMBER: moveamt -= ((moveamt * 3) / 4); break;
			case EXT_ENCUMBER: moveamt -= ((moveamt * 7) / 8); break;
			default: break;
		    }

		    youmonst.movement += moveamt;
		    if (youmonst.movement < 0) youmonst.movement = 0;
		    /* a ring of stealth leaves no tracks (NetHack 5.0) */
		    if (!((uleft && uleft->otyp == RIN_STEALTH) ||
			  (uright && uright->otyp == RIN_STEALTH)))
			settrack();

		    monstermoves++;
		    moves++;
		    if (u.uinspire > 0) u.uinspire--;
		    if (u.ucoldres > 0 && --u.ucoldres == 0) {
			ECold_resistance &= ~W_ARTI;
			You_feel("less resistant to cold.");
		    }

		    /********************************/
		    /* once-per-turn things go here */
		    /********************************/

		    if (flags.bypasses) clear_bypasses();
		    if(Glib) glibr();
		    nh_timeout();
		    run_regions();

		    if (u.ublesscnt)  u.ublesscnt--;
		    if(flags.time && !flags.run)
			flags.botl = 1;

                    if ((prev_dgl_extrainfo == 0) || (prev_dgl_extrainfo < (moves + 250))) {
                        prev_dgl_extrainfo = moves;
                        mk_dgl_extrainfo();
                    }

		    /* One possible result of prayer is healing.  Whether or
		     * not you get healed depends on your current hit points.
		     * If you are allowed to regenerate during the prayer, the
		     * end-of-prayer calculation messes up on this.
		     * Another possible result is rehumanization, which requires
		     * that encumbrance and movement rate be recalculated.
		     */
		    if (u.uinvulnerable) {
			/* for the moment at least, you're in tiptop shape */
			wtcap = UNENCUMBERED;
		    } else if (Upolyd && youmonst.data->mlet == S_EEL && !is_pool(u.ux,u.uy) && !Is_waterlevel(&u.uz)) {
			if (u.mh > 1) {
			    u.mh--;
			    flags.botl = 1;
			} else if (u.mh < 1)
			    rehumanize();
		    } else if (Upolyd && u.mh < u.mhmax) {
			if (u.mh < 1)
			    rehumanize();
			else if (Regeneration ||
				    (wtcap < MOD_ENCUMBER && !(moves%20))) {
			    flags.botl = 1;
			    u.mh++;
			}
		    } else if (u.uhp < u.uhpmax &&
			 (wtcap < MOD_ENCUMBER || !u.umoved || Regeneration)) {
			if (u.ulevel > 9 && !(moves % 3)) {
			    int heal, Con = (int) ACURR(A_CON);

			    if (Con <= 12) {
				heal = 1;
			    } else {
				heal = rnd(Con);
				if (heal > u.ulevel-9) heal = u.ulevel-9;
			    }
			    flags.botl = 1;
			    u.uhp += heal;
			    if(u.uhp > u.uhpmax)
				u.uhp = u.uhpmax;
			} else if (Regeneration ||
			     (u.ulevel <= 9 &&
			      !(moves % ((MAXULEV+12) / (u.ulevel+2) + 1)))) {
			    flags.botl = 1;
			    u.uhp++;
			}
		    }

		    /* Vilya: one extra hit point of regeneration per turn */
		    if (vilya_worn() && !u.uinvulnerable) {
			if (Upolyd) {
			    if (u.mh >= 1 && u.mh < u.mhmax) {
				u.mh++;
				flags.botl = 1;
			    }
			} else if (u.uhp < u.uhpmax) {
			    u.uhp++;
			    flags.botl = 1;
			}
			find_ac();	/* Intelligence may have changed */
		    }

		    /* moving around while encumbered is hard work */
		    if (wtcap > MOD_ENCUMBER && u.umoved) {
			if(!(wtcap < EXT_ENCUMBER ? moves%30 : moves%10)) {
			    if (Upolyd && u.mh > 1) {
				u.mh--;
			    } else if (!Upolyd && u.uhp > 1) {
				u.uhp--;
			    } else {
				You("pass out from exertion!");
				exercise(A_CON, FALSE);
				fall_asleep(-10, FALSE);
			    }
			}
		    }

			 /* wizards regen slightly faster than others; energy regeneration is
			  * faster than both; wizards + energy regeneration (Eye) is fastest of all */
		    if (u.uen < u.uenmax && wtcap < MOD_ENCUMBER) {
				if (Energy_regeneration) {
					regen_frequency = (MAXULEV - (Role_if(PM_WIZARD) ? 5 : 0) - u.ulevel) * (Role_if(PM_WIZARD) ? 2 : 3) / 6;
				} else {
					regen_frequency = (MAXULEV + 8 - u.ulevel) * (Role_if(PM_WIZARD) ? 3 : 4) / 6;
				}
				/* regen_freq should never hit 0 or negative, but let's cover it here just in case
				 * since we saw a crash earlier at this line -- this is the only thing it could be */
				if (regen_frequency < 1) { regen_frequency = 1; }
				if (!(moves % regen_frequency)) {
					u.uen += rn1((int)(ACURR(A_WIS) + ACURR(A_INT)) / 15 + 1,1);
					if (u.uen > u.uenmax) {
						u.uen = u.uenmax;
					}
					flags.botl = 1;
				}
		    }

		    if(!u.uinvulnerable) {
			if(Teleportation && !rn2(85)) {
			    xchar old_ux = u.ux, old_uy = u.uy;
			    tele();
			    if (u.ux != old_ux || u.uy != old_uy) {
				if (!next_to_u()) {
				    check_leash(old_ux, old_uy);
				}
#ifdef REDO
				/* clear doagain keystrokes */
				pushch(0);
				savech(0);
#endif
			    }
			}
			/* delayed change may not be valid anymore */
			if ((change == 1 && !Polymorph) ||
			    (change == 2 && u.ulycn == NON_PM))
			    change = 0;
			if(Polymorph && !rn2(100))
			    change = 1;
			else if (u.ulycn >= LOW_PM && !Upolyd &&
				 !rn2(80 - (20 * night())))
			    change = 2;
			if (change && !Unchanging) {
			    if (multi >= 0) {
				if (occupation)
				    stop_occupation();
				else
				    nomul(0, 0);
				if (change == 1) polyself(FALSE);
				else you_were();
				change = 0;
			    }
			}
		    }

		    if(Searching && multi >= 0) (void) dosearch0(1);
		    dosounds();
		    do_storms();
		    gethungry();
		    age_spells();
		    exerchk();
		    invault();
		    if (u.uhave.amulet) amulet();
		    if (!rn2(40+(int)(ACURR(A_DEX)*3)))
			u_wipe_engr(rnd(3));
		    if (u.uevent.udemigod && !u.uinvulnerable) {
			if (u.udg_cnt) u.udg_cnt--;
			if (!u.udg_cnt) {
			    intervene();
			    u.udg_cnt = rn1(100, 50);
			}
		    }
		    restore_attrib();
		    /* underwater and waterlevel vision are done here */
		    if (Is_waterlevel(&u.uz))
			movebubbles();
		    else if (Underwater)
			under_water(0);
		    /* vision while buried done here */
		    else if (u.uburied) under_ground(0);

		    /* when immobile, count is in turns */
		    if(multi < 0) {
			if (++multi == 0) {	/* finished yet? */
			    unmul((char *)0);
			    /* if unmul caused a level change, take it now */
			    if (u.utotype) deferred_goto();
			}
		    }
		}
	    } while (youmonst.movement<NORMAL_SPEED); /* hero can't move loop */

	    /******************************************/
	    /* once-per-hero-took-time things go here */
	    /******************************************/

		 /* move this here so player can check inventory safely in lava
		  * but any actual action will cause time to pass */

	} /* actual time passed */

	/****************************************/
	/* once-per-player-input things go here */
	/****************************************/

	find_ac();
	if(!flags.mv || Blind) {
	    /* redo monsters if hallu or wearing a helm of telepathy */
	    if (Hallucination) {	/* update screen randomly */
		see_monsters();
		see_objects();
		see_traps();
		if (u.uswallow) swallowed(0);
	    } else if (Unblind_telepat) {
		see_monsters();
	    } else if (Warning || Warn_of_mon)
	     	see_monsters();

	    if (vision_full_recalc) vision_recalc(0);	/* vision! */
	}

#ifdef REALTIME_ON_BOTL
        if(iflags.showrealtime) {
            /* Update the bottom line if the number of minutes has
             * changed */
            if(get_realtime() / 60 != realtime_data.last_displayed_time / 60)
                flags.botl = 1;
        }
#endif
  
	if(flags.botl || flags.botlx) bot();

	flags.move = 1;

	if(multi >= 0 && occupation) {
#if defined(MICRO) || defined(WIN32)
	    abort_lev = 0;
	    if (kbhit()) {
		if ((ch = Getchar()) == ABORT)
		    abort_lev++;
# ifdef REDO
		else
		    pushch(ch);
# endif /* REDO */
	    }
	    if (!abort_lev && (*occupation)() == 0)
#else
	    if ((*occupation)() == 0)
#endif
		occupation = 0;
	    if(
#if defined(MICRO) || defined(WIN32)
		   abort_lev ||
#endif
		   monster_nearby()) {
		stop_occupation();
		reset_eat();
	    }
#if defined(MICRO) || defined(WIN32)
	    if (!(++occtime % 7))
		display_nhwindow(WIN_MAP, FALSE);
#endif
	    continue;
	}

	if ((u.uhave.amulet || Clairvoyant) &&
	    !In_endgame(&u.uz) && !BClairvoyant &&
	    !(moves % 15) && !rn2(2))
		do_vicinity_map();

                if (u.utrap && u.utraptype == TT_LAVA) {
                        if(!is_lava(u.ux,u.uy))
                                u.utrap = 0;
                        else if (!u.uinvulnerable) {
                                u.utrap -= 1<<8;
                                if (u.utrap < 1<<8) {
                                        killer_format = KILLED_BY;
                                        killer = "molten lava";
                                        You("sink below the surface and die.");
                                        done(DISSOLVED);
                                } else if(didmove && !u.umoved) {
                                        Norep("You sink deeper into the lava.");
                                        u.utrap += rnd(4);
                                }
                        }
                }

#ifdef WIZARD
	if (iflags.sanity_check)
	    sanity_check();
#endif

#ifdef CLIPPING
	/* just before rhack */
	cliparound(u.ux, u.uy);
#endif

	u.umoved = FALSE;

	if (multi > 0) {
	    lookaround();
	    if (!multi) {
		/* lookaround may clear multi */
		flags.move = 0;
		if (flags.time) flags.botl = 1;
		continue;
	    }
	    if (flags.mv) {
		if(multi < COLNO && !--multi)
		    flags.travel = iflags.travel1 = flags.mv = flags.run = 0;
		domove();
	    } else {
		--multi;
		rhack(save_cm);
	    }
	} else if (multi == 0) {
	    ck_server_admin_msg();
#ifdef MAIL
	    ckmailstatus();
#endif
	    rhack((char *)0);
	}
	if (u.utotype)		/* change dungeon level */
	    deferred_goto();	/* after rhack() */
	/* !flags.move here: multiple movement command stopped */
	else if (flags.time && (!flags.move || !flags.mv))
	    flags.botl = 1;

	if (vision_full_recalc) vision_recalc(0);	/* vision! */
	/* when running in non-tport mode, this gets done through domove() */
	if ((!flags.run || iflags.runmode == RUN_TPORT) &&
		(multi && (!flags.travel ? !(multi % 7) : !(moves % 7L)))) {
	    if (flags.time && flags.run) flags.botl = 1;
	    display_nhwindow(WIN_MAP, FALSE);
	}
    }
}

#endif /* OVL0 */
#ifdef OVL1

void
stop_occupation()
{
	if(occupation) {
		if (!maybe_finished_meal(TRUE))
		    You("stop %s.", occtxt);
		occupation = 0;
		flags.botl = 1; /* in case u.uhs changed */
/* fainting stops your occupation, there's no reason to sync.
		sync_hunger();
*/
#ifdef REDO
		nomul(0, 0);
		pushch(0);
#endif
	}
}

#endif /* OVL1 */
#ifdef OVLB

void
display_gamewindows()
{
    curses_stupid_hack = 0;
    WIN_MESSAGE = create_nhwindow(NHW_MESSAGE);
    WIN_STATUS = create_nhwindow(NHW_STATUS);
    WIN_MAP = create_nhwindow(NHW_MAP);
    WIN_INVEN = create_nhwindow(NHW_MENU);

#ifdef MAC
    /*
     * This _is_ the right place for this - maybe we will
     * have to split display_gamewindows into create_gamewindows
     * and show_gamewindows to get rid of this ifdef...
     */
	if ( ! strcmp ( windowprocs . name , "mac" ) ) {
	    SanePositions ( ) ;
	}
#endif

    /*
     * The mac port is not DEPENDENT on the order of these
     * displays, but it looks a lot better this way...
     */
    display_nhwindow(WIN_STATUS, FALSE);
    display_nhwindow(WIN_MESSAGE, FALSE);
    clear_glyph_buffer();
    display_nhwindow(WIN_MAP, FALSE);
}

void
newgame()
{
	int i;

#ifdef MFLOPPY
	gameDiskPrompt();
#endif

	flags.ident = 1;

	for (i = 0; i < NUMMONS; i++)
		mvitals[i].mvflags = mons[i].geno & G_NOCORPSE;

	init_objects();		/* must be before u_init() */

	flags.pantheon = -1;	/* role_init() will reset this */
	role_init();		/* must be before init_dungeons(), u_init(),
				 * and init_artifacts() */

	init_dungeons();	/* must be before u_init() to avoid rndmonst()
				 * creating odd monsters for any tins and eggs
				 * in hero's initial inventory */
	init_artifacts();	/* before u_init() in case $WIZKIT specifies
				 * any artifacts */
	u_init();

#ifndef NO_SIGNAL
	(void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
#ifdef NEWS
	if(iflags.news) display_file(NEWS, FALSE);
#endif
	load_qtlist();	/* load up the quest text info */
/*	quest_init();*/	/* Now part of role_init() */

	mklev();
	u_on_upstairs();
	vision_reset();		/* set up internals for level (after mklev) */
	check_special_room(FALSE);

	flags.botlx = 1;

	/* Move the monster from under you or else
	 * makedog() will fail when it calls makemon().
	 *			- ucsfcgl!kneller
	 */
	if(MON_AT(u.ux, u.uy)) mnexto(m_at(u.ux, u.uy));
	(void) makedog();
	docrt();

	if (flags.legacy) {
		flush_screen(1);
		com_pager(1);
	}

#ifdef INSURANCE
	save_currentstate();
#endif
	program_state.something_worth_saving++;	/* useful data now exists */

#if defined(RECORD_REALTIME) || defined(REALTIME_ON_BOTL)

        /* Start the timer here */
        realtime_data.realtime = (time_t)0L;

#if defined(BSD) && !defined(POSIX_TYPES)
        (void) time((long *)&realtime_data.restoretime);
#else
        (void) time(&realtime_data.restoretime);
#endif

#endif /* RECORD_REALTIME || REALTIME_ON_BOTL */

	/* Success! */
	welcome(TRUE);
	return;
}

/* show "welcome [back] to nethack" message at program startup */
void
welcome(new_game)
boolean new_game;	/* false => restoring an old game */
{
    char buf[BUFSZ];
    boolean currentgend = Upolyd ? u.mfemale : flags.female;

    /*
     * The "welcome back" message always describes your innate form
     * even when polymorphed or wearing a helm of opposite alignment.
     * Alignment is shown unconditionally for new games; for restores
     * it's only shown if it has changed from its original value.
     * Sex is shown for new games except when it is redundant; for
     * restores it's only shown if different from its original value.
     */
    *buf = '\0';
    if (new_game || u.ualignbase[A_ORIGINAL] != u.ualignbase[A_CURRENT])
	Sprintf(eos(buf), " %s", align_str(u.ualignbase[A_ORIGINAL]));
    if (!urole.name.f &&
	    (new_game ? (urole.allow & ROLE_GENDMASK) == (ROLE_MALE|ROLE_FEMALE) :
	     currentgend != flags.initgend))
	Sprintf(eos(buf), " %s", genders[currentgend].adj);

    pline(new_game ? "%s %s, welcome to JadeHack!  You are a%s %s %s."
		   : "%s %s, the%s %s %s, welcome back to JadeHack!",
	  Hello((struct monst *) 0), plname, buf, urace.adj,
	  (currentgend && urole.name.f) ? urole.name.f : urole.name.m);
}

#ifdef POSITIONBAR
STATIC_DCL void
do_positionbar()
{
	static char pbar[COLNO];
	char *p;
	
	p = pbar;
	/* up stairway */
	if (upstair.sx &&
	   (glyph_to_cmap(level.locations[upstair.sx][upstair.sy].glyph) ==
	    S_upstair ||
 	    glyph_to_cmap(level.locations[upstair.sx][upstair.sy].glyph) ==
	    S_upladder)) {
		*p++ = '<';
		*p++ = upstair.sx;
	}
	if (sstairs.sx &&
	   (glyph_to_cmap(level.locations[sstairs.sx][sstairs.sy].glyph) ==
	    S_upstair ||
 	    glyph_to_cmap(level.locations[sstairs.sx][sstairs.sy].glyph) ==
	    S_upladder)) {
		*p++ = '<';
		*p++ = sstairs.sx;
	}

	/* down stairway */
	if (dnstair.sx &&
	   (glyph_to_cmap(level.locations[dnstair.sx][dnstair.sy].glyph) ==
	    S_dnstair ||
 	    glyph_to_cmap(level.locations[dnstair.sx][dnstair.sy].glyph) ==
	    S_dnladder)) {
		*p++ = '>';
		*p++ = dnstair.sx;
	}
	if (sstairs.sx &&
	   (glyph_to_cmap(level.locations[sstairs.sx][sstairs.sy].glyph) ==
	    S_dnstair ||
 	    glyph_to_cmap(level.locations[sstairs.sx][sstairs.sy].glyph) ==
	    S_dnladder)) {
		*p++ = '>';
		*p++ = sstairs.sx;
	}

	/* hero location */
	if (u.ux) {
		*p++ = '@';
		*p++ = u.ux;
	}
	/* fence post */
	*p = 0;

	update_positionbar(pbar);
}
#endif

#if defined(REALTIME_ON_BOTL) || defined (RECORD_REALTIME)
time_t
get_realtime(void)
{
    time_t curtime;

    /* Get current time */
#if defined(BSD) && !defined(POSIX_TYPES)
    (void) time((long *)&curtime);
#else
    (void) time(&curtime);
#endif

    /* Since the timer isn't set until the game starts, this prevents us
     * from displaying nonsense on the bottom line before it does. */
    if(realtime_data.restoretime == 0) {
        curtime = realtime_data.realtime;
    } else {
        curtime -= realtime_data.restoretime;
        curtime += realtime_data.realtime;
    }
 
    return curtime;
}
#endif /* REALTIME_ON_BOTL || RECORD_REALTIME */


#endif /* OVLB */

/*allmain.c*/
