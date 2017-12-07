// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom Script Commands
 * Place the body of custom script commands in this file.
 * Format:
 *	BUILDIN_FUNC(command_name)
 *	{
 *		<code>;
 *	}
 **/

//BUILDIN_FUNC(example)
//{
//	ShowInfo("buildin_example: It works!\n");
//	script_pushint(st,1);
//	return 0;
//}

//mobeffect <GID>,<effect_type>; [kuku]
BUILDIN_FUNC(mobeffect) {
	int type = script_getnum(st,3);
	struct block_list *bl = map_id2bl(script_getnum(st,2));
	if (bl) {
		clif_specialeffect(bl,type,AREA);
	}
	return SCRIPT_CMD_SUCCESS;
}

//根据gid删除对象
// unitremove <GID>{,<Type>};
// type:
// CLR_OUTSIGHT / CLR_DEAD / CLR_RESPAWN / CLR_TELEPORT / CLR_TRICKDEAD
// type:
//    0 = out of sight
//    1 = died
//    2 = logged out
//    3 = teleport
//    4 = trickdead
BUILDIN_FUNC(unitremove)
{
	struct block_list *bl;
	int gid, type;
	enum clr_type clrtype;

	gid = script_getnum(st, 2);
	bl = map_id2bl(gid);

	if (bl == NULL)
		return SCRIPT_CMD_SUCCESS;

	type = script_hasdata(st, 3) ? script_getnum(st, 3) : CLR_OUTSIGHT;
	if (type < 0 || type > 4) type = 0;
	clrtype = (enum clr_type)(type);

	clif_clearunit_area(bl, clrtype);

	switch (bl->type) {
	case BL_PC:
		map_quit((struct map_session_data *) bl);
		break;
	case BL_MOB:
		unit_free(bl, CLR_OUTSIGHT);
		break;
	case BL_ITEM:
		map_clearflooritem(bl);
		break;
	case BL_NPC:
		npc_unload((struct npc_data *)bl, true);
		break;
	case BL_SKILL:
		skill_delunit((struct skill_unit *) bl);
		break;
	case BL_MER:
		mercenary_delete((struct mercenary_data *) bl, 1);
		break;
	case BL_PET:
		pet_menu(((struct pet_data *)bl)->master, 3);
		break;
	case BL_ELEM:
		elemental_delete((struct elemental_data *)bl);
		break;
	case BL_HOM:
		hom_menu(((struct homun_data *)bl)->master, 2);
		break;
	default:
		ShowError("buildin_unitremove: Do not support remove this block, type: '%d'. \n", bl->type);
		return SCRIPT_CMD_FAILURE;
	}
	return SCRIPT_CMD_SUCCESS;
}


static int buildin_getareagid_sub(struct block_list *bl, va_list ap)
{
	int *count = va_arg(ap, int *);
	mapreg_setreg(reference_uid(add_str("$@areagid"), (*count)), bl->id);
	(*count)++;
	return 0;
}


//*getareagid("<地图名>",<类型>,<x0>,<y0>,<x1>,<y1>);
//*getareagid("<地图名>",<类型>,<中心坐标x>,<中心坐标y>,<半径>);
//*getareagid("<地图名>",<类型>);
//		第一种方法为获取区域内 GID；
//		第二种方法为获取中心坐标周围的 GID；
//		第三种方法为获取地图上的 GID；
//
//		当地图名为 Location 时，为玩家所在地图，此时必须有关联玩家。
//		类型：BL_PC
//			  BL_MOB
//			  BL_PET
//			  BL_HOM
//			  BL_MER
//			  BL_ITEM
//			  BL_SKILL
//			  BL_NPC
//			  BL_CHAT
//			  BL_ELEM
//
//		返回 -2 为没找到地图
//		返回 -1 指定地图为 Location 但是没有关联玩家
//		返回 0  没找到相关类型的实体
//		返回 >0 为找到的相关数量
//		
//		如果找到了相关实体，数组将会保存在 $@areagid 中
BUILDIN_FUNC(getareagid)
{
	int map_id, type, count;
	const char* mapname = NULL;
	type = script_getnum(st, 3);
	mapname = script_getstr(st, 2);

	if (strcmp(mapname, "this") == 0) {
		TBL_PC *sd;
		if (!script_rid2sd(sd))
			return SCRIPT_CMD_SUCCESS;
		map_id = map_mapindex2mapid(sd->mapindex);
	}
	else
		map_id = map_mapname2mapid(mapname);

	if (map_id < 0) {
		script_pushint(st, -2);
		return SCRIPT_CMD_SUCCESS;
	}
	count = 0;
	if (script_hasdata(st, 7)) {
		int x0, y0, x1, y1;
		x0 = script_getnum(st, 4);
		y0 = script_getnum(st, 5);
		x1 = script_getnum(st, 6);
		y1 = script_getnum(st, 7);
		map_foreachinarea(buildin_getareagid_sub, map_id, x0, y0, x1, y1, type, &count);
	}
	else {
		struct block_list center;
		int range;
		if (script_hasdata(st, 6)) {
			center.m = map_id;
			center.x = script_getnum(st, 4);
			center.y = script_getnum(st, 5);
			range = script_getnum(st, 6);
			map_foreachinrange(buildin_getareagid_sub, &center, range, type, &count);
		}
		else {
			map_foreachinmap(buildin_getareagid_sub, map_id, type, &count);
		}
	}
	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}