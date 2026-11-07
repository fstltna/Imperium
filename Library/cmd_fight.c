/*
 * Imperium
 *
 * $Id: cmd_fight.c,v 1.5 2000/05/26 22:55:19 marisa Exp $
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
 * $Log: cmd_fight.c,v $
 * Revision 1.5  2000/05/26 22:55:19  marisa
 * Combat changes
 *
 * Revision 1.4  2000/05/26 18:18:42  marisa
 * Fixed bug in cmd_fire
 *
 * Revision 1.3  2000/05/23 20:25:14  marisa
 * Added pl_weapon[] element to Planet_t struct
 * Began working on ship<->planet combat code
 *
 * Revision 1.2  2000/05/18 06:50:01  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:47  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:24:12  marisag
 * patch20: N/A
 *
 */

#include "../config.h"
#include "../Include/ImpStr.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdio.h>
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"
#include "Combat.h"

static const char rcsid[] __attribute__((unused)) = "$Id: cmd_fight.c,v 1.5 2000/05/26 22:55:19 marisa Exp $";



/* Maximum range for photon torps is 25 sectors */
#define MAX_PHO_RNG 25

/*
 * maxShields - returns the maximum number of units of shield energy
 *          the ship may use.
 *          Will read in big items, so ship must NOT be in the buffer!
 */

USHORT maxShields(IMP, register Ship_t *sh)
{
    register USHORT inst;

    inst = numInst(IS, sh, bp_shield);
    if (inst == 0)
    {
        return 0;
    }
    return(((((inst * 50) * getShipEff(IS, sh, bp_shield)) / 100) *
        getShipTechFactor(IS, sh, bp_shield)) / 100);
}

#ifdef BUBBA
/*
 * defend - allow a planet which is under attack to be defended by any
 *      defending fort. Return the percent damage done.
 *      Convention: the planet under attack is present but not locked.
 */

USHORT defend(IMP, USHORT r, USHORT c, USHORT aRow, USHORT aCol, USHORT dFactor)
{
    register Planet_t *rpl;
    register USHORT damage, techFactor;
    USHORT dRow, dCol;
    ULONG rangeSq;
    USHORT guns, mapped, victim;

    rpl = &IS->is_request.rq_u.ru_planet;
    damage = 0;
    if (rs->s_defender != NO_DEFEND)
    {
        getDefender(r, c, rs, &dRow, &dCol);
        mapped = mapSector(dRow, dCol);
        victim = rs->s_owner;
        techFactor = getTechFactor(victim);
        server(IS, rt_lockSector, mapped);
        updateSector(dRow, dCol);
        guns = umin(IS->is_world.w_gunMax, readQuan(rs, it_guns));
        rangeSq = techFactor * guns;
        rangeSq = rangeSq * rangeSq / IS->is_world.w_rangeDivisor / 100;
        if ((rs->s_type == s_fortress) && (rs->s_owner == victim) &&
            (readQuan(rs, it_military) >= 5) &&
            (rs->s_quantity[it_missiles] != 0) &&
            (guns != 0) && (rs->s_efficiency >= 60) &&
            (rangeSq >= findDistance(IS, aRow, aCol, dRow, dCol)))
        {
            writeQuan(rs, it_missiles, readQuan(rs, it_missiles) - 1);
            server(IS, rt_unlockSector, mapped);
            damage = guns * rs->s_efficiency / 100 + 10;
            damage =
                umin(100, damage * dFactor * IS->is_world.w_gunScale / 100);
            userS(IS, "Defensive fire from ", dRow, dCol, " inflicts ");
            userN(IS, damage);
            user(IS, "% damage!!\n");
            if (victim != 0)
            {
               news(IS, n_fire_back, victim, IS->is_player.p_number);
            }
        }
        else
        {
            server(IS, rt_unlockSector, mapped);
        }
        /* keep the calling convention maintained */
        server(IS, rt_readSector, mapSector(r, c));
    }
    return damage;
}

/*
 * fight - general purpose fighting routine. Passed the coordinates and
 *      updated planet under attack, along with pointers to 4 counts of
 *      attackers and their strength bonuses. These counts are updated.
 *      This routine automatically handles any defenders and their strength,
 *      along with checking the attacker's BTU's.
 *      Convention: the planet under attack will be read and written.
 */

void fight(IMP, ULONG plNum, USHORT *pCTop, USHORT sTop, USHORT *pCLeft,
    USHORT sLeft, USHORT *pCRight, USHORT sRight, USHORT *pCBot, USHORT sBot)
{
    register Planet_t *rpl;
    register Player_t *rp;
    register uint damage, cDef, attackers;
    uint defenders, cTop, cLeft, cRight, cBot, sDef, guns, mapped, victim;
    long btus;
    char ch;

    server(IS, rt_readPlanet, plNum);
    rpl = &IS->is_request.rq_u.ru_planet;
    rp = &IS->is_request.rq_u.ru_player
    victim = rpl->pl_owner;
    cTop = pCTop*;
    cLeft = pCLeft*;
    cRight = pCRight*;
    cBot = pCBot*;
    cDef = readQuan(rpl, it_military);
    sDef = 1;
    /* defensive fire from this planet */
    guns = umin(IS->is_world.w_gunMax, readQuan(rpl, it_guns));
    if ((readPlQuan(IS, rpl, it_officers) >= 5) &&
        (readPlQuan(IS, rpl, it_missiles) != 0) && (guns != 0) &&
        (rpl->pl_efficiency >= EFFIC_WARN))
    {
        damage = guns * rpl->pl_efficiency / 100 + 10;
        damage = umin(100, damage * IS->is_world.w_gunScale / 100);
        server(IS, rt_lockPlanet, plNum);
        if (readQuan(rpl, it_missiles) != 0)
        {
            writeQuan(rpl, it_missiles, readQuan(rpl, it_missiles) - 1);
            server(IS, rt_unlockPlanet, plNum);
            userP(IS, "Defensive fire from ", rpl, " inflicts ");
            userN(IS, damage);
            user(IS, "% damage!!\n");
            cTop = damageUnit(cTop, damage);
            cLeft = damageUnit(cLeft, damage);
            cRight = damageUnit(cRight, damage);
            cBot = damageUnit(cBot, damage);
        }
        else
        {
            server(IS, rt_unlockPlanet, plNum);
        }
    }
    /* defensive fire from any defending fortress */
    damage = defend(r, c, r, c, 1);
    cTop = damageUnit(cTop, damage);
    cLeft = damageUnit(cLeft, damage);
    cRight = damageUnit(cRight, damage);
    cBot = damageUnit(cBot, damage);
    /* scale BTU's to get required accuracy */
    btus = pretend(IS->is_player.p_btu * 100, long);
    while(
        attackers = cTop   * sTop   + cLeft * sLeft +
                     cRight * sRight + cBot  * sBot;
        defenders = cDef * sDef;
        defenders = defenders * IS->is_world.w_assAdv / 100;
        btus > 0 and attackers != 0 and defenders != 0)
    {
        if (impRandom(attackers + defenders) >= attackers)
        {
            /* an attacker has bit the bullet */
            btus -= IS->is_world.w_deathFactor;
            ch = '@';
            damage = impRandom(IS, attackers);
            if (damage < cTop * sTop)
            {
                cTop--;
                if (cTop == 0)
                {
                    ch = '*';
                }
            }
            else if (damage < cTop * sTop + cLeft * sLeft)
            {
                cLeft--;
                if (cLeft == 0)
                {
                    ch = '*';
                }
            }
            else if (damage < cTop * sTop + cLeft * sLeft + cRight * sRight)
            {
                cRight--;
                if (cRight == 0)
                {
                    ch = '*';
                }
            }
            else
            {
                if (cBot == 0)
                {
                    user(IS, "!!! fight mucked up - tell system owner !!!\n");
                    cBot = 1;
                }
                cBot--;
                if (cBot == 0)
                {
                    ch = '*';
                }
            }
        }
        else
        {
            /* a defender has bit the bullet */
            cDef--;
            ch = '!';
        }
        impSleep(impRandom(IS, 20));
        userC(IS, ch);
        uFlush(IS);
    }
    userNL(IS);
    userP(IS, "Your planet ", rpl, " has been assaulted");
    if (cDef == 0)
    {
        user3(IS, " and taken over by ", &IS->is_player.p_name[0], "!");
        notify(IS, victim);
        user(IS, "You have won the planet!\n");
        if (victim != NO_OWNER)
        {
           news(IS, n_won_planet, IS->is_player.p_number, victim);
        }
        /* the attacker gets NONE of any pending work in the taken-over planet.
           If you want him to, you should only do the following line if the
           planet WAS owned by the Deity, and you should call
           adjustForNewWorkers after putting military into the planet. */
        server(IS, rt_lockPlanet, plNum);
        rpl->pl_lastUpdate = timeNow();
        rpl->pl_owner = IS->is_player.p_number;
        rpl->pl_defender = NO_DEFEND;
        rpl->pl_checkPoint[0] = '\0';
        rpl->pl_price = 0;
        if (IS->is_player.p_planetCount == 0)
        {
           IS->is_noWrite = TRUE;
           user(IS, "There is hope for you yet!\n");
           news(IS, n_reborn, IS->is_player.p_number, 0);
           server(IS, rt_unlockSector, mapped);
           server(IS, rt_lockPlayer, IS->is_player.p_number);
           IS->is_request.rq_u.ru_player.p_status = ps_active;
           server(IS, rt_unlockPlayer, IS->is_player.p_number);
           IS->is_player.p_status = ps_active;
           IS->is_noWrite = FALSE;
           uFlush(IS);
           server(IS, rt_lockSector, mapped);
        }
        /* the taken-over planet should be occupied */
        attackers = 0;
        while ((attackers != MAX_WORK) && (cTop + cLeft + cRight + cBot != 0))
        {
            attackers++;
            damage = impRandom(cTop + cLeft + cRight + cBot);
            if (damage < cTop)
            {
                cTop--;
            }
            else if (damage < cTop + cLeft)
            {
                cLeft--;
            }
            else if (damage < cTop + cLeft + cRight)
            {
                cRight--;
            }
            else
            {
                if (cBot == 0)
                {
                    /* printed while planet locked, but I've never seen this
                       message, so this code must work. */
                    user(IS, "!!! fight blew it, tell system owner !!!\n");
                    cBot = 1;
                }
                cBot--;
            }
        }
        writeQuan(rs, it_military, attackers);
        server(IS, rt_unlockSector, mapped);
        if (victim != NO_OWNER)
        {
            server(IS, rt_lockPlayer, victim);
            rp->p_planetCount--;
            server(IS, rt_unlockPlayer, victim);
        }
        server(IS, rt_lockPlayer, IS->is_player.p_number);
        rp->p_planetCount++;
        server(IS, rt_unlockPlayer, IS->is_player.p_number);
        IS->is_player.p_planetCount = rp->p_planetCount;
    }
    else
    {
        user3(IS, " by ", &IS->is_player.p_name[0], "!");
        notify(IS, victim);
        if (cTop + cLeft + cRight + cBot == 0)
        {
            if (victim != NO_OWNER)
            {
               news(IS, n_lost_planet, IS->is_player.p_number, victim);
            }
            user(IS, "You have been defeated!\n");
        }
        else
        {
            user3(IS, "You ran out of BTU's - assault aborted!\n");
        }
        server(IS, rt_lockSector, mapped);
        writeQuan(rs, it_military, cDef);
        server(IS, rt_unlockSector, mapped);
    }
    if (btus < 0)
    {
        btus = 0;
    }
    IS->is_player.p_btu = (USHORT)(btus / 100);
    server(IS, rt_lockplayer, IS->is_player.p_number);
    rp->p_btu = IS->is_player.p_btu;
    server(IS, rt_unlockPlayer, IS->is_player.p_number);
    *pCTop = cTop;
    *pCLeft = cLeft;
    *pCRight = cRight;
    *pCBot = cBot;
}
#endif

/*
 * needShells - returns TRUE if the given big item requires some form of
 *              "shells" to be able to fire.
 */

BOOL needShells(BigPart_t which)
{
    /* make sure that any new items you add come AFTER the following two */
    /* choices */
    switch(which)
    {
        case bp_blaser:
            return FALSE;
            break;
        case bp_photon:
            return TRUE;
            break;
        default:
            return FALSE;
            break;
    }
}

/*
 * Returns TRUE if the given big item type is a weapon
 */

BOOL isAWeapon(BigPart_t which)
{
    switch(which)
    {
        case bp_blaser:
        case bp_photon:
            return TRUE;
            break;
        /* Make sure you add any new weapons before the "default" line */
        default:
            return FALSE;
            break;
    }
}

/*
 * reqShipWeapType - returns a user-selected weapon type, depending on what
 *              is installed in the ship. Assumes that the ship is NOT in the
 *              buffer.
 *
 * Return codes:
 *              0 = the ship has no weapons installed
 *              1 = canceled
 *          2 - 5 = reserved
 *              * = weapon desired
 */

BigPart_t reqShipWeapType(IMP, register Ship_t *sh)
{
    register USHORT weap;
    register BigItem_t *rbi;
    USHORT i, j, which;
    BigPart_t weapTp;
    USHORT numType[BP_LAST+1][3];
    char *choices;
    char *prompt;

#define P_NUM   0
#define P_EFF   1
#define P_TF    2

    /* set up the pointers */
    rbi = &IS->is_request.rq_u.ru_bigItem;
    /* set up the default */
    weapTp = 1;

    for (i = 0; i <= BP_LAST; i++)
    {
        for (j = 0; j < 3; j++)
        {
           numType[i][j] = 0;
        }
    }
    /* first scan the ship to see what types they have on board */
    weap = 0;
    while ((weap < MAX_WEAP) && (sh->sh_weapon[weap] != NO_ITEM))
    {
        /* read the item in */
        server(IS, rt_readBigItem, sh->sh_weapon[weap]);
        /* increment the number of these items */
        /* keep running totals - averaged below (this used to average */
        /* each item with the running value, so the first item counted */
        /* half and the table showed numbers that were too low) */
        numType[rbi->bi_part][P_NUM]++;
        numType[rbi->bi_part][P_EFF] += rbi->bi_effic;
        numType[rbi->bi_part][P_TF] += getTechFactor(rbi->bi_techLevel);
        weap++;
    }
    /* make sure they had at least one weapon */
    if (weap == 0)
    {
        userSh(IS, "No weapons installed on ship #", sh, "\n");
        /* 0 = no weapons installed */
        return 0;
    }
    (void) skipBlanks(IS);

    /* if they do not have any type-ahead, then display a chart of what is */
    /* available */
    if (*IS->is_textInPos == '\0')
    {
        user(IS, "Weapon Type             # Inst  TF   EF\n");
        dash(IS, 39);
        /* Note that this is assuming that bp_photon is the first weapon */
        for (i = bp_photon; i <= BP_LAST; i++)
        {
            if ((numType[i][P_NUM] != 0) && isAWeapon(i))
            {
                numType[i][P_EFF] /= numType[i][P_NUM];
                numType[i][P_TF] /= numType[i][P_NUM];
                switch(i)
                {
                    case bp_blaser:
                        user(IS, "Blaser                   ");
                        break;
                    case bp_photon:
                        user(IS, "Photon torpedos          ");
                        break;
                    /* add new weapon types above here */
                    default:
                        user(IS, "*UNKNOWN WEAPON TYPE*    ");
                        break;
                }
                userF(IS, numType[i][P_NUM], 4);
                user(IS, "  ");
                userF(IS, numType[i][P_TF], 3);
                user(IS, "  ");
                userF(IS, numType[i][P_EFF], 3);
                userNL(IS);
            }
        }
    }
/*
  Put in here things to add more selections if a weapon is available
  besides the standard weapons
*/
    choices = "blaser\0photon\0";
    prompt = "Fire what (blasers/photon)";
    if (reqChoice(IS, &which, choices, prompt))
    {
        /* get the correct weapon type */
        /* make sure that any new items you add come AFTER the above two */
        /* choices */
        switch(which)
        {
            case 0:
                weapTp = bp_blaser;
                break;
            case 1:
                weapTp = bp_photon;
                break;
            default:
		break;
        }
    }
    return weapTp;
}

/*
 * reqPlanetWeapType - returns a user-selected weapon type, depending on what
 *              is installed in the planet. Assumes that the planet is NOT in the
 *              buffer.
 *
 * Return codes:
 *              0 = the ship has no weapons installed
 *              1 = canceled
 *          2 - 5 = reserved
 *              * = weapon desired
 */

BigPart_t reqPlanetWeapType(IMP, register Planet_t *pl)
{
    register USHORT weap;
    register BigItem_t *rbi;
    USHORT i, j, which;
    BigPart_t weapTp;
    USHORT numType[BP_LAST+1][3];
    char *choices;
    char *prompt;

#define P_NUM   0
#define P_EFF   1
#define P_TF    2

    /* set up the pointers */
    rbi = &IS->is_request.rq_u.ru_bigItem;
    /* set up the default */
    weapTp = 1;

    for (i = 0; i <= BP_LAST; i++)
    {
        for (j = 0; j < 3; j++)
        {
           numType[i][j] = 0;
        }
    }
    /* first scan the planet to see what types they have activated */
    weap = 0;
    while ((weap < MAX_WEAP_PL) && (pl->pl_weapon[weap] != NO_ITEM))
    {
        /* read the item in */
        server(IS, rt_readBigItem, pl->pl_weapon[weap]);
        /* increment the number of these items */
        /* keep running totals - averaged below (this used to average */
        /* each item with the running value, so the first item counted */
        /* half and the table showed numbers that were too low) */
        numType[rbi->bi_part][P_NUM]++;
        numType[rbi->bi_part][P_EFF] += rbi->bi_effic;
        numType[rbi->bi_part][P_TF] += getTechFactor(rbi->bi_techLevel);
        weap++;
    }
    /* make sure they had at least one weapon */
    if (weap == 0)
    {
        userP(IS, "No weapons activated on planet #", pl, "\n");
        /* 0 = no weapons activated */
        return 0;
    }
    (void) skipBlanks(IS);

    /* if they do not have any type-ahead, then display a chart of what is */
    /* available */
    if (*IS->is_textInPos == '\0')
    {
        user(IS, "Weapon Type             # Actv  TF   EF\n");
        dash(IS, 39);
        /* Note that this is assuming that bp_photon is the first weapon */
        for (i = bp_photon; i <= BP_LAST; i++)
        {
            if ((numType[i][P_NUM] != 0) && isAWeapon(i))
            {
                numType[i][P_EFF] /= numType[i][P_NUM];
                numType[i][P_TF] /= numType[i][P_NUM];
                switch(i)
                {
                    case bp_blaser:
                        user(IS, "Blaser                   ");
                        break;
                    case bp_photon:
                        user(IS, "Photon torpedos          ");
                        break;
                    /* add new weapon types above here */
                    default:
                        user(IS, "*UNKNOWN WEAPON TYPE*    ");
                        break;
                }
                userF(IS, numType[i][P_NUM], 4);
                user(IS, "  ");
                userF(IS, numType[i][P_TF], 3);
                user(IS, "  ");
                userF(IS, numType[i][P_EFF], 3);
                userNL(IS);
            }
        }
    }
/*
  Put in here things to add more selections if a weapon is available
  besides the standard weapons
*/
    choices = "blaser\0photon\0";
    prompt = "Fire what (blasers/photon)";
    if (reqChoice(IS, &which, choices, prompt))
    {
        /* get the correct weapon type */
        /* make sure that any new items you add come AFTER the above two */
        /* choices */
        switch(which)
        {
            case 0:
                weapTp = bp_blaser;
                break;
            case 1:
                weapTp = bp_photon;
                break;
            default:
		break;
        }
    }
    return weapTp;
}

/*
 * hasWeap - returns TRUE if the ship has at least one weapon that is
 *          capable of being fired.
 *          Will read big items so ship must NOT be in the buffer.
 */

BOOL hasWeap(IMP, register Ship_t *sh)
{
    register USHORT weap;
    register BigItem_t *rbi;

    /* set up the pointer */
    rbi = &IS->is_request.rq_u.ru_bigItem;
    weap = 0;
    while ((weap < MAX_WEAP) && (sh->sh_weapon[weap] != NO_ITEM))
    {
        /* read the item in */
        server(IS, rt_readBigItem, sh->sh_weapon[weap]);
        if (rbi->bi_effic >= EFFIC_WARN)
        {
            return TRUE;
        }
        weap++;
    }
    return FALSE;
}

/*
 * hasWeapPl - returns TRUE if the planet has at least one weapon that is
 *          capable of being fired.
 *          Will read big items so planet must NOT be in the buffer.
 */

BOOL hasWeapPl(IMP, register Planet_t *pl)
{
    register USHORT weap;
    register BigItem_t *rbi;

    /* set up the pointer */
    rbi = &IS->is_request.rq_u.ru_bigItem;
    weap = 0;
    while ((weap < MAX_WEAP_PL) && (pl->pl_weapon[weap] != NO_ITEM))
    {
        /* read the item in */
        server(IS, rt_readBigItem, pl->pl_weapon[weap]);
        if (rbi->bi_effic >= EFFIC_WARN)
        {
            return TRUE;
        }
        weap++;
    }
    return FALSE;
}

/*
 * getShipWeapRange - returns the range of the ships weapons in
 *                    subsectors.
 * Assumes: The ship is NOT in the buffer and the buffer does not have
 *          to be preserved.
 */

ULONG getShipWeapRange(IMP, register Ship_t *srcShip, BigPart_t weapType)
{
    ULONG weapRange = 0;

    if (numInst(IS, srcShip, weapType) > 0)
    {
	switch(weapType)
	{
	    case bp_photon:
		weapRange = (((MAX_PHO_RNG * getShipTechFactor(IS, srcShip,
		    bp_photon)) / 100) * getShipEff(IS, srcShip, bp_photon))
		    / 100;
		break;
	    case bp_blaser:
		weapRange = (((IS->is_world.w_phaserRange * getShipTechFactor(IS,
		    srcShip, bp_blaser)) / 100) * getShipEff(IS, srcShip,
		    bp_blaser)) / 100;
		break;
	    default:
		break;
	}
    }
    return(weapRange);
}

/*
 * fireBlaser() - handles firing a blaser on a ship or planet
 *              Assumes:
 *                  The ship DOING the firing is NOT in the buffer,
 *                      but IS locked (and thus must be unlocked as
 *                      needed).
 *                  The caller will flush the final output (if any).
 *              Returns:
 *                  TRUE if the player should be charged BTUs.
 */

BOOL fireBlaser(IMP, Ship_t *aShip, register USHORT units, ULONG target,
    BOOL targetIsShip, BOOL playIsFire)
{
    register Ship_t *rsh;
    UBYTE aWeapTf, aWeapEff, aCompEff, aCompTf, dmgPct, oddsMod,
        TargOwn, origTargOwn, aShOwn;
    USHORT blasInst, maxUnits, tmpUnits, damage, tmpOdds;
    ShipDamRes_t damRes;
    Ship_t saveShip;

    /*
     * You might ask why we don't just avoid calling this function if
     * it will not be effective against planets. The reason is that it
     * MIGHT be possible later, and then we only have to modify the
     * code here to support it
     */
    if (!targetIsShip)
    {
	/* If the current player is firing, send him a text message */
        if (playIsFire)
        {
            user(IS, "Blasers have no effect on planets!\n");
        }
        return(FALSE);
    }
    /* Set the pointer to the correct location */
    rsh = &IS->is_request.rq_u.ru_ship;
    aShOwn = aShip->sh_owner;
    blasInst = numInst(IS, aShip, bp_blaser);
    /* We check for blasInst == 0 here because they could have been fired */
    /* upon (if our locking isn't working right :-) */
    if (blasInst == 0)
    {
	/* If the current player is firing, send him a text message */
        if (playIsFire)
        {
            user(IS, "You do not have any blasers installed in your ship!\n");
        }
        return(FALSE);
    }
    /* Check the number of crew members vs. that required for use */
    if (((aShip->sh_items[it_military] <= IS->is_world.w_crewReq[CR_PHA].cr_mil)
	&& (aShip->sh_items[it_officers] <= IS->is_world.w_crewReq[CR_PHA].cr_mil))
	|| (aShip->sh_items[it_officers] <= IS->is_world.w_crewReq[CR_PHA].cr_ofc))
    {
	/* If the current player is firing, send him a text message */
        if (playIsFire)
        {
            user(IS, "You do not have enough crew on your ship to fire "
		"blasers!\n");
        }
        return(FALSE);
    }
    /* get the ship's weapon TF & Eff */
    /* The weapon's TF & EFF will determine how many units they can fire */
    /* safely */
    aWeapTf = getShipTechFactor(IS, aShip, bp_blaser);
    aWeapEff = getShipEff(IS, aShip, bp_blaser);
    /* The computer's TF & EFF will determine the odds of hitting the target */
    /* ship */
    aCompTf = getShipTechFactor(IS, aShip, bp_computer);
    aCompEff = getShipEff(IS, aShip, bp_computer);
    
    /* Make sure this ship can fire ... */
    if ((aWeapTf == 0) || (aWeapEff == 0) ||
        (aCompTf == 0) || (aCompEff == 0))
    {
	/* If the current player is firing, send him a text message */
        if (playIsFire)
        {
            user(IS, "Your ship is incapable of firing that type of weapon!\n");
        }
        return(FALSE);
    }
    /* calculate the range to the target */
    server(IS, rt_readShip, target);
    saveShip = *rsh;
    if (getShipWeapRange(IS, aShip, bp_blaser) < findDistance(IS,
	aShip->sh_row, aShip->sh_col, saveShip.sh_row, saveShip.sh_col))
    {
        if (playIsFire)
        {
            userSh(IS, "Ship #", &saveShip, " is out of range of your ship's "
                "blasers.\n");
        }
        return(TRUE);
    }
    /* oddsMod holds modifiers to make it harder to hit a ship */
    oddsMod = 0;
    /* See if the ship is one a planet */
    if (saveShip.sh_planet != NO_ITEM)
    {
        /* It is, so see if the planet is highly gaseous, thus making it */
        /* harder to hit */
        server(IS, rt_readPlanet, saveShip.sh_planet);
        if (IS->is_request.rq_u.ru_planet.pl_gas > 75)
        {
            oddsMod = (IS->is_request.rq_u.ru_planet.pl_gas - 75) * 2;
        }
    }
    /* See if they are requesting too many units to fire */
    if (units > aShip->sh_energy)
    {
        units = aShip->sh_energy;
        if (units == 0)
        {
            if (playIsFire)
            {
                user(IS, "Insufficient energy to fire!\n");
            }
            return(FALSE);
        }
        if (playIsFire)
        {
            userN3(IS, "Reducing units to fire down to ", units, "...\n");
        }
    }
    /* Get the maximum amount of energy they can discharge in one blast */
    /* SAFELY */
    maxUnits = (((((blasInst * 40) * aWeapEff) / 100) * aWeapTf) / 100);
    if (!playIsFire)
    {
        units = umin(units, maxUnits);
    }
    /* dmgPct holds the odds of the weapon being damaged by firing it */
    dmgPct = 0;
    /* Now see if they are using too much energy for their weapons to */
    /* handle */
    if (units > maxUnits)
    {
        /* We only need to do this kind of checking if they are operating */
        /* their weapons over the rated limits */
        tmpUnits = maxUnits / 10;
        if (units < (maxUnits + tmpUnits))
        {
            dmgPct = 5;
        }
        else
        {
            if (units < (maxUnits + (tmpUnits * 2)))
            {
                dmgPct = 10;
            }
            else
            {
                if (units < (maxUnits + (tmpUnits * 3)))
                {
                    dmgPct = 25;
                }
                else
                {
                    if (units < (maxUnits + (tmpUnits * 4)))
                    {
                        dmgPct = 50;
                    }
                    else
                    {
                        userN3(IS, "Requested energy output is too high - "
                            "reducing to ", maxUnits, " units\n");
                        units = maxUnits;
                    }
                }
            }
        }
    }
    /* reduce the energy on the attacking ship */
    aShip->sh_energy -= units;
    *rsh = *aShip;
    /* Now unlock the ship so that we can damage the target ship */
    server(IS, rt_unlockShip, aShip->sh_number);
    /* Clean out the buffer so we can generate any needed comments */
    if (!playIsFire)
    {
        user2(IS, "Your ", getShipName(aShip->sh_type));
        userN2(IS, " ship #", aShip->sh_number);
        userN3(IS, " is firing on ship #", target, "!\n");
        notify(IS, aShip->sh_owner);
    }
    else
    {
        uFlush(IS);
    }
    /* Lock the target ship */
    server(IS, rt_lockShip, target);
    origTargOwn = rsh->sh_owner;
    /* Do some more sanity check */
    if (rsh->sh_owner == NO_OWNER)
    {
        user(IS, "Another ship has destroyed the target ship!");
        server(IS, rt_unlockShip, target);
    }
    else if (rsh->sh_owner == aShOwn)
    {
        user(IS, "Your men refuse to fire upon one of their own ships "
            "and discharge the blast into open space!\n");
        server(IS, rt_unlockShip, target);
    }
    else
    {
        /* Make sure they have at least a 65% chance of hitting */
        if (aCompTf < 65)
        {
            tmpOdds = 65 - aCompTf;
        }
        else
        {
            tmpOdds = 0;
        }
        /* See if they hit the ship */
        if ((impRandom(IS, 1000) + (oddsMod * 10)) <
            (((aCompTf + tmpOdds) * aCompEff) / 10))
        {
            /* They did, so damage the target ship */
            user2(IS, "Your ", getShipName(rsh->sh_type));
            userN2(IS, " #", target);
	    userN3(IS, " was fired upon by a blaser from ship #",
		aShip->sh_number, "!\n");
            damRes = damageShip(IS, target, units, TRUE);
            server(IS, rt_unlockShip, target);
	    TargOwn = rsh->sh_owner;
            if (TargOwn == NO_OWNER)
	    {
	        user(IS, "The ship was destroyed!\n");
	    }
	    else
            {
                if (damRes.sdr_shields != 0)
                {
                    userN3(IS, "Your shields absorbed ", damRes.sdr_shields,
                        " units.\n");
                }
                if (damRes.sdr_armour != 0)
                {
                    userN3(IS, "Your armour absorbed ", damRes.sdr_armour,
                        " units.\n");
                }
                if (damRes.sdr_main != 0)
                {
                    userN3(IS, "Various parts of the ship took ",
                        damRes.sdr_main, " units of damage.\n");
                }
                /* See if they are able to detect the owner of the firing ship */
                /* NOT DONE */
            }
            if (playIsFire)
            {
                notify(IS, origTargOwn);
            }
            else
            {
                uFlush(IS);
            }
            news(IS, n_blase_ship, aShOwn, origTargOwn);
            user(IS, "Target hit!\n");
            if (TargOwn == NO_OWNER)
            {
                user(IS, "Your attack destroyed the ship!\n");
            }
            else
            {
		if (damRes.sdr_main != 0)
                {
		    userN3(IS, "The ship appears to have suffered ",
			damRes.sdr_main, " units of structural damage.\n");
                }
            }
        }
        else
        {
            /* (the target used to be left locked here) */
            server(IS, rt_unlockShip, target);
            user(IS, "You missed!\n");
        }
    }
    /* Relock the firing ship */
    server(IS, rt_lockShip, aShip->sh_number);
    /* See if the ship has been damaged by outside forces */
    if (rsh->sh_owner == NO_OWNER)
    {
        userN3(IS, "Your ship #", rsh->sh_number, " has been "
            "destroyed!\n");
    }
    else
    {
        /* See if we need to do any damage */
        /* relating to the over-stressing of the weapons */
        if (dmgPct > 0)
        {
            /* Scale up the percentages */
            if (impRandom(IS, 10000) < dmgPct * 100)
            {
                damage = IS->is_world.w_phaserDmg * (units - maxUnits);
                userN2(IS, "Your ship #", rsh->sh_number);
                userN3(IS, " took ", damage, " units of damage from "
                    "overloaded blasers!\n");
                /* Damage the ship */
                /* NOTE: This should be improved to damage the */
                /* blasers first, THEN the rest of the ship */
                damRes = damageShip(IS, rsh->sh_number, damage, FALSE);
                if (rsh->sh_owner == NO_OWNER)
                {
                    userN3(IS, "Your ship #", rsh->sh_number, " has been "
                        "destroyed!\n");
                }
                else
                {
		    /* NOTE: if working correctly this will not happen */
                    if (damRes.sdr_shields != 0)
                    {
                        userN3(IS, "Your shields absorbed ",
                            damRes.sdr_shields, " units.\n");
                    }
		    /* NOTE: if working correctly this will not happen */
                    if (damRes.sdr_armour != 0)
                    {
                        userN3(IS, "Your armour absorbed ",
                            damRes.sdr_armour, " units.\n");
                    }
                    if (damRes.sdr_main != 0)
                    {
                        userN3(IS, "Various parts of the ship "
                            "took ", damRes.sdr_main, " units of"
                            "damage.\n");
                    }
                }
            }
        }
    }
    /* Copy the ship back into the structure */
    *aShip = *rsh;
    return(TRUE);
}

/*
 * firePhoton() - handles firing photon torpedo(s) on a ship or planet
 *              Assumes:
 *                  The ship DOING the firing is NOT in the buffer,
 *                      but IS locked (and thus must be unlocked as
 *                      needed).
 *                  The caller will flush the final output (if any).
 *              Returns:
 *                  TRUE if the player should be charged BTUs.
 */

BOOL firePhoton(IMP, void *attacker, register USHORT units, ULONG target,
    BOOL targetIsShip, BOOL playIsFire)
{
    Ship_t *rsh, *aShip;
    UBYTE aWeapTf, aWeapEff, aCompEff, aCompTf, oddsMod,
        TargOwn, origTargOwn, attOwn;
    USHORT photInst, rangeMod;
    ULONG weapRange, targRange;
    ShipDamRes_t damRes;
    Ship_t saveShip;
    Planet_t savePlanet, *rpl;

    /* Set the pointer to the correct location */
        rsh = &IS->is_request.rq_u.ru_ship;
        rpl = &IS->is_request.rq_u.ru_planet;
	aShip = (Ship_t *)attacker;
        /* Check their energy */
        if (aShip->sh_energy < IS->is_world.w_torpMobCost)
        {
	    /* If the current player is firing, send him a text message */
            if (playIsFire)
            {
                user(IS, "You do not have enough energy to use photon "
	        	"torpedos!\n");
            }
            return(FALSE);
        }
        attOwn = aShip->sh_owner;
        photInst = numInst(IS, aShip, bp_photon);
    /* We check for photInst == 0 here because they could have been fired */
    /* upon (if our locking isn't working right :-) */
    if (photInst == 0)
    {
	/* If the current player is firing, send him a text message */
        if (playIsFire)
        {
                user(IS, "You do not have any photon torpedo tubes installed "
		    "in your ship!\n");
        }
        return(FALSE);
    }
    /* Check the number of crew members vs. that required for use */
    if (((aShip->sh_items[it_military] <= IS->is_world.w_crewReq[CR_PHO].cr_mil)
	&& (aShip->sh_items[it_officers] <= IS->is_world.w_crewReq[CR_PHO].cr_mil))
	|| (aShip->sh_items[it_officers] <= IS->is_world.w_crewReq[CR_PHO].cr_ofc))
    {
	/* If the current player is firing, send him a text message */
        if (playIsFire)
        {
            user(IS, "You do not have enough crew to fire "
		"photon torpedos!\n");
        }
        return(FALSE);
    }
    /* get the ship's weapon TF & Eff */
    /* The weapon's TF & EFF will determine the range & accuracy */
    aWeapTf = getShipTechFactor(IS, aShip, bp_photon);
    aWeapEff = getShipEff(IS, aShip, bp_photon);
    /* The computer's TF & EFF only determine if they can fire or not */
    aCompTf = getShipTechFactor(IS, aShip, bp_computer);
    aCompEff = getShipEff(IS, aShip, bp_computer);
    
    /* Make sure this ship can fire ... */
    if ((aWeapTf == 0) || (aWeapEff == 0) ||
        (aCompTf == 0) || (aCompEff == 0))
    {
	/* If the current player is firing, send him a text message */
        if (playIsFire)
        {
            user(IS, "Your ship is incapable of firing that type of weapon!\n");
        }
        return(FALSE);
    }
    weapRange = getShipWeapRange(IS, aShip, bp_photon);
    if (targetIsShip)
    {
        server(IS, rt_readShip, target);
        saveShip = *rsh;
    }
    else
    {
        server(IS, rt_readPlanet, target);
        savePlanet = *rpl;
    }
    /* calculate the range to the target */
    if (targetIsShip)
    {
        targRange = findDistance(IS, aShip->sh_row, aShip->sh_col,
	    saveShip.sh_row, saveShip.sh_col);
    }
    else
    {
        targRange = findDistance(IS, aShip->sh_row, aShip->sh_col,
	    savePlanet.pl_row, savePlanet.pl_col);
    }
    if (weapRange < targRange)
    {
        if (playIsFire)
        {
    	    if (targetIsShip)
    	    {
                userSh(IS, "Ship #", &saveShip, " is out of range of your ship's "
                    "photon torpedos.\n");
	    }
	    else
    	    {
                userP(IS, "Planet #", &savePlanet, " is out of range of your ship's "
                    "photon torpedos.\n");
	    }
        }
        return(TRUE);
    }
    /* In the first 1/3 of max range we get full odds */
    rangeMod = IS->is_world.w_torpAcc;
    if ((targRange * 10) > ((weapRange * 10) / 3))
    {
	/* In the first 1/2 of max range we get 90% odds */
	if ((targRange * 10) <= ((weapRange * 10) / 2))
	{
	    rangeMod = ((rangeMod * 90) / 100);
	}
	else
	{
	    /* In the first 3/4 of max range we get 75% odds */
	    if ((targRange * 10) <= ((weapRange * 75) / 10))
	    {
		rangeMod = ((rangeMod * 75) / 100);
	    }
	    else
	    {
		/* Otherwise they get 60% odds */
		rangeMod = ((rangeMod * 60) / 100);
	    }
	}
    }
    /* oddsMod holds modifiers to make it harder to hit a ship */
    oddsMod = 0;
    /* See if the ship is on a planet (saveShip is only set for ships) */
    if (targetIsShip && (saveShip.sh_planet != NO_ITEM))
    {
        /* It is, so see if the planet is highly gaseous, thus making it */
        /* harder to hit */
        server(IS, rt_readPlanet, saveShip.sh_planet);
        if (IS->is_request.rq_u.ru_planet.pl_gas > 75)
        {
            oddsMod = (IS->is_request.rq_u.ru_planet.pl_gas - 75) * 2;
        }
    }
    /* See if they are requesting too many units to fire */
    /* Only one round per tube installed */
    if (units > photInst)
    {
	units = photInst;
    }
    if ((units * IS->is_world.w_torpCost) > aShip->sh_items[it_missiles])
    {
	if (aShip->sh_items[it_missiles] < IS->is_world.w_torpCost)
	{
	    units = 0;
	}
	else
	{
            units = aShip->sh_items[it_missiles] / IS->is_world.w_torpCost;
	}
        if (units == 0)
        {
            if (playIsFire)
            {
                user(IS, "Not enough missiles to fire!\n");
            }
            return(FALSE);
        }
        if (playIsFire)
        {
            userN3(IS, "Reducing torpedoes to fire down to ", units, "...\n");
        }
    }
    /* reduce the missiles on the attacking ship */
    aShip->sh_items[it_missiles] -= (units * IS->is_world.w_torpCost);
    aShip->sh_cargo -= ((units * IS->is_world.w_torpCost)
	* IS->is_world.w_weight[it_missiles]);
    aShip->sh_energy -= IS->is_world.w_torpMobCost;
    *rsh = *aShip;
    /* Now unlock the ship so that we can damage the target ship */
    server(IS, rt_unlockShip, aShip->sh_number);
    /* Clean out the buffer so we can generate any needed comments */
    if (!playIsFire)
    {
        user2(IS, "Your ", getShipName(aShip->sh_type));
        userN2(IS, " ship #", aShip->sh_number);
	if (targetIsShip)
	{
            userN3(IS, " is firing on ship #", target, "!\n");
	}
	else
	{
            userN3(IS, " is firing on planet #", target, "!\n");
	}
        notify(IS, aShip->sh_owner);
    }
    else
    {
        uFlush(IS);
    }
    /* Lock the target */
    if (targetIsShip)
    {
        server(IS, rt_lockShip, target);
        origTargOwn = rsh->sh_owner;
    }
    else
    {
        server(IS, rt_lockPlanet, target);
        origTargOwn = rpl->pl_owner;
    }
    /* Do some more sanity check */
    if (origTargOwn == NO_OWNER)
    {
        if (targetIsShip)
	{
            user(IS, "Another ship has destroyed the target ship!");
            server(IS, rt_unlockShip, target);
	}
	else
	{
            user(IS, "Another ship has wiped out the target planet!");
            server(IS, rt_unlockPlanet, target);
	}
    }
    else if (origTargOwn == attOwn)
    {
	if (targetIsShip)
	{
            user(IS, "Your men refuse to fire upon one of their own ships!\n");
            server(IS, rt_unlockShip, target);
	}
	else
	{
            user(IS, "Your men refuse to fire upon one of their own planets!\n");
            server(IS, rt_unlockPlanet, target);
	}
    }
    else
    {
        /* See if they hit the target */
        if ((impRandom(IS, 1000) + (oddsMod * 10)) <
            ((((aWeapTf * aWeapEff) / 10) * rangeMod) / 100))
        {
          /* They did, so damage the target */
	  if (targetIsShip)
	  {
            user2(IS, "Your ", getShipName(rsh->sh_type));
            userN2(IS, " #", target);
	    userN3(IS, " was fired upon by a photon torpedoe from ship #",
		aShip->sh_number, "!\n");
            damRes = damageShip(IS, target, (units *
		(IS->is_world.w_torpBase +
		(impRandom(IS, IS->is_world.w_torpRand * 10) / 10))), TRUE);
            server(IS, rt_unlockShip, target);
	    TargOwn = rsh->sh_owner;
            if (TargOwn == NO_OWNER)
	    {
	        user(IS, "The ship was destroyed!\n");
	    }
	    else
            {
                if (damRes.sdr_shields != 0)
                {
                    userN3(IS, "Your shields absorbed ", damRes.sdr_shields,
                        " units.\n");
                }
                if (damRes.sdr_armour != 0)
                {
                    userN3(IS, "Your armour absorbed ", damRes.sdr_armour,
                        " units.\n");
                }
                if (damRes.sdr_main != 0)
                {
                    userN3(IS, "Various parts of the ship took ",
                        damRes.sdr_main, " units of damage.\n");
                }
                /* See if they are able to detect the owner of the firing ship */
                /* NOT DONE */
            }
            if (playIsFire)
            {
                notify(IS, origTargOwn);
            }
            else
            {
                uFlush(IS);
            }
            news(IS, n_torp_ship, attOwn, origTargOwn);
            user(IS, "Target hit!\n");
            if (TargOwn == NO_OWNER)
            {
		news(IS, n_torp_dest, attOwn, origTargOwn);
                user(IS, "Your attack destroyed the ship!\n");
            }
            else
            {
		if (damRes.sdr_main != 0)
                {
		    userN3(IS, "The ship appears to have suffered ",
			damRes.sdr_main, " units of structural damage.\n");
                }
            }
	  }
	  else
	  {
            user2(IS, "Your ", getPlanetName(rpl->pl_class));
            userN2(IS, " #", target);
	    userN3(IS, " was fired upon by ship #",
		aShip->sh_number, "!\n");
            {
                USHORT pDamage = (units * (IS->is_world.w_torpBase +
                    (impRandom(IS, IS->is_world.w_torpRand * 10) / 10)));

                damagePlanet(IS, pDamage, attOwn);
                server(IS, rt_unlockPlanet, target);
                TargOwn = rpl->pl_owner;
                /* installed weapons and other big items get hit too */
                damagePlanetItems(IS, target, pDamage);
            }
            if (TargOwn == NO_OWNER)
	    {
	        user(IS, "The planet was wiped out!\n");
	    }
	    else
            {
                /* See if they are able to detect the owner of the firing ship */
                /* NOT DONE */
            }
            if (playIsFire)
            {
                notify(IS, origTargOwn);
            }
            else
            {
                uFlush(IS);
            }
            news(IS, n_bomb_planet, attOwn, origTargOwn);
            user(IS, "Target hit!\n");
            if (TargOwn == NO_OWNER)
            {
		news(IS, n_bomb_dest, attOwn, origTargOwn);
                user(IS, "Your attack wiped out the planet!\n");
            }
	  }
        }
        else
        {
	    if (targetIsShip)
	    {
                server(IS, rt_unlockShip, target);
	    }
	    else
	    {
                server(IS, rt_unlockPlanet, target);
	    }
            user(IS, "You missed!\n");
        }
    }
    /* Relock the firing ship */
    server(IS, rt_lockShip, aShip->sh_number);
    /* See if the ship has been damaged by outside forces */
    if (rsh->sh_owner == NO_OWNER)
    {
        userN3(IS, "Your ship #", rsh->sh_number, " has been "
            "destroyed!\n");
    }
    /* Copy the ship back into the structure */
    *aShip = *rsh;
    return(TRUE);
}

/*
 * fireWeap() - handles firing of the various weapon types
 *              Assumes:
 *                  The ship/plan DOING the firing IS in the buffer
 *                      and IS locked.
 *                  The caller will flush the (if any) output when they
 *                      unlock the firing ship.
 *              Returns:
 *                  TRUE if the player should be charged BTUs.
 */

BOOL fireWeap(IMP, BigPart_t weapType, register USHORT units,
    ULONG target, BOOL targetIsShip, BOOL playIsFire)
{
    Ship_t *rsh;
    BOOL saveNoWrite, retCode;
    Ship_t saveShip;

    /* First do stupidity checks */
    if ((units == 0) || (target == NO_ITEM))
    {
        return(FALSE);
    }
        rsh = &IS->is_request.rq_u.ru_ship;
        /* Be safe */
        updateShip(IS);
        if (rsh->sh_owner == NO_OWNER)
        {
	    return FALSE;
        }
    retCode = FALSE;
    saveNoWrite = IS->is_noWrite;
    IS->is_noWrite = TRUE;
    switch (weapType)
    {
        case bp_blaser:
            /* Blaser's are fairly simple */
            saveShip = *rsh;
            retCode = fireBlaser(IS, &saveShip, units, target, targetIsShip,
                playIsFire);
            *rsh = saveShip;
            break;
        case bp_photon:
               saveShip = *rsh;
               retCode = firePhoton(IS, &saveShip, units, target, targetIsShip,
                   playIsFire);
               *rsh = saveShip;
            break;
        default:
            userN3(IS, ">>> Error - alert the system owner right away that "
                "fireWeap was called with invalid code ", weapType, " !\n");
            break;
    }
    IS->is_noWrite = saveNoWrite;
    return(retCode);
}

/*
 * ammoAvail - returns the amount of "ammo" (shots) that may be fired
 *
 * Assumes: Ship is NOT in the buffer and the buffer does NOT have to
 *          be preserved.
 */

USHORT ammoAvail(IMP, register Ship_t *srcShip, BigPart_t weapType)
{
    switch(weapType)
    {
	case bp_blaser:
	    return(srcShip->sh_energy);
	    break;
	case bp_photon:
	    return(umin((srcShip->sh_items[it_missiles] /
		IS->is_world.w_torpCost), numInst(IS, srcShip, bp_photon)));
	    break;
	default:
	    return(0);
	    break;
    }
}

/*
 * ammoAvailPl - returns the amount of "ammo" (shots) that may be fired
 *
 * Assumes: Planet is NOT in the buffer and the buffer does NOT have to
 *          be preserved.
 */

USHORT ammoAvailPl(IMP, register Planet_t *srcPlanet, BigPart_t weapType)
{
    switch(weapType)
    {
	case bp_blaser:
	    /* Planets user ore for energy */
	    return(srcPlanet->pl_quantity[it_ore]);
	    break;
	case bp_photon:
	    return(umin((srcPlanet->pl_quantity[it_missiles] /
		IS->is_world.w_torpCost), numInstPl(IS, srcPlanet, bp_photon)));
	    break;
	default:
	    return(0);
	    break;
    }
}

/*
 * pickBestWeap - returns the best weapon to use (that is installed) for a
 *                given ship, or bp_blaser if none are installed. Thus
 *                you MUST check numInst() after using this function!
 *              Note: Reads big items, so the ship must NOT be in the
 *                    buffer.
 */

BigPart_t pickBestWeap(IMP, register Ship_t *fShip, ULONG tNum, BOOL isShip,
    USHORT tRow, USHORT tCol)
{
    ULONG range;
    USHORT photInst, blasInst, photAmmo, blasAmmo;
    UBYTE photTF, photEff, blasTF, blasEff;
    signed long photScore = 0, blasScore = 0;

    /* First try to eliminate weapons based on maximum range */
    range = findDistance(IS, tRow, tCol, fShip->sh_row, fShip->sh_col);
    if (range >= MAX_PHO_RNG)
    {
	/* Right now this would be the only choice if the target is out of
	   range of photon torpedos. Even if they don't have a blaser
	   installed, it is OK to return this, since it will be checking the
	   number of weapons installed, and so will not fire. Which is what
	   we want, since it is out of range of torpedos anyway
	*/
	return(bp_blaser);
    }
    /* Ok, we are in range of both types of weapons, so see what to use */
    if ((photInst = numInst(IS, fShip, bp_photon)) == 0)
    {
	/* See above about not checking further in this function */
	return(bp_blaser);
    }
    if ((blasInst = numInst(IS, fShip, bp_blaser)) == 0)
    {
	return(bp_photon);
    }
    /* They have both types of weapons, so let's get to work */

    /* First score the weapons based on ammo available */
    photAmmo = ammoAvail(IS, fShip, bp_photon);
    if (photAmmo == 0)
    {
	return(bp_blaser);
    }
    blasAmmo = ammoAvail(IS, fShip, bp_blaser);
    if (blasAmmo == 0)
    {
	return(bp_photon);
    }
    if ((photAmmo * 20) >= blasAmmo)
    {
	photScore += 1;
    }
    else
    {
	blasScore += 1;
    }
    photTF = getShipTechFactor(IS, fShip, bp_photon);
    photEff = getShipEff(IS, fShip, bp_photon);
    if ((photTF < 15) || (photEff < EFFIC_WARN))
    {
	return(bp_blaser);
    }
    blasTF = getShipTechFactor(IS, fShip, bp_blaser);
    blasEff = getShipEff(IS, fShip, bp_blaser);
    if ((blasTF < 15) || (blasEff < EFFIC_WARN))
    {
	return(bp_photon);
    }
    if (photTF < 30)
    {
	photScore -= 1;
    }
    photTF = (photTF * photEff) / 100;
    if (blasTF < 30)
    {
	blasScore -= 1;
    }
    blasTF = (blasTF * blasEff) / 100;
    if (photTF >= blasTF)
    {
	if ((photTF / 2) > blasTF)
	{
	    if ((photTF / 3) > blasTF)
	    {
		photScore += 4;
	    }
	    else
	    {
		photScore += 3;
	    }
	}
	else
	{
	    photScore += 2;
	}
    }
    else
    {
	if ((blasTF / 2) > photTF)
	{
	    if ((blasTF / 3) > photTF)
	    {
		photScore += 8;
	    }
	    else
	    {
		photScore += 4;
	    }
	}
	else
	{
	    photScore += 2;
	}
    }
    /* Ok - Pick weapon based on score */
    if (blasScore > photScore)
    {
	return(bp_blaser);
    }
    /* Note that if the two weapons have an equal score we are still */
    /* picking photon torpedos */
    return(bp_photon);
}

/*
 * safeBlasFire - returns the amount of energy that is safe to be fired by
 *          the ship's blasers
 */

USHORT safeBlasFire(IMP, register Ship_t *fShip)
{
    register USHORT blasInst;

    /* get the ship's weapon TF & Eff */
    /* The weapon's TF & EFF will determine how many units they can fire */
    /* safely */
    blasInst = numInst(IS, fShip, bp_blaser);
    if (blasInst == 0)
    {
        return(0);
    }
    /* Get the maximum amount of energy they can discharge in one blast */
    /* SAFELY */
    return(((((blasInst * 40) * getShipEff(IS, fShip, bp_blaser)) / 100) *
        getShipTechFactor(IS, fShip, bp_blaser)) / 100);
}

/*
 * pickAmtToFire - returns the amount that should be fired from a given ship
 *              Note: Reads big items, so the ship must NOT be in the
 *                    buffer.
 */

USHORT pickAmtToFire(IMP, register Ship_t *fShip, BigPart_t weapType,
    BOOL wasAttacked)
{
    USHORT amtToFire = 0;
    USHORT ceilAmt;

    switch(weapType)
    {
        case bp_blaser:
            amtToFire = safeBlasFire(IS, fShip);
            if (wasAttacked)
            {
                /* If the ship was attacked, use up to 45% of the ship's */
                /* energy to fire back */
                ceilAmt = ((fShip->sh_energy * 45) / 100);
            }
            else
            {
                /* If the ship is just defending another, only use up to */
                /* 20% of it's energy */
                ceilAmt = ((fShip->sh_energy * 20) / 100);
            }
            /* Make sure we only fire the above amount or less */
            if (ceilAmt < amtToFire)
            {
                amtToFire = ceilAmt;
            }
            break;
	case bp_photon:
	    amtToFire = ammoAvail(IS, fShip, bp_photon);
	    if (!wasAttacked && (amtToFire > 0))
	    {
		amtToFire = (amtToFire / 3);
		if (amtToFire < 1)
		{
		    amtToFire = 1;
		}
	    }
	    break;
        default:
            break;
    }
    return(amtToFire);
}

BOOL doDefendOwner(IMP, UBYTE defender, UBYTE defendRace, UBYTE owner);

/*
 * ===================== Planet weapons =====================
 *
 * Planets can have blasers and photon torpedo tubes installed (see the
 * install command). They work like a ship's weapons, except:
 *  - aiming uses the planet's own tech level and efficiency (planets have
 *    no computers)
 *  - blasers are powered by ore, one ore per unit of energy
 *  - launching torpedoes costs planet mobility instead of ship energy
 *  - the planet needs the same military/officers a ship would to fire
 */

/*
 * plHasCrew - returns TRUE if the planet has enough people to work the
 *          given kind of weapon (same rule as for ships)
 */

static BOOL plHasCrew(IMP, Planet_t *pl, USHORT crewType)
{
    USHORT mil, ofc;

    mil = pl->pl_quantity[it_military];
    ofc = pl->pl_quantity[it_officers];
    if (((mil <= IS->is_world.w_crewReq[crewType].cr_mil) &&
        (ofc <= IS->is_world.w_crewReq[crewType].cr_mil)) ||
        (ofc <= IS->is_world.w_crewReq[crewType].cr_ofc))
    {
        return FALSE;
    }
    return TRUE;
}

/*
 * getPlWeapRange - returns the range of the planet's weapons, the same
 *          way getShipWeapRange() does for ships.
 * Assumes: The planet is NOT in the buffer.
 */

ULONG getPlWeapRange(IMP, register Planet_t *pl, BigPart_t weapType)
{
    ULONG weapRange = 0;

    if (numInstPl(IS, pl, weapType) > 0)
    {
        switch(weapType)
        {
            case bp_photon:
                weapRange = (((MAX_PHO_RNG * getPlTechFactor(IS, pl,
                    bp_photon)) / 100) * getPlEff(IS, pl, bp_photon)) / 100;
                break;
            case bp_blaser:
                weapRange = (((IS->is_world.w_phaserRange *
                    getPlTechFactor(IS, pl, bp_blaser)) / 100) *
                    getPlEff(IS, pl, bp_blaser)) / 100;
                break;
            default:
                break;
        }
    }
    return(weapRange);
}

/*
 * safeBlasFirePl - returns the amount of energy the planet's blasers can
 *          safely discharge at once
 */

USHORT safeBlasFirePl(IMP, register Planet_t *pl)
{
    register USHORT blasInst;

    blasInst = numInstPl(IS, pl, bp_blaser);
    if (blasInst == 0)
    {
        return(0);
    }
    return(((((blasInst * 40) * getPlEff(IS, pl, bp_blaser)) / 100) *
        getPlTechFactor(IS, pl, bp_blaser)) / 100);
}

/*
 * plFireBlaser() - fire a planet's blasers at a ship
 *              Assumes:
 *                  The planet DOING the firing is NOT in the buffer,
 *                      but IS locked (and thus must be unlocked as
 *                      needed).
 *                  The caller will flush the final output (if any).
 *              Returns:
 *                  TRUE if the player should be charged BTUs.
 */

BOOL plFireBlaser(IMP, Planet_t *aPl, register USHORT units, ULONG target,
    BOOL targetIsShip, BOOL playIsFire)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    UBYTE aWeapTf, aWeapEff, aAimEff, aAimTf, dmgPct, oddsMod,
        TargOwn, origTargOwn, aPlOwn;
    USHORT blasInst, maxUnits, tmpUnits, damage, tmpOdds;
    ShipDamRes_t damRes;
    Ship_t saveShip;

    if (!targetIsShip)
    {
        if (playIsFire)
        {
            user(IS, "Blasers have no effect on planets!\n");
        }
        return(FALSE);
    }
    rsh = &IS->is_request.rq_u.ru_ship;
    rpl = &IS->is_request.rq_u.ru_planet;
    aPlOwn = aPl->pl_owner;
    blasInst = numInstPl(IS, aPl, bp_blaser);
    if (blasInst == 0)
    {
        if (playIsFire)
        {
            user(IS, "That planet does not have any blasers installed!\n");
        }
        return(FALSE);
    }
    if (!plHasCrew(IS, aPl, CR_PHA))
    {
        if (playIsFire)
        {
            user(IS, "That planet does not have enough military/officers "
                "to fire blasers!\n");
        }
        return(FALSE);
    }
    aWeapTf = getPlTechFactor(IS, aPl, bp_blaser);
    aWeapEff = getPlEff(IS, aPl, bp_blaser);
    /* the planet's own tech and efficiency take the place of a computer */
    aAimTf = getTechFactor(aPl->pl_techLevel);
    aAimEff = aPl->pl_efficiency;
    if ((aWeapTf == 0) || (aWeapEff == 0) || (aAimTf == 0) || (aAimEff == 0))
    {
        if (playIsFire)
        {
            user(IS, "That planet is incapable of firing that type of "
                "weapon!\n");
        }
        return(FALSE);
    }
    server(IS, rt_readShip, target);
    saveShip = *rsh;
    if (getPlWeapRange(IS, aPl, bp_blaser) < findDistance(IS, aPl->pl_row,
        aPl->pl_col, saveShip.sh_row, saveShip.sh_col))
    {
        if (playIsFire)
        {
            userSh(IS, "Ship #", &saveShip, " is out of range of the "
                "planet's blasers.\n");
        }
        return(TRUE);
    }
    oddsMod = 0;
    if (saveShip.sh_planet != NO_ITEM)
    {
        server(IS, rt_readPlanet, saveShip.sh_planet);
        if (rpl->pl_gas > 75)
        {
            oddsMod = (rpl->pl_gas - 75) * 2;
        }
    }
    /* blasers on planets are powered by ore */
    if (units > aPl->pl_quantity[it_ore])
    {
        units = aPl->pl_quantity[it_ore];
        if (units == 0)
        {
            if (playIsFire)
            {
                user(IS, "There is no ore on the planet to power the "
                    "blasers!\n");
            }
            return(FALSE);
        }
        if (playIsFire)
        {
            userN3(IS, "Reducing units to fire down to ", units, "...\n");
        }
    }
    maxUnits = (((((blasInst * 40) * aWeapEff) / 100) * aWeapTf) / 100);
    if (!playIsFire)
    {
        units = umin(units, maxUnits);
    }
    dmgPct = 0;
    if (units > maxUnits)
    {
        tmpUnits = maxUnits / 10;
        if (units < (maxUnits + tmpUnits))
        {
            dmgPct = 5;
        }
        else if (units < (maxUnits + (tmpUnits * 2)))
        {
            dmgPct = 10;
        }
        else if (units < (maxUnits + (tmpUnits * 3)))
        {
            dmgPct = 25;
        }
        else if (units < (maxUnits + (tmpUnits * 4)))
        {
            dmgPct = 50;
        }
        else
        {
            userN3(IS, "Requested energy output is too high - reducing to ",
                maxUnits, " units\n");
            units = maxUnits;
        }
    }
    if (units == 0)
    {
        if (playIsFire)
        {
            user(IS, "The planet's blasers are not able to fire!\n");
        }
        return(FALSE);
    }
    /* use up the ore */
    aPl->pl_quantity[it_ore] -= units;
    *rpl = *aPl;
    server(IS, rt_unlockPlanet, aPl->pl_number);
    if (!playIsFire)
    {
        user2(IS, "Your ", getPlanetName(aPl->pl_class));
        userN2(IS, " planet #", aPl->pl_number);
        userN3(IS, " is firing its blasers on ship #", target, "!\n");
        notify(IS, aPlOwn);
    }
    else
    {
        uFlush(IS);
    }
    server(IS, rt_lockShip, target);
    origTargOwn = rsh->sh_owner;
    if (rsh->sh_owner == NO_OWNER)
    {
        server(IS, rt_unlockShip, target);
        user(IS, "Something else has destroyed the target ship!\n");
    }
    else if (rsh->sh_owner == aPlOwn)
    {
        server(IS, rt_unlockShip, target);
        user(IS, "Your men refuse to fire upon one of their own ships "
            "and discharge the blast into open space!\n");
    }
    else
    {
        /* Make sure they have at least a 65% chance of hitting */
        tmpOdds = (aAimTf < 65) ? (65 - aAimTf) : 0;
        if ((impRandom(IS, 1000) + (oddsMod * 10)) <
            (((aAimTf + tmpOdds) * aAimEff) / 10))
        {
            user2(IS, "Your ", getShipName(rsh->sh_type));
            userN2(IS, " #", target);
            userN3(IS, " was fired upon by blasers from planet #",
                aPl->pl_number, "!\n");
            damRes = damageShip(IS, target, units, TRUE);
            server(IS, rt_unlockShip, target);
            TargOwn = rsh->sh_owner;
            if (TargOwn == NO_OWNER)
            {
                user(IS, "The ship was destroyed!\n");
            }
            else
            {
                if (damRes.sdr_shields != 0)
                {
                    userN3(IS, "Your shields absorbed ", damRes.sdr_shields,
                        " units.\n");
                }
                if (damRes.sdr_armour != 0)
                {
                    userN3(IS, "Your armour absorbed ", damRes.sdr_armour,
                        " units.\n");
                }
                if (damRes.sdr_main != 0)
                {
                    userN3(IS, "Various parts of the ship took ",
                        damRes.sdr_main, " units of damage.\n");
                }
            }
            if (playIsFire)
            {
                notify(IS, origTargOwn);
            }
            else
            {
                uFlush(IS);
            }
            news(IS, n_blase_ship, aPlOwn, origTargOwn);
            user(IS, "Target hit!\n");
            if (TargOwn == NO_OWNER)
            {
                user(IS, "The attack destroyed the ship!\n");
            }
            else if (damRes.sdr_main != 0)
            {
                userN3(IS, "The ship appears to have suffered ",
                    damRes.sdr_main, " units of structural damage.\n");
            }
        }
        else
        {
            server(IS, rt_unlockShip, target);
            user(IS, "The blast missed!\n");
        }
    }
    /* Relock the firing planet */
    server(IS, rt_lockPlanet, aPl->pl_number);
    if (rpl->pl_owner != aPlOwn)
    {
        userN3(IS, "Planet #", rpl->pl_number, " is no longer yours!\n");
    }
    else if ((dmgPct > 0) && (impRandom(IS, 10000) < dmgPct * 100))
    {
        /* overloading the blasers damages the planet */
        damage = umin((USHORT) 100, IS->is_world.w_phaserDmg *
            (units - maxUnits));
        userN2(IS, "Planet #", rpl->pl_number);
        userN3(IS, " took ", damage, " units of damage from overloaded "
            "blasers!\n");
        damagePlanet(IS, damage, aPlOwn);
    }
    *aPl = *rpl;
    return(TRUE);
}

/*
 * plFirePhoton() - fire photon torpedoes from a planet at a ship or planet
 *              Assumes:
 *                  The planet DOING the firing is NOT in the buffer,
 *                      but IS locked (and thus must be unlocked as
 *                      needed).
 *                  The caller will flush the final output (if any).
 *              Returns:
 *                  TRUE if the player should be charged BTUs.
 */

BOOL plFirePhoton(IMP, Planet_t *aPl, register USHORT units, ULONG target,
    BOOL targetIsShip, BOOL playIsFire)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    UBYTE aWeapTf, aWeapEff, aAimTf, aAimEff, oddsMod, TargOwn,
        origTargOwn, aPlOwn;
    USHORT photInst, rangeMod, tRow, tCol, damage;
    ULONG weapRange, targRange;
    ShipDamRes_t damRes;
    Ship_t saveShip;
    Planet_t savePlanet;

    rsh = &IS->is_request.rq_u.ru_ship;
    rpl = &IS->is_request.rq_u.ru_planet;
    aPlOwn = aPl->pl_owner;
    if (aPl->pl_mobility < IS->is_world.w_torpMobCost)
    {
        if (playIsFire)
        {
            user(IS, "The planet does not have enough mobility to launch "
                "photon torpedos!\n");
        }
        return(FALSE);
    }
    photInst = numInstPl(IS, aPl, bp_photon);
    if (photInst == 0)
    {
        if (playIsFire)
        {
            user(IS, "That planet does not have any photon torpedo tubes "
                "installed!\n");
        }
        return(FALSE);
    }
    if (!plHasCrew(IS, aPl, CR_PHO))
    {
        if (playIsFire)
        {
            user(IS, "That planet does not have enough military/officers "
                "to fire photon torpedos!\n");
        }
        return(FALSE);
    }
    aWeapTf = getPlTechFactor(IS, aPl, bp_photon);
    aWeapEff = getPlEff(IS, aPl, bp_photon);
    aAimTf = getTechFactor(aPl->pl_techLevel);
    aAimEff = aPl->pl_efficiency;
    if ((aWeapTf == 0) || (aWeapEff == 0) || (aAimTf == 0) || (aAimEff == 0))
    {
        if (playIsFire)
        {
            user(IS, "That planet is incapable of firing that type of "
                "weapon!\n");
        }
        return(FALSE);
    }
    weapRange = getPlWeapRange(IS, aPl, bp_photon);
    oddsMod = 0;
    if (targetIsShip)
    {
        server(IS, rt_readShip, target);
        saveShip = *rsh;
        tRow = saveShip.sh_row;
        tCol = saveShip.sh_col;
    }
    else
    {
        server(IS, rt_readPlanet, target);
        savePlanet = *rpl;
        tRow = savePlanet.pl_row;
        tCol = savePlanet.pl_col;
    }
    targRange = findDistance(IS, aPl->pl_row, aPl->pl_col, tRow, tCol);
    if (weapRange < targRange)
    {
        if (playIsFire)
        {
            if (targetIsShip)
            {
                userSh(IS, "Ship #", &saveShip, " is out of range of the "
                    "planet's photon torpedos.\n");
            }
            else
            {
                userP(IS, "Planet #", &savePlanet, " is out of range of the "
                    "planet's photon torpedos.\n");
            }
        }
        return(TRUE);
    }
    /* same accuracy falloff with range as ships */
    rangeMod = IS->is_world.w_torpAcc;
    if ((targRange * 10) > ((weapRange * 10) / 3))
    {
        if ((targRange * 10) <= ((weapRange * 10) / 2))
        {
            rangeMod = ((rangeMod * 90) / 100);
        }
        else if ((targRange * 10) <= ((weapRange * 75) / 10))
        {
            rangeMod = ((rangeMod * 75) / 100);
        }
        else
        {
            rangeMod = ((rangeMod * 60) / 100);
        }
    }
    if (targetIsShip && (saveShip.sh_planet != NO_ITEM))
    {
        server(IS, rt_readPlanet, saveShip.sh_planet);
        if (rpl->pl_gas > 75)
        {
            oddsMod = (rpl->pl_gas - 75) * 2;
        }
    }
    if (units > photInst)
    {
        units = photInst;
    }
    if ((units * IS->is_world.w_torpCost) > aPl->pl_quantity[it_missiles])
    {
        units = aPl->pl_quantity[it_missiles] / IS->is_world.w_torpCost;
        if (units == 0)
        {
            if (playIsFire)
            {
                user(IS, "Not enough missiles on the planet to fire!\n");
            }
            return(FALSE);
        }
        if (playIsFire)
        {
            userN3(IS, "Reducing torpedoes to fire down to ", units, "...\n");
        }
    }
    aPl->pl_quantity[it_missiles] -= (units * IS->is_world.w_torpCost);
    aPl->pl_mobility -= IS->is_world.w_torpMobCost;
    *rpl = *aPl;
    server(IS, rt_unlockPlanet, aPl->pl_number);
    if (!playIsFire)
    {
        user2(IS, "Your ", getPlanetName(aPl->pl_class));
        userN2(IS, " planet #", aPl->pl_number);
        if (targetIsShip)
        {
            userN3(IS, " is launching torpedoes at ship #", target, "!\n");
        }
        else
        {
            userN3(IS, " is launching torpedoes at planet #", target, "!\n");
        }
        notify(IS, aPlOwn);
    }
    else
    {
        uFlush(IS);
    }
    if (targetIsShip)
    {
        server(IS, rt_lockShip, target);
        origTargOwn = rsh->sh_owner;
    }
    else
    {
        server(IS, rt_lockPlanet, target);
        origTargOwn = rpl->pl_owner;
    }
    if ((origTargOwn == NO_OWNER) || (origTargOwn == aPlOwn))
    {
        if (targetIsShip)
        {
            server(IS, rt_unlockShip, target);
        }
        else
        {
            server(IS, rt_unlockPlanet, target);
        }
        if (origTargOwn == NO_OWNER)
        {
            user(IS, "Something else has already wiped out the target!\n");
        }
        else
        {
            user(IS, "Your men refuse to fire upon their own side!\n");
        }
    }
    else if ((impRandom(IS, 1000) + (oddsMod * 10)) <
        ((((aWeapTf * aWeapEff) / 10) * rangeMod) / 100))
    {
        damage = units * (IS->is_world.w_torpBase +
            (impRandom(IS, IS->is_world.w_torpRand * 10) / 10));
        if (targetIsShip)
        {
            user2(IS, "Your ", getShipName(rsh->sh_type));
            userN2(IS, " #", target);
            userN3(IS, " was hit by photon torpedoes from planet #",
                aPl->pl_number, "!\n");
            damRes = damageShip(IS, target, damage, TRUE);
            server(IS, rt_unlockShip, target);
            TargOwn = rsh->sh_owner;
            if (TargOwn == NO_OWNER)
            {
                user(IS, "The ship was destroyed!\n");
            }
            else
            {
                if (damRes.sdr_shields != 0)
                {
                    userN3(IS, "Your shields absorbed ", damRes.sdr_shields,
                        " units.\n");
                }
                if (damRes.sdr_armour != 0)
                {
                    userN3(IS, "Your armour absorbed ", damRes.sdr_armour,
                        " units.\n");
                }
                if (damRes.sdr_main != 0)
                {
                    userN3(IS, "Various parts of the ship took ",
                        damRes.sdr_main, " units of damage.\n");
                }
            }
            if (playIsFire)
            {
                notify(IS, origTargOwn);
            }
            else
            {
                uFlush(IS);
            }
            news(IS, n_torp_ship, aPlOwn, origTargOwn);
            user(IS, "Target hit!\n");
            if (TargOwn == NO_OWNER)
            {
                news(IS, n_torp_dest, aPlOwn, origTargOwn);
                user(IS, "The attack destroyed the ship!\n");
            }
        }
        else
        {
            user2(IS, "Your ", getPlanetName(rpl->pl_class));
            userN2(IS, " #", target);
            userN3(IS, " was hit by photon torpedoes from planet #",
                aPl->pl_number, "!\n");
            damagePlanet(IS, damage, aPlOwn);
            server(IS, rt_unlockPlanet, target);
            TargOwn = rpl->pl_owner;
            damagePlanetItems(IS, target, damage);
            if (TargOwn == NO_OWNER)
            {
                user(IS, "The planet was wiped out!\n");
            }
            if (playIsFire)
            {
                notify(IS, origTargOwn);
            }
            else
            {
                uFlush(IS);
            }
            news(IS, n_bomb_planet, aPlOwn, origTargOwn);
            user(IS, "Target hit!\n");
            if (TargOwn == NO_OWNER)
            {
                news(IS, n_bomb_dest, aPlOwn, origTargOwn);
                user(IS, "The attack wiped out the planet!\n");
            }
        }
    }
    else
    {
        if (targetIsShip)
        {
            server(IS, rt_unlockShip, target);
        }
        else
        {
            server(IS, rt_unlockPlanet, target);
        }
        user(IS, "The torpedoes missed!\n");
    }
    /* Relock the firing planet */
    server(IS, rt_lockPlanet, aPl->pl_number);
    if (rpl->pl_owner != aPlOwn)
    {
        userN3(IS, "Planet #", rpl->pl_number, " is no longer yours!\n");
    }
    *aPl = *rpl;
    return(TRUE);
}

/*
 * fireWeapPl() - fire one of a planet's weapons
 *              Assumes:
 *                  The planet DOING the firing IS in the buffer
 *                      and IS locked.
 *              Returns:
 *                  TRUE if the player should be charged BTUs.
 */

BOOL fireWeapPl(IMP, BigPart_t weapType, register USHORT units,
    ULONG target, BOOL targetIsShip, BOOL playIsFire)
{
    Planet_t *rpl;
    BOOL saveNoWrite, retCode;
    Planet_t savePl;

    if ((units == 0) || (target == NO_ITEM))
    {
        return(FALSE);
    }
    rpl = &IS->is_request.rq_u.ru_planet;
    if (rpl->pl_owner == NO_OWNER)
    {
        return FALSE;
    }
    retCode = FALSE;
    saveNoWrite = IS->is_noWrite;
    IS->is_noWrite = TRUE;
    savePl = *rpl;
    switch (weapType)
    {
        case bp_blaser:
            retCode = plFireBlaser(IS, &savePl, units, target, targetIsShip,
                playIsFire);
            break;
        case bp_photon:
            retCode = plFirePhoton(IS, &savePl, units, target, targetIsShip,
                playIsFire);
            break;
        default:
            userN3(IS, ">>> Error - alert the system owner right away that "
                "fireWeapPl was called with invalid code ", weapType, " !\n");
            break;
    }
    *rpl = savePl;
    IS->is_noWrite = saveNoWrite;
    return(retCode);
}

/*
 * pickBestWeapPl - pick which of a planet's weapons to fire at a target.
 *          Like pickBestWeap(): returns bp_blaser if nothing better is
 *          usable, so callers MUST check numInstPl() afterwards.
 *          Blasers can't hurt planets, so planets are always torpedoed.
 */

BigPart_t pickBestWeapPl(IMP, register Planet_t *pl, BOOL isShip,
    USHORT tRow, USHORT tCol)
{
    USHORT photAmmo, blasAmmo;
    UBYTE photVal, blasVal;
    ULONG range;

    if (!isShip)
    {
        return(bp_photon);
    }
    range = findDistance(IS, tRow, tCol, pl->pl_row, pl->pl_col);
    if ((range >= MAX_PHO_RNG) || (numInstPl(IS, pl, bp_photon) == 0))
    {
        return(bp_blaser);
    }
    if (numInstPl(IS, pl, bp_blaser) == 0)
    {
        return(bp_photon);
    }
    photAmmo = ammoAvailPl(IS, pl, bp_photon);
    if ((photAmmo == 0) || (pl->pl_mobility < IS->is_world.w_torpMobCost))
    {
        return(bp_blaser);
    }
    blasAmmo = ammoAvailPl(IS, pl, bp_blaser);
    if (blasAmmo == 0)
    {
        return(bp_photon);
    }
    /* both are usable - pick the one in better shape */
    photVal = (getPlTechFactor(IS, pl, bp_photon) *
        getPlEff(IS, pl, bp_photon)) / 100;
    blasVal = (getPlTechFactor(IS, pl, bp_blaser) *
        getPlEff(IS, pl, bp_blaser)) / 100;
    if (blasVal > photVal)
    {
        return(bp_blaser);
    }
    return(bp_photon);
}

/*
 * pickAmtToFirePl - how much a planet fires when defending. The planet
 *          that was attacked fires harder than ones helping out.
 */

USHORT pickAmtToFirePl(IMP, register Planet_t *pl, BigPart_t weapType,
    BOOL wasAttacked)
{
    USHORT amtToFire = 0;
    USHORT ceilAmt;

    switch(weapType)
    {
        case bp_blaser:
            amtToFire = safeBlasFirePl(IS, pl);
            /* use up to 45% (attacked) or 20% (helping) of the ore */
            ceilAmt = (pl->pl_quantity[it_ore] * (wasAttacked ? 45 : 20)) /
                100;
            if (ceilAmt < amtToFire)
            {
                amtToFire = ceilAmt;
            }
            break;
        case bp_photon:
            amtToFire = ammoAvailPl(IS, pl, bp_photon);
            if (!wasAttacked && (amtToFire > 0))
            {
                amtToFire = (amtToFire / 3 < 1) ? 1 : amtToFire / 3;
            }
            break;
        default:
            break;
    }
    return(amtToFire);
}

/*
 * doDefend - returns TRUE if the given ship should be used for
 *            defending other ships
 * Assumes: Ship is NOT in the buffer and the buffer does NOT
 *          have to be preserved.
 */

BOOL doDefend(IMP, UBYTE defender, UBYTE defendRace, register Ship_t *sh)
{
    return doDefendOwner(IS, defender, defendRace, sh->sh_owner);
}

/*
 * doDefendOwner - returns TRUE if things (ships or planets) owned by
 *            "owner" should help defend the player "defender": they are
 *            the defender's own, or belong to an ally of the defender
 *            (but never to the player doing the attacking).
 */

BOOL doDefendOwner(IMP, UBYTE defender, UBYTE defendRace, UBYTE owner)
{
    if (owner == defender)
    {
	return(TRUE);
    }
    if ((owner != NO_OWNER) && (owner != IS->is_player.p_number))
    {
	server(IS, rt_readPlayer, owner);
	if (relationTo(defender, defendRace,
	    &IS->is_request.rq_u.ru_player) == r_allied)
	{
	    return(TRUE);
	}
    }
    return(FALSE);
}

/*
 * attackerGone - returns TRUE if the ship or planet that started a fight has
 *            been destroyed (or lost) by defensive fire
 */

static BOOL attackerGone(IMP, ULONG aNum, BOOL aIsShip)
{
    if (aIsShip)
    {
        server(IS, rt_readShip, aNum);
        return (IS->is_request.rq_u.ru_ship.sh_owner != IS->is_player.p_number);
    }
    server(IS, rt_readPlanet, aNum);
    return (IS->is_request.rq_u.ru_planet.pl_owner != IS->is_player.p_number);
}

/*
 * markHostile - firing on someone ends an alliance (to neutral) or turns
 *            neutral/default relations into war
 */

static void markHostile(IMP, USHORT defender)
{
    register Player_t *rp;

    rp = &IS->is_request.rq_u.ru_player;
    server(IS, rt_readPlayer, defender);
    if (rp->p_playrel[IS->is_player.p_number] == r_allied)
    {
        server(IS, rt_lockPlayer, defender);
        rp->p_playrel[IS->is_player.p_number] = r_neutral;
        server(IS, rt_unlockPlayer, defender);
        news(IS, n_disavow_ally, defender, IS->is_player.p_number);
        news(IS, n_decl_neut, defender, IS->is_player.p_number);
    }
    else if ((rp->p_playrel[IS->is_player.p_number] == r_neutral) ||
        (rp->p_playrel[IS->is_player.p_number] == r_default))
    {
        server(IS, rt_lockPlayer, defender);
        rp->p_playrel[IS->is_player.p_number] = r_war;
        server(IS, rt_unlockPlayer, defender);
        news(IS, n_decl_war, defender, IS->is_player.p_number);
    }
}

/*
 * shipDefendShot - let one ship fire back at the attacker, if it is willing
 *            and able. Returns TRUE if the attacker was destroyed.
 */

static BOOL shipDefendShot(IMP, USHORT defender, UBYTE defendRace,
    ULONG dNum, BOOL wasAttacked, ULONG aNum, BOOL aIsShip, USHORT aRow,
    USHORT aCol)
{
    register Ship_t *rsh;
    Ship_t saveShip;
    BigPart_t dWeap;
    USHORT dGuns, dShells;

    rsh = &IS->is_request.rq_u.ru_ship;
    server(IS, rt_readShip, dNum);
    saveShip = *rsh;
    if (!doDefend(IS, defender, defendRace, &saveShip))
    {
        return FALSE;
    }
    /* blasers can't hurt planets, so a planet attacker gets torpedoes */
    dWeap = aIsShip ? pickBestWeap(IS, &saveShip, aNum, TRUE, aRow, aCol) :
        bp_photon;
    if (getShipWeapRange(IS, &saveShip, dWeap) < findDistance(IS, aRow, aCol,
        saveShip.sh_row, saveShip.sh_col))
    {
        return FALSE;
    }
    accessShip(IS, dNum);
    saveShip = *rsh;
    /* We have to do this again due to updating the ship */
    if (!doDefend(IS, defender, defendRace, &saveShip) ||
        !hasWeap(IS, &saveShip))
    {
        return FALSE;
    }
    if (aIsShip)
    {
        dWeap = pickBestWeap(IS, &saveShip, aNum, TRUE, aRow, aCol);
    }
    dShells = ammoAvail(IS, &saveShip, dWeap);
    dGuns = numInst(IS, &saveShip, dWeap);
    if ((saveShip.sh_efficiency < EFFIC_WARN) || (dGuns == 0) ||
        (dShells == 0))
    {
        return FALSE;
    }
    dGuns = pickAmtToFire(IS, &saveShip, dWeap, wasAttacked);
    server(IS, rt_lockShip, dNum);
    IS->is_noWrite = TRUE;
    (void) fireWeap(IS, dWeap, dGuns, aNum, aIsShip, FALSE);
    server(IS, rt_unlockShip, dNum);
    notify(IS, defender);
    IS->is_noWrite = FALSE;
    feShDirty(IS, dNum);
    if (defender == saveShip.sh_owner)
    {
        news(IS, n_fire_back, defender, IS->is_player.p_number);
    }
    return attackerGone(IS, aNum, aIsShip);
}

/*
 * planetDefendShot - let one armed planet fire at the attacker, if it is
 *            willing and able. Returns TRUE if the attacker was destroyed.
 */

static BOOL planetDefendShot(IMP, USHORT defender, UBYTE defendRace,
    ULONG plNum, BOOL wasAttacked, ULONG aNum, BOOL aIsShip, USHORT aRow,
    USHORT aCol)
{
    register Planet_t *rpl;
    Planet_t savePl;
    BigPart_t dWeap;
    USHORT dGuns;

    rpl = &IS->is_request.rq_u.ru_planet;
    server(IS, rt_readPlanet, plNum);
    /* quick checks first - most planets have no weapons */
    if ((rpl->pl_weapon[0] == NO_ITEM) || (rpl->pl_owner == NO_OWNER) ||
        (rpl->pl_efficiency < EFFIC_WARN))
    {
        return FALSE;
    }
    savePl = *rpl;
    if (!doDefendOwner(IS, defender, defendRace, savePl.pl_owner))
    {
        return FALSE;
    }
    refreshPlWeapons(IS, &savePl);
    if (!hasWeapPl(IS, &savePl))
    {
        return FALSE;
    }
    dWeap = pickBestWeapPl(IS, &savePl, aIsShip, aRow, aCol);
    if ((numInstPl(IS, &savePl, dWeap) == 0) ||
        (getPlWeapRange(IS, &savePl, dWeap) < findDistance(IS, aRow, aCol,
        savePl.pl_row, savePl.pl_col)))
    {
        return FALSE;
    }
    accessPlanet(IS, plNum);
    savePl = *rpl;
    if (!doDefendOwner(IS, defender, defendRace, savePl.pl_owner) ||
        (savePl.pl_efficiency < EFFIC_WARN))
    {
        return FALSE;
    }
    if ((ammoAvailPl(IS, &savePl, dWeap) == 0) ||
        ((dGuns = pickAmtToFirePl(IS, &savePl, dWeap, wasAttacked)) == 0))
    {
        return FALSE;
    }
    server(IS, rt_lockPlanet, plNum);
    IS->is_noWrite = TRUE;
    (void) fireWeapPl(IS, dWeap, dGuns, aNum, aIsShip, FALSE);
    server(IS, rt_unlockPlanet, plNum);
    notify(IS, defender);
    IS->is_noWrite = FALSE;
    fePlDirty(IS, plNum);
    if (defender == savePl.pl_owner)
    {
        news(IS, n_fire_back, defender, IS->is_player.p_number);
    }
    return attackerGone(IS, aNum, aIsShip);
}

/*
 * defensiveFire - after the player fires on something, the defender gets
 *            to fire back:
 *              1) an attacked planet fires back with full strength
 *              2) an attacked ship fires back with full strength, and ships
 *                 set to DEFEND help with part of their weapons
 *              3) armed planets help with part of their weapons
 *            Only things owned by the defender or their allies, and in
 *            range of the attacker, take part. Stops if the attacker is
 *            destroyed.
 */

static void defensiveFire(IMP, USHORT defender, ULONG odNum, BOOL dIsShip,
    ULONG aNum, BOOL aIsShip, USHORT aRow, USHORT aCol)
{
    register Ship_t *rsh;
    ULONG dNum;
    UBYTE defendRace;

    rsh = &IS->is_request.rq_u.ru_ship;
    server(IS, rt_readPlayer, defender);
    defendRace = IS->is_request.rq_u.ru_player.p_race;
    user(IS, "Checking for defensive fire...\n");
    /* the planet that was fired on shoots back first */
    if (!dIsShip && planetDefendShot(IS, defender, defendRace, odNum, TRUE,
        aNum, aIsShip, aRow, aCol))
    {
        return;
    }
    for (dNum = 0; dNum < IS->is_world.w_shipNext; dNum++)
    {
        BOOL isTarget = (dIsShip && (dNum == odNum));

        server(IS, rt_readShip, dNum);
        /* See if they even WANT this ship to defend other ships. (This */
        /* used to compare against the target even when it was a planet, */
        /* so the ship with the same number as the planet fired at full */
        /* strength.) */
        if ((rsh->sh_flags & SHF_DEFEND) || isTarget)
        {
            if (shipDefendShot(IS, defender, defendRace, dNum, isTarget, aNum,
                aIsShip, aRow, aCol))
            {
                return;
            }
        }
    }
    for (dNum = 0; dNum < IS->is_world.w_planetNext; dNum++)
    {
        if (!dIsShip && (dNum == odNum))
        {
            /* already fired above */
            continue;
        }
        if (planetDefendShot(IS, defender, defendRace, dNum, FALSE, aNum,
            aIsShip, aRow, aCol))
        {
            return;
        }
    }
}

/*
 * cmd_fire - top-level command that allows player's to fire from
 *            ships or planets onto planets or ships
 */

BOOL cmd_fire(IMP)
{
    register Planet_t *rpl;
    register Ship_t *rsh;
    char buf[100];
    register USHORT aGuns, aShells;
    ULONG dNum, aNum, temp;
    USHORT aEffic, defender;
    USHORT aRow, aCol;
    BOOL dIsShip, aIsShip, fired;
    BigPart_t aWeap;
    Ship_t saveShip;
    Planet_t savePl;

    if (!reqPlanetOrShip(IS, &dNum, &dIsShip, "Planet or ship to fire on"))
    {
        return FALSE;
    }
    rpl = &IS->is_request.rq_u.ru_planet;
    rsh = &IS->is_request.rq_u.ru_ship;
    (void) skipBlanks(IS);
    if (!reqPlanetOrShip(IS, &aNum, &aIsShip,
        "Ship or planet (e.g. 12p) to fire from"))
    {
        return FALSE;
    }
    if (aIsShip)
    {
        server(IS, rt_readShip, aNum);
        if (rsh->sh_owner != IS->is_player.p_number)
        {
            err(IS, "you don't own that ship");
            return TRUE;
        }
        accessShip(IS, aNum);
        saveShip = *rsh;
        aEffic = saveShip.sh_efficiency;
        aRow = saveShip.sh_row;
        aCol = saveShip.sh_col;
        if (!hasWeap(IS, &saveShip))
        {
            feShDirty(IS, aNum);
            user(IS, "That ship has no weapons on board capable of "
                "firing\n");
            return TRUE;
        }
        aWeap = reqShipWeapType(IS, &saveShip);
        /* NOTE: This assumes bp_photon is the first weapon */
        if (aWeap < bp_photon)
        {
            /* reqShipWeapType() should have already printed msgs */
            feShDirty(IS, aNum);
            return TRUE;
        }
        aShells = ammoAvail(IS, &saveShip, aWeap);
        aGuns = numInst(IS, &saveShip, aWeap);
    }
    else
    {
        server(IS, rt_readPlanet, aNum);
        if (rpl->pl_owner != IS->is_player.p_number)
        {
            err(IS, "you don't own that planet");
            return TRUE;
        }
        accessPlanet(IS, aNum);
        savePl = *rpl;
        refreshPlWeapons(IS, &savePl);
        aEffic = savePl.pl_efficiency;
        aRow = savePl.pl_row;
        aCol = savePl.pl_col;
        if (!hasWeapPl(IS, &savePl))
        {
            fePlDirty(IS, aNum);
            user(IS, "That planet has no weapons installed that are capable "
                "of firing (see INSTALL)\n");
            return TRUE;
        }
        aWeap = reqPlanetWeapType(IS, &savePl);
        if (aWeap < bp_photon)
        {
            fePlDirty(IS, aNum);
            return TRUE;
        }
        aShells = ammoAvailPl(IS, &savePl, aWeap);
        aGuns = numInstPl(IS, &savePl, aWeap);
    }
    if (aGuns == 0)
    {
        user(IS, "No weapons of that type available\n");
        return TRUE;
    }
    if (aShells == 0)
    {
        if (aIsShip || (aWeap == bp_photon))
        {
            user(IS, "You don't have the needed components to fire that "
                "kind of weapon!\n");
        }
        else
        {
            user(IS, "Planet blasers are powered by ore, and there is none "
                "on that planet!\n");
        }
        return TRUE;
    }
    if (aEffic < EFFIC_WARN)
    {
        user(IS, aIsShip ? "That ship is not efficient enough to fire.\n" :
            "That planet is not efficient enough to fire.\n");
        return TRUE;
    }
    /* check the target */
    if (dIsShip)
    {
        server(IS, rt_readShip, dNum);
        defender = rsh->sh_owner;
        if (defender == NO_OWNER)
        {
            userN3(IS, "Ship #", dNum, " no longer exists!\n");
            return TRUE;
        }
        if (defender == IS->is_player.p_number)
        {
            user(IS, "Your men refuse to fire on one of your own ships\n");
            return TRUE;
        }
    }
    else
    {
        if (!aIsShip && (aNum == dNum))
        {
            user(IS, "A planet can't fire on itself!\n");
            return TRUE;
        }
        server(IS, rt_readPlanet, dNum);
        defender = rpl->pl_owner;
        if (defender == NO_OWNER)
        {
            userN3(IS, "Planet #", dNum, " is currently unowned\n");
            return TRUE;
        }
        if (defender == IS->is_player.p_number)
        {
            user(IS, "Your men refuse to fire on one of your own planets\n");
            return TRUE;
        }
    }
    /* how much to fire */
    (void) skipBlanks(IS);
    switch(aWeap)
    {
        case bp_blaser:
            aGuns = aShells;
            userN3(IS, "Fire how many units of energy (max ", aGuns, ")? ");
            break;
        case bp_photon:
            aGuns = umin(aGuns, aShells);
            userN3(IS, "Fire how many tubes (max ", aGuns, ")? ");
            break;
        default:
            aGuns = umin(aGuns, aShells);
            userN3(IS, "Fire how many guns (max ", aGuns, ")? ");
            break;
    }
    getPrompt(IS, &buf[0], IMP_BUFSIZE(buf));
    if (!reqPosRange(IS, &temp, aGuns, &buf[0]))
    {
        return TRUE;
    }
    aGuns = temp;
    /* bring the target up to date and make sure it is still a target */
    if (dIsShip)
    {
        accessShip(IS, dNum);
        defender = rsh->sh_owner;
        if (defender == NO_OWNER)
        {
            feShDirty(IS, dNum);
            user(IS, "An unknown force destroyed that ship for you!\n");
            return TRUE;
        }
    }
    else
    {
        accessPlanet(IS, dNum);
        defender = rpl->pl_owner;
        if (defender == NO_OWNER)
        {
            fePlDirty(IS, dNum);
            user(IS, "An unknown force wiped out the population of that "
                "planet for you!\n");
            return TRUE;
        }
    }
    if (defender == IS->is_player.p_number)
    {
        user(IS, dIsShip ? "You now own that ship!\n" :
            "You now own that planet!\n");
        return TRUE;
    }
    /* fire! */
    if (aIsShip)
    {
        server(IS, rt_lockShip, aNum);
        fired = fireWeap(IS, aWeap, aGuns, dNum, dIsShip, TRUE);
        server(IS, rt_unlockShip, aNum);
        feShDirty(IS, aNum);
    }
    else
    {
        server(IS, rt_lockPlanet, aNum);
        fired = fireWeapPl(IS, aWeap, aGuns, dNum, dIsShip, TRUE);
        server(IS, rt_unlockPlanet, aNum);
        fePlDirty(IS, aNum);
    }
    uFlush(IS);
    if (dIsShip)
    {
        feShDirty(IS, dNum);
    }
    else
    {
        fePlDirty(IS, dNum);
    }
    if (!fired)
    {
        return FALSE;
    }
    markHostile(IS, defender);
    defensiveFire(IS, defender, dNum, dIsShip, aNum, aIsShip, aRow, aCol);
    return TRUE;
}

/*
 * cmd_board - attempt to board (take over) one ship using another
 */

BOOL cmd_board(IMP)
{
    char buf[100];
    register Ship_t *rp;
    register Ship_t *rsh;
    register USHORT attackers, defenders;
    USHORT aCrew, dMCrew, dOCrew, victim, aRow, aCol;
    ULONG dShNum, aShNum, temp;
    long btus;
    ShipType_t dType;
    char ch;
    BOOL defended;

    if (!reqShip(IS, &dShNum, "Board which ship? "))
    {
        return FALSE;
    }
    (void) skipBlanks(IS);
    if (reqShip(IS, &aShNum, "Board from which ship? "))
    {
        server2(IS, rt_readShipPair, aShNum, dShNum);
        rp = &IS->is_request.rq_u.ru_shipPair[0];
        rsh = &IS->is_request.rq_u.ru_ship;
        aRow = rp[0].sh_row;
        aCol = rp[0].sh_col;
        if (rp[0].sh_owner != IS->is_player.p_number)
        {
            err(IS, "you don't own that ship");
        }
        else if ((rp[0].sh_items[it_military] == 0) &&
            (rp[0].sh_items[it_officers] == 0))
        {
            err(IS, "attacking ship has no crew on board");
        }
        else if (rp[0].sh_efficiency < EFFIC_WARN)
        {
            err(IS, "attacking ship isn't efficient enough");
        }
        else if (rp[1].sh_owner == NO_OWNER)
        {
            err(IS, "target ship is destroyed");
        }
        else if ((rp[1].sh_row != aRow) || (rp[1].sh_col != aCol))
        {
            err(IS, "the two ships must be in the same sector for boarding");
        }
        else if (rp[1].sh_owner == IS->is_player.p_number)
        {
            err(IS, "Your crew refuses to board one of your own ships");
        }
        else if (rp[1].sh_shields != 0)
	{
            err(IS, "destination ship has shields up");
	}
        else
        {
            accessShip(IS, aShNum);
            aCrew = rsh->sh_items[it_military];
            accessShip(IS, dShNum);
            victim = rsh->sh_owner;
            dType = rsh->sh_type;
            defended = FALSE;
            if (!defended)
            {
                (void) skipBlanks(IS);
                userN3(IS, "Board with how many military (max ", aCrew,
                    ")? ");
                getPrompt(IS, &buf[0], IMP_BUFSIZE(buf));
                if (!reqPosRange(IS, &temp, aCrew, &buf[0]))
                {
		    feShDirty(IS, aShNum);
		    feShDirty(IS, dShNum);
                    return TRUE;
                }
                if (temp == 0)
                {
		    feShDirty(IS, aShNum);
		    feShDirty(IS, dShNum);
                    return TRUE;
                }
                server(IS, rt_lockShip, aShNum);
                rsh->sh_items[it_military] -= temp;
                rsh->sh_cargo -= (temp * IS->is_world.w_weight[it_military]);
                server(IS, rt_unlockShip, aShNum);
                attackers = temp;
                server(IS, rt_lockShip, dShNum);
                dMCrew = rsh->sh_items[it_military];
                dOCrew = rsh->sh_items[it_officers];
                defenders = dMCrew + dOCrew;
                rsh->sh_items[it_military] = 0;
                rsh->sh_items[it_officers] = 0;
                rsh->sh_cargo -= ((dMCrew *
                    IS->is_world.w_weight[it_military]) + (dOCrew *
                    IS->is_world.w_weight[it_officers]));
                server(IS, rt_unlockShip, dShNum);
                btus = (long)(IS->is_player.p_btu * 100);
                while ((attackers != 0) && (defenders != 0) && (btus > 0))
                {
                    if (impRandom(IS, attackers +
                        defenders * IS->is_world.w_boardAdv / 100) >=
                            attackers)
                    {
                        /* an attacker has died */
                        ch = '@';
                        attackers--;
                        btus -= IS->is_world.w_deathFactor;
                    }
                    else
                    {
                        if (dOCrew > 0)
                        {
                            if (dMCrew > 0)
                            {
                                if (impRandom(IS, 100) >= 30)
                                {
                                    ch = '!';
                                    dMCrew--;
                                    defenders--;
                                }
                                else
                                {
                                    ch = '$';
                                    dOCrew--;
                                    defenders--;
                                }
                            }
                            else
                            {
                                ch = '$';
                                dOCrew--;
                                defenders--;
                            }
                        }
                        else
                        {
                            ch = '!';
                            dMCrew--;
                            defenders--;
                        }
                    }
                    impSleep(IS, impRandom(IS, 20));
                    userC(IS, ch);
                    uFlush(IS);
                }
                userNL(IS);
                if (defenders == 0)
                {
                    userN3(IS, "You have taken ship #",dShNum,"!\n");
                    server(IS, rt_lockShip, dShNum);
                    removeFromFleet(IS, victim, dShNum);
                    rsh->sh_owner = IS->is_player.p_number;
                    defenders = (IS->is_world.w_shipCargoLim[rsh->sh_type] -
                        rsh->sh_cargo);
                    if (attackers < defenders)
                    {
                        defenders = attackers;
                    }
                    attackers -= defenders;
                    rsh->sh_items[it_military] = defenders;
                    server(IS, rt_unlockShip, dShNum);
                    server(IS, rt_lockShip, aShNum);
                    rsh->sh_items[it_military] += attackers;
                    rsh->sh_cargo += (attackers *
                        IS->is_world.w_weight[it_military]);
                    server(IS, rt_unlockShip, aShNum);
                    news(IS, n_board_ship, IS->is_player.p_number, victim);
                    user3(IS, &IS->is_player.p_name[0], " boarded your ",
                          getShipName(dType));
                    userN3(IS, " #", dShNum, "!");
                    notify(IS, victim);
                }
                else
                {
                    server(IS, rt_lockShip, dShNum);
                    rsh->sh_items[it_military] = dMCrew;
                    rsh->sh_items[it_officers] = dOCrew;
                    rsh->sh_cargo += ((dMCrew *
                        IS->is_world.w_weight[it_military]) + (dOCrew *
                        IS->is_world.w_weight[it_officers]));
                    server(IS, rt_unlockShip, dShNum);
                    if (attackers == 0)
                    {
                        news(IS, n_failed_board, IS->is_player.p_number,
                            victim);
                        user(IS, "You were repelled!\n");
                    }
                    else
                    {
                        user(IS, "You ran out of BTUs - boarding aborted.\n");
                    }
                    user3(IS, &IS->is_player.p_name[0],
                          " attempted to board your ",
                          getShipName(dType));
                    userN3(IS, " #", dShNum, ".");
                    notify(IS, victim);
                }
                server(IS, rt_lockPlayer, IS->is_player.p_number);
                if (btus < 0)
                {
                    btus = 0;
                }
                IS->is_request.rq_u.ru_player.p_btu =
                    (USHORT)(btus / 100);
                server(IS, rt_unlockPlayer, IS->is_player.p_number);
                IS->is_player.p_btu =
                    IS->is_request.rq_u.ru_player.p_btu;
            }
	    feShDirty(IS, aShNum);
	    feShDirty(IS, dShNum);
        }
        return TRUE;
    }
    return FALSE;
}

#ifdef BUBBA
BOOL cmd_assault(IMP)
{
    char buf[100];
    register Planet_t *rpl;
    register Ship_t *rsh;
    USHORT crew, attackers, z;
    ULONG shipNumber, plNum;
    USHORT r, c, sRow, sCol;
    BOOL reduced;
    Ship_t saveSh;

    if (reqShip(IS, &shipNumber, "Assault from which ship"))
    {
        accessShip(IS, shipNumber);
        rpl = &IS->is_request.rq_u.ru_planet;
        rsh = &IS->is_request.rq_u.ru_ship;
        crew = readShipQuan(IS, rsh, it_military);
        sRow = rsh->sh_row;
        sCol = rsh->sh_col;
        saveSh = *rsh;
        if (rsh->sh_owner != IS->is_player.p_number)
        {
            err(IS, "you don't own that ship");
        }
        else
        {
            plNum = whichPlanet(IS, rsh->sh_row, rsh->sh_col);
            if (saveSh.sh_efficiency < EFFIC_WARN)
            {
                err(IS, "ship not efficient enough to assault");
            }
            else if (crew == 0)
            {
                err(IS, "no crew to assault with");
            }
            else if (plNum == NO_ITEM)
            {
                err(IS, "that ship isn't orbiting a planet");
            }
            else
            {
                accessPlanet(IS, plNum);
                if (rpl->pl_owner == IS->is_player.p_number)
                {
                   err("The ship's crew refuses to assault one of "
                       "your own planets!");
                }
                else
                {
                    (void) skipBlanks(IS);
                    userN3(IS, "Assault with how many marines (max ", crew, ")");
                    getPrompt(IS, &buf[0], IMP_BUFSIZE(buf));
                    if (reqPosRange(IS, &attackers, crew, &buf[0]) &&
                        (attackers != 0))
                    {
                        server(IS, rt_lockShip, shipNumber);
                        crew = readShipQuan(rsh, it_military);
                        reduced = FALSE;
                        if (attackers > crew)
                        {
                            reduced = TRUE;
                            attackers = crew;
                        }
                        writeShipQuan(IS, rsh, it_military, crew - attackers);
                        server(IS, rt_unlockShip, shipNumber);
                        if (reduced)
                        {
                            userN3(IS, "Actions reduced your force to ",
                                attackers, "!\n");
                        }
                        z = 0;
                        fight(IS, r, c, &attackers, 1, &z, 0, &z, 0, &z, 0);
                    }
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}
#endif

/*
 * cmd_configure - allows the player to set up the various flags in the
 *          ship that relate to combat
 */

BOOL cmd_configure(IMP)
{
    register Ship_t *rsh;
    USHORT what, option, subOpt, xferAmt, maxAmt;
    ULONG shNum;
    Ship_t saveShip;
    char buff[91];

    /* Get the number of the ship they want to work with */
    if (reqShip(IS, &shNum, "Enter the ship to configure"))
    {
        server(IS, rt_readShip, shNum);
        /* Set the pointer to the correct location */
        rsh = &IS->is_request.rq_u.ru_ship;
        if (rsh->sh_owner != IS->is_player.p_number)
        {
            user(IS, "You don't own that ship!\n");
            return(FALSE);
        }
        if (rsh->sh_type == st_m)
        {
            user(IS, "That ship is a miner!\n");
            return(TRUE);
        }
        (void) skipBlanks(IS);
        /* find out what they want to configure */
        if (reqChoice(IS, &what, "defend\0shield\0",
            "Configure what (defend/shield)"))
        {
            (void) skipBlanks(IS);
            switch(what)
            {
                case 0:
                    if (reqChoice(IS, &option, "off\0no\0on\0yes\0",
                        "Enable defend (no/yes)"))
                    {
			switch(option)
			{
			  case 0:
			  case 1:
			    option = 0;
			    break;
			  case 2:
			  case 3:
			    option = 1;
			    break;
			}
			server(IS, rt_lockShip, shNum);
			if (rsh->sh_owner != IS->is_player.p_number)
			{
			    server(IS, rt_unlockShip, shNum);
			    feShDirty(IS, shNum);
                            user(IS, "You don't own that ship any longer!\n");
			    return(FALSE);
			}
			if (option == 0)
			{
                            rsh->sh_flags &= ~SHF_DEFEND;
			}
			else
			{
			    rsh->sh_flags |= SHF_DEFEND;
			}
			server(IS, rt_unlockShip, shNum);
			feShDirty(IS, shNum);
                    }
                    else
                    {
                        return(FALSE);
                    }
                    break;
                case 1:
                    if (reqChoice(IS, &option, "off\0down\0up\0on\0auto\0"
                        "keep\0level\0", "Shield options (auto/down/keep/"
                        "level/up)"))
                    {
                        switch(option)
                        {
                            case 0:
                            case 1:
                                option = 0;
                                break;
                            case 2:
                            case 3:
                                option = 1;
                                break;
                            case 4:
                                option = 2;
                                break;
                            case 5:
                                option = 3;
                                break;
                            case 6:
                                option = 4;
                                break;
                        }
			/* Shields up */
                        if (option == 1)
                        {
                            server(IS, rt_lockShip, shNum);
                            updateShip(IS);
                            if (rsh->sh_owner != IS->is_player.p_number)
                            {
                                server(IS, rt_unlockShip, shNum);
				feShDirty(IS, shNum);
                                user(IS, "You no longer own that ship!\n");
                                return(TRUE);
                            }
                            saveShip = *rsh;
                            /* Make sure they have some shields in the ship */
                            if (numInst(IS, &saveShip, bp_shield) == 0)
                            {
                                *rsh = saveShip;
                                server(IS, rt_unlockShip, shNum);
				feShDirty(IS, shNum);
                                user(IS, "That ship does not have any "
                                    "shields installed!\n");
                                return(TRUE);
                            }
                            /* Make sure they are not already at the level */
                            if (saveShip.sh_shields >= saveShip.sh_shieldKeep)
                            {
                                *rsh = saveShip;
                                server(IS, rt_unlockShip, shNum);
				feShDirty(IS, shNum);
                                user(IS, "That ship already has the energy "
                                    "in it's shields that you have "
                                    "requested!\n");
                                return(TRUE);
                            }
                            maxAmt = maxShields(IS, &saveShip);
                            if (maxAmt == 0)
                            {
                                *rsh = saveShip;
                                server(IS, rt_unlockShip, shNum);
				feShDirty(IS, shNum);
                                user(IS, "That ship is incapable of supporting "
                                    "shields!\n");
                                return(TRUE);
                            }
                            /* Get the amount to transfer in */
                            xferAmt = saveShip.sh_shieldKeep -
                                saveShip.sh_shields;
                            IS->is_noWrite = TRUE;
                            if (xferAmt > maxAmt)
                            {
                                userN3(IS, "Your ship can not support a "
                                    "shield level of ", xferAmt, " units.\n");
                                userN3(IS, "Reducing to ", maxAmt, " units.\n");
                                xferAmt = maxAmt;
                            }
                            /* Make sure this is not more than the ship's */
                            /* total energy */
                            if (xferAmt > saveShip.sh_energy)
                            {
                                userN3(IS, "Your ship does not have enough "
                                    "energy for the requested transfer amount "
                                    "- transfering only ", saveShip.sh_energy,
                                    " units.\n");
                                xferAmt = saveShip.sh_energy;
                            }
                            *rsh = saveShip;
                            rsh->sh_energy -= xferAmt;
                            rsh->sh_shields += xferAmt;
                            server(IS, rt_unlockShip, shNum);
                            IS->is_noWrite = FALSE;
                            uFlush(IS);
			    feShDirty(IS, shNum);
                            return(TRUE);
                        }
                        /* Handle setting the shield level */
                        if (option == 4)
                        {
                            server(IS, rt_readShip, shNum);
                            saveShip = *rsh;
                            (void) skipBlanks(IS);
                            /* pre-fill prompt */
                            impStrCopy(&buff[0], "Enter the desired shield "
                                "level", IMP_BUFSIZE(buff));
                            IS->is_BOOL1 = FALSE;
                            xferAmt = repNum(IS, saveShip.sh_shieldKeep, 0,
                                maxShields(IS, &saveShip), &buff[0]);
                            server(IS, rt_lockShip, shNum);
                            updateShip(IS);
                            if (rsh->sh_owner != IS->is_player.p_number)
                            {
                                server(IS, rt_unlockShip, shNum);
				feShDirty(IS, shNum);
                                user(IS, "You no longer own that ship!\n");
                                return(TRUE);
                            }
                            saveShip = *rsh;
                            /* Make sure they have some shields in the ship */
                            if (numInst(IS, &saveShip, bp_shield) == 0)
                            {
                                *rsh = saveShip;
                                rsh->sh_energy += rsh->sh_shields;
                                rsh->sh_shields = 0;
                                server(IS, rt_unlockShip, shNum);
				feShDirty(IS, shNum);
                                user(IS, "That ship does not have any "
                                    "shields installed!\n");
                                return(TRUE);
                            }
                            maxAmt = maxShields(IS, &saveShip);
                            if (maxAmt == 0)
                            {
                                *rsh = saveShip;
                                rsh->sh_energy += rsh->sh_shields;
                                rsh->sh_shields = 0;
                                server(IS, rt_unlockShip, shNum);
				feShDirty(IS, shNum);
                                user(IS, "That ship is incapable of energizing "
                                    "it's shields!\n");
                                return(TRUE);
                            }
                            IS->is_noWrite = TRUE;
                            if (xferAmt > maxAmt)
                            {
                                userN3(IS, "Your ship can not support a "
                                    "shield level of ", xferAmt, " units.\n");
                                userN3(IS, "Reducing to ", maxAmt, " units.\n");
                                xferAmt = maxAmt;
                            }
                            else if (xferAmt == saveShip.sh_shieldKeep)
                            {
                                *rsh = saveShip;
                                server(IS, rt_unlockShip, shNum);
                                IS->is_noWrite = FALSE;
				feShDirty(IS, shNum);
                                return(TRUE);
                            }
                            *rsh = saveShip;
                            if (xferAmt < saveShip.sh_shieldKeep)
                            {
                                xferAmt = rsh->sh_shieldKeep - xferAmt;
                                rsh->sh_shieldKeep -= xferAmt;
                                if (rsh->sh_shieldKeep < rsh->sh_shields)
                                {
                                    xferAmt = rsh->sh_shields -
                                        rsh->sh_shieldKeep;
                                    rsh->sh_energy += xferAmt;
                                    rsh->sh_shields -= xferAmt;
                                    userN3(IS, "Reduced shields by ", xferAmt,
                                        " units.\n");
                                    
                                }
                                server(IS, rt_unlockShip, shNum);
                                IS->is_noWrite = FALSE;
                                uFlush(IS);
				feShDirty(IS, shNum);
                                return(TRUE);
                            }
                            /* Change the ships "keep" level */
                            rsh->sh_shieldKeep = xferAmt;
                            if (rsh->sh_shieldKeep > rsh->sh_shields)
                            {
                                xferAmt = rsh->sh_shieldKeep - rsh->sh_shields;
                            }
                            /* Make sure this is not more than the ship's */
                            /* total energy */
                            if (xferAmt > rsh->sh_energy)
                            {
                                userN3(IS, "Your ship does not have enough "
                                    "energy for the requested transfer amount"
                                    ".\nTransfering only ", saveShip.sh_energy,
                                    " units to the shields.\n");
                                xferAmt = rsh->sh_energy;
                            }
                            rsh->sh_energy -= xferAmt;
                            rsh->sh_shields += xferAmt;
                            server(IS, rt_unlockShip, shNum);
                            IS->is_noWrite = FALSE;
                            uFlush(IS);
			    feShDirty(IS, shNum);
                            return(TRUE);
                        }
			/* Shield keep & auto */
                        if ((option == 2) || (option == 3))
                        {
                            if (reqChoice(IS, &subOpt, "off\0no\0on\0yes\0",
                                "Enable (no/yes)"))
                            {
                                switch(subOpt)
                                {
                                    case 0:
                                    case 1:
                                        subOpt = 0;
                                        break;
                                    case 2:
                                    case 3:
                                        subOpt = 1;
                                        break;
                                }
                            }
                            else
                            {
                                return(FALSE);
                            }
                        }
                        server(IS, rt_lockShip, shNum);
                        if (rsh->sh_owner != IS->is_player.p_number)
                        {
                            server(IS, rt_unlockShip, shNum);
			    feShDirty(IS, shNum);
                            user(IS, "You don't own that ship any longer!\n");
                            return(FALSE);
                        }
                        switch(option)
                        {
                            case 0:
                                rsh->sh_energy += rsh->sh_shields;
                                rsh->sh_shields = 0;
                                break;
                            case 2:
                                if (subOpt == 0)
                                {
                                    rsh->sh_flags &= ~SHF_SHIELDS;
                                }
                                else
                                {
                                    rsh->sh_flags |= SHF_SHIELDS;
                                }
                                break;
                            case 3:
                                if (subOpt == 0)
                                {
                                    rsh->sh_flags &= ~SHF_SHKEEP;
                                }
                                else
                                {
                                    rsh->sh_flags |= SHF_SHKEEP;
                                }
                                break;
                            default:
                                break;
                        }
                        server(IS, rt_unlockShip, shNum);
			feShDirty(IS, shNum);
                    }
                    else
                    {
                        return(FALSE);
                    }
                    break;
            }
            return(TRUE);
        }
    }
    return(FALSE);
}
