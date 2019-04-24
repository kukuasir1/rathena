// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom battle config structure
 * Place the definition of custom battle configuration structure members in this file.
 *
 * Make sure that the name you want is not already taken by looking into battle.hpp
 * Do not forget to add initilization logic to battle_config_init.inc
 *
 * Format:
 *	 <datatype> name;
 **/
	int mvp_tomb_name_type; //设置mob tomb上怪物名称类型, 0: name 1: jname [kuku]
	int	ammo_decrement_weapon_attack; //普通武器攻击弹药是否消耗. 1:消耗, 0:不消耗
