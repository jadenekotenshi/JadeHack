/*	SCCS Id: @(#)artifact.c 3.4	2003/08/11	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"
#ifdef OVLB
#include "artilist.h"
#else
STATIC_DCL struct artifact artilist[];
#endif
/*
 * Note:  both artilist[] and artiexist[] have a dummy element #0,
 *	  so loops over them should normally start at #1.  The primary
 *	  exception is the save & restore code, which doesn't care about
 *	  the contents, just the total size.
 */

/*#define DEBUG*/
/* Uncomment for some complaint messages */

extern boolean notonhead;	/* for long worms */

#define get_artifact(o) \
		(((o)&&(o)->oartifact) ? &artilist[(int) (o)->oartifact] : 0)

STATIC_DCL int FDECL(spec_applies, (const struct artifact *,struct monst *));
STATIC_DCL int FDECL(arti_invoke, (struct obj*));
STATIC_DCL boolean FDECL(Mb_hit, (struct monst *magr,struct monst *mdef,
				  struct obj *,int *,int,BOOLEAN_P,char *));

/* The amount added to the victim's total hit points to insure that the
   victim will be killed even after damage bonus/penalty adjustments.
   Most such penalties are small, and 200 is plenty; the exception is
   half physical damage.  3.3.1 and previous versions tried to use a very
   large number to account for this case; now, we just compute the fatal
   damage by adding it to 2 times the total hit points instead of 1 time.
   Note: this will still break if they have more than about half the number
   of hit points that will fit in a 15 bit integer. */
#define FATAL_DAMAGE_MODIFIER 200

#ifndef OVLB
STATIC_DCL int spec_dbon_applies;
STATIC_DCL xchar artidisco[NROFARTIFACTS];
#else	/* OVLB */
/* coordinate effects from spec_dbon() with messages in artifact_hit() */
STATIC_OVL int spec_dbon_applies = 0;

/* flags including which artifacts have already been created */
static boolean artiexist[1+NROFARTIFACTS+1];
/* and a discovery list for them (no dummy first entry here) */
STATIC_OVL xchar artidisco[NROFARTIFACTS];

STATIC_DCL void NDECL(hack_artifacts);
STATIC_DCL boolean FDECL(attacks, (int,struct obj *));
STATIC_DCL void FDECL(reaver_steal, (struct monst *));
STATIC_DCL int FDECL(sea_change, (struct obj *));
STATIC_DCL void FDECL(detect_artifacts, (struct obj *));

/* Narya and Nenya: find the real ring type that currently has a given
   appearance (the ring appearances are shuffled for each game) */
STATIC_OVL int
ring_with_appearance(desc)
const char *desc;
{
	int i;

	for (i = bases[RING_CLASS];
	     i < NUM_OBJECTS && objects[i].oc_class == RING_CLASS; i++)
	    if (OBJ_NAME(objects[i]) && OBJ_DESCR(objects[i]) &&
		    !strcmp(OBJ_DESCR(objects[i]), desc))
		return i;
	return 0;
}

STATIC_OVL int
lotr_base_ring(first, second, fallback)
const char *first, *second;
int fallback;
{
	int i, t;
	const char *name[2];

	name[0] = first;
	name[1] = second;
	for (i = 0; i < 2; i++) {
	    t = ring_with_appearance(name[i]);
	    if (t && t != RIN_POLYMORPH && t != RIN_CONFLICT &&
		    t != RIN_AGGRAVATE_MONSTER && t != RIN_HUNGER)
		return t;
	}
	return fallback;
}

/* choose the base rings of Narya and Nenya; must be redone after the
   appearances are restored from a saved game */
void
fix_lotr_rings()
{
	artilist[ART_NARYA].otyp =
		lotr_base_ring("ruby", "gold", RIN_FIRE_RESISTANCE);
	artilist[ART_NENYA].otyp =
		lotr_base_ring("diamond", "platinum", RIN_COLD_RESISTANCE);
	artilist[ART_VILYA].otyp =
		lotr_base_ring("sapphire", "brass", RIN_PROTECTION);
}

/* extra damage from Narya (non fire resistant foes) and Nenya (non cold
   resistant foes) while worn; applies to melee, thrown and fired attacks */
int
lotr_ring_damage(mon)
struct monst *mon;
{
	int dmg = 0;

	if (!mon) return 0;
	if (((uleft && uleft->oartifact == ART_NARYA) ||
	     (uright && uright->oartifact == ART_NARYA)) && !resists_fire(mon))
	    dmg += rnd(10);
	if (((uleft && uleft->oartifact == ART_NENYA) ||
	     (uright && uright->oartifact == ART_NENYA)) && !resists_cold(mon))
	    dmg += rnd(10);
	return dmg;
}

/* is Vilya being worn? */
boolean
vilya_worn()
{
	return (boolean)((uleft && uleft->oartifact == ART_VILYA) ||
			 (uright && uright->oartifact == ART_VILYA));
}

/* Vilya's AC bonus: the D&D 3rd edition Intelligence modifier,
   (Int - 10) / 2 rounded down, but never a penalty */
int
vilya_ac_bonus()
{
	int mod = ((int) ACURR(A_INT) - 10) / 2;

	return (vilya_worn() && mod > 0) ? mod : 0;
}

/* Charisma-based spellcasting bonus from each of the two rings worn */
int
lotr_cha_bonus()
{
	int n = 0;

	if ((uleft && uleft->oartifact == ART_NARYA) ||
	    (uright && uright->oartifact == ART_NARYA)) n++;
	if ((uleft && uleft->oartifact == ART_NENYA) ||
	    (uright && uright->oartifact == ART_NENYA)) n++;
	return n * ACURR(A_CHA);
}

/* handle some special cases; must be called after u_init() */
STATIC_OVL void
hack_artifacts()
{
	struct artifact *art;
	int alignmnt = aligns[flags.initalign].value;

	/* Fix up the alignments of "gift" artifacts */
	for (art = artilist+1; art->otyp; art++)
	    if (art->role == Role_switch && art->alignment != A_NONE)
		art->alignment = alignmnt;

	/* Excalibur can be used by any lawful character, not just knights
		So can Dirge, for that matter */
	if (!Role_if(PM_KNIGHT)) {
	    artilist[ART_EXCALIBUR].role = NON_PM;
		 artilist[ART_DIRGE].role = NON_PM;
	 }

	/* Demonbane should always be in the Priest's list of available toys
	 * even though this might look a little odd when it comes out Chaotic */
	if (Role_if(PM_PRIEST)) {
		artilist[ART_DEMONBANE].alignment = alignmnt;
	}

	/* Fix up the quest artifact */
	if (urole.questarti) {
	    artilist[urole.questarti].alignment = alignmnt;
	    artilist[urole.questarti].role = Role_switch;
	}
	fix_lotr_rings();
	/* Bards' first sacrifice gift is the Luck Blade, in their alignment */
	if (Role_if(PM_BARD)) {
	    artilist[ART_LUCK_BLADE].alignment = alignmnt;
	    artilist[ART_LUCK_BLADE].role = PM_BARD;
	}
	return;
}

/* zero out the artifact existence list */
void
init_artifacts()
{
	(void) memset((genericptr_t) artiexist, 0, sizeof artiexist);
	(void) memset((genericptr_t) artidisco, 0, sizeof artidisco);
	hack_artifacts();
}

void
save_artifacts(fd)
int fd;
{
	bwrite(fd, (genericptr_t) artiexist, sizeof artiexist);
	bwrite(fd, (genericptr_t) artidisco, sizeof artidisco);
}

void
restore_artifacts(fd)
int fd;
{
	mread(fd, (genericptr_t) artiexist, sizeof artiexist);
	mread(fd, (genericptr_t) artidisco, sizeof artidisco);
	hack_artifacts();	/* redo non-saved special cases */
}

const char *
artiname(artinum)
int artinum;
{
	if (artinum <= 0 || artinum > NROFARTIFACTS) return("");
	return(artilist[artinum].name);
}

/*
   Make an artifact.  If a specific alignment is specified, then an object of
   the appropriate alignment is created from scratch, or 0 is returned if
   none is available.  (If at least one aligned artifact has already been
   given, then unaligned ones also become eligible for this.)
   If no alignment is given, then 'otmp' is converted
   into an artifact of matching type, or returned as-is if that's not possible.
   For the 2nd case, caller should use ``obj = mk_artifact(obj, A_NONE);''
   for the 1st, ``obj = mk_artifact((struct obj *)0, some_alignment);''.
 */
struct obj *
mk_artifact(otmp, alignment)
struct obj *otmp;	/* existing object; ignored if alignment specified */
aligntyp alignment;	/* target alignment, or A_NONE */
{
	const struct artifact *a;
	int n, m;
	boolean by_align = (alignment != A_NONE);
	short o_typ = (by_align || !otmp) ? 0 : otmp->otyp;
	boolean unique = !by_align && otmp && objects[o_typ].oc_unique;
	short eligible[NROFARTIFACTS];

	/* gather eligible artifacts */
	for (n = 0, a = artilist+1, m = 1; a->otyp; a++, m++)
	    if ((!by_align ? a->otyp == o_typ :
		    (a->alignment == alignment ||
			(a->alignment == A_NONE && u.ugifts > 0))) &&
		(!(a->spfx & SPFX_NOGEN) || unique) && !artiexist[m]) {
		/* Pirates are only ever given the Marauder's Map, and
		   nobody else is */
		if (by_align && (Role_if(PM_PIRATE) ?
			m != ART_MARAUDER_S_MAP : m == ART_MARAUDER_S_MAP))
		    continue;
		if (by_align && a->race != NON_PM && race_hostile(&mons[a->race]))
		    continue;	/* skip enemies' equipment */
		else if (by_align && Role_if(a->role))
		    goto make_artif;	/* 'a' points to the desired one */
		else
		    eligible[n++] = m;
	    }

	if (n) {		/* found at least one candidate */
	    m = eligible[rn2(n)];	/* [0..n-1] */
	    a = &artilist[m];

	    /* make an appropriate object if necessary, then christen it */
make_artif: if (by_align) otmp = mksobj((int)a->otyp, TRUE, FALSE);
	    otmp = oname(otmp, a->name);
	    otmp->oartifact = m;
	    artiexist[m] = TRUE;
	} else {
	    /* nothing appropriate could be found; return the original object */
	    if (by_align) otmp = 0;	/* (there was no original object) */
	}
	return otmp;
}


/*** Object properties (see ITEM_xxx in obj.h) ***/

/* give a weapon or armor some magical property; allow_detrimental permits
   the cursed fumbling/hunger properties on armor */
struct obj *
create_oprop(obj, allow_detrimental)
struct obj *obj;
boolean allow_detrimental;
{
    struct obj *otmp = obj;
    int i, tries = 0;
    long j;

    if (!otmp) return otmp;
    if (otmp->oartifact || objects[otmp->otyp].oc_unique) return otmp;
    if (otmp->otyp == WORM_TOOTH || otmp->otyp == UNICORN_HORN ||
	otmp->otyp == CRYSKNIFE)
	return otmp;
    /* items that are already magical get a property a tenth as often */
    if (objects[otmp->otyp].oc_magic && rn2(10)) return otmp;
    if (otmp->oclass != WEAPON_CLASS && !is_weptool(otmp) &&
	otmp->oclass != ARMOR_CLASS)
	return otmp;
    /* dragon scales and scale mail are fine as they are */
    if (otmp->otyp >= GRAY_DRAGON_SCALE_MAIL &&
	otmp->otyp <= AMETHYST_DRAGON_SCALES)
	return otmp;

    while (!otmp->oprops && tries++ < 200) {
	i = rn2(MAX_ITEM_PROPS);
	j = 1L << i;
	if ((j & (ITEM_FUMBLING | ITEM_HUNGER)) && !allow_detrimental)
	    continue;
	if (otmp->oclass == WEAPON_CLASS || is_weptool(otmp)) {
	    if (j & (ITEM_DRLI | ITEM_FUMBLING | ITEM_HUNGER | ITEM_OILSKIN))
		continue;
	    if (is_launcher(otmp) &&
		(j & (ITEM_DMG_PROPS | ITEM_THUNDERING)))
		continue;
	    if ((is_ammo(otmp) || is_missile(otmp)) &&
		(j & (ITEM_ESP | ITEM_EXCEL | ITEM_SEARCHING | ITEM_WARNING)))
		continue;
	} else {
	    /* armor */
	    if (j & ITEM_THUNDERING) continue;
	    if ((j & ITEM_OILSKIN) && objects[otmp->otyp].oc_material != CLOTH)
		continue;
	    if (otmp->otyp == OILSKIN_CLOAK && (j & ITEM_OILSKIN)) continue;
	    if (is_helmet(otmp) && (j & ITEM_ESP)) continue;
	}
	otmp->oprops |= j;
    }

    if (otmp->oprops & (ITEM_FUMBLING | ITEM_HUNGER)) {
	if (!otmp->cursed) curse(otmp);
	otmp->spe = -rne(3);
    } else if (!rn2(8)) {
	blessorcurse(otmp, 8);
	otmp->spe = otmp->cursed ? -rne(3) : rne(3);
    }
    if (otmp->oprops & ITEM_OILSKIN) otmp->greased = 1;
    return otmp;
}

/* turn an object's properties on or off as it is worn or wielded */
void
set_oprop_intrinsics(obj, on, wp_mask)
struct obj *obj;
boolean on;
long wp_mask;
{
    long p = obj->oprops;
    boolean armor = (obj->oclass == ARMOR_CLASS);

    if (!p) return;
#define OPROP_E(flag, field) \
    if (p & (flag)) { if (on) field |= wp_mask; else field &= ~wp_mask; }
    if (armor) {
	OPROP_E(ITEM_FIRE, EFire_resistance);
	OPROP_E(ITEM_FROST, ECold_resistance);
	OPROP_E(ITEM_SHOCK, EShock_resistance);
	OPROP_E(ITEM_VENOM, EPoison_resistance);
	OPROP_E(ITEM_DRLI, EDrain_resistance);
	OPROP_E(ITEM_FUMBLING, EFumbling);
	OPROP_E(ITEM_HUNGER, EHunger);
    }
    OPROP_E(ITEM_ESP, ETelepat);
    OPROP_E(ITEM_SEARCHING, ESearching);
    OPROP_E(ITEM_WARNING, EWarning);
    OPROP_E(ITEM_EXCEL, ELucky);
#undef OPROP_E
    if (on) obj->oprops_known |= (p & ~ITEM_THUNDERING) &
		(armor ? ITEM_PROP_MASK : ~ITEM_DMG_PROPS);
    if (p & ITEM_ESP) see_monsters();
}

/* weapons with properties add damage and side effects when they hit */
void
oprop_hit(magr, mdef, otmp, dmgptr)
struct monst *magr, *mdef;
struct obj *otmp;
int *dmgptr;
{
    boolean youattack = (magr == &youmonst), youdefend = (mdef == &youmonst);
    long p = otmp->oprops;
    const char *verb = 0;
    int extra = 0;

    if (!(p & (ITEM_DMG_PROPS | ITEM_THUNDERING))) return;
    if (otmp->oclass != WEAPON_CLASS && !is_weptool(otmp)) return;

    if (p & ITEM_FIRE) {
	if (!(youdefend ? how_resistant(FIRE_RES) > 99 : resists_fire(mdef))) {
	    extra = rnd(5) + 3; verb = "burns";
	}
    } else if (p & ITEM_FROST) {
	if (!(youdefend ? how_resistant(COLD_RES) > 99 : resists_cold(mdef))) {
	    extra = rnd(5) + 3; verb = "freezes";
	}
    } else if (p & ITEM_SHOCK) {
	if (!(youdefend ? how_resistant(SHOCK_RES) > 99 : resists_elec(mdef))) {
	    extra = rnd(5) + 3; verb = "shocks";
	}
    } else if (p & ITEM_VENOM) {
	if (!(youdefend ? how_resistant(POISON_RES) > 99 : resists_poison(mdef))) {
	    extra = rnd(6); verb = "poisons";
	}
    }
    if (verb) {
	*dmgptr += extra;
	if (youattack || youdefend || canseemon(mdef)) {
	    pline("%s %s %s!", The(xname(otmp)), verb,
		  youdefend ? "you" : mon_nam(mdef));
	    if (youattack) otmp->oprops_known |= (p & ITEM_DMG_PROPS);
	}
    }
    if ((p & ITEM_THUNDERING) && !rn2(2)) {
	if (youdefend) {
	    pline("The thunderous blow leaves you reeling!");
	    make_stunned(HStun + (long)rnd(4), FALSE);
	} else if (!mdef->mstun) {
	    mdef->mstun = 1;
	    mdef->mstuntime = rnd(4);
	    if (youattack || canseemon(mdef))
		pline("%s is stunned by the thunderous blow!", Monnam(mdef));
	    if (youattack) otmp->oprops_known |= ITEM_THUNDERING;
	}
    }
}

/*
 * Returns the full name (with articles and correct capitalization) of an
 * artifact named "name" if one exists, or NULL, it not.
 * The given name must be rather close to the real name for it to match.
 * The object type of the artifact is returned in otyp if the return value
 * is non-NULL.
 */
const char*
artifact_name(name, otyp)
const char *name;
short *otyp;
{
    register const struct artifact *a;
    register const char *aname;

    if(!strncmpi(name, "the ", 4)) name += 4;

    for (a = artilist+1; a->otyp; a++) {
	aname = a->name;
	if(!strncmpi(aname, "the ", 4)) aname += 4;
	if(!strcmpi(name, aname)) {
	    *otyp = a->otyp;
	    return a->name;
	}
    }

    return (char *)0;
}

boolean
exist_artifact(otyp, name)
register int otyp;
register const char *name;
{
	register const struct artifact *a;
	register boolean *arex;

	if (otyp && *name)
	    for (a = artilist+1,arex = artiexist+1; a->otyp; a++,arex++)
		if ((int) a->otyp == otyp && !strcmp(a->name, name))
		    return *arex;
	return FALSE;
}

void
artifact_exists(otmp, name, mod)
register struct obj *otmp;
register const char *name;
register boolean mod;
{
	register const struct artifact *a;

	if (otmp && *name)
	    for (a = artilist+1; a->otyp; a++)
		if (a->otyp == otmp->otyp && !strcmp(a->name, name)) {
		    register int m = a - artilist;
		    otmp->oartifact = (char)(mod ? m : 0);
		    otmp->age = 0;
		    if(otmp->otyp == RIN_INCREASE_DAMAGE)
			otmp->spe = 0;
		    artiexist[m] = mod;
		    break;
		}
	return;
}

int
nartifact_exist()
{
    int a = 0;
    int n = SIZE(artiexist);

    while(n > 1)
	if(artiexist[--n]) a++;

    return a;
}
#endif /* OVLB */
#ifdef OVL0

boolean
spec_ability(otmp, abil)
struct obj *otmp;
unsigned long abil;
{
	const struct artifact *arti = get_artifact(otmp);

	return((boolean)(arti && (arti->spfx & abil)));
}

/* used so that callers don't need to known about SPFX_ codes */
boolean
confers_luck(obj)
struct obj *obj;
{
    /* might as well check for this too */
    if (obj->otyp == LUCKSTONE) return TRUE;

    return (obj->oartifact && spec_ability(obj, SPFX_LUCK));
}

/* used to check whether a monster is getting reflection from an artifact */
boolean
arti_reflects(obj)
struct obj *obj;
{
    const struct artifact *arti = get_artifact(obj);

    if (arti) {      
	/* while being worn */
	if ((obj->owornmask & ~W_ART) && (arti->spfx & SPFX_REFLECT))
	    return TRUE;
	/* just being carried */
	if (arti->cspfx & SPFX_REFLECT) return TRUE;
    }
    return FALSE;
}

#endif /* OVL0 */
#ifdef OVLB

/*
 * ...Side effect of allowing Sting produces the famous artinaming bug.
 * We can fix this; naming something an artifact name for an artifact that is
 * SPFX_RESTR should produce an outright failure.
 */

boolean
restrict_name(otmp, name)  /* returns 1 if name is restricted for otmp->otyp */
register struct obj *otmp;
register const char *name;
{
	register const struct artifact *a;
	register const char *aname;

	if (!*name) return FALSE;
	if (!strncmpi(name, "the ", 4)) name += 4;

		/* Since almost every artifact is SPFX_RESTR, it doesn't cost
		   us much to do the string comparison before the spfx check.
		   Bug fix:  don't name multiple elven daggers "Sting".
		 */
	for (a = artilist+1; a->otyp; a++) {
	    aname = a->name;
	    if (!strncmpi(aname, "the ", 4)) aname += 4;
	    if (!strcmp(aname, name)) {
			return ((boolean)((a->spfx & (SPFX_NOGEN|SPFX_RESTR)) != 0 || otmp->quan > 1L));
		 }
	}

	return FALSE;
}

STATIC_OVL boolean
attacks(adtyp, otmp)
register int adtyp;
register struct obj *otmp;
{
	register const struct artifact *weap;

	if ((weap = get_artifact(otmp)) != 0)
		return((boolean)(weap->attk.adtyp == adtyp));
	return FALSE;
}

boolean
defends(adtyp, otmp)
register int adtyp;
register struct obj *otmp;
{
	register const struct artifact *weap;
	unsigned long mask;

	if ((weap = get_artifact(otmp)) != 0) {
		switch (adtyp) {
			case AD_FIRE:
				mask = (weap->defn & SPDF_FIRE);
				break;
			case AD_COLD:
				mask = (weap->defn & SPDF_COLD);
				break;
			case AD_ELEC:
				mask = (weap->defn & SPDF_ELEC);
				break;
			case AD_SLEE:
				mask = (weap->defn & SPDF_SLEEP);
				break;
			case AD_DISN:
				mask = (weap->defn & SPDF_DISINT);
				break;
			case AD_DRST:
				mask = (weap->defn & SPDF_POISON);
				break;
			case AD_ACID:
				mask = (weap->defn & SPDF_ACID);
				break;
			case AD_MAGM:
				mask = (weap->defn & SPDF_MAGIC);
				break;
			case AD_BLND:
				mask = (weap->defn & SPDF_BLIND);
				break;
			case AD_WERE:
				mask = (weap->defn & SPDF_WERE);
				break;
			case AD_DRLI:
				mask = (weap->defn & SPDF_DRAIN);
				break;
			case AD_STUN:
				mask = (weap->defn & SPDF_STUN);
				break;
			case AD_CONF:
				mask = (weap->defn & SPDF_CONFUSE);
				break;
			case AD_DISE:
			case AD_DRIN:
				mask = 0;	/* none yet */
				break;
			default:
#ifdef DEBUG
				/* Don't whine about this unless we really want to */
				pline("strange attack type in defends(): %d",adtyp);
#endif
				return FALSE;
		}
		return (mask > 0);  /* straight cast will fail */
	}
	return FALSE;
}

/* used for monsters */
boolean
protects(adtyp, otmp)
int adtyp;
struct obj *otmp;
{
	register const struct artifact *weap;

	if ((weap = get_artifact(otmp)) != 0)
		return (boolean)(weap->cary.adtyp == adtyp);
	return FALSE;
}

/*
 * a potential artifact has just been worn/wielded/picked-up or
 * unworn/unwielded/dropped.  Pickup/drop only set/reset the W_ART mask.
 */
void
set_artifact_intrinsic(otmp,on,wp_mask)
register struct obj *otmp;
boolean on;
long wp_mask;
{
	long *mask = 0;
	register const struct artifact *oart = get_artifact(otmp);
	uchar dtyp;
	long spfx;

	if (!oart) return;

	/* effects from the cary field */
	dtyp = (wp_mask != W_ART) ? 0 : oart->cary.adtyp;

	if (dtyp == AD_FIRE)
	    mask = &EFire_resistance;
	else if (dtyp == AD_COLD)
	    mask = &ECold_resistance;
	else if (dtyp == AD_ELEC)
	    mask = &EShock_resistance;
	else if (dtyp == AD_MAGM)
	    mask = &EAntimagic;
	else if (dtyp == AD_DISN)
	    mask = &EDisint_resistance;
	else if (dtyp == AD_DRST)
	    mask = &EPoison_resistance;

	if (mask && wp_mask == W_ART && !on) {
	    /* find out if some other artifact also confers this intrinsic */
	    /* if so, leave the mask alone */
	    register struct obj* obj;
	    for(obj = invent; obj; obj = obj->nobj)
		if(obj != otmp && obj->oartifact) {
		    register const struct artifact *art = get_artifact(obj);
		    if(art->cary.adtyp == dtyp) {
			mask = (long *) 0;
			break;
		    }
		}
	}
	if (mask) {
	    if (on) *mask |= wp_mask;
	    else *mask &= ~wp_mask;
	}

	/* effects from the defn field; could be more than one
		...Note that we don't have to check for other stuff, because
		this uses wp_mask directly and you can only have one of 
		any given thing in the appropriate slot */
	if (wp_mask & (W_WEP | W_ARMOR | W_AMUL | W_RING | W_TOOL)) {
		if ((oart->defn & SPDF_FIRE) && otmp->otyp != RIN_FIRE_RESISTANCE) {
			if (on) EFire_resistance |= wp_mask;
			else EFire_resistance &= ~wp_mask;
		}
		if ((oart->defn & SPDF_COLD) && otmp->otyp != RIN_COLD_RESISTANCE) {
			if (on) ECold_resistance |= wp_mask;
			else ECold_resistance &= ~wp_mask;
		}
		if (oart->defn & SPDF_ELEC) {
			if (on) EShock_resistance |= wp_mask;
			else EShock_resistance &= ~wp_mask;
		}
		if (oart->defn & SPDF_SLEEP) {
			if (on) ESleep_resistance |= wp_mask;
			else ESleep_resistance &= ~wp_mask;
		}
		if (oart->defn & SPDF_POISON) {
			if (on) EPoison_resistance |= wp_mask;
			else EPoison_resistance &= ~wp_mask;
		}
		if (oart->defn & SPDF_ACID) {
			if (on) EAcid_resistance |= wp_mask;
			else EAcid_resistance &= ~wp_mask;
		}
		if (oart->defn & SPDF_DISINT) {
			if (on) EDisint_resistance |= wp_mask;
			else EDisint_resistance &= ~wp_mask;
		}
		if (oart->defn & SPDF_DRAIN) {
			if (on) EDrain_resistance |= wp_mask;
			else EDrain_resistance &= ~wp_mask;
		}
		if (oart->defn & SPDF_MAGIC) {
			if (on) EAntimagic |= wp_mask;
			else EAntimagic &= ~wp_mask;
		}
		if (oart->defn & SPDF_WERE) {
			/* Doesn't have a specific resistance in the table */
		}
		if (oart->defn & SPDF_BLIND) {
			if (on) EBlind_resistance |= wp_mask;
			else EBlind_resistance &= ~wp_mask;
		}
		if (oart->defn & SPDF_CONFUSE) {
			/* Doesn't have a specific resistance in the table */
		}
		if (oart->defn & SPDF_STUN) {
			/* Doesn't have a specific resistance in the table */
		}
	}

	/* intrinsics from the spfx field; there could be more than one */
	spfx = (wp_mask != W_ART) ? oart->spfx : oart->cspfx;
	if(spfx && wp_mask == W_ART && !on) {
	    /* don't change any spfx also conferred by other artifacts */
	    register struct obj* obj;
	    for(obj = invent; obj; obj = obj->nobj)
		if(obj != otmp && obj->oartifact) {
		    register const struct artifact *art = get_artifact(obj);
		    spfx &= ~art->cspfx;
		}
	}

	if (spfx & SPFX_SEARCH) {
	    if(on) ESearching |= wp_mask;
	    else ESearching &= ~wp_mask;
	}
	if (spfx & SPFX_FAST) {
	    /* already fast or very fast: wielding speeds you up further */
	    if (on && !restoring && !ESuper_fast && Fast)
		You("speed up even more!");
	    if (on) ESuper_fast |= wp_mask;
	    else ESuper_fast &= ~wp_mask;
	}
	if (spfx & SPFX_POLYC) {
		if (on) EPolymorph_control |= wp_mask;
		else EPolymorph_control &= ~wp_mask;
	}
	if (spfx & SPFX_HALRES) {
	    /* make_hallucinated must (re)set the mask itself to get
	     * the display right */
	    /* restoring needed because this is the only artifact intrinsic
	     * that can print a message--need to guard against being printed
	     * when restoring a game
	     */
	    (void) make_hallucinated((long)!on, restoring ? FALSE : TRUE, wp_mask);
	}
	if (spfx & SPFX_ESP) {
	    if(on) ETelepat |= wp_mask;
	    else ETelepat &= ~wp_mask;
	    see_monsters();
	}
	if (spfx & SPFX_STLTH) {
	    if (on) EStealth |= wp_mask;
	    else EStealth &= ~wp_mask;
	}
	if (spfx & SPFX_REGEN) {
	    if (on) ERegeneration |= wp_mask;
	    else ERegeneration &= ~wp_mask;
	}
	if (spfx & SPFX_TCTRL) {
	    if (on) ETeleport_control |= wp_mask;
	    else ETeleport_control &= ~wp_mask;
	}
	if (spfx & SPFX_WARN) {
		/* right now the only things that warn against a specific monster type
		 * are wielded artifacts.  this may change in the future but for now
		 * this is probably the smallest change to clean things up without
		 * screwing around adding more M2 flags */
		if (spfx & (SPFX_DCLAS | SPFX_DFLAG2)) {
			if (on) {
				if (spfx & SPFX_DCLAS)
					flags.warntype = oart->mtype;
				else
					flags.warntype |= spec_m2(otmp);
				EWarn_of_mon |= wp_mask;
			} else {
				if (spfx & SPFX_DCLAS)
					flags.warntype = 0L;
				else
					flags.warntype &= ~spec_m2(otmp);
				EWarn_of_mon &= ~wp_mask;
			}
			see_monsters();
	    } else {
			if (on) EWarning |= wp_mask;
				else EWarning &= ~wp_mask;
	    }
	}
	if (otmp->oartifact == ART_AEGIS && wp_mask != W_ART) {
	    /* the Gorgon's head: reflects and cannot turn you to stone */
	    if (on) {
		EReflecting |= wp_mask;
		EStone_resistance |= wp_mask;
	    } else {
		EReflecting &= ~wp_mask;
		EStone_resistance &= ~wp_mask;
	    }
	}
	if (otmp->oartifact == ART_SWORD_OF_KAS && wp_mask != W_ART) {
	    if (on) EStone_resistance |= wp_mask;
	    else EStone_resistance &= ~wp_mask;
	}
	if (otmp->oartifact == ART_VILYA && wp_mask != W_ART) {
	    if (on) EMagical_breathing |= wp_mask;
	    else EMagical_breathing &= ~wp_mask;
	}
	if (spfx & SPFX_EREGEN) {
	    if (on) EEnergy_regeneration |= wp_mask;
	    else EEnergy_regeneration &= ~wp_mask;
	}
	if (spfx & SPFX_HSPDAM) {
	    if (on) EHalf_spell_damage |= wp_mask;
	    else EHalf_spell_damage &= ~wp_mask;
	}
	if (spfx & SPFX_HPHDAM) {
	    if (on) EHalf_physical_damage |= wp_mask;
	    else EHalf_physical_damage &= ~wp_mask;
	}
	if (spfx & SPFX_XRAY) {
	    /* this assumes that no one else is using xray_range */
	    if (on) u.xray_range = 3;
	    else u.xray_range = -1;
	    vision_full_recalc = 1;
	}
	if (spfx & SPFX_REFLECT) {
		/* Knights only have to carry the mirror; everyone else must wield it */
		if (Role_if(PM_KNIGHT)) {
			if (on) {
				EReflecting |= wp_mask;
			} else {
				EReflecting &= ~wp_mask;
			}
		} else if (wp_mask & W_WEP) {
	    if (on) EReflecting |= wp_mask;
	    else EReflecting &= ~wp_mask;
	}
	}

	if(wp_mask == W_ART && !on && oart->inv_prop) {
	    /* might have to turn off invoked power too */
	    if (oart->inv_prop <= LAST_PROP &&
		(u.uprops[oart->inv_prop].extrinsic & W_ARTI))
		(void) arti_invoke(otmp);
	}
}

/*
 * creature (usually player) tries to touch (pick up or wield) an artifact obj.
 * Returns 0 if the object refuses to be touched.
 * This routine does not change any object chains.
 * Ignores such things as gauntlets, assuming the artifact is not
 * fooled by such trappings.
 */
int
touch_artifact(obj,mon)
    struct obj *obj;
    struct monst *mon;
{
    register const struct artifact *oart = get_artifact(obj);
    boolean badclass, badalign, self_willed, yours;

    if(!oart) return 1;

    yours = (mon == &youmonst);
    /* all quest artifacts are self-willed; it this ever changes, `badclass'
       will have to be extended to explicitly include quest artifacts */
    self_willed = ((oart->spfx & SPFX_INTEL) != 0);
    if (yours) {
	badclass = self_willed &&
		   ((oart->role != NON_PM && !Role_if(oart->role)) ||
		    (oart->race != NON_PM && !Race_if(oart->race)));
	badalign = (oart->spfx & SPFX_RESTR) && oart->alignment != A_NONE &&
		   (oart->alignment != u.ualign.type || u.ualign.record < 0);
    } else if (!is_covetous(mon->data) && !is_mplayer(mon->data)) {
	badclass = self_willed &&
		   oart->role != NON_PM && oart != &artilist[ART_EXCALIBUR];
	badalign = (oart->spfx & SPFX_RESTR) && oart->alignment != A_NONE &&
		   (oart->alignment != sgn(mon->data->maligntyp));
    } else {    /* an M3_WANTSxxx monster or a fake player */
	/* special monsters trying to take the Amulet, invocation tools or
	   quest item can touch anything except for `spec_applies' artifacts */
	badclass = badalign = FALSE;
    }
    /* weapons which attack specific categories of monsters are
       bad for them even if their alignments happen to match */
    if (!badalign && (oart->spfx & SPFX_DBONUS) != 0) {
	struct artifact tmp;

	tmp = *oart;
	tmp.spfx &= SPFX_DBONUS;
	badalign = !!spec_applies(&tmp, mon);
    }

    if (((badclass || badalign) && self_willed) ||
       (badalign && (!yours || !rn2(4))))  {
	int dmg;
	char buf[BUFSZ];

	if (!yours) return 0;
	You("are blasted by %s power!", s_suffix(the(xname(obj))));
	dmg = d((Antimagic ? 2 : 4), (self_willed ? 10 : 4));
	Sprintf(buf, "touching %s", oart->name);
	losehp(dmg, buf, KILLED_BY);
	exercise(A_WIS, FALSE);
    }

    /* can pick it up unless you're totally non-synch'd with the artifact */
    if (badclass && badalign && self_willed) {
	if (yours) pline("%s your grasp!", Tobjnam(obj, "evade"));
	return 0;
    }

    return 1;
}

#endif /* OVLB */
#ifdef OVL1

/* decide whether an artifact's special attacks apply against mtmp */
STATIC_OVL int
spec_applies(weap, mtmp)
register const struct artifact *weap;
struct monst *mtmp;
{
	struct permonst *ptr;
	boolean yours;

	if(!(weap->spfx & (SPFX_DBONUS | SPFX_ATTK)))
	    return(weap->attk.adtyp == AD_PHYS);

	yours = (mtmp == &youmonst);
	ptr = mtmp->data;

	if (weap->spfx & SPFX_DMONS) {
	    return (ptr == &mons[(int)weap->mtype]);
	} else if (weap->spfx & SPFX_DCLAS) {
	    /* S_UNDEAD and S_WERE are not real monster classes: they stand for
	       "undead" and "lycanthrope" (they also drive the warning) */
	    if (weap->mtype == S_UNDEAD)
		return (boolean)((ptr->mflags2 & M2_UNDEAD) != 0L);
	    if (weap->mtype == S_WERE)
		return (boolean)((ptr->mflags2 & M2_WERE) != 0L ||
				 (yours && u.ulycn >= LOW_PM));
	    return (weap->mtype == (unsigned long)ptr->mlet);
	} else if (weap->spfx & SPFX_DFLAG1) {
	    return ((ptr->mflags1 & weap->mtype) != 0L);
	} else if (weap->spfx & SPFX_DFLAG2) {
	    return ((ptr->mflags2 & weap->mtype) || (yours &&
			((!Upolyd && (urace.selfmask & weap->mtype)) ||
			 ((weap->mtype & M2_WERE) && u.ulycn >= LOW_PM))));
	} else if (weap->spfx & SPFX_DALIGN) {
	    return yours ? (u.ualign.type != weap->alignment) :
			   (ptr->maligntyp == A_NONE ||
				sgn(ptr->maligntyp) != weap->alignment);
	} else if (weap->spfx & SPFX_ATTK) {
	    struct obj *defending_weapon = (yours ? uwep : MON_WEP(mtmp));

	    if (defending_weapon && defending_weapon->oartifact &&
		    defends((int)weap->attk.adtyp, defending_weapon))
		return FALSE;
	    switch(weap->attk.adtyp) {
		case AD_FIRE:
			return !(!yours ? resists_fire(mtmp) : (how_resistant(FIRE_RES) > 99) ? TRUE : FALSE);
		case AD_COLD:
			return !(!yours ? resists_cold(mtmp) : (how_resistant(COLD_RES) > 99) ? TRUE : FALSE);
		case AD_ELEC:
			return !(!yours ? resists_elec(mtmp) : (how_resistant(SHOCK_RES) > 99) ? TRUE : FALSE);
		case AD_MAGM:
		case AD_STUN:
			return !(yours ? Antimagic : (rn2(100) < ptr->mr));
		case AD_DRST:
			return !(!yours ? resists_poison(mtmp) : (how_resistant(POISON_RES) > 99) ? TRUE : FALSE);
		case AD_DRLI:
			return !(yours ? Drain_resistance : resists_drli(mtmp));
		case AD_STON:
			return !(yours ? Stone_resistance : resists_ston(mtmp));
		default:	impossible("Weird weapon special attack.");
	    }
	}
	return(0);
}

/* return the M2 flags of monster that an artifact's special attacks apply against */
long
spec_m2(otmp)
struct obj *otmp;
{
	register const struct artifact *artifact = get_artifact(otmp);
	if (artifact)
		return artifact->mtype;
	return 0L;
}

/* special attack bonus */
int
spec_abon(otmp, mon)
struct obj *otmp;
struct monst *mon;
{
	register const struct artifact *weap = get_artifact(otmp);

	/* no need for an extra check for `NO_ATTK' because this will
	   always return 0 for any artifact which has that attribute */

	if (weap && otmp->oartifact == ART_MOUSER_S_SCALPEL)
	    return 5;			/* flat +5 to hit */
	if (weap && otmp->oartifact == ART_DRAMBORLEG)
	    return rnd(8);		/* +d8 to hit against everything */
	if (weap && otmp->oartifact == ART_GUNGNIR)
	    return 5;			/* flat +5 to hit against everything */
	if (weap && weap->attk.damn && spec_applies(weap, mon))
	    return rnd((int)weap->attk.damn);
	return 0;
}

/* special damage bonus */
int
spec_dbon(otmp, mon, tmp)
struct obj *otmp;
struct monst *mon;
int tmp;
{
	register const struct artifact *weap = get_artifact(otmp);

	if (otmp->oclass == GEM_CLASS && uwep && uwep->oartifact == ART_GIANTSLAYER) {
		weap = get_artifact(uwep);
	}

	if (!weap || (weap->attk.adtyp == AD_PHYS && /* check for `NO_ATTK' */
			weap->attk.damn == 0 && weap->attk.damd == 0))
	    spec_dbon_applies = FALSE;
	else if (otmp->oartifact == ART_GRIMTOOTH ||
		 otmp->oartifact == ART_DRAMBORLEG)
	    /* these warn against one kind of monster, but the bonus applies
	       to everything */
	    spec_dbon_applies = TRUE;
	else
	    spec_dbon_applies = spec_applies(weap, mon);

	if (spec_dbon_applies && otmp->oartifact) {
	    /* flat damage bonuses */
	    switch (otmp->oartifact) {
		case ART_LUCK_BLADE:
		case ART_QUICKSILVER:
		case ART_SPINESEEKER:	return 4;
		case ART_GUNGNIR:	return 10;
		case ART_PERNICIMORDAX:	return d(4,4);
		case ART_KIKU_ICHIMONJI: return d(2,6);
		case ART_REAVER:	return d(2,8);
		case ART_GRAYSWANDIR:	return d(2,8);
		default: break;
	    }
	}
	if (spec_dbon_applies)
	    return weap->attk.damd ? rnd((int)weap->attk.damd) : max(tmp,1);
	return 0;
}

/* add identified artifact to discoveries list */
void
discover_artifact(m)
xchar m;
{
    int i;

    /* look for this artifact in the discoveries list;
       if we hit an empty slot then it's not present, so add it */
    for (i = 0; i < NROFARTIFACTS; i++)
	if (artidisco[i] == 0 || artidisco[i] == m) {
	    artidisco[i] = m;
	    return;
	}
    /* there is one slot per artifact, so we should never reach the
       end without either finding the artifact or an empty slot... */
    impossible("couldn't discover artifact (%d)", (int)m);
}

/* used to decide whether an artifact has been fully identified */
boolean
undiscovered_artifact(m)
xchar m;
{
    int i;

    /* look for this artifact in the discoveries list;
       if we hit an empty slot then it's undiscovered */
    for (i = 0; i < NROFARTIFACTS; i++)
	if (artidisco[i] == m)
	    return FALSE;
	else if (artidisco[i] == 0)
	    break;
    return TRUE;
}

/* display a list of discovered artifacts; return their count */
int
disp_artifact_discoveries(tmpwin)
winid tmpwin;		/* supplied by dodiscover() */
{
    int i, m, otyp;
    char buf[BUFSZ];

    for (i = 0; i < NROFARTIFACTS; i++) {
	if (artidisco[i] == 0) break;	/* empty slot implies end of list */
	if (i == 0) putstr(tmpwin, iflags.menu_headings, "Artifacts");
	m = artidisco[i];
	otyp = artilist[m].otyp;
	Sprintf(buf, "  %s [%s %s]", artiname(m),
		align_str(artilist[m].alignment), simple_typename(otyp));
	putstr(tmpwin, 0, buf);
    }
    return i;
}

#endif /* OVL1 */

#ifdef OVLB


	/*
	 * Magicbane's intrinsic magic is incompatible with normal
	 * enchantment magic.  Thus, its effects have a negative
	 * dependence on spe.  Against low mr victims, it typically
	 * does "double athame" damage, 2d4.  Occasionally, it will
	 * cast unbalancing magic which effectively averages out to
	 * 4d4 damage (3d4 against high mr victims), for spe = 0.
	 *
	 * Prior to 3.4.1, the cancel (aka purge) effect always
	 * included the scare effect too; now it's one or the other.
	 * Likewise, the stun effect won't be combined with either
	 * of those two; it will be chosen separately or possibly
	 * used as a fallback when scare or cancel fails.
	 *
	 * [Historical note: a change to artifact_hit() for 3.4.0
	 * unintentionally made all of Magicbane's special effects
	 * be blocked if the defender successfully saved against a
	 * stun attack.  As of 3.4.1, those effects can occur but
	 * will be slightly less likely than they were in 3.3.x.]
	 */
#define MB_MAX_DIEROLL		8	/* rolls above this aren't magical */
static const char * const mb_verb[2][4] = {
	{ "probe", "stun", "scare", "cancel" },
	{ "prod", "amaze", "tickle", "purge" },
};
#define MB_INDEX_PROBE		0
#define MB_INDEX_STUN		1
#define MB_INDEX_SCARE		2
#define MB_INDEX_CANCEL		3

/* called when someone is being hit by Magicbane */
STATIC_OVL boolean
Mb_hit(magr, mdef, mb, dmgptr, dieroll, vis, hittee)
struct monst *magr, *mdef;	/* attacker and defender */
struct obj *mb;			/* Magicbane */
int *dmgptr;			/* extra damage target will suffer */
int dieroll;			/* d20 that has already scored a hit */
boolean vis;			/* whether the action can be seen */
char *hittee;			/* target's name: "you" or mon_nam(mdef) */
{
    struct permonst *old_uasmon;
    const char *verb;
    boolean youattack = (magr == &youmonst),
	    youdefend = (mdef == &youmonst),
	    resisted = FALSE, do_stun, do_confuse, result;
    int attack_indx, scare_dieroll = MB_MAX_DIEROLL / 2;

    result = FALSE;		/* no message given yet */
    /* the most severe effects are less likely at higher enchantment */
    if (mb->spe >= 3)
	scare_dieroll /= (1 << (mb->spe / 3));
    /* if target successfully resisted the artifact damage bonus,
       reduce overall likelihood of the assorted special effects */
    if (!spec_dbon_applies) dieroll += 1;

    /* might stun even when attempting a more severe effect, but
       in that case it will only happen if the other effect fails;
       extra damage will apply regardless; 3.4.1: sometimes might
       just probe even when it hasn't been enchanted */
    do_stun = (max(mb->spe,0) < rn2(spec_dbon_applies ? 11 : 7));

    /* the special effects also boost physical damage; increments are
       generally cumulative, but since the stun effect is based on a
       different criterium its damage might not be included; the base
       damage is either 1d4 (athame) or 2d4 (athame+spec_dbon) depending
       on target's resistance check against AD_STUN (handled by caller)
       [note that a successful save against AD_STUN doesn't actually
       prevent the target from ending up stunned] */
    attack_indx = MB_INDEX_PROBE;
    *dmgptr += rnd(4);			/* (2..3)d4 */
    if (do_stun) {
	attack_indx = MB_INDEX_STUN;
	*dmgptr += rnd(4);		/* (3..4)d4 */
    }
    if (dieroll <= scare_dieroll) {
	attack_indx = MB_INDEX_SCARE;
	*dmgptr += rnd(4);		/* (3..5)d4 */
    }
    if (dieroll <= (scare_dieroll / 2)) {
	attack_indx = MB_INDEX_CANCEL;
	*dmgptr += rnd(4);		/* (4..6)d4 */
    }

    /* give the hit message prior to inflicting the effects */
    verb = mb_verb[!!Hallucination][attack_indx];
    if (youattack || youdefend || vis) {
	result = TRUE;
	pline_The("magic-absorbing blade %s %s!",
		  vtense((const char *)0, verb), hittee);
	/* assume probing has some sort of noticeable feedback
	   even if it is being done by one monster to another */
	if (attack_indx == MB_INDEX_PROBE && !canspotmon(mdef))
	    map_invisible(mdef->mx, mdef->my);
    }

    /* now perform special effects */
    switch (attack_indx) {
    case MB_INDEX_CANCEL:
	old_uasmon = youmonst.data;
	/* No mdef->mcan check: even a cancelled monster can be polymorphed
	 * into a golem, and the "cancel" effect acts as if some magical
	 * energy remains in spellcasting defenders to be absorbed later.
	 */
	if (!cancel_monst(mdef, mb, youattack, FALSE, FALSE)) {
	    resisted = TRUE;
	} else {
	    do_stun = FALSE;
	    if (youdefend) {
		if (youmonst.data != old_uasmon)
		    *dmgptr = 0;    /* rehumanized, so no more damage */
		if (u.uenmax > 0) {
		    You("lose magical energy!");
		    u.uenmax--;
		    if (u.uen > 0) u.uen--;
		    flags.botl = 1;
		}
	    } else {
		if (mdef->data == &mons[PM_CLAY_GOLEM])
		    mdef->mhp = 1;	/* cancelled clay golems will die */
		if (youattack && attacktype(mdef->data, AT_MAGC)) {
		    You("absorb magical energy!");
		    u.uenmax++;
		    u.uen++;
		    flags.botl = 1;
		}
	    }
	}
	break;

    case MB_INDEX_SCARE:
	if (youdefend) {
	    if (Antimagic) {
		resisted = TRUE;
	    } else {
		nomul(-3, "being scared stiff");
		nomovemsg = "";
		if (magr && magr == u.ustuck && sticks(youmonst.data)) {
		    u.ustuck = (struct monst *)0;
		    You("release %s!", mon_nam(magr));
		}
	    }
	} else {
	    if (rn2(2) && resist(mdef, WEAPON_CLASS, 0, NOTELL))
		resisted = TRUE;
	    else
		monflee(mdef, 3, FALSE, (mdef->mhp > *dmgptr));
	}
	if (!resisted) do_stun = FALSE;
	break;

    case MB_INDEX_STUN:
	do_stun = TRUE;		/* (this is redundant...) */
	break;

    case MB_INDEX_PROBE:
	if (youattack && (mb->spe == 0 || !rn2(3 * abs(mb->spe)))) {
	    pline_The("%s is insightful.", verb);
	    /* pre-damage status */
	    probe_monster(mdef);
	}
	break;
    }
    /* stun if that was selected and a worse effect didn't occur */
    if (do_stun) {
	if (youdefend)
	    make_stunned((HStun + 3), FALSE);
	else
	    mdef->mstun = 1;
	/* avoid extra stun message below if we used mb_verb["stun"] above */
	if (attack_indx == MB_INDEX_STUN) do_stun = FALSE;
    }
    /* lastly, all this magic can be confusing... */
    do_confuse = !rn2(12);
    if (do_confuse) {
	if (youdefend)
	    make_confused(HConfusion + 4, FALSE);
	else
	    mdef->mconf = 1;
    }

    if (youattack || youdefend || vis) {
	(void) upstart(hittee);	/* capitalize */
	if (resisted) {
	    pline("%s %s!", hittee, vtense(hittee, "resist"));
	    shieldeff(youdefend ? u.ux : mdef->mx,
		      youdefend ? u.uy : mdef->my);
	}
	if ((do_stun || do_confuse) && flags.verbose) {
	    char buf[BUFSZ];

	    buf[0] = '\0';
	    if (do_stun) Strcat(buf, "stunned");
	    if (do_stun && do_confuse) Strcat(buf, " and ");
	    if (do_confuse) Strcat(buf, "confused");
	    pline("%s %s %s%c", hittee, vtense(hittee, "are"),
		  buf, (do_stun && do_confuse) ? '!' : '.');
	}
    }

    return result;
}
  
/* Function used when someone attacks someone else with an artifact
 * weapon.  Only adds the special (artifact) damage, and returns a 1 if it
 * did something special (in which case the caller won't print the normal
 * hit message).  This should be called once upon every artifact attack;
 * dmgval() no longer takes artifact bonuses into account.  Possible
 * extension: change the killer so that when an orc kills you with
 * Stormbringer it's "killed by Stormbringer" instead of "killed by an orc".
 */
boolean
artifact_hit(magr, mdef, otmp, dmgptr, dieroll)
struct monst *magr, *mdef;
struct obj *otmp;
int *dmgptr;
int dieroll; /* needed for Magicbane and vorpal blades */
{
	boolean youattack = (magr == &youmonst);
	boolean youdefend = (mdef == &youmonst);
	boolean vis = (!youattack && magr && cansee(magr->mx, magr->my))
	    || (!youdefend && cansee(mdef->mx, mdef->my))
	    || (youattack && u.uswallow && mdef == u.ustuck && !Blind);
	boolean realizes_damage;
	const char *wepdesc;
	static const char you[] = "you";
	char hittee[BUFSZ];
	struct artifact* atmp;
	int j;
	char buf2[BUFSZ];

	Strcpy(hittee, youdefend ? you : mon_nam(mdef));

	/* The following takes care of most of the damage, but not all--
	 * the exception being for level draining, which is specially
	 * handled.  Messages are done in this function, however.
	 */
	*dmgptr += spec_dbon(otmp, mdef, *dmgptr);

	/* Kusanagi does double damage to snakes, dragons and other serpents */
	if (otmp->oartifact == ART_KUSANAGI && !youdefend &&
		(mdef->data->mlet == S_SNAKE || mdef->data->mlet == S_NAGA ||
		 mdef->data->mlet == S_DRAGON || slithy(mdef->data)))
	    *dmgptr *= 2;

	/* the Sword of Kas: triple damage against Vecna, double against liches
	   (other non-chaotic foes are doubled by the artifact itself), and a
	   poisoned edge that a lawful hero finds dishonorable */
	if (otmp->oartifact == ART_SWORD_OF_KAS && !youdefend) {
	    if (mdef->data == &mons[PM_VECNA]) *dmgptr *= 3;
	    else if (mdef->data->mlet == S_LICH && !spec_dbon_applies)
		*dmgptr *= 2;
	    if (!resists_poison(mdef)) *dmgptr += rnd(6);
	    if (youattack && u.ualign.type == A_LAWFUL) {
		You("dishonorably use a poisoned weapon!");
		adjalign(-1);
	    }
	}

	/* Pernicimordax is deadly to merozalghi, on top of its cold */
	if (otmp->oartifact == ART_PERNICIMORDAX && !youdefend &&
		mdef->data == &mons[PM_MEROZALGHU])
	    *dmgptr *= 2;

	if (youattack && youdefend) {
	    impossible("attacking yourself with weapon?");
	    return FALSE;
	}

	realizes_damage = (youdefend || vis || 
			   /* feel the effect even if not seen */
			   (youattack && mdef == u.ustuck));

	/* the four basic attacks: fire, cold, shock and missiles */
	if (attacks(AD_FIRE, otmp)) {
	    if (realizes_damage)
		pline_The("fiery blade %s %s%c",
			!spec_dbon_applies ? "hits" :
			(mdef->data == &mons[PM_WATER_ELEMENTAL]) ?
			"vaporizes part of" : "burns",
			hittee, !spec_dbon_applies ? '.' : '!');
	    if (!rn2(4)) (void) destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
	    if (!rn2(4)) (void) destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
	    if (!rn2(7)) (void) destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
	    if (youdefend && Slimed) burn_away_slime();
	    return realizes_damage;
	}
	if (attacks(AD_COLD, otmp)) {
	    if (realizes_damage)
		pline_The("ice-cold blade %s %s%c",
			!spec_dbon_applies ? "hits" : "freezes",
			hittee, !spec_dbon_applies ? '.' : '!');
	    if (!rn2(4)) (void) destroy_mitem(mdef, POTION_CLASS, AD_COLD);
	    return realizes_damage;
	}
	if (attacks(AD_ELEC, otmp)) {
	    if (realizes_damage)
		pline_The("massive hammer hits%s %s%c",
			  !spec_dbon_applies ? "" : "!  Lightning strikes",
			  hittee, !spec_dbon_applies ? '.' : '!');
	    if (!rn2(5)) (void) destroy_mitem(mdef, RING_CLASS, AD_ELEC);
	    if (!rn2(5)) (void) destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
	    return realizes_damage;
	}
	if (attacks(AD_MAGM, otmp)) {
	    if (realizes_damage)
		pline_The("imaginary widget hits%s %s%c",
			  !spec_dbon_applies ? "" :
				"!  A hail of magic missiles strikes",
			  hittee, !spec_dbon_applies ? '.' : '!');
	    return realizes_damage;
	}
	/* the fifth basic attack: poison */
	if (attacks(AD_DRST, otmp)) {
		if (realizes_damage) {
			pline_The("venomous blade %s %s%c",spec_dbon_applies ? "strikes" : "nicks",
				hittee, spec_dbon_applies ? '!' : '.');
			return realizes_damage;
		}
	}

	if (attacks(AD_STUN, otmp) && dieroll <= MB_MAX_DIEROLL) {
	    /* Magicbane's special attacks (possibly modifies hittee[]) */
	    return Mb_hit(magr, mdef, otmp, dmgptr, dieroll, vis, hittee);
	}

	/* Sunsword burns anything that is not fire resistant */
	if (otmp->oartifact == ART_SUNSWORD &&
		!(youdefend ? (how_resistant(FIRE_RES) > 99) : resists_fire(mdef)))
	    *dmgptr += rnd(6);

	/* Ichigeki: 1/20 chance to strike a fatal weakness in anything that
	   has a definite shape (not shapeless, oozy or incorporeal) */
	if (otmp->oartifact == ART_ICHIGEKI && !youdefend && !rn2(20) &&
		!amorphous(mdef->data) && !noncorporeal(mdef->data) &&
		!unsolid(mdef->data) && !is_rider(mdef->data) &&
		mdef->data->mlet != S_PUDDING && mdef->data->mlet != S_JELLY &&
		mdef->data->mlet != S_BLOB && !(youattack && u.uswallow)) {
	    *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
	    if (youattack || canseemon(mdef))
		pline("Ichigeki hits %s fatal weakness!",
		      s_suffix(mon_nam(mdef)));
	    otmp->dknown = TRUE;
	    return TRUE;
	}

	/* Mouser's Scalpel: 1/5 chance of an extra blow, which may chain */
	if (otmp->oartifact == ART_MOUSER_S_SCALPEL && !youdefend && !rn2(5)) {
	    int blows = 0;
	    do {
		*dmgptr += rnd(8) + 1 + otmp->spe;
		blows++;
	    } while (blows < 6 && rn2(11) < 5);
	    if (youattack || canseemon(mdef))
		pline("%s %s %s %s!", youattack ? "You" : Monnam(magr),
		      youattack ? "strike" : "strikes",
		      blows > 1 ? "again and again at" : "again at",
		      mon_nam(mdef));
	}

	/* Reaver snatches an item from the foe: a Pirate does so one hit in
	   five, anyone else one in ten */
	if (otmp->oartifact == ART_REAVER && youattack && !youdefend &&
		mdef->minvent && !rn2(Role_if(PM_PIRATE) ? 5 : 10))
	    reaver_steal(mdef);

	/* Spineseeker does double damage to fleeing foes */
	if (otmp->oartifact == ART_SPINESEEKER && !youdefend && mdef->mflee)
	    *dmgptr *= 2;

	/* Luck Blade: sometimes strikes deep (1 in 14 - Luck, so every time
	   at the maximum Luck of 13) */
	if (otmp->oartifact == ART_LUCK_BLADE && youattack &&
		!rn2(max(1, 14 - Luck))) {
	    You("strike deep into %s!", mon_nam(mdef));
	    *dmgptr += d(2,8);
	    return TRUE;
	}

	/* Ravenbrand: darkness slows, lacerates and blinds */
	if (otmp->oartifact == ART_RAVENBRAND) {
	    boolean slowed = FALSE, hurt = FALSE, blinded = FALSE;
	    char who[BUFSZ];

	    if (!rn2(5)) {
		if (youdefend) {
		    if (HFast) { HFast = 0L; slowed = TRUE; }
		} else if (mdef->mspeed != MSLOW) {
		    mon_adjust_speed(mdef, -2, (struct obj *)0);
		    slowed = TRUE;
		}
	    }
	    if (!rn2(3)) { *dmgptr += rnd(10); hurt = TRUE; }
	    if (!rn2(5)) {
		if (youdefend) {
		    if (!Blind_resistance && !Blind) {
			make_blinded(Blinded + (long)rnd(20), FALSE);
			blinded = TRUE;
		    }
		} else if (haseyes(mdef->data) && mdef->mcansee &&
			   !resists_blnd(mdef)) {
		    mdef->mcansee = 0;
		    mdef->mblinded = rn1(20, 10);
		    blinded = TRUE;
		}
	    }
	    Strcpy(who, youdefend ? "you" : mon_nam(mdef));
	    if (realizes_damage) {
		if (slowed && hurt)
		    pline("%s %s entangled by barbed chains of darkness!",
			  youdefend ? "You" : upstart(strcpy(buf2, who)),
			  youdefend ? "are" : "is");
		else if (slowed)
		    pline("Strands of darkness entangle %s!", who);
		else if (hurt)
		    pline("Shards of darkness lacerate %s!", who);
		if (blinded)
		    pline("Darkness clings to %s!", who);
		if (slowed || hurt || blinded) return TRUE;
	    }
	}

	if (!spec_dbon_applies) {
	    /* since damage bonus didn't apply, nothing more to do;  
	       no further attacks have side-effects on inventory */
	    return FALSE;
	}

	/* Are we about to do something special vs. a monster type? */
	atmp = &artilist[otmp->oartifact];
	if (otmp->oclass == GEM_CLASS && uwep && uwep->oartifact == ART_GIANTSLAYER) {
		atmp = &artilist[ART_GIANTSLAYER];
		otmp->oartifact = ART_GIANTSLAYER;	/* really miserable hack */
	}
	if (atmp->spfx & (SPFX_DFLAG2 | SPFX_DCLAS)) {
		j = rn2(2);	  /* 50% chance of instakill for some artifacts */
		switch (otmp->oartifact) {
			case ART_DRAMBORLEG:
				/* Dramborleg slays balrogs outright */
				if (!youdefend && mdef->data == &mons[PM_BALROG]) {
					if (youattack)
					    You("obliterate %s with a powerful strike!",
						mon_nam(mdef));
					*dmgptr = (2 * mdef->mhp + FATAL_DAMAGE_MODIFIER);
					return TRUE;
				}
				return FALSE;
			case ART_DRAGONBANE:
				if (youattack && j) {
					You("pierce the heart of %s!", mon_nam(mdef));
					*dmgptr = (2 * mdef->mhp + FATAL_DAMAGE_MODIFIER);
				} else if (youdefend && j) {
					pline("The deadly spear pierces your heart!");
					*dmgptr = (2 * Upolyd ? u.mh : u.uhp + FATAL_DAMAGE_MODIFIER);
				} else {
					return FALSE;
				}
				return TRUE;
			case ART_WEREBANE:
				if (youattack && j) {
					*dmgptr = (2 * mdef->mhp + FATAL_DAMAGE_MODIFIER);
				} else if (youdefend && j) {
					*dmgptr = (2 * Upolyd ? u.mh : u.uhp + FATAL_DAMAGE_MODIFIER);
				} else {
					return FALSE;
				}
				return TRUE;
			case ART_GIANTSLAYER:
				if (otmp->oclass == GEM_CLASS) {	 /* second part of miserable hack */
					otmp->oartifact = 0L;
				}
				if (youattack) {
					You("strike %s in the forehead!",mon_nam(mdef));
					*dmgptr = (2 * mdef->mhp + FATAL_DAMAGE_MODIFIER);
				} else if (youdefend) {
					*dmgptr = (2 * Upolyd ? u.mh : u.uhp + FATAL_DAMAGE_MODIFIER);
					You("are hit in the center of your forehead!");
				}
				return TRUE;
			case ART_OGRESMASHER:
				if (youattack && j) {
					You("crush the skull of %s!", mon_nam(mdef));
					*dmgptr = (2 * mdef->mhp + FATAL_DAMAGE_MODIFIER);
				} else if (youdefend && j) {
					pline("The monstrous hammer crushes your skull!");
					*dmgptr = (2 * Upolyd ? u.mh : u.uhp + FATAL_DAMAGE_MODIFIER);
				} else {
					return FALSE;
				}
				return TRUE;
			case ART_TROLLSBANE:
				if (youattack && j) {
					pline("As you strike %s, it bursts into flame!", mon_nam(mdef));
					*dmgptr = (2 * mdef->mhp + FATAL_DAMAGE_MODIFIER);
				} else if (youdefend && j) {
					You("burst into flame as you are hit!");
					*dmgptr = (2 * Upolyd ? u.mh : u.uhp + FATAL_DAMAGE_MODIFIER);
				} else {
					return FALSE;
				}
				return TRUE;
			/* below this we don't get any additional handling, so drop through
			* just listed here for potential future reference */
			case ART_DEMONBANE:
			case ART_SUNSWORD:
			case ART_ORCRIST:
			case ART_STING:
			default:
				break;
		}

	}

	/* We really want "on a natural 20" but Nethack does it in */
	/* reverse from AD&D. */
	if (spec_ability(otmp, SPFX_BEHEAD)) {
	    if (otmp->oartifact == ART_TSURUGI_OF_MURAMASA && dieroll == 1) {
		wepdesc = "The razor-sharp blade";
		/* not really beheading, but so close, why add another SPFX */
		if (youattack && u.uswallow && mdef == u.ustuck) {
		    You("slice %s wide open!", mon_nam(mdef));
		    *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
		    return TRUE;
		}
		if (!youdefend) {
			/* allow normal cutworm() call to add extra damage */
			if(notonhead)
			    return FALSE;

			if (bigmonst(mdef->data)) {
				if (youattack)
					You("slice deeply into %s!",
						mon_nam(mdef));
				else if (vis)
					pline("%s cuts deeply into %s!",
					      Monnam(magr), hittee);
				*dmgptr *= 2;
				return TRUE;
			}
			*dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
			pline("%s cuts %s in half!", wepdesc, mon_nam(mdef));
			otmp->dknown = TRUE;
			return TRUE;
		} else {
			if (bigmonst(youmonst.data)) {
				pline("%s cuts deeply into you!",
				      magr ? Monnam(magr) : wepdesc);
				*dmgptr *= 2;
				return TRUE;
			}

			/* Players with negative AC's take less damage instead
			 * of just not getting hit.  We must add a large enough
			 * value to the damage so that this reduction in
			 * damage does not prevent death.
			 */
			*dmgptr = 2 * (Upolyd ? u.mh : u.uhp) + FATAL_DAMAGE_MODIFIER;
			pline("%s cuts you in half!", wepdesc);
			otmp->dknown = TRUE;
			return TRUE;
		}
	    } else if (otmp->oartifact == ART_VORPAL_BLADE &&
			(dieroll == 1 || mdef->data == &mons[PM_JABBERWOCK])) {
		static const char * const behead_msg[2] = {
		     "%s beheads %s!",
		     "%s decapitates %s!"
		};

		if (youattack && u.uswallow && mdef == u.ustuck)
			return FALSE;
		wepdesc = artilist[ART_VORPAL_BLADE].name;
		if (!youdefend) {
			if (!has_head(mdef->data) || notonhead || u.uswallow) {
				if (youattack)
					pline("Somehow, you miss %s wildly.",
						mon_nam(mdef));
				else if (vis)
					pline("Somehow, %s misses wildly.",
						mon_nam(magr));
				*dmgptr = 0;
				return ((boolean)(youattack || vis));
			}
			if (noncorporeal(mdef->data) || amorphous(mdef->data)) {
				pline("%s slices through %s %s.", wepdesc,
				      s_suffix(mon_nam(mdef)),
				      mbodypart(mdef,NECK));
				return TRUE;
			}
			*dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
			pline(behead_msg[rn2(SIZE(behead_msg))],
			      wepdesc, mon_nam(mdef));
			otmp->dknown = TRUE;
			return TRUE;
		} else {
			if (!has_head(youmonst.data)) {
				pline("Somehow, %s misses you wildly.",
				      magr ? mon_nam(magr) : wepdesc);
				*dmgptr = 0;
				return TRUE;
			}
			if (noncorporeal(youmonst.data) || amorphous(youmonst.data)) {
				pline("%s slices through your %s.",
				      wepdesc, body_part(NECK));
				return TRUE;
			}
			*dmgptr = 2 * (Upolyd ? u.mh : u.uhp)
				  + FATAL_DAMAGE_MODIFIER;
			pline(behead_msg[rn2(SIZE(behead_msg))],
			      wepdesc, "you");
			otmp->dknown = TRUE;
			/* Should amulets fall off? */
			return TRUE;
		}
	    }
	}
	if (spec_ability(otmp, SPFX_DRLI)) {
		if (!youdefend) {
			if (vis) {
			    if(otmp->oartifact == ART_STORMBRINGER)
				pline_The("%s blade draws the life from %s!",
				      hcolor(NH_BLACK),
				      mon_nam(mdef));
			    else
				pline("%s draws the life from %s!",
				      The(distant_name(otmp, xname)),
				      mon_nam(mdef));
			}
			if (mdef->m_lev == 0) {
			    *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
			} else {
			    int drain = rnd(8);
			    *dmgptr += drain;
			    mdef->mhpmax -= drain;
			    mdef->m_lev--;
			    drain /= 2;
			    if (drain) healup(drain, 0, FALSE, FALSE);
			}
			return vis;
		} else { /* youdefend */
			int oldhpmax = u.uhpmax;

			if (Blind)
				You_feel("an %s drain your life!",
				    otmp->oartifact == ART_STORMBRINGER ?
				    "unholy blade" : "object");
			else if (otmp->oartifact == ART_STORMBRINGER)
				pline_The("%s blade drains your life!",
				      hcolor(NH_BLACK));
			else
				pline("%s drains your life!",
				      The(distant_name(otmp, xname)));
			losexp("life drainage");
			if (magr && magr->mhp < magr->mhpmax) {
			    magr->mhp += (oldhpmax - u.uhpmax)/2;
			    if (magr->mhp > magr->mhpmax) magr->mhp = magr->mhpmax;
			}
			return TRUE;
		}
	}
	return FALSE;
}

static NEARDATA const char recharge_type[] = { ALLOW_COUNT, ALL_CLASSES, 0 };
static NEARDATA const char invoke_types[] = { ALL_CLASSES, 0 };
		/* #invoke: an "ugly check" filters out most objects */

/*** The Pirate's artifacts ***/

/* the Treasury of Proteus, if you carry it in your open inventory */
struct obj *
carried_treasury()
{
	struct obj *otmp;

	for (otmp = invent; otmp; otmp = otmp->nobj)
	    if (otmp->oartifact == ART_TREASURY_OF_PROTEUS) return otmp;
	return (struct obj *)0;
}

/* a Pirate carrying the Marauder's Map makes artifacts likelier */
boolean
carrying_marauders_map(pirate_only)
boolean pirate_only;
{
	struct obj *otmp;

	if (pirate_only && !Role_if(PM_PIRATE)) return FALSE;
	for (otmp = invent; otmp; otmp = otmp->nobj)
	    if (otmp->oartifact == ART_MARAUDER_S_MAP) return TRUE;
	return FALSE;
}

/* odds are 1 in n normally, but four times better for a Pirate who
   carries the Marauder's Map */
int
artifact_chance(n)
int n;
{
	if (carrying_marauders_map(TRUE)) n = max(1, n / 4);
	return n;
}

/* The Treasury absorbs curses meant for what you carry.  It always does
   if it is not itself cursed, and otherwise 19 times in 20; a blessed
   Treasury becomes uncursed and an uncursed one becomes cursed. */
boolean
treasury_blocks_curse(otmp)
struct obj *otmp;
{
	struct obj *tr = carried_treasury();
	struct obj *top;

	if (!tr || otmp == tr) return FALSE;
	/* only for things you carry (or that are in what you carry) */
	for (top = otmp; top->where == OBJ_CONTAINED; top = top->ocontainer)
	    ;
	if (top->where != OBJ_INVENT) return FALSE;
	if (tr->cursed && !rn2(20)) return FALSE;
	if (tr->blessed) unbless(tr);
	else if (!tr->cursed) curse(tr);
	pline("%s has just blocked a curse.", The(xname(tr)));
	return TRUE;
}

/* the kind of pirate that comes after a Pirate holding the Treasury */
int
king_pirate_type()
{
	if (In_hell(&u.uz) || In_endgame(&u.uz)) return PM_DAMNED_PIRATE;
	return PM_SKELETAL_PIRATE;
}

/* Reaver snatches a random unworn item from its victim */
STATIC_OVL void
reaver_steal(mdef)
struct monst *mdef;
{
	struct obj *otmp, *pick = (struct obj *)0;
	int n = 0;

	for (otmp = mdef->minvent; otmp; otmp = otmp->nobj) {
	    if (otmp->owornmask) continue;	/* worn or wielded */
	    /* don't touch a cockatrice corpse bare-handed */
	    if (otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm])
		    && !uarmg) continue;
	    if (otmp->otyp == LEASH && otmp->leashmon) continue;
	    if (!rn2(++n)) pick = otmp;
	}
	if (!pick) return;
	obj_extract_self(pick);
	pick->owornmask = 0L;
	You("snatch %s from %s!", doname(pick), mon_nam(mdef));
	(void) hold_another_object(pick, "Ye snatched but dropped %s.",
				   doname(pick), (const char *)0);
}

/* recursively sense the artifacts on an object list */
STATIC_OVL void
sense_artifacts(list, found)
struct obj *list;
int *found;
{
	struct obj *otmp;

	for (otmp = list; otmp; otmp = otmp->nobj) {
	    if (otmp->oartifact) {
		You_feel("the presence of %s.", the(artiname(otmp->oartifact)));
		(*found)++;
	    }
	    if (Has_contents(otmp)) sense_artifacts(otmp->cobj, found);
	}
}

/* #invoke the Marauder's Map: detect objects, and reveal the artifacts */
STATIC_OVL void
detect_artifacts(map)
struct obj *map;
{
	struct monst *mtmp;
	int found = 0;

	(void) object_detect(map, 0);
	sense_artifacts(fobj, &found);
	sense_artifacts(level.buriedobjlist, &found);
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    sense_artifacts(mtmp->minvent, &found);
	}
	if (!found) pline("There are no artifacts to be found here.");
}

/* #invoke the Treasury of Proteus: the sea changes one thing inside it */
STATIC_OVL int
sea_change(chest)
struct obj *chest;
{
	struct obj *item, *newobj;
	winid win;
	anything any;
	menu_item *pick = (menu_item *)0;
	int n, count = 0;

	for (item = chest->cobj; item; item = item->nobj)
	    if (!item->oartifact && !objects[item->otyp].oc_unique &&
		    !(Is_container(item) && Has_contents(item)))
		count++;
	if (!count) {
	    pline("There is nothing inside %s that the sea could change.",
		  the(xname(chest)));
	    return 0;
	}
	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	any.a_void = 0;
	for (item = chest->cobj; item; item = item->nobj) {
	    if (item->oartifact || objects[item->otyp].oc_unique ||
		    (Is_container(item) && Has_contents(item)))
		continue;
	    any.a_obj = item;
	    add_menu(win, obj_to_glyph(item), &any, 0, 0, ATR_NONE,
		     doname(item), MENU_UNSELECTED);
	}
	end_menu(win, "Let the sea change which item?");
	n = select_menu(win, PICK_ONE, &pick);
	destroy_nhwindow(win);
	if (n <= 0) return 0;
	item = pick[0].item.a_obj;
	free((genericptr_t)pick);

	You_hear("distant waves as the sea changes %s.", doname(item));
	newobj = poly_obj(item, STRANGE_OBJECT);
	chest->owt = weight(chest);
	if (newobj) pline("Now it is %s.", doname(newobj));
	if (carried(chest)) update_inventory();
	return 1;
}

int
doinvoke()
{
    register struct obj *obj;

    obj = getobj(invoke_types, "invoke");
    if (!obj) return 0;
    if (obj->oartifact && !touch_artifact(obj, &youmonst)) return 1;
    return arti_invoke(obj);
}

STATIC_OVL int
arti_invoke(obj)
    register struct obj *obj;
{
    register const struct artifact *oart = get_artifact(obj);

    if(!oart || !oart->inv_prop) {
	if(obj->otyp == CRYSTAL_BALL)
	    use_crystal_ball(obj);
	else
	    pline("%s", nothing_happens);
	return 1;
    }

    if(oart->inv_prop > LAST_PROP) {
	/* It's a special power, not "just" a property */
	if(obj->age > monstermoves) {
	    /* the artifact is tired :-) */
	    You_feel("that %s %s ignoring you.",
		     the(xname(obj)), otense(obj, "are"));
	    /* and just got more so; patience is essential... */
	    obj->age += (long) d(3,10);
	    return 1;
	}
	obj->age = monstermoves + rnz(100);

	switch(oart->inv_prop) {
	case TAMING: {
	    struct obj pseudo;

	    pseudo = zeroobj;	/* neither cursed nor blessed */
	    pseudo.otyp = SCR_TAMING;
	    (void) seffects(&pseudo);
	    break;
	  }
	case HEALING: {
	    int healamt = (u.uhpmax + 1 - u.uhp) / 2;
	    long creamed = (long)u.ucreamed;

	    if (Upolyd) healamt = (u.mhmax + 1 - u.mh) / 2;
	    if (healamt || Sick || Slimed || Blinded > creamed)
		You_feel("better.");
	    else
		goto nothing_special;
	    if (healamt > 0) {
		if (Upolyd) u.mh += healamt;
		else u.uhp += healamt;
	    }
	    if(Sick) make_sick(0L,(char *)0,FALSE,SICK_ALL);
	    if(Slimed) Slimed = 0L;
	    if (Blinded > creamed) make_blinded(creamed, FALSE);
	    flags.botl = 1;
	    break;
	  }
	case ENERGY_BOOST: {
	    int epboost = (u.uenmax + 1 - u.uen) / 2;
	    if (epboost > 120) epboost = 120;		/* arbitrary */
	    else if (epboost < 12) epboost = u.uenmax - u.uen;
	    if(epboost) {
		You_feel("re-energized.");
		u.uen += epboost;
		flags.botl = 1;
	    } else
		goto nothing_special;
	    break;
	  }
	case UNTRAP: {
	    if(!untrap(TRUE)) {
		obj->age = 0; /* don't charge for changing their mind */
		return 0;
	    }
	    break;
	  }
	case CHARGE_OBJ: {
	    struct obj *otmp = getobj(recharge_type, "charge");
	    boolean b_effect;

	    if (!otmp) {
		obj->age = 0;
		return 0;
	    }
	    b_effect = obj->blessed &&
		(Role_switch == oart->role || !oart->role);
	    recharge(otmp, b_effect ? 1 : obj->cursed ? -1 : 0);
	    update_inventory();
	    break;
	  }
	case LEV_TELE:
	    level_tele();
	    break;
	case CREATE_PORTAL: {
	    int i, num_ok_dungeons, last_ok_dungeon = 0;
	    d_level newlev;
	    extern int n_dgns; /* from dungeon.c */
	    winid tmpwin = create_nhwindow(NHW_MENU);
	    anything any;

	    any.a_void = 0;	/* set all bits to zero */
	    start_menu(tmpwin);
	    /* use index+1 (cant use 0) as identifier */
	    for (i = num_ok_dungeons = 0; i < n_dgns; i++) {
		if (!dungeons[i].dunlev_ureached) continue;
		any.a_int = i+1;
		add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
			 dungeons[i].dname, MENU_UNSELECTED);
		num_ok_dungeons++;
		last_ok_dungeon = i;
	    }
	    end_menu(tmpwin, "Open a portal to which dungeon?");
	    if (num_ok_dungeons > 1) {
		/* more than one entry; display menu for choices */
		menu_item *selected;
		int n;

		n = select_menu(tmpwin, PICK_ONE, &selected);
		if (n <= 0) {
		    destroy_nhwindow(tmpwin);
		    goto nothing_special;
		}
		i = selected[0].item.a_int - 1;
		free((genericptr_t)selected);
	    } else
		i = last_ok_dungeon;	/* also first & only OK dungeon */
	    destroy_nhwindow(tmpwin);

	    /*
	     * i is now index into dungeon structure for the new dungeon.
	     * Find the closest level in the given dungeon, open
	     * a use-once portal to that dungeon and go there.
	     * The closest level is either the entry or dunlev_ureached.
	     */
	    newlev.dnum = i;
	    if(dungeons[i].depth_start >= depth(&u.uz))
		newlev.dlevel = dungeons[i].entry_lev;
	    else
		newlev.dlevel = dungeons[i].dunlev_ureached;
	    if(u.uhave.amulet || In_endgame(&u.uz) || In_endgame(&newlev) ||
	       newlev.dnum == u.uz.dnum) {
		You_feel("very disoriented for a moment.");
	    } else {
		if(!Blind) You("are surrounded by a shimmering sphere!");
		else You_feel("weightless for a moment.");
		goto_level(&newlev, FALSE, FALSE, FALSE);
	    }
	    break;
	  }
	case ENLIGHTENING:
	    enlightenment(0);
	    break;
	case DETECT_ARTIFACTS:
	    detect_artifacts(obj);
	    break;
	case SEA_CHANGE:
	    if (!sea_change(obj)) {
		obj->age = monstermoves;
		return 0;
	    }
	    break;
	case CREATE_YA: {
	    struct obj *otmp = mksobj(YA, FALSE, FALSE);

	    if (!otmp) goto nothing_special;
	    otmp->quan = (long) d(2,12);
	    otmp->blessed = obj->blessed;
	    otmp->cursed = obj->cursed;
	    otmp->bknown = obj->bknown;
	    otmp->spe = obj->spe;
	    otmp->owt = weight(otmp);
	    otmp = hold_another_object(otmp, "Suddenly %s out.",
				       aobjnam(otmp, "fall"), (const char *)0);
	    break;
	  }
	case WIND_BLAST: {
	    struct obj pseudo;

	    if (!getdir((char *)0)) {
		pline("%s", Never_mind);
		obj->age = monstermoves;
		return 0;
	    }
	    pseudo = zeroobj;	/* act like a wand of wind */
	    pseudo.otyp = WAN_WIND;
	    pseudo.oclass = WAND_CLASS;
	    pseudo.quan = 1L;
	    pseudo.spe = 1;
	    if (!u.dx && !u.dy && !u.dz)
		(void) zapyourself(&pseudo, TRUE);
	    else
		weffects(&pseudo);
	    break;
	  }
	case TIMED_COLD_RES:
	    if (!u.ucoldres) You_feel("a chill run through you.");
	    else You_feel("the chill deepen.");
	    u.ucoldres += 100 + d(3,100);
	    ECold_resistance |= W_ARTI;
	    break;
	case FLING_POISON: {
	    struct obj *otmp;

	    if (!getdir((char *)0)) {
		pline("%s", Never_mind);
		obj->age = monstermoves;
		return 0;
	    }
	    otmp = mksobj(rn2(2) ? BLINDING_VENOM : ACID_VENOM, TRUE, FALSE);
	    otmp->spe = 1;	/* the poison is yours */
	    throwit(otmp, 0L, FALSE);
	    break;
	  }
	case CREATE_AMMO: {
	    struct obj *otmp = mksobj(ARROW, TRUE, FALSE);

	    if (!otmp) goto nothing_special;
	    otmp->blessed = obj->blessed;
	    otmp->cursed = obj->cursed;
	    otmp->bknown = obj->bknown;
	    if (obj->blessed) {
		if (otmp->spe < 0) otmp->spe = 0;
		otmp->quan += rnd(10);
	    } else if (obj->cursed) {
		if (otmp->spe > 0) otmp->spe = 0;
	    } else
		otmp->quan += rnd(5);
	    otmp->owt = weight(otmp);
	    otmp = hold_another_object(otmp, "Suddenly %s out.",
				       aobjnam(otmp, "fall"), (const char *)0);
	    break;
	  }
	}
    } else {
	long eprop = (u.uprops[oart->inv_prop].extrinsic ^= W_ARTI),
	     iprop = u.uprops[oart->inv_prop].intrinsic;
	boolean on = (eprop & W_ARTI) != 0; /* true if invoked prop just set */

	if(on && obj->age > monstermoves) {
	    /* the artifact is tired :-) */
	    u.uprops[oart->inv_prop].extrinsic ^= W_ARTI;
	    You_feel("that %s %s ignoring you.",
		     the(xname(obj)), otense(obj, "are"));
	    /* can't just keep repeatedly trying */
	    obj->age += (long) d(3,10);
	    return 1;
	} else if(!on) {
	    /* when turning off property, determine downtime */
	    /* arbitrary for now until we can tune this -dlc */
	    obj->age = monstermoves + rnz(100);
	}

	if ((eprop & ~W_ARTI) || iprop) {
nothing_special:
	    /* you had the property from some other source too */
	    if (carried(obj))
		You_feel("a surge of power, but nothing seems to happen.");
	    return 1;
	}
	switch(oart->inv_prop) {
	case CONFLICT:
	    if(on) You_feel("like a rabble-rouser.");
	    else You_feel("the tension decrease around you.");
	    break;
	case LEVITATION:
	    if(on) {
		float_up();
		spoteffects(FALSE);
	    } else (void) float_down(I_SPECIAL|TIMEOUT, W_ARTI);
	    break;
	case INVIS:
	    if (BInvis || Blind) goto nothing_special;
	    newsym(u.ux, u.uy);
	    if (on)
		Your("body takes on a %s transparency...",
		     Hallucination ? "normal" : "strange");
	    else
		Your("body seems to unfade...");
	    break;
	}
    }

    return 1;
}


/* may artifact (or ordinary weapon) "sec" be held in the off hand while
   "pri" is the primary weapon?  Only certain artifacts accept it. */
boolean
artifact_offhand_ok(pri, sec)
struct obj *pri, *sec;
{
    int p = pri ? pri->oartifact : 0;

    if (!sec || !sec->oartifact) return TRUE;
    /* only artifact weapons are restricted; other artifacts never matter */
    if (sec->oclass != WEAPON_CLASS && !is_weptool(sec)) return TRUE;
    /* Grayswandir, Frost Brand and Pernicimordax will not accept any
       off-hand artifact (and none of them is accepted in the off hand) */
    if (p == ART_GRAYSWANDIR || p == ART_FROST_BRAND ||
	    p == ART_PERNICIMORDAX) return FALSE;

    switch (sec->oartifact) {
	case ART_GRIMTOOTH:
	    return (boolean)(p != ART_STING && p != ART_ORCRIST);
	case ART_STING:
	case ART_ORCRIST:
	    return (boolean)(p != ART_GRIMTOOTH);
	case ART_KUSANAGI:
	    return (boolean)(p == ART_KIKU_ICHIMONJI || p == ART_ICHIGEKI);
	case ART_ICHIGEKI:
	    return (boolean)(p == ART_KIKU_ICHIMONJI || p == ART_KUSANAGI);
	case ART_KIKU_ICHIMONJI:
	    return (boolean)(p == ART_ICHIGEKI || p == ART_KUSANAGI);
	/* Spineseeker and the Banes (but not Sunsword) go with anything */
	case ART_SPINESEEKER:
	case ART_DRAGONBANE:
	case ART_DEMONBANE:
	case ART_WEREBANE:
	case ART_TROLLSBANE:
	case ART_OGRESMASHER:
	    return TRUE;
	/* Quicksilver and the Luck Blade are a pair */
	case ART_LUCK_BLADE:
	    return (boolean)(p == ART_QUICKSILVER);
	case ART_QUICKSILVER:
	    return (boolean)(p == ART_LUCK_BLADE);
	/* only a Pirate can hold Reaver in the off hand */
	case ART_REAVER:
	    return (boolean)Role_if(PM_PIRATE);
	default:
	    break;
    }
    return FALSE;
}

/* WAC return TRUE if artifact is always lit */
boolean
artifact_light(obj)
    struct obj *obj;
{
    return (get_artifact(obj) && (obj->oartifact == ART_SUNSWORD ||
				       obj->oartifact == ART_CLAIOMH_SOLAIS));
}

/* KMH -- Talking artifacts are finally implemented */
void
arti_speak(obj)
    struct obj *obj;
{
	register const struct artifact *oart = get_artifact(obj);
	const char *line;
	char buf[BUFSZ];


	/* Is this a speaking artifact? */
	if (!oart || !(oart->spfx & SPFX_SPEAK))
		return;

	line = getrumor(bcsign(obj), buf, TRUE);
	if (!*line)
		line = "NetHack rumors file closed for renovation.";
	pline("%s:", Tobjnam(obj, "whisper"));
	verbalize("%s", line);
	return;
}

boolean
artifact_has_invprop(otmp, inv_prop)
struct obj *otmp;
uchar inv_prop;
{
	const struct artifact *arti = get_artifact(otmp);

	return((boolean)(arti && (arti->inv_prop == inv_prop)));
}

/* Return the price sold to the hero of a given artifact or unique item */
long
arti_cost(otmp)
struct obj *otmp;
{
	if (!otmp->oartifact)
	    return ((long)objects[otmp->otyp].oc_cost);
	else if (artilist[(int) otmp->oartifact].cost)
	    return (artilist[(int) otmp->oartifact].cost);
	else
	    return (100L * (long)objects[otmp->otyp].oc_cost);
}

#endif /* OVLB */

/*artifact.c*/
