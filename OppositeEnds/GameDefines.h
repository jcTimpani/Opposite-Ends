#pragma once

#include "Defines.h"

enum class eSprite : UINT {
    PlayButton,
    Tile,
    Wall,
    Ice,
    Crate,
	Background,
    PrimaryCharaDown,
    PrimaryCharaLeft,
    PrimaryCharaRight,
    PrimaryCharaUp,
    OppositeCharaDown,
    OppositeCharaLeft,
    OppositeCharaRight,
    OppositeCharaUp,
	PrimaryGoal,
	OppositeGoal,
	SharedGoal,
	PrimaryPortal,
	OppositePortal,
	CratePortal,
	PrimaryPortalDestination,
	OppositePortalDestination,
	CratePortalDestination,
    SharedPortal,
	SharedPortalDestination,
    Size
};
