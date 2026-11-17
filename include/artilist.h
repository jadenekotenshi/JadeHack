/*	SCCS Id: @(#)artilist.h 3.4	2003/02/12	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef MAKEDEFS_C
/* in makedefs.c, all we care about is the list of names */

#define A(nam,typ,s1,s2,mt,atk,dfn,cry,inv,al,cl,rac,cost) nam

static const char *artifact_names[] = {
#else
/* in artifact.c, set up the actual artifact list structure */

#define A(nam,typ,s1,s2,mt,atk,dfn,cry,inv,al,cl,rac,cost) \
 { typ, nam, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac, cost }

#define     NO_ATTK	{0,0,0,0}		/* no attack */
#define     NO_CARY	{0,0,0,0}		/* no carry effects */
#define     DFNS(c)	{0,c,0,0}
#define     CARY(c)	{0,c,0,0}
#define     PHYS(a,b)	{0,AD_PHYS,a,b}		/* physical */
#define     DRLI(a,b)	{0,AD_DRLI,a,b}		/* life drain */
#define     COLD(a,b)	{0,AD_COLD,a,b}
#define     FIRE(a,b)	{0,AD_FIRE,a,b}
#define     ELEC(a,b)	{0,AD_ELEC,a,b}		/* electrical shock */
#define     STUN(a,b)	{0,AD_STUN,a,b}		/* magical attack */
#define		DRST(a,b)	{0,AD_DRST,a,b}		/* poison attack */

STATIC_OVL NEARDATA struct artifact artilist[] = {
#endif	/* MAKEDEFS_C */

/* Artifact cost rationale:
 * 1.  The more useful the artifact, the better its cost.
 * 2.  Quest artifacts are highly valued.
 * 3.  Chaotic artifacts are inflated due to scarcity (and balance).
 */


/*  dummy element #0, so that all interesting indices are non-zero */
A("",				STRANGE_OBJECT,
	0, 0, 0, NO_ATTK, SPDF_NONE, NO_CARY, 0, A_NONE, NON_PM, NON_PM, 0L ),

A("Excalibur",			LONG_SWORD,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_SEEK|SPFX_DEFN|SPFX_INTEL|SPFX_SEARCH),0,0,
	PHYS(5,10),	SPDF_DRAIN,	NO_CARY,	0, A_LAWFUL, PM_KNIGHT, NON_PM, 4000L ),
/* 
 * The Knight needed a chaotic longsword to obtain, since Excalibur
 * will be restricted from him... 
*/
A("Dirge",			LONG_SWORD,
	(SPFX_ATTK|SPFX_NOGEN|SPFX_RESTR|SPFX_DEFN|SPFX_INTEL),0,0,
	DRST(5,0),	SPDF_DRAIN,	NO_CARY,	0, A_CHAOTIC, NON_PM, NON_PM, 3000L ),
/*
 *	Stormbringer only has a 2 because it can drain a level,
 *	providing 8 more.
 */
A("Stormbringer",		RUNESWORD,
	(SPFX_RESTR|SPFX_ATTK|SPFX_DEFN|SPFX_INTEL|SPFX_DRLI), 0, 0,
	DRLI(5,2),	SPDF_DRAIN,	NO_CARY,	0, A_CHAOTIC, NON_PM, NON_PM, 8000L ),
/*
 * Lifestealer gets the same restrictions as Stormy, but it's designed
 * to be generated in Vlad's hands anyway, so this isn't such a big deal.
 * It's two-handed so players (hopefully) won't really want to use it...
 */
A("Lifestealer",		TWO_HANDED_SWORD,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_ATTK|SPFX_DEFN|SPFX_INTEL|SPFX_DRLI), 0, 0,
	DRLI(5,2),	SPDF_DRAIN,	NO_CARY,	0, A_CHAOTIC, NON_PM, NON_PM, 8000L ),
/*
 *	Mjollnir will return to the hand of the wielder when thrown
 *	if the wielder is a Valkyrie wearing Gauntlets of Power.
 */
A("Mjollnir",			WAR_HAMMER,		/* Mjo:llnir */
	(SPFX_RESTR|SPFX_ATTK),  0, 0,
	ELEC(5,24),	SPDF_NONE,	NO_CARY,	0, A_NEUTRAL, PM_VALKYRIE, NON_PM, 4000L ),

A("Cleaver",			BATTLE_AXE,
	SPFX_RESTR, 0, 0,
	PHYS(3,6),	SPDF_NONE,	NO_CARY,	0, A_NEUTRAL, PM_BARBARIAN, NON_PM, 1500L ),

/*
 *	Grimtooth (as in NetHack 5.0): warns of elves, but its damage bonus
 *	applies to everything (special case in spec_dbon()).  Permanently
 *	poisoned, protects from poison when wielded, and can fling venom
 *	when invoked.
 */
A("Grimtooth",			ORCISH_DAGGER,
	(SPFX_RESTR|SPFX_WARN|SPFX_DFLAG2), 0, M2_ELF,
	PHYS(2,6),	SPDF_POISON,	NO_CARY,	FLING_POISON, A_CHAOTIC, NON_PM, PM_ORC, 1200L ),
/*
 *	Orcrist and Sting have same alignment as elves.
 *	The combination of SPFX_WARN and M2_something on an artifact
 *	will trigger EWarn_of_mon for all monsters that have the appropriate
 *	M2_something flags.  In Sting's case it will trigger EWarn_of_mon
 *	for M2_SPIDER monsters; in Orcrist, for M2_ORC.
 */
A("Orcrist",			ELVEN_BROADSWORD,
	(SPFX_WARN|SPFX_DCLAS), 0, S_ORC,
	PHYS(5,4),	SPDF_NONE,	NO_CARY,	0, A_CHAOTIC, NON_PM, PM_ELF, 2000L ),

A("Sting",			ELVEN_DAGGER,
	(SPFX_WARN|SPFX_DCLAS), 0, S_SPIDER,
	PHYS(5,3),	SPDF_NONE,	NO_CARY,	0, A_CHAOTIC, NON_PM, PM_ELF, 800L ),
/*
 *	Magicbane is a bit different!  Its magic fanfare
 *	unbalances victims in addition to doing some damage.
 */
A("Magicbane",			ATHAME,
	(SPFX_RESTR|SPFX_ATTK|SPFX_DEFN), 0, 0,
	STUN(3,4),	SPDF_MAGIC,	NO_CARY,	0, A_NEUTRAL, PM_WIZARD, NON_PM, 3500L ),

A("Frost Brand",		BROADSWORD,
	(SPFX_RESTR|SPFX_ATTK|SPFX_DEFN), 0, 0,
	COLD(5,0),	SPDF_COLD,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 3000L ),

A("Fire Brand",			TWO_HANDED_SWORD,
	(SPFX_RESTR|SPFX_ATTK|SPFX_DEFN), 0, 0,
	FIRE(5,0),	SPDF_FIRE,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 3000L ),

A("Dragonbane",			DWARVISH_SPEAR,
	(SPFX_WARN|SPFX_RESTR|SPFX_DCLAS), 0, S_DRAGON,
	PHYS(5,4),	SPDF_ELEMENTAL,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 1500L ),

A("Demonbane",			SILVER_MACE,
	(SPFX_WARN|SPFX_RESTR|SPFX_DCLAS), 0, S_DEMON,
	PHYS(5,4),	SPDF_NONE,	NO_CARY,	0, A_LAWFUL, PM_PRIEST, NON_PM, 2500L ),

A("Werebane",			SILVER_SABER,
	(SPFX_WARN|SPFX_RESTR|SPFX_DCLAS|SPFX_POLYC), 0, S_WERE,
	PHYS(5,4),	SPDF_WERE,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 1500L ),

A("Grayswandir",		SILVER_SABER,
	(SPFX_RESTR|SPFX_HALRES), 0, 0,
	PHYS(5,0),	SPDF_NONE,	NO_CARY,	0, A_LAWFUL, NON_PM, NON_PM, 8000L ),

A("Giantslayer",		SLING,
	(SPFX_WARN|SPFX_RESTR|SPFX_DCLAS|SPFX_STR), 0, S_GIANT,
	PHYS(5,8),	SPDF_NONE,	NO_CARY,	0, A_NEUTRAL, NON_PM, NON_PM, 1200L ),

A("Ogresmasher",		WAR_HAMMER,
	(SPFX_WARN|SPFX_RESTR|SPFX_DCLAS|SPFX_CON), 0, S_OGRE,
	PHYS(5,4),	SPDF_NONE,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 1200L ),

A("Trollsbane",			MORNING_STAR,
	(SPFX_WARN|SPFX_RESTR|SPFX_DCLAS|SPFX_REGEN), 0, S_TROLL,
	PHYS(5,4),	SPDF_NONE,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 1200L ),
/*
 *	Two problems:  1) doesn't let trolls regenerate heads,
 *	2) doesn't give unusual message for 2-headed monsters (but
 *	allowing those at all causes more problems than worth the effort).
 */
A("Vorpal Blade",		LONG_SWORD,
	(SPFX_RESTR|SPFX_BEHEAD), 0, 0,
	PHYS(5,1),	SPDF_NONE,	NO_CARY,	0, A_NEUTRAL, NON_PM, NON_PM, 4000L ),
/*
 *	(Snickersnee, now Kiku-Ichimonji, does 2d6; see spec_dbon().)
 *	Ah, never shall I forget the cry,
 *		or the shriek that shrieked he,
 *	As I gnashed my teeth, and from my sheath
 *		I drew my Snickersnee!
 *			--Koko, Lord high executioner of Titipu
 *			  (From Sir W.S. Gilbert's "The Mikado")
 */
A("Kiku-Ichimonji",		KATANA,
	SPFX_RESTR, 0, 0,
	PHYS(5,6),	SPDF_NONE,	NO_CARY,	0, A_LAWFUL, PM_SAMURAI, NON_PM, 1200L ),

A("Sunsword",			SILVER_LONG_SWORD,
	(SPFX_WARN|SPFX_RESTR|SPFX_DCLAS), 0, S_UNDEAD,
	PHYS(5,4),	SPDF_BLIND,	NO_CARY,	0, A_LAWFUL, NON_PM, NON_PM, 1500L ),

A("Keolewa",CLUB,
	SPFX_RESTR, 0, 0,
	PHYS(5,6), SPDF_NONE, NO_CARY, 0, A_NEUTRAL, PM_CAVEMAN, NON_PM, 1000L ),

/*
 *	JadeHack originals.  The flat damage bonuses (+4, +10) and Gungnir's
 *	flat +5 to-hit are applied in spec_dbon()/spec_abon(); the damd value
 *	here only has to be non-zero.  Luck Blade is a luckstone while carried
 *	(SPFX_LUCK), Quicksilver gives Super Fast (speed 24) when wielded (SPFX_FAST),
 *	Ravenbrand blocks blindness (SPDF_BLIND); the rest is in artifact_hit().
 */
A("Luck Blade",			SHORT_SWORD,
	(SPFX_RESTR|SPFX_LUCK), 0, 0,
	PHYS(5,4),	SPDF_NONE,	NO_CARY,	0, A_NEUTRAL, NON_PM, NON_PM, 3000L ),

A("Quicksilver",		SILVER_SHORT_SWORD,
	(SPFX_RESTR|SPFX_FAST), 0, 0,
	PHYS(5,4),	SPDF_NONE,	NO_CARY,	0, A_NEUTRAL, NON_PM, NON_PM, 3500L ),

A("Ravenbrand",			SCIMITAR,
	SPFX_RESTR, 0, 0,
	PHYS(5,10),	SPDF_BLIND,	NO_CARY,	0, A_NEUTRAL, NON_PM, NON_PM, 3500L ),

A("Pernicimordax",		CRYSTEEL_FALCHION,
	(SPFX_RESTR|SPFX_ATTK), 0, 0,
	COLD(5,6),	SPDF_NONE,	NO_CARY,	TIMED_COLD_RES, A_NEUTRAL, NON_PM, NON_PM, 3500L ),

/*
 *	Dramborleg (as in EvilHack): warns of demons, but its +d8 to hit and
 *	+d8 damage apply to everything, and it kills balrogs outright.
 */
A("Dramborleg",			DWARVISH_BEARDED_AXE,
	(SPFX_RESTR|SPFX_INTEL|SPFX_WARN|SPFX_DFLAG2), 0, M2_DEMON,
	PHYS(8,8),	SPDF_MAGIC,	NO_CARY,	0, A_LAWFUL, NON_PM, NON_PM, 9000L ),

/*
 *	Mouser's Scalpel (as in Hack'EM): +5 to hit, +1 damage, and each hit
 *	may strike again for +d8+1+enchantment (see artifact_hit()).
 */
A("Mouser's Scalpel",		RAPIER,
	SPFX_RESTR, 0, 0,
	PHYS(5,1),	SPDF_NONE,	NO_CARY,	0, A_NEUTRAL, NON_PM, NON_PM, 600L ),

A("Spineseeker",		STILETTO,
	SPFX_RESTR, 0, 0,
	PHYS(5,4),	SPDF_NONE,	NO_CARY,	0, A_CHAOTIC, PM_ROGUE, NON_PM, 2500L ),

A("Gungnir",			SPEAR,
	SPFX_RESTR, 0, 0,
	PHYS(5,10),	SPDF_NONE,	NO_CARY,	0, A_NEUTRAL, NON_PM, NON_PM, 5000L ),

/*
 *	Claiomh Solais: a lawful falchion that shines like Sunsword
 *	(artifact_light()), +d5 to hit and +d10 damage against everything,
 *	and polymorph control while wielded.  Not self-willed.
 */
A("Claiomh Solais",		FALCHION,
	(SPFX_RESTR|SPFX_POLYC), 0, 0,
	PHYS(5,10),	SPDF_NONE,	NO_CARY,	0, A_LAWFUL, NON_PM, NON_PM, 4500L ),

/*
 *	Ichigeki: +d3 to hit, +d6 damage against everything, and a 1 in 20
 *	chance per hit to strike a fatal weakness (see artifact_hit()).
 */
A("Ichigeki",			DAGGER,
	SPFX_RESTR, 0, 0,
	PHYS(3,6),	SPDF_NONE,	NO_CARY,	0, A_NEUTRAL, NON_PM, NON_PM, 3500L ),

/*
 *	Narya and Nenya: unaligned rings whose base ring is chosen after the
 *	ring appearances are shuffled (fix_lotr_rings() in artifact.c); the
 *	otyp given here is only the fallback.  Their spell, damage and
 *	Charisma effects are in spell.c and uhitm.c via ring_spell_free(),
 *	lotr_ring_damage() and lotr_cha_bonus(); the resistance comes from the
 *	defn field (skipped when the base ring already confers it).
 */
A("Narya",			RIN_FIRE_RESISTANCE,
	SPFX_RESTR, 0, 0,
	NO_ATTK,	SPDF_FIRE,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 5000L ),

A("Nenya",			RIN_COLD_RESISTANCE,
	SPFX_RESTR, 0, 0,
	NO_ATTK,	SPDF_COLD,	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 5000L ),

/*
 *	Vilya: shock and disintegration resistance, warning (spfx) and magical
 *	breathing (set_artifact_intrinsic()); an Intelligence-based AC bonus
 *	(find_ac()), +1 HP regeneration (moveloop) and failure-free healing
 *	spells (spell.c).  Base ring chosen in fix_lotr_rings().
 */
A("Vilya",			RIN_PROTECTION,
	(SPFX_RESTR|SPFX_WARN), 0, 0,
	NO_ATTK,	(SPDF_ELEC|SPDF_DISINT),	NO_CARY,	0, A_NONE, NON_PM, NON_PM, 5000L ),

/*
 *	The artifacts for the quest dungeon, all self-willed.
 */

A("The Orb of Detection",	CRYSTAL_BALL,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL), (SPFX_ESP|SPFX_HSPDAM), 0,
	NO_ATTK,	SPDF_NONE,	CARY(AD_MAGM),
	INVIS,		A_LAWFUL, PM_ARCHEOLOGIST, NON_PM, 2500L ),

A("The Heart of Ahriman",	LUCKSTONE,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL), SPFX_STLTH, 0,
	/* this stone does double damage if used as a projectile weapon */
	PHYS(5,0),	SPDF_NONE,	NO_CARY,
	LEVITATION,	A_NEUTRAL, PM_BARBARIAN, NON_PM, 2500L ),

A("The Sceptre of Might",	MACE,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL|SPFX_DALIGN), 0, 0,
	PHYS(5,0),	SPDF_NONE,	CARY(AD_MAGM),
	CONFLICT,	A_LAWFUL, PM_CAVEMAN, NON_PM, 2500L ),

#if 0	/* OBSOLETE */
A("The Palantir of Westernesse",	CRYSTAL_BALL,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL),
		(SPFX_ESP|SPFX_REGEN|SPFX_HSPDAM), 0,
	NO_ATTK,	NO_DFNS,	NO_CARY,
	TAMING,		A_CHAOTIC, NON_PM , PM_ELF, 8000L ),
#endif

A("The Staff of Aesculapius",	QUARTERSTAFF,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_ATTK|SPFX_INTEL|SPFX_DRLI|SPFX_REGEN), 0,0,
	DRLI(0,0),	SPDF_DRAIN,	NO_CARY,
	HEALING,	A_NEUTRAL, PM_HEALER, NON_PM, 5000L ),

A("The Magic Mirror of Merlin", MIRROR,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL|SPFX_SPEAK|SPFX_REFLECT), 
	(SPFX_REFLECT|SPFX_ESP|SPFX_HSPDAM), 0,
	NO_ATTK,	SPDF_NONE,	NO_CARY,
	0,		A_LAWFUL, PM_KNIGHT, NON_PM, 1500L ),

A("The Eyes of the Overworld",	LENSES,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL|SPFX_XRAY), 0, 0,
	NO_ATTK,	SPDF_NONE,	CARY(AD_MAGM),
	ENLIGHTENING,	A_NEUTRAL,	 PM_MONK, NON_PM, 2500L ),

A("The Mitre of Holiness",	HELM_OF_BRILLIANCE,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_DFLAG2|SPFX_INTEL), 0, M2_UNDEAD,
	NO_ATTK,	SPDF_NONE,	CARY(AD_FIRE),
	ENERGY_BOOST,	A_LAWFUL, PM_PRIEST, NON_PM, 2000L ),

A("The Longbow of Diana", BOW,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL|SPFX_REFLECT), SPFX_ESP, 0,
	PHYS(5,0),	SPDF_NONE,	NO_CARY,
	CREATE_AMMO, A_CHAOTIC, PM_RANGER, NON_PM, 4000L ),

A("The Master Key of Thievery", SKELETON_KEY,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL|SPFX_SPEAK),
		(SPFX_WARN|SPFX_TCTRL|SPFX_HPHDAM), 0,
	NO_ATTK,	SPDF_NONE,	NO_CARY,
	UNTRAP,		A_CHAOTIC, PM_ROGUE, NON_PM, 3500L ),

A("The Tsurugi of Muramasa",	TSURUGI,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL|SPFX_BEHEAD|SPFX_LUCK), 0, 0,
	PHYS(0,8),	SPDF_NONE,	NO_CARY,
	0,		A_LAWFUL, PM_SAMURAI, NON_PM, 4500L ),

#ifdef TOURIST
A("The Platinum Yendorian Express Card", CREDIT_CARD,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL|SPFX_DEFN),
		(SPFX_ESP|SPFX_HSPDAM), 0,
	NO_ATTK,	SPDF_NONE,	CARY(AD_MAGM),
	CHARGE_OBJ,	A_NEUTRAL, PM_TOURIST, NON_PM, 7000L ),
#endif

A("The Orb of Fate",		CRYSTAL_BALL,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL|SPFX_LUCK),
		(SPFX_WARN|SPFX_HPHDAM), 0,
	NO_ATTK,	SPDF_NONE,	NO_CARY,
	LEV_TELE,	A_NEUTRAL, PM_VALKYRIE, NON_PM, 3500L ),

A("The Eye of the Aethiopica",	AMULET_OF_POWER,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL), (SPFX_ESP|SPFX_HSPDAM), 0,
	NO_ATTK,	SPDF_NONE,	CARY(AD_MAGM),
	CREATE_PORTAL,	A_NEUTRAL, PM_WIZARD, NON_PM, 4000L ),

A("The Lyre of Orpheus",	WOODEN_HARP,
	(SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL), 0, 0,
	NO_ATTK,	SPDF_NONE,	CARY(AD_MAGM),
	TAMING,		A_NEUTRAL, PM_BARD, NON_PM, 5000L ),

/*
 *  terminator; otyp must be zero
 */
A(0, 0, 0, 0, 0, NO_ATTK, SPDF_NONE, NO_CARY, 0, A_NONE, NON_PM, NON_PM, 0L )

};	/* artilist[] (or artifact_names[]) */

#undef	A

#ifndef MAKEDEFS_C
#undef	NO_ATTK
#undef	DFNS
#undef	PHYS
#undef	DRLI
#undef	COLD
#undef	FIRE
#undef	ELEC
#undef	STUN
#undef	DRST
#endif

/*artilist.h*/
