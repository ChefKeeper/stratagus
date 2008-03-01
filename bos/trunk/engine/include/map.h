//     ____                _       __               
//    / __ )____  _____   | |     / /___ ___________
//   / __  / __ \/ ___/   | | /| / / __ `/ ___/ ___/
//  / /_/ / /_/ (__  )    | |/ |/ / /_/ / /  (__  ) 
// /_____/\____/____/     |__/|__/\__,_/_/  /____/  
//                                              
//       A futuristic real-time strategy game.
//          This file is part of Bos Wars.
//
/**@name map.h - The map headerfile. */
//
//      (c) Copyright 1998-2008 by Vladi Shabanski, Lutz Sammer, and
//                                 Jimmy Salmon
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

#ifndef __MAP_H__
#define __MAP_H__

//@{

/*----------------------------------------------------------------------------
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @class CMapField map.h
**
**  \#include "map.h"
**
**  This class contains all information about a field on map.
**  It contains its look, properties and content.
**
**  The map-field class members:
**
**  CMapField::Tile
**
**    Tile is number defining the graphic image display for the
**    map-field. 65535 different tiles are supported. A tile is
**    currently 32x32 pixels. In the future is planned to support
**    animated tiles.
**
**  CMapField::SeenTile
**
**    This is the tile number, that the player sitting on the computer
**    currently knows. Idea: Can be uses for illusions.
**
**  CMapField::Flags
**
**    Contains special information of that tile. What units are
**    on this field, what units could be placed on this field.
**
**    This is the list of all flags currently used:
**
**    ::MapFieldVisible field is visible.
**    ::MapFieldExplored field is explored.
**    ::MapFieldLandAllowed land units are allowed.
**    ::MapFieldCoastAllowed coast units (transporter) and coast
**      buildings (shipyard) are allowed.
**    ::MapFieldWaterAllowed water units allowed.
**    ::MapFieldNoBuilding no buildings allowed.
**    ::MapFieldUnpassable field is movement blocked.
**    ::MapFieldLandUnit land unit on field.
**    ::MapFieldAirUnit air unit on field.
**    ::MapFieldSeaUnit water unit on field.
**    ::MapFieldBuilding building on field.
**
**    Note: We want to add support for more unit-types like under
**      ground units.
**
**  CMapField::Cost
**
**    Unit cost to move in this tile.
**
**  CMapField::Visible[]
**
**    Counter how many units of the player can see this field. 0 the
**    field is not explored, 1 explored, n-1 unit see it. Currently
**    no more than 253 units can see a field.
**
**  CMapField::Radar[]
**
**    Visiblity for radar.
**
**  CMapField::RadarJammer[]
**
**    Jamming capabilities.
**
**  CMapField::UnitCache
**
**    Contains a vector of all units currently on this field.
**    Note: currently units are only inserted at the insert point.
**    This means units of the size of 2x2 fields are inserted at the
**    top and right most map coordinate.
*/

/**
**  @class CMap map.h
**
**  \#include "map.h"
**
**  This class contains all information about a Stratagus map.
**  A map is a rectangle of any size.
**
**  The map class members:
**
**  CMap::Fields
**
**    An array CMap::Info::Width * CMap::Info::Height of all fields
**    belonging to this map.
**
**  CMap::NoFogOfWar
**
**    Flag if true, the fog of war is disabled.
**
**  CMap::Tileset
**
**    Tileset data for the map. See ::CTileset. This contains all
**    information about the tile.
**
**  CMap::FogGraphic
**
**    Graphic for fog of war
**
**  CMap::Info
**
**    Descriptive information of the map. See ::CMapInfo.
*/

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <string>
#include <vector>
#include "iocompat.h"
#include "tileset.h"
#include "patch_manager.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CGraphic;
class CPlayer;
class CFile;
class CUnit;
class CUnitType;

/*----------------------------------------------------------------------------
--  Map
----------------------------------------------------------------------------*/

#define MaxMapWidth  256  /// max map width supported
#define MaxMapHeight 256  /// max map height supported

/*----------------------------------------------------------------------------
--  Map - field
----------------------------------------------------------------------------*/

	/// Describes a field of the map
class CMapField {
public:
	CMapField() : Tile(0), SeenTile(0), Flags(0), Cost(0)
	{
		memset(Visible, 0, sizeof(Visible));
		memset(Radar, 0, sizeof(Radar));
		memset(RadarJammer, 0, sizeof(RadarJammer));
	}

	unsigned short Tile;      /// graphic tile number
	unsigned short SeenTile;  /// last seen tile (FOW)
	unsigned short Flags;     /// field flags
	unsigned char Cost;       /// unit cost to move in this tile

	unsigned short Visible[PlayerMax];    /// Seen counter 0 unexplored
	unsigned char Radar[PlayerMax];       /// Visiblity for radar.
	unsigned char RadarJammer[PlayerMax]; /// Jamming capabilities.
};

// Not used until now:
#define MapFieldSpeedMask 0x0007  /// Move faster on this tile

#define MapFieldLandAllowed  0x0010  /// Land units allowed
#define MapFieldCoastAllowed 0x0020  /// Coast (transporter) units allowed
#define MapFieldWaterAllowed 0x0040  /// Water units allowed
#define MapFieldNoBuilding   0x0080  /// No buildings allowed

#define MapFieldUnpassable 0x0100  /// Field is movement blocked

#define MapFieldLandUnit 0x1000  /// Land unit on field
#define MapFieldAirUnit  0x2000  /// Air unit on field
#define MapFieldSeaUnit  0x4000  /// Water unit on field
#define MapFieldBuilding 0x8000  /// Building on field

/*----------------------------------------------------------------------------
--  Map info structure
----------------------------------------------------------------------------*/

/**
**  Get info about a map.
*/
class CMapInfo {
public:
	std::string Description;     /// Map description
	std::string Filename;        /// Map filename
	int MapWidth;          /// Map width
	int MapHeight;         /// Map height
	int PlayerType[PlayerMax];  /// Same player->Type
	int PlayerSide[PlayerMax];  /// Same player->Side
	unsigned int MapUID;   /// Unique Map ID (hash)
};

/*----------------------------------------------------------------------------
--  Map itself
----------------------------------------------------------------------------*/

	/// Describes the world map
class CMap {
public:

	/// Alocate and initialise map table.
	void Create();
	/// Build tables for map
	void Init();
	/// Clean the map
	void Clean();
	/// Cleanup memory for fog of war tables
	void CleanFogOfWar();

	/// Find if a tile is visible (with shared vision).
	unsigned short IsTileVisible(const CPlayer *player, int x, int y) const;

	/// Check if a field for the user is explored.
	bool IsFieldExplored(const CPlayer *player, int x, int y) const
	{
		return IsTileVisible(player, x, y) > 0;
	}
	/// Check if a field for the user is visible.
	bool IsFieldVisible(const CPlayer *player, int x, int y) const
	{
		return IsTileVisible(player, x, y) > 1;
	}
	/// Mark a tile as seen by the player.
	void MarkSeenTile(int x, int y);

	/// Reveal the complete map, make everything known.
	void Reveal(void);
	/// Save the map.
	void Save(CFile *file) const;

	/// Get the MapField at location x,y
	inline CMapField *Field(int x, int y) const {
		return &this->Fields[x + y * this->Info.MapWidth];
	}

//
//  Tile type.
//
	/// Returns true, if water on the map tile field
	bool WaterOnMap(int x, int y) const;
	/// Returns true, if coast on the map tile field
	bool CoastOnMap(int x, int y) const;


private:
	/// Build tables for fog of war
	void InitFogOfWar(void);

public:
	CMapField *Fields;                /// fields on map
	unsigned *Visible[PlayerMax];     /// visible bit-field

	bool NoFogOfWar;                  /// fog of war disabled

	CTileset Tileset;                 /// tileset data
	static CGraphic *FogGraphic;      /// graphic for fog of war

	CPatchManager PatchManager;

	CMapInfo Info;                    /// descriptive information
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern CMap Map;  /// The current map
extern char CurrentMapPath[1024]; /// Path to the current map

	/// Vision Table to see where to locate goals and vision
extern unsigned char *VisionTable[3];
	/// Companion table for fast lookups
extern int *VisionLookup;

	/// Flag must reveal the map
extern int FlagRevealMap;
	/// Flag must reveal map when in replay
extern int ReplayRevealMap;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

//
// in map_fog.cpp
//
/// Function to (un)mark the vision table.
typedef void MapMarkerFunc(const CPlayer *player, int x, int y);

	/// Filter map flags through fog
extern int MapFogFilterFlags(CPlayer *player, int x, int y, int mask);
	/// Mark a tile for normal sight
extern MapMarkerFunc MapMarkTileSight;
	/// Unmark a tile for normal sight
extern MapMarkerFunc MapUnmarkTileSight;

	/// Mark sight changes
extern void MapSight(const CPlayer *player, int x, int y, int w,
	int h, int range, MapMarkerFunc *marker);
	/// Mark tiles with fog of war to be redrawn
extern void MapUpdateFogOfWar(int x, int y);
	/// Update fog of war
extern void UpdateFogOfWarChange(void);

	/// Builds Vision and Goal Tables
extern void InitVisionTable(void);
	/// Cleans up Vision and Goal Tables
extern void FreeVisionTable(void);

//
// in map_radar.cpp
//

	/// Check if a unit is visible on radar
extern bool UnitVisibleOnRadar(const CPlayer *pradar, const CUnit *punit);
	/// Check if a tile is visible on radar
extern unsigned char IsTileRadarVisible(const CPlayer *pradar, const CPlayer *punit, int x, int y);
	/// Mark a tile as radar visible, or incrase radar vision
extern void MapMarkTileRadar(const CPlayer *player, int x, int y);
	/// Unmark a tile as radar visible, decrease is visible by other radar
extern void MapUnmarkTileRadar(const CPlayer *player, int x, int y);
	/// Mark a tile as radar jammed, or incrase radar jamming'ness
extern void MapMarkTileRadarJammer(const CPlayer *player, int x, int y);
	/// Unmark a tile as jammed, decrease is jamming'ness
extern void MapUnmarkTileRadarJammer(const CPlayer *player, int x, int y);

//
// in script_map.cpp
//
	/// Set a tile
extern void SetTile(int tile, int w, int h, int value = 0);
	/// register ccl features
extern void MapCclRegister(void);

//
// mixed sources
//
	/// Save a stratagus map (smp format)
extern int SaveStratagusMap(const std::string &filename, CMap *map);


	/// Load map presentation
extern void LoadStratagusMapInfo(const std::string &mapname);
	/// Release info for a map
extern void FreeMapInfo(CMapInfo *info);

	/// Returns true, if the unit-type(mask can enter field with bounds check
extern bool CheckedCanMoveToMask(int x, int y, int mask);
	/// Returns true, if the unit-type can enter the field
extern bool UnitTypeCanBeAt(const CUnitType *type, int x, int y);
	/// Returns true, if the unit can enter the field
extern bool UnitCanBeAt(const CUnit *unit, int x, int y);

	/// Preprocess map, for internal use.
extern void PreprocessMap(void);

// in unit.cpp

/// Mark on vision table the Sight of the unit.
void MapMarkUnitSight(CUnit *unit);
/// Unmark on vision table the Sight of the unit.
void MapUnmarkUnitSight(CUnit *unit);

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

	/// Can a unit with 'mask' enter the field
inline bool CanMoveToMask(int x, int y, int mask) {
	return !(Map.Field(x, y)->Flags & mask);
}

inline void MapMarkSight(const CPlayer *player, int x, int y, int w, int h, int range) {
	MapSight(player, x, y, w, h, range, MapMarkTileSight);
}
inline void MapUnmarkSight(const CPlayer *player, int x, int y, int w, int h, int range) {
	MapSight(player, x, y, w, h, range, MapUnmarkTileSight);
}

	/// Handle Marking and Unmarking of radar vision
inline void MapMarkRadar(const CPlayer *player, int x, int y, int w, int h, int range) {
	MapSight(player, x, y, w, h, range, MapMarkTileRadar);
}
inline void MapUnmarkRadar(const CPlayer *player, int x, int y, int w, int h, int range) {
	MapSight(player, x, y, w, h, range, MapUnmarkTileRadar);
}
	/// Handle Marking and Unmarking of radar vision
inline void MapMarkRadarJammer(const CPlayer *player, int x, int y, int w, int h, int range) {
	MapSight(player, x, y, w, h, range, MapMarkTileRadarJammer);
}
inline void MapUnmarkRadarJammer(const CPlayer *player, int x, int y, int w, int h, int range) {
	MapSight(player, x, y, w, h, range, MapUnmarkTileRadarJammer);
}

//@}

#endif // !__MAP_H__
