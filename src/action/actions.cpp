//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name actions.cpp - The actions. */
//
//      (c) Copyright 1998-2005 by Lutz Sammer, Russell Smith, and Jimmy Salmon
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "stratagus.h"

#include "actions.h"
#include "video.h"
#include "unittype.h"
#include "animation.h"
#include "player.h"
#include "unit.h"
#include "missile.h"
#include "interface.h"
#include "map.h"
#include "sound.h"
#include "spells.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

unsigned SyncHash; /// Hash calculated to find sync failures


extern void AiReduceMadeInBuilt(PlayerAi &pai, const CUnitType &type);


/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

COrder::COrder(const COrder &rhs): Goal(rhs.Goal), Range(rhs.Range),
	 MinRange(rhs.MinRange), Width(rhs.Width), Height(rhs.Height),
	 Action(rhs.Action), CurrentResource(rhs.CurrentResource),
	 goalPos(rhs.goalPos)
 {
	if (Goal) {
		Goal->RefsIncrease();
	}

	memcpy(&Arg1, &rhs.Arg1, sizeof(Arg1));
	memcpy(&Data, &rhs.Data, sizeof (Data));
	if (Action == UnitActionResource && Arg1.Resource.Mine) {
		Arg1.Resource.Mine->RefsIncrease();
	}
}



/* static */ COrder* COrder::NewActionAttack(const CUnit &attacker, CUnit &target)
{
	COrder *order = new COrder;

	order->Action = UnitActionAttack;

	if (target.Destroyed) {
		order->goalPos = target.tilePos + target.Type->GetHalfTileSize();
	} else {
		// Removed, Dying handled by action routine.
		order->SetGoal(&target);
		order->Range = attacker.Stats->Variables[ATTACKRANGE_INDEX].Max;
		order->MinRange = attacker.Type->MinAttackRange;
	}
	return order;
}

/* static */ COrder* COrder::NewActionAttack(const CUnit &attacker, const Vec2i &dest)
{
	Assert(Map.Info.IsPointOnMap(dest));

	COrder *order = new COrder;

	order->Action = UnitActionAttack;

	if (Map.WallOnMap(dest) && Map.IsFieldExplored(*attacker.Player, dest)) {
		// FIXME: look into action_attack.cpp about this ugly problem
		order->goalPos = dest;
		order->Range = attacker.Stats->Variables[ATTACKRANGE_INDEX].Max;
		order->MinRange = attacker.Type->MinAttackRange;
	} else {
		order->goalPos = dest;
	}
	return order;
}

/* static */ COrder* COrder::NewActionAttackGround(const CUnit &attacker, const Vec2i &dest)
{
	COrder *order = new COrder;

	order->Action = UnitActionAttackGround;
	order->goalPos = dest;
	order->Range = attacker.Stats->Variables[ATTACKRANGE_INDEX].Max;
	order->MinRange = attacker.Type->MinAttackRange;

	return order;
}


/* static */ COrder* COrder::NewActionBoard(CUnit &unit)
{
	COrder *order = new COrder;

	order->Action = UnitActionBoard;
	order->SetGoal(&unit);
	order->Range = 1;

	return order;
}


/* static */ COrder* COrder::NewActionBuild(const CUnit &builder, const Vec2i &pos, CUnitType &building)
{
	COrder *order = new COrder;

	order->Action = UnitActionBuild;

	order->goalPos = pos;
	order->Width = building.TileWidth;
	order->Height = building.TileHeight;
	if (building.BuilderOutside) {
		order->Range = builder.Type->RepairRange;
	} else {
		// If building inside, but be next to stop
		if (building.ShoreBuilding && builder.Type->UnitType == UnitTypeLand) {
				// Peon won't dive :-)
			order->Range = 1;
		}
	}
	order->Arg1.Type = &building;
	if (building.BuilderOutside) {
		order->MinRange = 1;
	}
	return order;
}

/* static */ COrder* COrder::NewActionBuilt()
{
	COrder *order = new COrder;

	order->Action = UnitActionBuilt;
	return order;
}

/* static */ COrder* COrder::NewActionDie()
{
	COrder *order = new COrder;

	order->Action = UnitActionDie;
	return order;
}

/* static */ COrder* COrder::NewActionFollow(CUnit &dest)
{
	COrder *order = new COrder;

	order->Action = UnitActionFollow;
	// Destination could be killed.
	// Should be handled in action, but is not possible!
	// Unit::Refs is used as timeout counter.
	if (dest.Destroyed) {
		order->goalPos = dest.tilePos + dest.Type->GetHalfTileSize();
	} else {
		order->SetGoal(&dest);
		order->Range = 1;
	}

	return order;
}


/* static */ COrder* COrder::NewActionMove(const Vec2i &pos)
{
	COrder *order = new COrder;

	order->Action = UnitActionMove;
	order->goalPos = pos;

	return order;
}


/* static */ COrder* COrder::NewActionPatrol(const Vec2i &currentPos, const Vec2i &dest)
{
	Assert(Map.Info.IsPointOnMap(currentPos));
	Assert(Map.Info.IsPointOnMap(dest));

	COrder *order = new COrder;

	order->Action = UnitActionPatrol;
	order->goalPos = dest;
	order->Arg1.Patrol = currentPos;

	return order;
}

/* static */ COrder* COrder::NewActionRepair(CUnit &unit, CUnit &target)
{
	COrder *order = new COrder;

	order->Action = UnitActionRepair;
	if (target.Destroyed) {
		order->goalPos = target.tilePos + target.Type->GetHalfTileSize();
	} else {
		order->SetGoal(&target);
		order->Range = unit.Type->RepairRange;
	}
	return order;
}

/* static */ COrder* COrder::NewActionRepair(const Vec2i &pos)
{
	Assert(Map.Info.IsPointOnMap(pos));

	COrder *order = new COrder;

	order->Action = UnitActionRepair;
	order->goalPos = pos;
	return order;
}



/* static */ COrder* COrder::NewActionResearch(CUnit &unit, CUpgrade &upgrade)
{
	COrder *order = new COrder;

	order->Action = UnitActionResearch;

	// FIXME: if you give quick an other order, the resources are lost!
	unit.Player->SubCosts(upgrade.Costs);

	order->Arg1.Upgrade = &upgrade;

	return order;
}

/* static */ COrder* COrder::NewActionResource(CUnit &harvester, const Vec2i &pos)
{
	COrder *order = new COrder;
	Vec2i ressourceLoc;

	order->Action = UnitActionResource;

	//  Find the closest piece of wood next to a tile where the unit can move
	if (!FindTerrainType(0, (harvester.Type->MovementMask), 1, 20, harvester.Player, pos, &ressourceLoc)) {
		DebugPrint("FIXME: Give up???\n");
	}
	// Max Value > 1
	if ((MyAbs(ressourceLoc.x - pos.x) | MyAbs(ressourceLoc.y - pos.y)) > 1) {
		if (!FindTerrainType(0, MapFieldForest, 0, 20, harvester.Player, ressourceLoc, &ressourceLoc)) {
			DebugPrint("FIXME: Give up???\n");
		}
	} else {
		// The destination is next to a reachable tile.
		ressourceLoc = pos;
	}
	order->goalPos = ressourceLoc;
	order->Range = 1;

	return order;
}

/* static */ COrder* COrder::NewActionResource(CUnit &mine)
{
	COrder *order = new COrder;

	order->Action = UnitActionResource;
	order->SetGoal(&mine);
	order->Range = 1;

	return order;
}



/* static */ COrder* COrder::NewActionReturnGoods(CUnit *depot)
{
	COrder *order = new COrder;

	order->Action = UnitActionReturnGoods;
	// Destination could be killed. NETWORK!
	if (depot && !depot->Destroyed) {
		order->SetGoal(depot);
	}
	order->Range = 1;

	return order;
}

/* static */ COrder* COrder::NewActionSpellCast(SpellType &spell, const Vec2i &pos, CUnit *target)
{
	COrder *order = new COrder;

	order->Action = UnitActionSpellCast;

	order->Range = spell.Range;
	if (target) {
		// Destination could be killed.
		// Should be handled in action, but is not possible!
		// Unit::Refs is used as timeout counter.
		if (target->Destroyed) {
			// FIXME: where check if spell needs a unit as destination?
			// FIXME: target->Type is now set to 0. maybe we shouldn't bother.
			const Vec2i diag = {order->Range, order->Range};
			order->goalPos = target->tilePos /* + target->Type->GetHalfTileSize() */ - diag;
			order->Range <<= 1;
		} else {
			order->SetGoal(target);
		}
	} else {
		order->goalPos = pos;
	}
	order->Arg1.Spell = &spell;

	return order;
}

/* static */ COrder* COrder::NewActionStandGround()
{
	COrder *order = new COrder;

	order->Action = UnitActionStandGround;
	return order;
}


/* static */ COrder* COrder::NewActionStill()
{
	COrder *order = new COrder;

	order->Action = UnitActionStill;
	return order;
}




/* static */ COrder* COrder::NewActionTrain(CUnit &trainer, CUnitType &type)
{
	COrder *order = new COrder;

	order->Action = UnitActionTrain;
	order->Arg1.Type = &type;
	// FIXME: if you give quick an other order, the resources are lost!
	trainer.Player->SubUnitType(type);

	return order;
}

/* static */ COrder* COrder::NewActionTransformInto(CUnitType &type)
{
	COrder *order = new COrder;

	order->Action = UnitActionTransformInto;

	order->Arg1.Type = &type;

	return order;
}

/* static */ COrder* COrder::NewActionUnload(const Vec2i &pos, CUnit *what)
{
	COrder *order = new COrder;

	order->Action = UnitActionUnload;
	order->goalPos = pos;
	if (what && !what->Destroyed) {
		order->SetGoal(what);
	}

	return order;
}

/* static */ COrder* COrder::NewActionUpgradeTo(CUnit &unit, CUnitType &type)
{
	COrder *order = new COrder;

	order->Action = UnitActionUpgradeTo;

	// FIXME: if you give quick an other order, the resources are lost!
	unit.Player->SubUnitType(type);
	order->Arg1.Type = &type;

	return order;
}

COrder& COrder::operator=(const COrder &rhs) {
	if (this != &rhs) {
		Action = rhs.Action;
		Range = rhs.Range;
		MinRange = rhs.MinRange;
		Width = rhs.Width;
		Height = rhs.Height;
		CurrentResource = rhs.CurrentResource;
		SetGoal(rhs.Goal);
		goalPos = rhs.goalPos;
		memcpy(&Arg1, &rhs.Arg1, sizeof(Arg1));
		memcpy(&Data, &rhs.Data, sizeof (Data));
		//FIXME: Hardcoded wood
		if (Action == UnitActionResource && Arg1.Resource.Mine) {
			Arg1.Resource.Mine->RefsIncrease();
		}
	}
	return *this;
}

COrder::~COrder()
{
	if (Goal) {
		Goal->RefsDecrease();
		Goal = NoUnitP;
	}
	if (Action == UnitActionResource && Arg1.Resource.Mine) {
		Arg1.Resource.Mine->RefsDecrease();
		Arg1.Resource.Mine = NoUnitP;
	}
}

void COrder::ReleaseRefs(CUnit &unit)
{
	// Release pending references.
	if (this->Action == UnitActionResource) {
		CUnit *mine = this->Arg1.Resource.Mine;

		if (mine) {
			unit.DeAssignWorkerFromMine(*mine);
			mine->RefsDecrease();
			this->Arg1.Resource.Mine = NULL;

		}
	}
	if (this->HasGoal()) {
		// If mining decrease the active count on the resource.
		if (this->Action == UnitActionResource) {
			if (unit.SubAction == 60 /* SUB_GATHER_RESOURCE */ ) {
				CUnit *goal = this->GetGoal();

				goal->Resource.Active--;
				Assert(goal->Resource.Active >= 0);
			}
		}
		// Still shouldn't have a reference unless attacking
		Assert(!(this->Action == UnitActionStill && !unit.SubAction));
		this->ClearGoal();
	}
#ifdef DEBUG
	 else {
		if (unit.CurrentResource &&
			!unit.Type->ResInfo[unit.CurrentResource]->TerrainHarvester) {
			Assert(this->Action != UnitActionResource);
		}
	}
#endif
}

void COrder::SetGoal(CUnit *const new_goal)
{
	if (new_goal) {
		new_goal->RefsIncrease();
	}
	if (Goal) {
		Goal->RefsDecrease();
	}
	Goal = new_goal;
}

void COrder::ClearGoal()
{
	if (Goal) {
		Goal->RefsDecrease();
	}
	Goal = NULL;
}


bool COrder::CheckRange() const
{
	return (Range <= Map.Info.MapWidth || Range <= Map.Info.MapHeight);
}

void COrder::FillSeenValues(CUnit &unit) const
{
	unit.Seen.State = (Action == UnitActionBuilt) | ((Action == UnitActionUpgradeTo) << 1);
	if (unit.CurrentAction() == UnitActionDie) {
		unit.Seen.State = 3;
	}
	if (Action == UnitActionBuilt) {
		unit.Seen.CFrame = Data.Built.Frame;
	} else {
		unit.Seen.CFrame = NULL;
	}
}

bool COrder::OnAiHitUnit(CUnit &unit, CUnit *attacker, int /*damage*/)
{
	Assert(unit.CurrentOrder() == this);

	switch (Action) {
		case UnitActionTrain:
		case UnitActionUpgradeTo:
		case UnitActionResearch:
		case UnitActionBuilt:
		case UnitActionBuild:
		case UnitActionTransformInto:
		case UnitActionBoard:
		case UnitActionUnload:
		case UnitActionReturnGoods:
			// Unit is working ?
			// Maybe AI should cancel action and save resources ???
			return true;
		case UnitActionResource:
			if (unit.SubAction >= 65) {
				//Normal return to depot
				return true;
			}
			if (unit.SubAction > 55  &&
				unit.ResourcesHeld > 0) {
				//escape to Depot with this what you have;
				Data.ResWorker.DoneHarvesting = 1;
				return true;
			}
		break;
		case UnitActionAttack:
		{
			CUnit *goal = GetGoal();
			if (goal) {
				if (goal == attacker ||
					(goal->CurrentAction() == UnitActionAttack &&
					goal->CurrentOrder()->GetGoal() == &unit))
				{
					//we already fight with one of attackers;
					return true;
				}
			}
		}
		default:
		break;
	}
	return false;
}


/** Called when unit is killed.
**  warn the AI module.
*/
void COrder::AiUnitKilled(CUnit& unit)
{
	switch (Action) {
		case UnitActionStill:
		case UnitActionAttack:
		case UnitActionMove:
			break;
		case UnitActionBuilt:
			DebugPrint("%d: %d(%s) killed, under construction!\n" _C_
				unit.Player->Index _C_ UnitNumber(unit) _C_ unit.Type->Ident.c_str());
			AiReduceMadeInBuilt(*unit.Player->Ai, *unit.Type);
			break;
		case UnitActionBuild:
			DebugPrint("%d: %d(%s) killed, with order %s!\n" _C_
				unit.Player->Index _C_ UnitNumber(unit) _C_
				unit.Type->Ident.c_str() _C_ Arg1.Type->Ident.c_str());
			if (!HasGoal()) {
				AiReduceMadeInBuilt(*unit.Player->Ai, *Arg1.Type);
			}
			break;
		default:
			DebugPrint("FIXME: %d: %d(%s) killed, with order %d!\n" _C_
				unit.Player->Index _C_ UnitNumber(unit) _C_
				unit.Type->Ident.c_str() _C_ Action);
			break;
	}
}

/**
**  Call when animation step is "attack"
*/
void COrder::OnAnimationAttack(CUnit &unit)
{
	Assert(unit.CurrentOrder() == this);

	if (Action == UnitActionSpellCast) {
		CUnit *goal = GetGoal();
		if (goal && !goal->IsVisibleAsGoal(*unit.Player)) {
			unit.ReCast = 0;
		} else {
			unit.ReCast = SpellCast(unit, Arg1.Spell, goal, goalPos.x, goalPos.y);
		}
	} else {
		FireMissile(unit);
	}
	UnHideUnit(unit); // unit is invisible until attacks
}


/*----------------------------------------------------------------------------
--  Animation
----------------------------------------------------------------------------*/

/**
**  Rotate a unit
**
**  @param unit    Unit to rotate
**  @param rotate  Number of frames to rotate (>0 clockwise, <0 counterclockwise)
*/
static void UnitRotate(CUnit &unit, int rotate)
{
	unit.Direction += rotate * 256 / unit.Type->NumDirections;
	UnitUpdateHeading(unit);
}

/**
**  Show unit animation.
**
**  @param unit  Unit of the animation.
**  @param anim  Animation script to handle.
**
**  @return      The flags of the current script step.
*/
int UnitShowAnimation(CUnit &unit, const CAnimation *anim)
{
	return UnitShowAnimationScaled(unit, anim, 8);
}

/**
**  Show unit animation.
**
**  @param unit   Unit of the animation.
**  @param anim   Animation script to handle.
**  @param scale  Scaling factor of the wait times in animation (8 means no scaling).
**
**  @return       The flags of the current script step.
*/
int UnitShowAnimationScaled(CUnit &unit, const CAnimation *anim, int scale)
{
	int move;
	int x,y;

	// Changing animations
	if (anim && unit.Anim.CurrAnim != anim) {
	// Assert fails when transforming unit (upgrade-to).
		Assert(!unit.Anim.Unbreakable);
		unit.Anim.Anim = unit.Anim.CurrAnim = anim;
		unit.Anim.Wait = 0;
	}

	// Currently waiting
	if (unit.Anim.Wait) {
		--unit.Anim.Wait;
		if (!unit.Anim.Wait) {
			// Advance to next frame
			unit.Anim.Anim = unit.Anim.Anim->Next;
			if (!unit.Anim.Anim) {
				unit.Anim.Anim = unit.Anim.CurrAnim;
			}
		}
		return 0;
	}

	move = 0;
	while (!unit.Anim.Wait) {
		switch (unit.Anim.Anim->Type) {
			case AnimationFrame:
				unit.Frame = unit.Anim.Anim->D.Frame.Frame;
				UnitUpdateHeading(unit);
				break;
			case AnimationExactFrame:
				unit.Frame = unit.Anim.Anim->D.Frame.Frame;
				break;

			case AnimationWait:
				unit.Anim.Wait = unit.Anim.Anim->D.Wait.Wait << scale >> 8;
				if (unit.Variable[SLOW_INDEX].Value) { // unit is slowed down
					unit.Anim.Wait <<= 1;
				}
				if (unit.Variable[HASTE_INDEX].Value && unit.Anim.Wait > 1) { // unit is accelerated
					unit.Anim.Wait >>= 1;
				}
				if (unit.Anim.Wait <= 0)
					unit.Anim.Wait = 1;
				break;
			case AnimationRandomWait:
				unit.Anim.Wait = unit.Anim.Anim->D.RandomWait.MinWait +
					SyncRand() % (unit.Anim.Anim->D.RandomWait.MaxWait - unit.Anim.Anim->D.RandomWait.MinWait + 1);
				break;

			case AnimationSound:
				if (unit.IsVisible(*ThisPlayer) || ReplayRevealMap) {
					PlayUnitSound(unit, unit.Anim.Anim->D.Sound.Sound);
				}
				break;
			case AnimationRandomSound:
				if (unit.IsVisible(*ThisPlayer) || ReplayRevealMap) {
					int sound;
					sound = SyncRand() % unit.Anim.Anim->D.RandomSound.NumSounds;
					PlayUnitSound(unit, unit.Anim.Anim->D.RandomSound.Sound[sound]);
				}
				break;

			case AnimationAttack:
			{
				unit.CurrentOrder()->OnAnimationAttack(unit);
				break;
			}
			case AnimationSpawnMissile:
				x = unit.tilePos.x * PixelTileSize.x + PixelTileSize.x / 2;  
				y = unit.tilePos.y * PixelTileSize.y + PixelTileSize.y / 2;
				MakeMissile(MissileTypeByIdent(unit.Anim.Anim->D.SpawnMissile.Missile),x,y,x,y);
				break;

			case AnimationRotate:
				UnitRotate(unit, unit.Anim.Anim->D.Rotate.Rotate);
				break;
			case AnimationRandomRotate:
				if ((SyncRand() >> 8) & 1) {
					UnitRotate(unit, -unit.Anim.Anim->D.Rotate.Rotate);
				} else {
					UnitRotate(unit, unit.Anim.Anim->D.Rotate.Rotate);
				}
				break;

			case AnimationMove:
				Assert(!move);
				move = unit.Anim.Anim->D.Move.Move;
				break;

			case AnimationUnbreakable:
				Assert(unit.Anim.Unbreakable ^ unit.Anim.Anim->D.Unbreakable.Begin);
				/*DebugPrint("UnitShowAnimationScaled: switch Unbreakable from %s to %s\n"
					_C_ unit.Anim.Unbreakable ? "TRUE" : "FALSE"
					_C_ unit.Anim.Anim->D.Unbreakable.Begin ? "TRUE" : "FALSE" );*/
				unit.Anim.Unbreakable = unit.Anim.Anim->D.Unbreakable.Begin;
				break;

			case AnimationNone:
			case AnimationLabel:
				break;

			case AnimationGoto:
				unit.Anim.Anim = unit.Anim.Anim->D.Goto.Goto;
				break;
			case AnimationRandomGoto:
				if (SyncRand() % 100 < unit.Anim.Anim->D.RandomGoto.Random) {
					unit.Anim.Anim = unit.Anim.Anim->D.RandomGoto.Goto;
				}
				break;
		}

		if (!unit.Anim.Wait) {
			// Advance to next frame
			unit.Anim.Anim = unit.Anim.Anim->Next;
			if (!unit.Anim.Anim) {
				unit.Anim.Anim = unit.Anim.CurrAnim;
			}
		}
	}

	--unit.Anim.Wait;
	if (!unit.Anim.Wait) {
		// Advance to next frame
		unit.Anim.Anim = unit.Anim.Anim->Next;
		if (!unit.Anim.Anim) {
			unit.Anim.Anim = unit.Anim.CurrAnim;
		}
	}
	return move;
}

/*----------------------------------------------------------------------------
--  Actions
----------------------------------------------------------------------------*/

/**
**  Unit does nothing!
**
**  @param unit  Unit pointer for none action.
*/
static void HandleActionNone(COrder&, CUnit &unit)
{
	DebugPrint("FIXME: Should not happen!\n");
	DebugPrint("FIXME: Unit (%d) %s has action none.!\n" _C_
		UnitNumber(unit) _C_ unit.Type->Ident.c_str());
}

/**
**  Unit has not written function.
**
**  @param unit  Unit pointer for not written action.
*/
static void HandleActionNotWritten(COrder&, CUnit &unit)
{
	DebugPrint("FIXME: Not written!\n");
	DebugPrint("FIXME: Unit (%d) %s has action %d.!\n" _C_
		UnitNumber(unit) _C_ unit.Type->Ident.c_str() _C_ unit.CurrentAction());
}

/**
**  Jump table for actions.
**
**  @note can move function into unit structure.
*/
static void (*HandleActionTable[256])(COrder&, CUnit &) = {
	HandleActionNone,
	HandleActionStill,
	HandleActionStandGround,
	HandleActionFollow,
	HandleActionMove,
	HandleActionAttack,
	HandleActionAttack, // HandleActionAttackGround,
	HandleActionDie,
	HandleActionSpellCast,
	HandleActionTrain,
	HandleActionUpgradeTo,
	HandleActionResearch,
	HandleActionBuilt,
	HandleActionBoard,
	HandleActionUnload,
	HandleActionPatrol,
	HandleActionBuild,
	HandleActionRepair,
	HandleActionResource,
	HandleActionReturnGoods,
	HandleActionTransformInto,
	HandleActionNotWritten,

	// Enough for the future ?
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
	HandleActionNotWritten, HandleActionNotWritten, HandleActionNotWritten,
};

/**
**  Increment a unit's health
**
**  @param unit  the unit to operate on
*/
static void HandleRegenerations(CUnit &unit)
{
	int f = 0;

	// Burn
	if (!unit.Removed && !unit.Destroyed && unit.Variable[HP_INDEX].Max &&
			unit.CurrentAction() != UnitActionBuilt &&
			unit.CurrentAction() != UnitActionDie) {
		f = (100 * unit.Variable[HP_INDEX].Value) / unit.Variable[HP_INDEX].Max;
		if (f <= unit.Type->BurnPercent && unit.Type->BurnDamageRate) {
			HitUnit(NoUnitP, unit, unit.Type->BurnDamageRate);
			f = 1;
		} else {
			f = 0;
		}
	}

	// Health doesn't regenerate while burning.
	unit.Variable[HP_INDEX].Increase = f ? 0 : unit.Stats->Variables[HP_INDEX].Increase;
}

/**
**  Handle things about the unit that decay over time
**
**  @param unit    The unit that the decay is handled for
**  @param amount  The amount of time to make up for.(in cycles)
*/
static void HandleBuffs(CUnit &unit, int amount)
{
	//
	// Look if the time to live is over.
	//
	if (unit.TTL && unit.TTL < (GameCycle - unit.Variable[HP_INDEX].Value)) {
		DebugPrint("Unit must die %lu %lu!\n" _C_ unit.TTL _C_ GameCycle);
		//
		// Hit unit does some funky stuff...
		//
		unit.Variable[HP_INDEX].Value -= amount;
		if (unit.Variable[HP_INDEX].Value <= 0) {
			LetUnitDie(unit);
		}
	}

	//
	//  decrease spells effects time.
	//
	unit.Variable[BLOODLUST_INDEX].Increase = -amount;
	unit.Variable[HASTE_INDEX].Increase = -amount;
	unit.Variable[SLOW_INDEX].Increase = -amount;
	unit.Variable[INVISIBLE_INDEX].Increase = -amount;
	unit.Variable[UNHOLYARMOR_INDEX].Increase = -amount;


	unit.Variable[SHIELD_INDEX].Increase = 1;

	// User defined variables
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		if (unit.Variable[i].Enable && unit.Variable[i].Increase) {
			if (i == INVISIBLE_INDEX &&
				unit.Variable[INVISIBLE_INDEX].Value > 0 &&
				unit.Variable[INVISIBLE_INDEX].Value +
				unit.Variable[INVISIBLE_INDEX].Increase <= 0)
			{
				UnHideUnit(unit);
			} else {
				unit.Variable[i].Value += unit.Variable[i].Increase;
				if (unit.Variable[i].Value <= 0) {
					unit.Variable[i].Value = 0;
				} else if (unit.Variable[i].Value > unit.Variable[i].Max) {
					unit.Variable[i].Value = unit.Variable[i].Max;
				}
			}
		}
	}
}

static void RunAction(COrder &order, CUnit &unit)
{
	HandleActionTable[order.Action](order, unit);
}


/**
**  Handle the action of a unit.
**
**  @param unit  Pointer to handled unit.
*/
static void HandleUnitAction(CUnit &unit)
{
	// If current action is breakable proceed with next one.
	if (!unit.Anim.Unbreakable) {
		if (unit.CriticalOrder != NULL) {
			RunAction(*unit.CriticalOrder, unit);
			delete unit.CriticalOrder;
			unit.CriticalOrder = NULL;
		}

		// o Look if we have a new order and old finished.
		// o Or the order queue should be flushed.
		if (unit.OrderCount > 1 &&
				(unit.CurrentAction() == UnitActionStill || unit.OrderFlush)) {

			if (unit.Removed) { // FIXME: johns I see this as an error
				DebugPrint("Flushing removed unit\n");
				// This happens, if building with ALT+SHIFT.
				return;
			}
			COrderPtr order = unit.CurrentOrder();

			order->ReleaseRefs(unit);

			// Shift queue with structure assignment.
			unit.OrderCount--;
			unit.OrderFlush = 0;
			delete unit.Orders[0];
			for (int z = 0; z < unit.OrderCount; ++z) {
				unit.Orders[z] = unit.Orders[z + 1];
			}
			unit.Orders.pop_back();

			//
			// Note subaction 0 should reset.
			//
			unit.SubAction = unit.State = 0;
			unit.Wait = 0;

			if (IsOnlySelected(unit)) { // update display for new action
				SelectedUnitChanged();
			}
		}
	}

	// Select action.
	RunAction(*unit.CurrentOrder(), unit);
}

/**
**  Update the actions of all units each game cycle.
**
**  @todo  To improve the preformance use slots for waiting.
*/
void UnitActions()
{
	CUnit *table[UnitMax];
	int blinkthiscycle;
	int buffsthiscycle;
	int regenthiscycle;
	int i;
	int tabsize;

	buffsthiscycle = regenthiscycle = blinkthiscycle = !(GameCycle % CYCLES_PER_SECOND);

	memcpy(table, Units, NumUnits * sizeof(CUnit *));
	tabsize = NumUnits;

	//
	// Check for things that only happen every few cycles
	// (faster in their own loops.)
	//
	//FIXME rb - why it is faseter as own loops ?
#if 0
	// 1) Blink flag.
	if (blinkthiscycle) {
		for (i = 0; i < tabsize; ++i) {
			if (table[i]->Destroyed) {
				table[i--] = table[--tabsize];
				continue;
			}
			if (table[i]->Blink) {
				--table[i]->Blink;
			}
		}
	}

	// 2) Buffs...
	if (buffsthiscycle) {
		for (i = 0; i < tabsize; ++i) {
			if (table[i]->Destroyed) {
				table[i--] = table[--tabsize];
				continue;
			}
			HandleBuffs(table[i], CYCLES_PER_SECOND);
		}
	}

	// 3) Increase health mana, burn and stuff
	if (regenthiscycle) {
		for (i = 0; i < tabsize; ++i) {
			if (table[i]->Destroyed) {
				table[i--] = table[--tabsize];
				continue;
			}
			HandleRegenerations(table[i]);
		}
	}
#else
	if (blinkthiscycle || buffsthiscycle || regenthiscycle) {
		for (i = 0; i < tabsize; ++i) {
			if (table[i]->Destroyed) {
				table[i--] = table[--tabsize];
				continue;
			}

			// 1) Blink flag.
			//if (blinkthiscycle && table[i]->Blink)
			if (table[i]->Blink)
			{
				--table[i]->Blink;
			}
			// 2) Buffs...
			//if (buffsthiscycle)
			{
				HandleBuffs(*table[i], CYCLES_PER_SECOND);
			}
			// 3) Increase health mana, burn and stuff
			//if (regenthiscycle)
			{
				HandleRegenerations(*table[i]);
			}
		}
	}
#endif

	//
	// Do all actions
	//
	for (i = 0; i < tabsize; ++i) {
		while (table[i]->Destroyed) {
			table[i] = table[--tabsize];
		}
			CUnit &unit = *table[i];

		HandleUnitAction(unit);

#ifdef DEBUG_LOG
		//
		// Dump the unit to find the network sync bugs.
		//
		{
		static FILE *logf;

		if (!logf) {
			time_t now;
			char buf[256];

			snprintf(buf, sizeof(buf), "log_of_stratagus_%d.log", ThisPlayer->Index);
			logf = fopen(buf, "wb");
			if (!logf) {
				return;
			}
			fprintf(logf, "; Log file generated by Stratagus Version "
					VERSION "\n");
			time(&now);
			fprintf(logf, ";\tDate: %s", ctime(&now));
			fprintf(logf, ";\tMap: %s\n\n", Map.Info.Description.c_str());
		}

		fprintf(logf, "%lu: ", GameCycle);
		fprintf(logf, "%d %s S%d/%d-%d P%d Refs %d: %X %d,%d %d,%d\n",
			UnitNumber(unit), unit.Type ? unit.Type->Ident.c_str() : "unit-killed",
			unit.State, unit.SubAction,
			!unit.Orders.empty() ? unit.CurrentAction() : -1,
			unit.Player ? unit.Player->Index : -1, unit.Refs,SyncRandSeed,
			unit.X, unit.Y, unit.IX, unit.IY);

#if 0
		SaveUnit(unit,logf);
#endif
		fflush(NULL);
		}
#endif
		//
		// Calculate some hash.
		//
		SyncHash = (SyncHash << 5) | (SyncHash >> 27);
		SyncHash ^= unit.Orders.size() > 0 ? unit.CurrentAction() << 18 : 0;
		SyncHash ^= unit.State << 12;
		SyncHash ^= unit.SubAction << 6;
		SyncHash ^= unit.Refs << 3;
	}
}

//@}
