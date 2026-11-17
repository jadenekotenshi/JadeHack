/*	SCCS Id: @(#)were.c	3.4	2002/11/07	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef OVL0

void
were_change(mon)
register struct monst *mon;
{
	if (!is_were(mon->data))
	    return;

	if (is_were_base(mon->data)) {
	    if (!Protection_from_shape_changers &&
		!rn2(night() ? (flags.moonphase == FULL_MOON ?  3 : 30)
			     : (flags.moonphase == FULL_MOON ? 10 : 50))) {
		new_were(mon);		/* change into animal form */
		if (flags.soundok && !canseemon(mon)) {
		    const char *howler;

		    switch (monsndx(mon->data)) {
		    case PM_WEREWOLF:	howler = "wolf";    break;
		    case PM_WEREJACKAL: howler = "jackal";  break;
		    case PM_WEREDEMON_HOUND: howler = "hound";  break;
		    default:		howler = (char *)0; break;
		    }
		    if (howler)
			You_hear("a %s howling at the moon.", howler);
		}
	    }
	} else if (!rn2(30) || Protection_from_shape_changers) {
	    new_were(mon);		/* change back into human form */
	}
}

#endif /* OVL0 */
#ifdef OVLB

STATIC_DCL int FDECL(counter_were,(int));

STATIC_OVL int
counter_were(pm)
int pm;
{
	switch(pm) {
	    case PM_WEREBEAR:	      return(PM_HUMAN_WEREBEAR);
	    case PM_HUMAN_WEREBEAR:   return(PM_WEREBEAR);
	    case PM_WERETIGER:	      return(PM_HUMAN_WERETIGER);
	    case PM_HUMAN_WERETIGER:  return(PM_WERETIGER);
	    case PM_WEREPANTHER:      return(PM_HUMAN_WEREPANTHER);
	    case PM_HUMAN_WEREPANTHER: return(PM_WEREPANTHER);
	    case PM_WERELYNX:	      return(PM_HUMAN_WERELYNX);
	    case PM_HUMAN_WERELYNX:   return(PM_WERELYNX);
	    case PM_WERESHARK:	      return(PM_HUMAN_WERESHARK);
	    case PM_HUMAN_WERESHARK:  return(PM_WERESHARK);
	    case PM_WERERAVEN:	      return(PM_HUMAN_WERERAVEN);
	    case PM_HUMAN_WERERAVEN:  return(PM_WERERAVEN);
	    case PM_WEREDEMON:	      return(PM_WEREDEMON_HOUND);
	    case PM_WEREDEMON_HOUND:  return(PM_WEREDEMON);
	    case PM_WEREWOLF:	      return(PM_HUMAN_WEREWOLF);
	    case PM_HUMAN_WEREWOLF:   return(PM_WEREWOLF);
	    case PM_WEREJACKAL:	      return(PM_HUMAN_WEREJACKAL);
	    case PM_HUMAN_WEREJACKAL: return(PM_WEREJACKAL);
	    case PM_WERERAT:	      return(PM_HUMAN_WERERAT);
	    case PM_HUMAN_WERERAT:    return(PM_WERERAT);
	    default:		      return(0);
	}
}

void
new_were(mon)
register struct monst *mon;
{
	register int pm;

	pm = counter_were(monsndx(mon->data));
	if(!pm) {
	    impossible("unknown lycanthrope %s.", mon->data->mname);
	    return;
	}

	/* a wereshark only takes its beast form in the water */
	if (pm == PM_WERESHARK && !is_pool(mon->mx, mon->my)) return;

	if(canseemon(mon) && !Hallucination)
	    pline("%s changes into a %s.", Monnam(mon),
			is_human(&mons[pm]) ? "human" :
			pm == PM_WEREDEMON ? "demon" :
			pm == PM_WEREDEMON_HOUND ? "hell hound" :
			mons[pm].mname+4);

	set_mon_data(mon, &mons[pm], 0);
	if (mon->msleeping || !mon->mcanmove) {
	    /* transformation wakens and/or revitalizes */
	    mon->msleeping = 0;
	    mon->mfrozen = 0;	/* not asleep or paralyzed */
	    mon->mcanmove = 1;
	}
	/* regenerate by 1/4 of the lost hit points */
	mon->mhp += (mon->mhpmax - mon->mhp) / 4;
	newsym(mon->mx,mon->my);
	mon_break_armor(mon, FALSE);
	possibly_unwield(mon, FALSE);
}

int
were_summon(ptr,yours,visible,genbuf)	/* were-creature (even you) summons a horde */
register struct permonst *ptr;
register boolean yours;
int *visible;			/* number of visible helpers created */
char *genbuf;
{
	register int i, typ, pm = monsndx(ptr);
	register struct monst *mtmp;
	int total = 0;

	*visible = 0;
	if(Protection_from_shape_changers && !yours)
		return 0;
	for(i = rnd(5); i > 0; i--) {
	   switch(pm) {

		case PM_WERERAT:
		case PM_HUMAN_WERERAT:
			typ = rn2(3) ? PM_SEWER_RAT : rn2(3) ? PM_GIANT_RAT : PM_RABID_RAT ;
			if (genbuf) Strcpy(genbuf, "rat");
			break;
		case PM_WEREJACKAL:
		case PM_HUMAN_WEREJACKAL:
			typ = PM_JACKAL;
			if (genbuf) Strcpy(genbuf, "jackal");
			break;
		case PM_WEREWOLF:
		case PM_HUMAN_WEREWOLF:
			typ = rn2(5) ? PM_WOLF : PM_WINTER_WOLF ;
			if (genbuf) Strcpy(genbuf, "wolf");
			break;
		case PM_WERELYNX:
		case PM_HUMAN_WERELYNX:
			typ = PM_LYNX;
			if (genbuf) Strcpy(genbuf, "lynx");
			break;
		case PM_WEREPANTHER:
		case PM_HUMAN_WEREPANTHER:
			typ = rn2(4) ? PM_PANTHER : PM_LYNX;
			if (genbuf) Strcpy(genbuf, "big cat");
			break;
		case PM_WERETIGER:
		case PM_HUMAN_WERETIGER:
			typ = rn2(3) ? PM_JAGUAR : PM_TIGER;
			if (genbuf) Strcpy(genbuf, "big cat");
			break;
		case PM_WEREBEAR:
		case PM_HUMAN_WEREBEAR:
			typ = rn2(4) ? PM_OWLBEAR : PM_YETI;
			if (genbuf) Strcpy(genbuf, "bear");
			break;
		case PM_WERERAVEN:
		case PM_HUMAN_WERERAVEN:
			typ = PM_RAVEN;
			if (genbuf) Strcpy(genbuf, "raven");
			break;
		case PM_WEREDEMON:
		case PM_WEREDEMON_HOUND:
			typ = rn2(3) ? PM_HELL_HOUND_PUP : PM_HELL_HOUND;
			if (genbuf) Strcpy(genbuf, "hell hound");
			break;
		case PM_WERESHARK:
		case PM_HUMAN_WERESHARK:
			/* sharks come only to the water */
			continue;
		default:
			continue;
	    }
	    mtmp = makemon(&mons[typ], u.ux, u.uy, NO_MM_FLAGS);
	    if (mtmp) {
		total++;
		if (canseemon(mtmp)) *visible += 1;
	    }
	    if (yours && mtmp)
		(void) tamedog(mtmp, (struct obj *) 0);
	}
	return total;
}

void
you_were()
{
	char qbuf[QBUFSZ];

	if (Unchanging || (u.umonnum == u.ulycn)) return;
	/* a wereshark can only change into its beast form in the water */
	if (u.ulycn == PM_WERESHARK && !is_pool(u.ux, u.uy)) return;
	if (Polymorph_control) {
	    /* `+4' => skip "were" prefix to get name of beast */
	    Sprintf(qbuf, "Do you want to change into %s? ",
		    an(mons[u.ulycn].mname+4));
	    if(yn(qbuf) == 'n') return;
	}
	(void) polymon(u.ulycn);
}

void
you_unwere(purify)
boolean purify;
{
	if (purify) {
	    You_feel("purified.");
	    u.ulycn = NON_PM;	/* cure lycanthropy */
	}
	if (!Unchanging && is_were(youmonst.data) &&
		(!Polymorph_control || yn("Remain in beast form?") == 'n'))
	    rehumanize();
}

#endif /* OVLB */

/*were.c*/
