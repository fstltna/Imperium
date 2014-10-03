/*
 * Imperium
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: update.c,v 1.2 2000/05/18 06:50:02 marisa Exp $
 *
 * $Log: update.c,v $
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:59  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:59:28  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:24:37  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:27  marisag
 * patch20: Fix empty revision.
 *
 */

#include "../config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <time.h>

#define UpdateC 1
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"

#ifdef DEBUG_LIB
#include <stdio.h>      /* For sprintf definition */
#endif

static const char rcsid[] = "$Id: update.c,v 1.2 2000/05/18 06:50:02 marisa Exp $";

/* the maximum amount of work that may be added at any one time */
#define MAX_ADD_WORK    255

/* maximum number of BTUs per planet */
#define MAX_PL_BTU      18

/* maximum amount of effic that may be added at one time */
#define MAX_EFFIC_INC   35

/* maximum planet mobility */
#define MAX_MOBIL       95

/*
 * maxPopulation - returns the maximum population of the given planet size
 *          It's done this way to support any sizes of the future in a
 *          downward way, and to eliminate any sanity checks in other parts
 *          of the library to ensure decent array elements
 */

ULONG maxPopulation(register Planet_t *pl)
{
    register ULONG pop;

    if (pl->pl_water == 100)
    {
        return 0;
    }
    switch(pl->pl_size)
    {
        case 9:
            pop = 1024000;
            break;
        case 8:
            pop = 512000;
            break;
        case 7:
            pop = 256000;
            break;
        case 6:
            pop = 128000;
            break;
        case 5:
            pop = 64000;
            break;
        case 4:
            pop = 32000;
            break;
        case 3:
            pop = 16000;
            break;
        case 2:
            pop = 8000;
            break;
        default:
            pop = 4000;
            break;
    }
    pop -= ((pop * pl->pl_water) / 100) / 2;
    return pop;
}

/*
 * calcPlagueFactor - calculate plague factor on a given planet.
 *          The factor will be scaled up by 100 for more precision
 */

USHORT calcPlagueFactor(IMP, register Planet_t *plan)
{
    register ULONG badLevel, goodLevel, population;

    badLevel = plan->pl_techLevel + (plan->pl_quantity[it_ore] / 50) +
        IS->is_world.w_plagueBooster;

    /* get the generic "good" level */
    goodLevel = plan->pl_resLevel + plan->pl_efficiency + plan->pl_mobility +
        100;

    /* get the total number of people on the planet, taking into account */
    /* their general grooming habits */
    population = plan->pl_quantity[it_civilians] +
        ((plan->pl_quantity[it_military] * 3) / 2) +
        ((plan->pl_quantity[it_officers] * 2) / 3) +
        (plan->pl_quantity[it_scientists] / 2);

    return (((population * 100) / maxPopulation(plan)) *
        (badLevel + ((badLevel * plan->pl_polution) / 100))) / goodLevel;
}

/*
 * getPct - returns the passed percent (0-100) of the given total, rounding
 *          down ANY fractions, so that repeated calls to the function
 *          will always be equal or less than the given totAmt
 */

ULONG getPct(register USHORT pct, register ULONG totAmt)
{
    /* sanity check */
    if (totAmt)
    {
        /* prevent any rounding up */
        totAmt--;
        /* do the percent calculation */
        return ((totAmt * pct) / 100);
    }
    return 0;
}

/*
 * The following routines are all part of 'updatePlanet', which is at the
 * heart of proper Imperium operation. Updating a planet needs access to the
 * player record of the owner of the planet. This is needed for a check of
 * money, plague factor calculation, etc. Thus, when
 * 'updatePlanet' is processing a planet, it reads and caches a copy of the
 * owning player (if it isn't the current player). A pointer to that copy
 * is passed around as needed.
 */

/*
 * makeProduction - produce production units in a planet.
 *      Convention: the planet is present and locked.
 *      Assumption: IS->is_noWrite is set.
 *      Returns: the money made on the work. The caller ('updatePlanet')
 *      must eventually add it to the owning player or race.
 */

long makeProduction(IMP, UBYTE prodClass, ULONG iwork, BOOL isMine,
    register Player_t *play, BOOL needsOre, USHORT orOre)
{
    register Planet_t *rp;
    register USHORT itemWork, curOre;
    long earnedMoney;

    /* Set the pointer to the planet in the buffer */
    rp = &IS->is_request.rq_u.ru_planet;
    earnedMoney = 0;
    /* verify that this player has enough money to update the planet */
    itemWork = 0;
    if (play->p_money > 0)
    {
        /* They do */
        /* The earned work should be the minimum of the passed work or */
        /* the amount of ore on the planet */
        itemWork = iwork * rp->pl_efficiency / 100;
        if (needsOre)
        {
            curOre = getPct(rp->pl_workPer[prodClass], orOre);
            itemWork = umin(itemWork, curOre);
        }
        /* If any special item handling is required, put it into the */
        /* switch statement */
        /* Set up itemWork as the amount of the itemWork allocated to */
        /* this particular type of items's production. */
        itemWork = (itemWork * rp->pl_workPer[prodClass]) / 100;
        /* Check if there is any amount allocated to this item type */
        if (itemWork > 0)
        {
            /* There is, so handle the resulting work */
            switch(prodClass)
            {
                case pp_cash:
                    /* sell the production units */
                    /* Note that pl_workPer[pp_cash] holds the tax rate */
                    earnedMoney = (itemWork * rp->pl_workPer[prodClass]) / 100;
                    if (isMine)
                    {
                        IS->is_contractEarnings += earnedMoney;
                    }
                    break;
                    /* If you have any special item handling, place it */
                    /* between here and the "default:" section */
                default:
                    /* Normally we want to just add the production */
                    /* into the planets production arrays, since some */
                    /* other function will actually create items from */
                    /* these arrays */
                    if ((rp->pl_prod[prodClass] + itemWork) > MAX_WORK)
                    {
                        /* If there is more work for this item than is */
                        /* allowed, then round it off */
                        if (isMine && !IS->is_quietUpdate)
                        {
                            /* And spew a message if this is the owner */
                            user2(IS, PPROD_NAME[prodClass],
                                "Production backlog ");
                            userP(IS, "on ", rp, "\n");
                        }
                        itemWork = MAX_WORK - rp->pl_prod[prodClass];
                    }
                    /* and now add the true itemWork into the planets */
                    /* work array */
                    rp->pl_prod[prodClass] += itemWork;
                    break;
            }
        }
    }
    /* reduce the amount of ore on the planet by the amount of work done */
    if (needsOre)
    {
        rp->pl_quantity[it_ore] -= itemWork;
    }
    /* return the amount of money earned, if any */
    return earnedMoney;
}

/*
 * updatePlanet - update the indicated planet.
 *      Convention: when we are called, the indicated planet is in the
 *      buffer, and is locked. When we return the planet will be still/back
 *      in the request and locked. We may do several partial writes during
 *      the update process.
 *      This function actually creates the items based on the planets current
 *      production points. New production points are added once items are
 *      built.
 */

void updatePlanet(IMP)
{
    register Planet_t *rp;
    register Player_t *play;
    register USHORT q;
    register long earnings;
    ULONG deltaTime, iwork;
    USHORT q1, q2, orOre;
    ULONG now, maxPeople, plNum, techLevel, resLevel;
    USHORT ownerNum, effic, civ, mil, offic, sci, workForce, plFact,
	revMil = 0, revOfc = 0;
    BOOL saveNoWrite, isMine = FALSE, verbose, plagued = FALSE, isHome,
	noAir = FALSE, hostile = FALSE, takeOver = FALSE;
    UBYTE prodType;
    Planet_t savePlanet;
    Player_t player;
    UBYTE race = NO_RACE, raceNum;
    World_t *rw;

    /* Set up the pointer to the buffer */
    rp = &IS->is_request.rq_u.ru_planet;
    rw = &IS->is_request.rq_u.ru_world;
    plNum = rp->pl_number;
    /* Get the amount of "normal" civilians */
    civ = readPlQuan(IS, rp, it_civilians);
    mil = readPlQuan(IS, rp, it_military);
    offic = readPlQuan(IS, rp, it_officers);
    sci = readPlQuan(IS, rp, it_scientists);
    /* now get the amount of ore currently on the planet */
    orOre = readPlQuan(IS, rp, it_ore);
    /* Get the planet owner number */
    ownerNum = rp->pl_owner;
    /* See if the planet has enough air to breathe... */
    if (rp->pl_gas < MIN_AIR)
    {
        noAir = TRUE;
    }
    /* get the current levels */
    techLevel = rp->pl_techLevel;
    resLevel = rp->pl_resLevel;
    if (rp->pl_transfer == pt_hostile)
    {
	hostile = TRUE;
    }

    /* set this up for later */
    raceNum = isHomePlanet(IS, plNum);
    isHome = ((raceNum == IS->is_player.p_race) && (ownerNum == NO_OWNER));
    /* make sure we aren't called for a bogus planet */
    if (((ownerNum == NO_OWNER) && !isHome) || (plNum == NO_ITEM))
    {
        return;
    }
    /* See if the current player is the owner of the planet */
    if (ownerNum == IS->is_player.p_number)
    {
        isMine = TRUE;
    }
    /* Note how we are building up workForce! */
    workForce = civ + (sci / 2) + (mil / 5) + (offic / 10);
    /* Set the start time for our work */
    now = IS->is_request.rq_time;
    /* If the planet has never been updated, or was updated in the future, */
    /* or was updated more than 2 weeks ago and the workforce is not zero, */
    /* then we throw away the changes */
    if ((rp->pl_lastUpdate == 0) || (rp->pl_lastUpdate > now) ||
        ((rp->pl_lastUpdate < (now - (14 * 24 * 60 * 60))) && (workForce != 0)))
    {
        rp->pl_lastUpdate = timeRound(IS, now);
    }
    deltaTime = (now - rp->pl_lastUpdate) / IS->is_world.w_secondsPerITU;
    /* Check for an overflow of work */
    deltaTime = umin((USHORT) deltaTime, (USHORT) MAX_ADD_WORK);

    /* iwork holds the amount of work produced by the given workforce in the */
    /* given amount of time */
    iwork = deltaTime * workForce;
    /* Verify that there is a minimum amount of work to do, and that either */
    /* the player owns the planet or the work is sufficient that it should be */
    /* updated anyway */
    if ((iwork >= 100) && (isMine || isHome || (iwork > 96 * 100)))
    {
        /* Set up the player pointer for later use */
        if (isMine || isHome)
        {
            /* If the current player is the one doing the update, just */
            /* set it to the player structure in the global ImpState_t */
            play = &IS->is_player;
        }
        else
        {
            /* We need to read in the player, so save planet & restore it */
            savePlanet = *rp;
            server(IS, rt_readPlayer, ownerNum);
            player = IS->is_request.rq_u.ru_player;
            play = &player;
            *rp = savePlanet;
        }

        /* set these up for later use */
        effic = rp->pl_efficiency;

        /* "verbose" will be true if the player owns the planet and wants to */
        /* see messages while updating it */
        verbose = ((isMine || isHome) && !IS->is_quietUpdate);

        /* clear out the earnings */
        earnings = 0;
        /* save the current state of is_noWrite, and then set it to true */
        /* se we can feel free to use the "user()" functions with items */
        /* safely locked */
        saveNoWrite = IS->is_noWrite;
        IS->is_noWrite = TRUE;

        /* try and increase the planets efficiency */
        q = 0;
        if ((play->p_money > 0) && !noAir)
        {
            q = umin(umin((USHORT) iwork / 100, (USHORT) MAX_EFFIC_INC),
		(USHORT) 100 - effic);
            rp->pl_mobility = umin((USHORT) MAX_MOBIL,
		(USHORT) rp->pl_mobility + ((deltaTime *
                IS->is_world.w_efficScale) / 100));
        }
        if (q > 0)
        {
            q = (q * IS->is_world.w_efficScale) / 100;
            effic += q;
            rp->pl_efficiency = effic;
            /* increase the planets polution by the */
            /* efficiency increase */
            rp->pl_polution += umin((USHORT) 100 - rp->pl_polution, (USHORT) q);
            q *= IS->is_world.w_efficCost;
            earnings -= q;
            if (isMine || isHome)
            {
                IS->is_improvementCost += q;
            }
        }

        /* Now we want to build up the number of NEW civilians on the planet */
        /* These do NOT add into the current work force */
        /* Set up the default amount for civs */
        q = deltaTime * umin((USHORT) civ, 127);

        /* Now we have to deal with the various sizes of planets determining */
        /* how many people a planet can hold */
        maxPeople = maxPopulation(rp);

        /* Now that we have the maximum that the planet will reproduce, */
        /* we need to store how many civs can be added */
        q1 = 0;
        if ((civ + mil + offic + sci) < ((maxPeople * 85) / 100))
        {
            q1 = ((maxPeople * 85) / 100) - (civ + mil + offic + sci);
        }
        q1 = ulmin(q1, q);

        /* If we can add any more people, do so */
        if ((q1 > 0) && !noAir)
        {
            /* If the number of civs is between 1/3rd and 2/3rds of the */
            /* maximum, we use the "high growth" rate */
            if (((civ + sci) > (maxPeople / 3)) && ((civ + sci) <
                ((maxPeople * 2) / 3)))
            {
               q2 = ulmin(q1, (civ + sci) + (q /
                   IS->is_world.w_highGrowthFactor));
            }
            else
            {
               q2 = ulmin(q1, (civ + sci) + (q /
                   IS->is_world.w_lowGrowthFactor));
            }
            /* Add these new people in */
            civ += q2;
            /* And remove them from the total left */
            q1 -= q2;
            /* If there is room left, add in contracted civs */
            if (q1 > 0)
            {
                if (rp->pl_workPer[pp_civ] > 0)
                {
                    q1 = ulmin(q1, ulmin((q * rp->pl_workPer[pp_civ]) /
                        100, getPct(rp->pl_workPer[pp_civ], orOre)));
                    civ += q1;
                    rp->pl_quantity[it_ore] -= q1;
                }
            }
        }

        /* build up the scientists */
        if ((rp->pl_workPer[pp_education] > 0) && !noAir)
        {
            /* Make sure there are 3% of the max # of civilians */
            /* and at least 1% of the population are scientists to train */
            /* them. Need to scale these down */
            if (civ > ((maxPeople * 3) / 100))
            {
 		if (((civ + sci) / 100) > 0)
		{
		    if ((sci / ((civ + sci) / 100)) > 0)
		    {
                	/* graduate a max of 100 students or civ / 2 */
                	q1 = ulmin(100, civ / 2);
                	q1 = ulmin(q1, ulmin((q * rp->pl_workPer[pp_education]) /
                	    100, getPct(rp->pl_workPer[pp_education], orOre) * 6));
                	sci += q1;
                	civ -= q1;
                	rp->pl_quantity[it_ore] -= (q1 / 6);
		    }
		}
            }
        }

        /* build up the military */
        if ((rp->pl_workPer[pp_mil] > 0) && !noAir)
        {
            /* Make sure there are 3% of the max # of civilians */
            /* and at least 1% of the population are officers to train them */
            /* Need to scale these down */
            if (civ > ((maxPeople * 3) / 100))
            {
 		if (((civ + offic) / 100) > 0)
		{
		    if ((offic / ((civ + offic) / 100)) > 0)
		    {
                	/* enlist a max of 100 cadets or civ / 2 */
                	q1 = ulmin(100, civ / 2);
                	q1 = ulmin(q1, ulmin((q * rp->pl_workPer[pp_mil]) /
                	    100, getPct(rp->pl_workPer[pp_mil], orOre) * 6));
			/* Take into account transfer status */
			if (hostile)
			{
			    mil += (q1 / 3);
			    civ -= (q1 / 3);
			}
			else
			{
			    mil += q1;
			    civ -= q1;
			}
                	rp->pl_quantity[it_ore] -= (q1 / 6);
		    }
		}
            }
        }

        /* build up the officers */
        if ((rp->pl_workPer[pp_ocs] > 0) && !noAir)
        {
            /* Make sure there are 3% of the max # of military */
            /* and at least 5% of the population are officers to train them */
            /* Need to scale these down */
            if (mil > ((maxPeople * 3) / 100))
            {
 		if (((mil + offic) / 100) > 0)
		{
		    if ((offic / ((mil + offic) / 100)) > 4)
		    {
	                /* enlist a max of 10 officers or mil / 2 */
                	q1 = ulmin(10, mil / 2);
                	q1 = ulmin(q1, ulmin((q * rp->pl_workPer[pp_ocs]) /
                	    100, getPct(rp->pl_workPer[pp_ocs], orOre) * 6));
                	offic += q1;
                	mil -= q1;
                	rp->pl_quantity[it_ore] -= (q1 / 6);
		    }
		}
            }
        }

        /* store the new number of people */
        rp->pl_quantity[it_civilians] = civ;
        rp->pl_quantity[it_military] = mil;
        rp->pl_quantity[it_officers] = offic;
        rp->pl_quantity[it_scientists] = sci;
        /* Set the current planets time */
        rp->pl_lastUpdate += deltaTime * IS->is_world.w_secondsPerITU;

        /* build up the amount of money needed for military payroll */
        /* note that officers cost twice as much as grunts */
	if (hostile)
	{
             q = (((((mil * 3) / 2) + (offic * 2)) / 32) * deltaTime) *
        	IS->is_world.w_milSuppliesCost;
	}
	else
	{
            q = (((mil + (offic * 2)) / 32) * deltaTime) *
 		IS->is_world.w_milSuppliesCost;
	}
        if (q != 0)
        {
            earnings -= q;
            if (isMine || isHome)
            {
                IS->is_militaryCost += q;
            }
        }

        /* decide if the planet should become infected by the plague */
        /* or kill people if no air on the planet */
        if (noAir)
        {
            if ((civ > 0) || (sci > 0) || (mil > 0) || (offic > 0))
            {
                q1 = civ;
                civ = 0;
                rp->pl_quantity[it_civilians] = civ;
                userN(IS, q1);
                q2 = sci;
                sci = 0;
                rp->pl_quantity[it_scientists] = sci;
                userN2(IS, " civilians,  ", q2);
                q1 = mil;
                mil = 0;
                rp->pl_quantity[it_military] = mil;
                userN2(IS, " scientists,  ", q1);
                q2 = offic;
                offic = 0;
                rp->pl_quantity[it_officers] = offic;
                userN2(IS, " military, and ", q2);
                userP(IS, " officers died of asphyxiation on ", rp,
                    ".\n");
                savePlanet = *rp;
		if (ownerNum != NO_OWNER);
		{
                    notify(IS, ownerNum);
		}
                plagued = TRUE;
                *rp = savePlanet;
            }
        }
        else
        {
            switch(rp->pl_plagueStage)
            {
                case 3:
                    /* terminal plague stage - kill people off */
                    q = ((100 - effic) + (MAX_MOBIL - rp->pl_mobility) +
                        113) * 100 / IS->is_world.w_plagueKiller;
                    q2 = umin((USHORT) q * (mil + offic) / 100, (USHORT) mil + offic);
                    q1 = umin((USHORT) q * (civ + (sci / 2)) / 100, (USHORT) civ + (sci / 2));
                    if ((q1 != 0) || (q2 != 0))
                    {
                        q1 = umin((USHORT) q * civ / 100, (USHORT) civ);
                        civ -= q1;
                        rp->pl_quantity[it_civilians] = civ;
                        userN(IS, q1);
                        q2 = umin((USHORT) q * (sci / 2) / 100, (USHORT) sci / 2);
                        sci -= q2;
                        rp->pl_quantity[it_scientists] = sci;
                        userN2(IS, " civilians,  ", q2);
                        q1 = umin((USHORT) q * mil / 100, mil);
                        mil -= q1;
                        rp->pl_quantity[it_military] = mil;
                        userN2(IS, " scientists,  ", q1);
                        q2 = umin((USHORT) q * offic / 100, offic);
                        offic -= q2;
                        rp->pl_quantity[it_officers] = offic;
                        userN2(IS, " military, and ", q2);
                        userP(IS, " officers died of the plague on ", rp,
                            ".\n");
                        savePlanet = *rp;
#ifdef DEBUG_LIB
			if (ownerNum != NO_OWNER)
			{
#endif
             		    notify(IS, ownerNum);
#ifdef DEBUG_LIB
			}
			else
			{
			    fprintf(stderr, "Was about to notify() NO_OWNER "
				"in updatePlanet()\n");
			}
#endif
                        news(IS, n_plague_die, (ownerNum != NO_OWNER) ?
			    ownerNum : 0, (USHORT) NOBODY);
			
                        if ((civ == 0) && (sci == 0) && (mil == 0) &&
                            (offic == 0))
                        {
                            plagued = TRUE;
                        }
                        *rp = savePlanet;
                    }
                    if (rp->pl_plagueTime <= deltaTime)
                    {
                        rp->pl_plagueStage = 0;
                    }
                    else
                    {
                        rp->pl_plagueTime -= deltaTime;
                    }
                    break;
                case 2:
                    /* infectious stage */
                    if (rp->pl_plagueTime <= deltaTime)
                    {
                        rp->pl_plagueStage = 3;
                        rp->pl_plagueTime = impRandom(IS,
                            IS->is_world.w_plagueThreeRand) +
                            IS->is_world.w_plagueThreeBase;
                    }
                    else
                    {
                        rp->pl_plagueTime -= deltaTime;
                    }
                    break;
                case 1:
                    /* gestatory stage */
                    if (rp->pl_plagueTime <= deltaTime)
                    {
                        rp->pl_plagueStage = 2;
                        rp->pl_plagueTime = impRandom(IS,
                        IS->is_world.w_plagueTwoRand) +
                        IS->is_world.w_plagueTwoBase;
                        savePlanet = *rp;
                        userP(IS, "Outbreak of plague reported on ",
                            &savePlanet, "!");
#ifdef DEBUG_LIB
			if (ownerNum != NO_OWNER)
			{
#endif
                            notify(IS, ownerNum);
#ifdef DEBUG_LIB
			}
			else
			{
			    fprintf(stderr, "Was about to notify() NO_OWNER "
				"in updatePlanet()\n");
			}
#endif
                        news(IS, n_plague_outbreak, (ownerNum != NO_OWNER) ?
			    ownerNum : 0, (USHORT) NOBODY);
                        *rp = savePlanet;
                    }
                    else
                    {
                        rp->pl_plagueTime -= deltaTime;
                    }
                    break;
                case 0:
                    /* not infected - see if planet should become infected. */
                    plFact = (calcPlagueFactor(IS, rp) / 100);
		    if (plFact > 15)
		    {
			if ((impRandom(IS, 1000) / 10) < plFact)
			{
			    rp->pl_plagueStage = 1;
			    rp->pl_plagueTime = impRandom(IS,
				IS->is_world.w_plagueOneRand) +
				IS->is_world.w_plagueOneBase;
			}
		    }
                    break;
            }
        }

        if (!noAir)
        {
            q = (deltaTime * IS->is_world.w_utilityRate) * rp->pl_size;
            earnings -= q;
            if (isMine || isHome)
            {
                IS->is_utilitiesCost += q;
            }
        }

        /* If the current player owns the planet, and is not a deity, */
        /* decrease the planets tech and res levels */
        if ((rp->pl_owner == play->p_number) && (play->p_status != ps_deity)
            && !noAir)
        {
            /* build up the planet's BTUs */
            rp->pl_btu = umin((USHORT) MAX_PL_BTU, (USHORT) rp->pl_btu +
                ((deltaTime * civ * effic) / IS->is_world.w_BTUDivisor));
            /* research and technology levels go down by 1% per day */
            rp->pl_resLevel -= rp->pl_resLevel * deltaTime *
                    IS->is_world.w_resDecreaser / 48000;
            rp->pl_techLevel -= rp->pl_techLevel * deltaTime *
                    IS->is_world.w_techDecreaser / 48000;
            techLevel = rp->pl_techLevel;
            resLevel = rp->pl_resLevel;
        }

        /* if the planet is over 60% efficient, begin producing items */
        if ((effic >= 60) && !noAir)
        {
            /* earn interest on gold bars on the planet */
            q = deltaTime * rp->pl_quantity[it_bars] * 4 *
                IS->is_world.w_interestRate / 100;
            earnings += q;
            if (isMine || isHome)
            {
                IS->is_interestEarnings += q;
            }

            /* Now loop for each of the item types that use production */
            for (prodType = 0; prodType <= PPROD_LAST; prodType++)
            {
                switch(prodType)
                {
                    case pp_technology:
                    case pp_research:
                        q = deltaTime * IS->is_world.w_utilityRate;
                        earnings -= q;
                        if (isMine || isHome)
                        {
                            IS->is_utilitiesCost += q;
                        }
                        if (play->p_money > 0)
                        {
                            if (prodType == pp_technology)
                            {
                                q2 = IS->is_world.w_techCost;
                            }
                            else
                            {
                                q2 = IS->is_world.w_resCost;
                            }
                            q = rp->pl_prod[prodType] / q2;
                            if (q != 0)
                            {
                                rp->pl_prod[prodType] -= q * q2;
                                if (prodType == pp_technology)
                                {
                                    rp->pl_techLevel += q;
                                    techLevel = rp->pl_techLevel;
                                    if (verbose)
                                    {
                                        userP(IS, "Technological advances "
                                            "on ", rp, "\n");
                                    }
                                }
                                else
                                {
                                    rp->pl_resLevel += q;
                                    resLevel = rp->pl_resLevel;
                                    /* reduce the planets polution */
                                    rp->pl_polution -= umin(rp->pl_polution,
                                        q);
                                    if (verbose)
                                    {
                                        userP(IS, "Medical breakthroughs on ",
                                            rp, "\n");
                                    }
                                }
                            }
                        }
                        if (prodType == pp_technology)
                        {
                            earnings += makeProduction(IS, prodType, (iwork *
                                IS->is_world.w_techScale) / 100, isMine, play,
                                TRUE, orOre);
                        }
                        else
                        {
                            earnings += makeProduction(IS, prodType, (iwork *
                                IS->is_world.w_resScale) / 100, isMine, play,
                                TRUE, orOre);
                        }
                        break;

                    case pp_planes:
                        if (play->p_money > 0)
                        {
                            q = umin((USHORT) rp->pl_prod[prodType] /
                                IS->is_world.w_planeCost, (USHORT) MAX_WORK -
                                rp->pl_quantity[it_planes]);
                            rp->pl_prod[prodType] -= q *
                                IS->is_world.w_planeCost;
                            rp->pl_quantity[it_planes] += q;
                            if (verbose && (q != 0))
                            {
                                userP(IS, "Fighters built on ", rp, "\n");
                            }
                        }
                        earnings += makeProduction(IS, prodType, (iwork *
                            IS->is_world.w_planeScale) / 100, isMine, play,
                            TRUE, orOre);
                        break;

                    case pp_weapon:
                        earnings += makeProduction(IS, prodType, (iwork *
                            IS->is_world.w_defenseScale) / 100, isMine, play,
                            TRUE, orOre);
                        break;

                    case pp_engine:
                        earnings += makeProduction(IS, prodType, (iwork *
                            IS->is_world.w_engineScale) / 100, isMine, play,
                            TRUE, orOre);
                        break;

                    case pp_hull:
                        earnings += makeProduction(IS, prodType, (iwork *
                            IS->is_world.w_hullScale) / 100, isMine, play,
                            TRUE, orOre);
                        break;

                    case pp_missile:
                        if (play->p_money > 0)
                        {
                            q = umin((USHORT) rp->pl_prod[prodType] /
                                IS->is_world.w_missCost, (USHORT) MAX_WORK -
                                rp->pl_quantity[it_missiles]);
                            rp->pl_prod[prodType] -= q *
                                IS->is_world.w_missCost;
                            rp->pl_quantity[it_missiles] += q;
                            if (verbose && (q != 0))
                            {
                                userP(IS, "Missiles built on ", rp, "\n");
                            }
                        }
                        earnings += makeProduction(IS, prodType, (iwork *
                            IS->is_world.w_missScale) / 100, isMine, play,
                            TRUE, orOre);
                        break;

                    case pp_airTank:
                        if (play->p_money > 0)
                        {
                            q = umin((USHORT) rp->pl_prod[prodType] /
                                IS->is_world.w_missCost, (USHORT) MAX_WORK -
                                rp->pl_quantity[it_airTanks]);
                            rp->pl_prod[prodType] -= q *
                                IS->is_world.w_missCost;
                            rp->pl_quantity[it_airTanks] += q;
                            if (verbose && (q != 0))
                            {
                                userP(IS, "Air tanks built on ", rp, "\n");
                            }
                        }
                        earnings += makeProduction(IS, prodType, (iwork *
                            IS->is_world.w_missScale) / 100, isMine, play,
                            TRUE, orOre);
                        break;

                    case pp_fuelTank:
                        if (play->p_money > 0)
                        {
                            q = umin((USHORT) rp->pl_prod[prodType] /
                                IS->is_world.w_missCost, (USHORT) MAX_WORK -
                                rp->pl_quantity[it_fuelTanks]);
                            rp->pl_prod[prodType] -= q *
                                IS->is_world.w_missCost;
                            rp->pl_quantity[it_fuelTanks] += q;
                            if (verbose && (q != 0))
                            {
                                userP(IS, "Fuel tanks built on ", rp, "\n");
                            }
                        }
                        earnings += makeProduction(IS, prodType, (iwork *
                            IS->is_world.w_missScale) / 100, isMine, play,
                            TRUE, orOre);
                        break;

                    case pp_electronics:
                        earnings += makeProduction(IS, prodType, (iwork *
                            IS->is_world.w_techScale) / 100, isMine, play,
                            TRUE, orOre);
                        break;

                    case pp_cash:
                        earnings += makeProduction(IS, prodType, iwork * 10
                            , isMine, play, FALSE, orOre);
                        break;

                    /* ore and gold mines last */
                    case pp_oMine:
                        if (play->p_money > 0)
                        {
                            q = umin(rp->pl_prod[prodType], (USHORT) MAX_WORK -
                                rp->pl_quantity[it_ore]);
                            rp->pl_prod[prodType] -= q;
                            rp->pl_quantity[it_ore] += q;
                            if (verbose && (q != 0))
                            {
                                userP(IS, "Ore ready for use on ", rp, "\n");
                            }
                        }
                        earnings += makeProduction(IS, prodType, (iwork *
                            rp->pl_minerals * IS->is_world.w_ironScale) /
                            10000, isMine, play, FALSE, orOre);
                        break;

                    case pp_gMine:
                        if (play->p_money > 0)
                        {
                            q = umin(rp->pl_prod[prodType] /
                                IS->is_world.w_barCost, (USHORT) MAX_WORK -
                                rp->pl_quantity[it_bars]);
                            rp->pl_prod[prodType] -= q * IS->is_world.w_barCost;
                            rp->pl_quantity[it_bars] += q;
                            if (verbose && (q != 0))
                            {
                                userP(IS, "Gold bars minted on ", rp, "\n");
                            }
			    if (q > rp->pl_gold)
			    {
			        q = rp->pl_gold;
			    }
			    if (q > 0)
			    {
                                rp->pl_gold -= q;
			    }
                        }
                        earnings += makeProduction(IS, prodType, (iwork *
                            rp->pl_gold * IS->is_world.w_barScale) /
                            10000, isMine, play, FALSE, orOre);
                        break;
                }
            }
        }

	/* Do the checks for civilian revolt & snipers */
	if (hostile && !plagued)
	{
	    ULONG civPct, combAmt;

	    /* Store figures */
	    civPct = ((civ + sci) * 2) / 10;
	    combAmt = mil + offic;
	    /* See if they are below the threshold */
	    if (combAmt < civPct)
	    {
		/* If there are any military, see if any get killed */
		if (combAmt > 0)
		{
		    /* There is a 25% chance of revolt */
		    if (impRandom(IS, 1000) > 750)
		    {
			/* Get the number of mil killed */
			revMil = mil / 5;
			/* If the number is 0, use the full amt of mil */
			if (revMil == 0)
			{
			    revMil = mil;
			}
			mil -= revMil;
			if (revMil > 0)
			{
			    userN(IS, revMil);
			    userP(IS, " military killed on ", rp, " due"
				" to revolt\n");
			}
			/* Get the number of offic killed */
			revOfc = offic / 5;
			/* If the number is 0, use the full amt of offic */
			if (revOfc == 0)
			{
			    revOfc = offic;
			}
			offic -= revOfc;
			if (revOfc > 0)
			{
			    userN(IS, revOfc);
			    userP(IS, " officers killed on ", rp, " due"
				" to revolt\n");
			}
			/* Rebuild the combined military force */
		    }
		    if (mil > 0)
		    {
			revMil = umin(mil, impRandom(IS, 5) + 1);
			if (revMil > 0)
			{
			    userN(IS, revMil);
			    userP(IS, " military killed by snipers on ",
				rp, "\n");
			    mil -= revMil;
			}
		    }
		    combAmt = mil + offic;
		}
		/* See if all the military have been wiped out */
		if (combAmt == 0)
		{
		    plagued = TRUE;
		    takeOver = TRUE;
		}
	    }
	}

        /* this is a rare case we have saved up so that we can do it when
           it is safe to unlock the planet. */

        if (plagued)
        {
	    UBYTE origOwn = NO_OWNER, origRace = NO_RACE;

	    if (takeOver)
	    {
		if (isHome)
		{
        	    rp->pl_owner = NO_OWNER;
        	    rp->pl_ownRace = NO_RACE;
		}
		else
		{
        	    rp->pl_owner = rp->pl_lastOwner;
		}
        	origOwn = rp->pl_owner;
		rp->pl_lastOwner = NO_OWNER;
		rp->pl_transfer = pt_peacefull;
        	race = rp->pl_ownRace;
		plagued = FALSE;
	    }
	    else
	    {
        	rp->pl_lastOwner = rp->pl_owner;
        	rp->pl_transfer = pt_trade;
        	rp->pl_owner = NO_OWNER;
        	race = rp->pl_ownRace;
        	rp->pl_ownRace = NO_RACE;
	    }
            memset(&rp->pl_checkpoint[0], '\0', PLAN_PSWD_LEN * sizeof(char));
            server(IS, rt_unlockPlanet, plNum);
            play = &IS->is_request.rq_u.ru_player;
	    if (takeOver)
	    {
		if (!isHome)
		{
		    server(IS, rt_lockPlayer, origOwn);
		    origRace = play->p_race;
                    play->p_planetCount++;
		    server(IS, rt_unlockPlayer, origOwn);
                    server(IS, rt_lockPlanet, plNum);
        	    rp->pl_ownRace = origRace;
                    server(IS, rt_unlockPlanet, plNum);
		}
	    }
	    if (isHome)
	    {
		server(IS, rt_lockPlayer, IS->is_player.p_number);
	    }
	    else
	    {
                server(IS, rt_lockPlayer, ownerNum);
                play->p_planetCount--;
	    }
            /* since we have the player locked anyway, put in any */
            /* money that may have been earned */
            play->p_money += earnings;
            if (isMine || isHome)
            {
                IS->is_player.p_money = play->p_money;
                IS->is_player.p_planetCount = play->p_planetCount;
            }
            if (!isHome && (play->p_planetCount == 0))
            {
                if (isMine)
                {
                    if (noAir)
                    {
                        user(IS, "\nYour last planet has just expired from "
                             "lack of air!\n\n");
                    }
		    else if (takeOver)
		    {
			user(IS, "\nYour last planet was lost due to "
			    "civilian revolt!\n\n");
		    }
                    else
                    {
                        user(IS, "\nYour last planet has just expired due "
                             "to the plague!\n\n");
                    }
                }
                server(IS, rt_unlockPlayer, (isHome) ? IS->is_player.p_number :
		    ownerNum);
                if (!noAir && !takeOver)
		{
              	    news(IS, n_plague_dest, (isHome) ? IS->is_player.p_number :
			ownerNum, (USHORT) NOBODY);
		}
            }
            else
            {
                server(IS, rt_unlockPlayer, (isHome) ? IS->is_player.p_number :
		    ownerNum);
            }
            if (race != NO_RACE)
            {
                server(IS, rt_lockWorld, 0);
                IS->is_request.rq_u.ru_world.w_race[race].r_planetCount--;
                /* see if this was the last planet (the home planet) */
                if (IS->is_request.rq_u.ru_world.w_race[race].r_homePlanet ==
                    plNum)
                {
                    /* set race status to dead */
                    IS->is_request.rq_u.ru_world.w_race[race].r_status =
                        rs_dead;
                }
                server(IS, rt_unlockWorld, 0);
                IS->is_world.w_race[race] =
                    IS->is_request.rq_u.ru_world.w_race[race];
                /* see if this killed the race */
                if (IS->is_world.w_race[race].r_status == rs_dead)
                {
                    if (race == IS->is_player.p_race)
                    {
                        user(IS, "Which destroyed your own race!\n");
                    }
                    else
                    {
                        user3(IS, "Which destroyed race ",
                            &IS->is_world.w_race[race].r_name[0], "!\n");
                    }
                    if (!noAir && !takeOver)
                    {
                        news(IS, n_plague_dest, IS->is_player.p_number, race);
                    }
                }
            }
            if (takeOver && (origRace != NO_RACE))
            {
                server(IS, rt_lockWorld, 0);
                IS->is_request.rq_u.ru_world.w_race[origRace].r_planetCount++;
                /* see if this was the last planet (the home planet) */
                if (IS->is_request.rq_u.ru_world.w_race[origRace].r_homePlanet ==
                    plNum)
                {
                    /* set race status to dead */
                    IS->is_request.rq_u.ru_world.w_race[race].r_status =
                        rs_active;
		}
                server(IS, rt_unlockWorld, 0);
                IS->is_world.w_race[origRace] =
                    IS->is_request.rq_u.ru_world.w_race[origRace];
	    }
            server(IS, rt_lockPlanet, plNum);
        }
	if (!plagued)
        {
        /* if the current player owns the planet, then the update of his
           "player structure" will be done in 'commands' in 'commands.c',
            with the goal to reduce locking/unlocking. */

        /* check if the owner accumulated any money from the planet */
            server(IS, rt_unlockPlanet, plNum);
            /* if this is a home planet, copy the tech/research factors */
            if (isHome)
            {
                server(IS, rt_lockWorld, 0);
                rw->w_race[raceNum].r_techLevel = techLevel;
                rw->w_race[raceNum].r_resLevel = resLevel;
                server(IS, rt_unlockWorld, 0);
                IS->is_world.w_race[raceNum].r_techLevel = techLevel;
                IS->is_world.w_race[raceNum].r_resLevel = resLevel;
            }
            server(IS, rt_lockPlayer, (isHome) ? IS->is_player.p_number :
		ownerNum);
            play = &IS->is_request.rq_u.ru_player;
            play->p_money += earnings;
            if (isMine || isHome)
            {
                IS->is_player.p_money = play->p_money;
                IS->is_player.p_planetCount = play->p_planetCount;
            }
            server(IS, rt_unlockPlayer, (isHome) ? IS->is_player.p_number :
		ownerNum);
            server(IS, rt_lockPlanet, plNum);
        }

        IS->is_noWrite = saveNoWrite;
    }
}

/*
 * updateBigItem - updates a big item
 *      Convention: similar to 'updatePlanet' - the item is locked.
 *      Note: items are never destroyed while being updated
 */

void updateBigItem(IMP)
{
    register BigItem_t *rbi;
    register ULONG deltaTime, now;

    /* set up the pointer into the buffer */
    rbi = &IS->is_request.rq_u.ru_bigItem;
    /* get the current time */
    now = IS->is_request.rq_time;
    /* see if we need to round off the update time */
    if ((rbi->bi_lastUpdate == 0) || (rbi->bi_lastUpdate > now) ||
        (rbi->bi_lastUpdate < (now - (14 * 24 * 60 * 60))))
    {
        rbi->bi_lastUpdate = timeRound(IS, now);
    }
    deltaTime = (now - rbi->bi_lastUpdate) / IS->is_world.w_secondsPerITU;
    if (deltaTime >= 3)
    {
        rbi->bi_effic = umin((USHORT) 100, rbi->bi_effic + deltaTime);
        rbi->bi_techLevel -= rbi->bi_techLevel * deltaTime *
            IS->is_world.w_shipTechDecreaser / 480000;
    }
}

/*
 * updateShip - update the passed ship
 *      Convention: similar to 'updatePlanet' - the ship is locked.
 *      Note: Since we may have to lock and unlock several other items,
 *      we will keep the outside world current on the state of the ship,
 *      and make no assumptions about any items being there while we are
 *      working on it, even though it will be slower.
 */

void updateShip(IMP)
{
    register Ship_t *rsh;
    register ULONG now, deltaTime;
    BOOL saveNoWrite;
    ULONG itemNum, shipNum, saveDelta;
    USHORT item, subItem, maxItems, civ, mil, offic, sci, q, q1, q2;
    ULONG *itArray;
    Ship_t saveSh;

    /* set up the pointer to the ship area of the buffer */
    rsh = &IS->is_request.rq_u.ru_ship;
    if (rsh->sh_type == st_m)
    {
        /* big trouble if this happens */
        return;
    }
    /* check if ship is destroyed */
    if (rsh->sh_hull == NO_ITEM)
    {
        /* it was, so don't bother trying to update the other items */
        return;
    }
    shipNum = rsh->sh_number;
    /* get the current time */
    now = IS->is_request.rq_time;
    /* see if we need to round off the update time */
    if ((rsh->sh_lastUpdate == 0) || (rsh->sh_lastUpdate > now) ||
        (rsh->sh_lastUpdate < (now - (14 * 24 * 60 * 60))))
    {
        rsh->sh_lastUpdate = timeRound(IS, now);
    }
    deltaTime = (now - rsh->sh_lastUpdate) / IS->is_world.w_secondsPerITU;
    if (deltaTime >= 3)
    {
        saveDelta = deltaTime;
        saveNoWrite = IS->is_noWrite;
        IS->is_noWrite = TRUE;
        rsh->sh_lastUpdate += deltaTime * IS->is_world.w_secondsPerITU;
        deltaTime = deltaTime * IS->is_world.w_shipWorkScale / 100;
        /* update the ship's hull */
        itemNum = rsh->sh_hull;
        server(IS, rt_unlockShip, shipNum);
        server(IS, rt_lockBigItem, itemNum);
        updateBigItem(IS);
        /* cheat for a sec here */
        subItem = IS->is_request.rq_u.ru_bigItem.bi_effic;
        item = getTechFactor(IS->is_request.rq_u.ru_bigItem.bi_techLevel);
        server(IS, rt_unlockBigItem, itemNum);
        server(IS, rt_lockShip, shipNum);
        /* set the ships effic from the amount in the hull */
        rsh->sh_efficiency = subItem;
        rsh->sh_hullTF = item;
        /* Loop through the items on the ship, updating them if they exist */
        for (item = IT_LAST_SMALL + 1; item <= IT_LAST; item++)
        {
            /* make sure there is at least one of the items */
            if (rsh->sh_items[item] > 0)
            {
                /* set up the pointer to the start of the array and the max */
                /* number of these items that may exist */
                switch(item)
                {
                    case it_computers:
                        itArray = rsh->sh_computer;
                        maxItems = MAX_COMP;
                        break;
                    case it_engines:
                        itArray = rsh->sh_engine;
                        maxItems = MAX_ENG;
                        break;
                    case it_lifeSupp:
                        itArray = rsh->sh_lifeSupp;
                        maxItems = MAX_LIFESUP;
                        break;
                    case it_elect:
                        itArray = rsh->sh_elect;
                        maxItems = MAX_ELECT;
                        break;
                    case it_weapons:
                        itArray = rsh->sh_weapon;
                        maxItems = MAX_WEAP;
                        break;
                    default:
                        itArray = rsh->sh_weapon;
                        maxItems = MAX_WEAP;
                        break;
                }
                /* loop through the items */
                /* note that this assumes that the end of list (when there */
                /* are less than the max of the item) is indicated by */
                /* NO_ITEM as well as the count */
                subItem = 0;
                while ((subItem < maxItems) && (itArray[subItem] !=
                    NO_ITEM))
                {
                    itemNum = itArray[subItem];
                    server(IS, rt_unlockShip, shipNum);
                    server(IS, rt_lockBigItem, itemNum);
                    updateBigItem(IS);
                    /* note that it is impossible for updating to destroy */
                    /* an item, so we don't need to check for it */
                    /* Set the ships update time to the most recent one */
                    deltaTime = IS->is_request.rq_u.ru_bigItem.bi_lastUpdate;
                    server(IS, rt_unlockBigItem, itemNum);
                    server(IS, rt_lockShip, shipNum);
                    if (deltaTime > rsh->sh_lastUpdate)
                    {
                        rsh->sh_lastUpdate = deltaTime;
                    }
                    subItem++;
                }
            }
        }
        civ = rsh->sh_items[it_civilians];
        mil = rsh->sh_items[it_military];
        offic = rsh->sh_items[it_officers];
        sci = rsh->sh_items[it_scientists];
        switch(rsh->sh_plagueStage)
        {
            case 3:
                /* terminal plague stage - kill people off */
                q = ((100 - rsh->sh_efficiency) + 113) * 100 /
                    IS->is_world.w_plagueKiller;
                q2 = umin((USHORT) q * (mil + offic) / 100, (USHORT) mil + offic);
                q1 = umin((USHORT) q * (civ + (sci / 2)) / 100, (USHORT) civ + (sci / 2));
                if ((q1 != 0) || (q2 != 0))
                {
                    q1 = umin((USHORT) q * civ / 100, civ);
                    civ -= q1;
                    rsh->sh_items[it_civilians] = civ;
                    userN(IS, q1);
                    q2 = umin((USHORT) q * (sci / 2) / 100, (USHORT) sci / 2);
                    sci -= q2;
                    rsh->sh_items[it_scientists] = sci;
                    userN2(IS, " civilians,  ", q2);
                    q1 = umin((USHORT) q * mil / 100, mil);
                    mil -= q1;
                    rsh->sh_items[it_military] = mil;
                    userN2(IS, " scientists,  ", q1);
                    q2 = umin((USHORT) q * offic / 100, offic);
                    offic -= q2;
                    rsh->sh_items[it_officers] = offic;
                    userN2(IS, " military, and ", q2);
                    userSh(IS, " officers died of the plague on ship #", rsh,
                        ".\n");
                    saveSh = *rsh;
                    notify(IS, saveSh.sh_owner);
                    *rsh = saveSh;
                }
                if (rsh->sh_plagueTime <= deltaTime)
                {
                    rsh->sh_plagueStage = 0;
                }
                else
                {
                    rsh->sh_plagueTime -= deltaTime;
                }
                break;
            case 2:
                /* infectious stage */
                if (rsh->sh_plagueTime <= deltaTime)
                {
                    rsh->sh_plagueStage = 3;
                    rsh->sh_plagueTime = impRandom(IS,
                        IS->is_world.w_plagueThreeRand) +
                        IS->is_world.w_plagueThreeBase;
                }
                else
                {
                    rsh->sh_plagueTime -= deltaTime;
                }
                break;
            case 1:
                /* gestatory stage */
                if (rsh->sh_plagueTime <= deltaTime)
                {
                    rsh->sh_plagueStage = 2;
                    rsh->sh_plagueTime = impRandom(IS,
                        IS->is_world.w_plagueTwoRand) +
                        IS->is_world.w_plagueTwoBase;
                    saveSh = *rsh;
                    userSh(IS, "Outbreak of plague reported on ship #",
                        &saveSh, "!");
                    notify(IS, saveSh.sh_owner);
                    *rsh = saveSh;
                }
                else
                {
                    rsh->sh_plagueTime -= deltaTime;
                }
                break;
            default:
                /* ships do not spontaneously catch the plague */
                break;
        }

        saveSh = *rsh;
        item = numInst(IS, &saveSh, bp_engines);
        subItem = umin((USHORT) ((item * 25) * saveDelta) / 10, (USHORT) MAX_ADD_WORK);
        subItem = umin(subItem, saveSh.sh_fuelLeft);
        subItem = umin(subItem, (USHORT) (item * 127) - saveSh.sh_energy);
        saveSh.sh_engTF = getShipTechFactor(IS, &saveSh, bp_engines);
        saveSh.sh_engEff = getShipEff(IS, &saveSh, bp_engines);
        *rsh = saveSh;
        rsh->sh_energy += subItem;
        rsh->sh_cargo -= subItem;
        rsh->sh_fuelLeft -= subItem;
        IS->is_noWrite = saveNoWrite;
    }
}

/*
 * divLev - divides up a given total of units based on two "level"
 *          oriented parameters. It is expected that the two levels
 *          will contain the split up amount on return from this
 *          function
 */

void divLev(register USHORT totAmt, register USHORT *lev1,
    register USHORT *lev2)
{
    register USHORT tempLev;

    /* check if either is zero */
    if ((*lev1 == 0) || (*lev2 == 0))
    {
        /* was it level 1? */
        if (*lev1 == 0)
        {
            /* is level 2 also 0 ? */
            if (*lev2 == 0)
            {
                /* nobody gets anything */
                return;
            }
            /* no, so return whatever was requested in level 2 */
            *lev2 = (totAmt * (*lev2 * 33)) / 100;
            return;
        }
        /* no, so return whatever was requested in level 1 */
        *lev1 = (totAmt * (*lev1 * 33)) / 100;
        return;
    }
    tempLev = totAmt / 2;
    *lev1 = (tempLev * (*lev1 * 33)) / 100;
    *lev2 = (tempLev * (*lev2 * 33)) / 100;
    return;
}

/*
 * updateMiner - updates the miner in the buffer (assumed to be locked)
 *          and will send out notifications based on the activity and
 *          the flags set by the miner's owner.
 */

void updateMiner(IMP)
{
    register Ship_t *rsh;
    register ULONG now, deltaTime;
    BOOL saveNoWrite, needTell;
    ULONG plNum, totWork;
    USHORT oOre, oGold, oEnergy, levOre, levBar, keepGold, itWork,
        usedEnergy, capacity;
    Planet_t savePlanet;
    Ship_t saveShip;
#ifdef DEBUG_LIB
    char numBuff[80];
#endif

    /* set up the pointer to the ship area of the buffer */
    rsh = &IS->is_request.rq_u.ru_ship;

    keepGold = 0;
    needTell = FALSE;

    /* do some sanity checking */
    if (rsh->sh_type != st_m)
    {
        /* big trouble if this happens */
#ifdef DEBUG_LIB
	if (IS->is_DEBUG)
	{
	    saveShip = *rsh;
	    log3(IS, "updateMiner called for non-miner ship", "", "");
	    *rsh = saveShip;
	}
#endif
	return;
    }
    /* get the current time */
    now = IS->is_request.rq_time;
    /* see if we need to round off the update time */
    if ((rsh->sh_lastUpdate == 0) || (rsh->sh_lastUpdate > now) ||
        (rsh->sh_lastUpdate < (now - (14 * 24 * 60 * 60))))
    {
        rsh->sh_lastUpdate = timeRound(IS, now);
    }

    deltaTime = (now - rsh->sh_lastUpdate) / IS->is_world.w_secondsPerITU;

    if (rsh->sh_owner == NO_OWNER)
    {
        /* this will keep miners that are destroyed from */
        /* continuing to improve afterwards */
        rsh->sh_lastUpdate += deltaTime * IS->is_world.w_secondsPerITU;
        return;
    }

    if (deltaTime >= 3)
    {
        saveNoWrite = IS->is_noWrite;
        IS->is_noWrite = TRUE;

        rsh->sh_lastUpdate += deltaTime * IS->is_world.w_secondsPerITU;
        deltaTime = deltaTime * IS->is_world.w_shipWorkScale / 100;
	/* save the current miner stats */
	saveShip = *rsh;

        /* note the use of weapon[1] for the verbosity level in miners */

        /* increase the miners effic if not on a planet */
        if (saveShip.sh_dragger != NO_ITEM)
        {
            saveShip.sh_efficiency = umin((USHORT) 100, rsh->sh_efficiency + deltaTime);
        }
        /* check it's efficiency */
        else if (saveShip.sh_efficiency < EFFIC_WARN)
	{
            if (saveShip.sh_efficiency < EFFIC_CRIT)
	    {
                if (saveShip.sh_weapon[1] > 0)
		{
                    userN3(IS, "Your miner #", saveShip.sh_number,
			   " is below ");
                    userN3(IS, "the critical efficiency level on planet #",
			   saveShip.sh_planet, "\n");
                    needTell = TRUE;
		}
	    }
            else
	    {
                if (saveShip.sh_weapon[1] > 1)
		{
                    userN3(IS, "Your miner #", saveShip.sh_number,
			   " is below ");
                    userN3(IS, "operating efficiency on planet #",
			   saveShip.sh_planet, "\n");
                    needTell = TRUE;
		}
	    }
	}
        else
	{
            /* if we get here we know that the miner isn't on a ship */
            /* and has at least EFFIC_WARN efficiency level */

            /* decrease the miners tech level */
            saveShip.sh_weapon[3] -= saveShip.sh_weapon[3] * deltaTime *
                IS->is_world.w_shipTechDecreaser / 480000;

            /* now set the miners tech factor */
            saveShip.sh_hullTF = getTechFactor(saveShip.sh_weapon[3]);

            /* get the current number of goods */
            oOre = saveShip.sh_items[it_ore];
            oGold = saveShip.sh_items[it_bars];
            oEnergy = saveShip.sh_energy;

            /* check the miners energy level */
            if (oEnergy == 0)
	    {
                /* see if they should be told */
                if ((saveShip.sh_weapon[1] > 0) &&
		    (saveShip.sh_dragger == NO_ITEM))
		{
                    /* tell them the miner is out of energy */
                    userN3(IS, "Your miner #", saveShip.sh_number,
			   " is out of ");
                    userN3(IS, "energy on planet #", saveShip.sh_planet, "\n");
                    needTell = TRUE;
		}
	    }
            else
	    {
                /* store the planet number */
                plNum = saveShip.sh_planet;
                /* read in the planet */
                server(IS, rt_readPlanet, plNum);
                /* store the planet */
                savePlanet = IS->is_request.rq_u.ru_planet;
                /* restore the miner */
                *rsh = saveShip;
                /* check the planets air level */
                if (savePlanet.pl_gas >= MIN_AIR)
		{
                    /* see if they should be told */
                    if (saveShip.sh_weapon[1] > 1)
		    {
                        /* tell them too much atmosphere */
                        userN3(IS, "Your miner #", saveShip.sh_number, " is on ");
                        userN3(IS, "a planet (#", saveShip.sh_planet,
			       ") which has too much air for it to operate.\n");
			needTell = TRUE;
		    }
		}
		else
		{
                    usedEnergy = 0;
		    /* Get the current levels desired for Ore */
                    levOre = saveShip.sh_weapon[0];
                    levBar = saveShip.sh_weapon[2];
                    /* get how much work we want to try and do */
                    divLev(umin(oEnergy, 65), &levOre, &levBar);
                    levOre = umin(levOre, 33);
                    levBar = umin(levBar, 33);

                    /* build up work for gold */
                    capacity = getShipCapacity(IS, it_bars, &saveShip);
                    totWork = umin(capacity - saveShip.sh_items[it_bars],
			(USHORT) saveShip.sh_elect[1] / (IS->is_world.w_barCost * 5));
                    saveShip.sh_items[it_bars] += totWork;
                    saveShip.sh_elect[1] -= totWork *
                        (IS->is_world.w_barCost * 5);
                    saveShip.sh_cargo += (totWork *
					  IS->is_world.w_weight[it_bars]);
                    keepGold = totWork;

                    /* now build up how much work we can do for gold */
                    totWork = deltaTime * levBar;
                    totWork = totWork * saveShip.sh_efficiency / 100;
                    itWork = (totWork * savePlanet.pl_gold *
			      IS->is_world.w_barScale) / 10000;
                    saveShip.sh_elect[1] += itWork;
                    usedEnergy += levBar;

	            /* now build up work for ore */
                    capacity = getShipCapacity(IS, it_ore, &saveShip);
                    totWork = umin(capacity - saveShip.sh_items[it_ore],
			(USHORT) saveShip.sh_elect[0] / 3);
                    saveShip.sh_items[it_ore] += totWork;
                    saveShip.sh_elect[0] -= totWork * 3;
                    saveShip.sh_cargo += totWork;

                    /* now build up how much work we can do for ore */
                    totWork = deltaTime * levOre;
                    totWork = totWork * saveShip.sh_efficiency / 100;
                    itWork = (totWork * savePlanet.pl_minerals *
			      IS->is_world.w_ironScale) / 10000;
                    saveShip.sh_elect[0] += itWork;
                    usedEnergy += levOre;

                    /* remove the ship's energy we used */
                    saveShip.sh_energy -= usedEnergy;
                    if ((saveShip.sh_weapon[1] > 2) && (usedEnergy != 0))
		    {
                        userN2(IS, "Your miner #", saveShip.sh_number);
                        userN3(IS, " used up ", usedEnergy, " units of "
			       "energy\n");
                        needTell = TRUE;
		    }
		}

                /* see if they want notifications */
                if (saveShip.sh_weapon[1] > 1)
		{
                    /* see if the amount of ore changed */
                    if (saveShip.sh_items[it_ore] != oOre)
		    {
                        if (saveShip.sh_items[it_ore] > oOre)
			{
                            userN3(IS, "Your miner #", saveShip.sh_number,
				   " was able to ");
                            userN3(IS, "mine ", saveShip.sh_items[it_ore] - oOre,
				   " tons of ore\n");
                            needTell = TRUE;
			}
                        else
			{
                            userN2(IS, "Your miner #", saveShip.sh_number);
                            userN3(IS, " lost ", oOre - saveShip.sh_items[it_ore],
				   " tons of ore\n");
                            needTell = TRUE;
			}
		    }
                    /* see if the number of bars changed */
                    if (saveShip.sh_items[it_bars] != oGold)
		    {
			if (saveShip.sh_items[it_bars] > oGold)
			{
                            userN3(IS, "Your miner #", saveShip.sh_number,
				   " was able to ");
                            userN3(IS, "mine ", saveShip.sh_items[it_bars] - oGold,
				   " gold bars\n");
                            needTell = TRUE;
			}
                        else
			{
                            userN2(IS, "Your miner #", saveShip.sh_number);
                            userN3(IS, " lost ", oGold - saveShip.sh_items[it_bars],
				   " gold bars\n");
                            needTell = TRUE;
			}
		    }
		}
	    }
	}

        /* restore the output flag */
        IS->is_noWrite = saveNoWrite;
        if (saveShip.sh_owner != IS->is_player.p_number)
        {
            /* see if we need to tell them anything */
            if (needTell)
            {
                /* send out the message */
                notify(IS, saveShip.sh_owner);
            }
        }
        else
        {
            /* it is always safe to flush output to the current player */
            uFlush(IS);
        }
	*rsh = saveShip;
        /* check if they were able to mine any gold */
        if (keepGold > 0)
        {
#ifdef DEBUG_LIB
            if (IS->is_DEBUG)
            {
		log3(IS, "!!! The miner DID mine some gold", "", "");
		*rsh = saveShip;
            }
#endif
            server(IS, rt_unlockShip, saveShip.sh_number);
#ifdef DEBUG_LIB
            if (IS->is_DEBUG)
            {
		sprintf(&numBuff[0], "%lu", savePlanet.pl_number);
		log3(IS, "!!! About to lock planet ",
		     &numBuff[0], "");
            }
#endif
            server(IS, rt_lockPlanet, savePlanet.pl_number);
	    if (IS->is_request.rq_u.ru_planet.pl_gold >= keepGold)
	    {
		IS->is_request.rq_u.ru_planet.pl_gold -= keepGold;
	    }
	    else
	    {
		IS->is_request.rq_u.ru_planet.pl_gold = 0;
	    }
            server(IS, rt_unlockPlanet, savePlanet.pl_number);
            server(IS, rt_lockShip, saveShip.sh_number);
        }
    }
}

