// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom Script Commands
 * Place the definition of custom script commands in this file.
 * Format:
 *	BUILDIN_DEF(command_name,"parameters"),
 **/

//BUILDIN_DEF(example,""),

BUILDIN_DEF(uniteffect,"ii?"),
BUILDIN_DEF2(uniteffect,"mobeffect","ii?"),
BUILDIN_DEF(unitremove, "i?"),
BUILDIN_DEF(getareagid, "si*"),
BUILDIN_DEF(script4each, "si*"),
BUILDIN_DEF(delinventory, "ii?"),
BUILDIN_DEF(countinventory, "i?"),
BUILDIN_DEF(identifybyidx, "i?"),
BUILDIN_DEF(equipinventory, "i?"),
BUILDIN_DEF(unequipinventory, "i?"),
BUILDIN_DEF(getinventoryinfo, "ii?"),
BUILDIN_DEF(searcharray, "rv"),
BUILDIN_DEF(setrenttime, "ii?"),
BUILDIN_DEF2(setrenttime, "resume", "ii?"),
BUILDIN_DEF(getequipexpiretick, "i?"),
BUILDIN_DEF2(getequipexpiretick, "isrental", "i?"),
BUILDIN_DEF(getunittarget, "i"),
BUILDIN_DEF(getunitmapxy, "irrr"),
BUILDIN_DEF(mobskill, "iviiii??"),