// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom battle config initializations
 * Place the definition of custom battle configuration initializations in this file.
 *
 * Do not forget to add a comma at the end of the line, if you want to add another config
 *
 * Format:
 *	 { "name", &battle_config.<variable name>, <default value>, <minimum value>, <maximum value> },
 **/

	{ "mvp_tomb_name_type",                &battle_config.mvp_tomb_name_type,             1,      0,      1,    },
	{ "ammo_decrement_weapon_attack",      &battle_config.ammo_decrement_weapon_attack,   1,      0,      1,    },
