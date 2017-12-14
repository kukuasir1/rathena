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

/*
* 根据GID让指定对象显示effect [kuku]
* uniteffect <GID>,<effect number>{,<send_target>}
*/
BUILDIN_FUNC(uniteffect) {
	struct block_list* bl;
	//effect编号
	int type = script_getnum(st, 3);
	//发送目标,默认为AREA
	enum send_target target = script_hasdata(st, 4) ? (send_target)script_getnum(st, 4) : AREA;
	//根据参数GID获取bl
	if (script_rid2bl(2, bl))
		clif_specialeffect(bl, type, target);
	return SCRIPT_CMD_SUCCESS;
}

/*根据gid删除对象
* unitremove <GID>{,<Type>};
* type:
* CLR_OUTSIGHT / CLR_DEAD / CLR_RESPAWN / CLR_TELEPORT / CLR_TRICKDEAD
* type:
*    0 = out of sight
*    1 = died
*    2 = logged out
*    3 = teleport
*    4 = trickdead
*/
BUILDIN_FUNC(unitremove)
{
	struct block_list *bl;
	int type;
	enum clr_type clrtype;

	//根据第一个参数GID获取bl
	if (!script_rid2bl(2, bl))
		//如果bl不存在跳出
		return SCRIPT_CMD_SUCCESS;

	//判断是否有第2个参数<Type>,有就是用参数,没有就用0
	type = script_hasdata(st, 3) ? script_getnum(st, 3) : 0;
	//如果使用错误的type则强制用0
	if (type < 0 || type > 4) type = 0;
	//强制转换成clr_type
	clrtype = (enum clr_type)type;

	//根据clrtype显示消失的效果
	clif_clearunit_area(bl, clrtype);

	//根据bl的类型进行判断
	switch (bl->type) {
	case BL_PC: //人物
		//人物断线
		map_quit((struct map_session_data *) bl);
		break;
	case BL_MOB: //怪物
		//怪物释放
		unit_free(bl, CLR_OUTSIGHT);
		break;
	case BL_ITEM: //道具
		//清除掉落地面的指定道具
		map_clearflooritem(bl);
		break;
	case BL_NPC: //NPC
		//卸载NPC
		npc_unload((struct npc_data *)bl, true);
		break;
	case BL_SKILL: //技能
		//移除指定技能对象
		skill_delunit((struct skill_unit *) bl);
		break;
	case BL_MER: //佣兵
		//删除佣兵
		mercenary_delete((struct mercenary_data *) bl, 1);
		break;
	case BL_PET: //宠物
		//还原为宠物蛋
		pet_menu(((struct pet_data *)bl)->master, 3);
		break;
	case BL_ELEM: //元素
		//移除指定对象
		elemental_delete((struct elemental_data *)bl);
		break;
	case BL_HOM: //生命体
		//删除生命体
		hom_menu(((struct homun_data *)bl)->master, 2);
		break;
	default: //其它类型则报错
		ShowError("buildin_unitremove: Do not support remove this block, type: '%d'. \n", bl->type);
		return SCRIPT_CMD_FAILURE;
	}
	return SCRIPT_CMD_SUCCESS;
}


/* 获得指定区域内的所有GID
* getareagid("<地图名>",<类型>,<x0>,<y0>,<x1>,<y1>,<arrayname>);
* getareagid("<地图名>",<类型>,<中心坐标x>,<中心坐标y>,<半径>,<arrayname>);
* getareagid("<地图名>",<类型>,<arrayname>);
*		第一种方法为获取区域内 GID；
*		第二种方法为获取中心坐标周围的 GID；
*		第三种方法为获取地图上的 GID；
*
*		当地图名为 Location 时，为玩家所在地图，此时必须有关联玩家。
*		类型：BL_PC
*			  BL_MOB
*			  BL_PET
*			  BL_HOM
*			  BL_MER
*			  BL_ITEM
*			  BL_SKILL
*			  BL_NPC
*			  BL_CHAT
*			  BL_ELEM
*
*		返回 -2 为没找到地图
*		返回 -1 指定地图为 Location 但是没有关联玩家
*		返回 0  没找到相关类型的实体
*		返回 >0 为找到的相关数量
*
*		如果找到了相关实体，数组将会保存在 $@areagid 中
*/
static int buildin_getareagid_sub(struct block_list *bl, va_list ap)
{
	//参数要按顺序读取
	struct script_state *st = va_arg(ap, struct script_state *);
	struct map_session_data *sd = va_arg(ap, struct map_session_data *);
	const char *varname = va_arg(ap,const char *);
	int *count = va_arg(ap, int *);
	setd_sub(st, sd, varname, *count, (void *)__64BPRTSIZE(bl->id), NULL);
	(*count)++;
	return 0;
}

BUILDIN_FUNC(getareagid)
{
	int map_id, type, count;
	const char *mapname = NULL, *varname = NULL;
	struct script_data *data = NULL;
	struct map_session_data *sd = NULL;

	//获得发送目标类型
	type = script_getnum(st, 3);
	//获得地图名
	mapname = script_getstr(st, 2);

	//获得数组参数(最后一个变量)
	data = script_getdata(st, script_lastdata(st));
	if (!data_isreference(data)) {
		//不是变量报错
		ShowWarning("buildin_getareagid: Error in argument! Please give a variable to store values in.\n");
		return SCRIPT_CMD_FAILURE;
	}
	//获得数组名
	varname = reference_getname(data);

	//如果为人物变量或者地图名为this,则判断人物是否关联
	if (not_server_variable(*varname) || !strcmp(mapname, "this")) {
		//人物变量则判断是否关联人物
		if (!script_rid2sd(sd)) {
			return SCRIPT_CMD_FAILURE;
		}
	}

	//如果地图为this则根据关联的人物获得地图id,否则通过地图名获得id
	map_id = (strcmp(mapname, "this") ? map_mapname2mapid(mapname) : map_mapindex2mapid(sd->mapindex));

	if (map_id < 0) {
		//地图不存在返回-2
		script_pushint(st, -2);
		return SCRIPT_CMD_SUCCESS;
	}

	//计数器初始为0
	count = 0;
	//如果7个参数(方法1)
	if (script_hasdata(st, 8)) {
		int x0, y0, x1, y1;
		x0 = script_getnum(st, 4);
		y0 = script_getnum(st, 5);
		x1 = script_getnum(st, 6);
		y1 = script_getnum(st, 7);
		map_foreachinarea(buildin_getareagid_sub, map_id, x0, y0, x1, y1, type, st, sd, varname, &count);
	}
	//如果6个参数(方法2)
	else if (script_hasdata(st, 7)) {
		struct block_list center;
		int range;
		center.m = map_id;
		center.x = script_getnum(st, 4);
		center.y = script_getnum(st, 5);
		range = script_getnum(st, 6);
		map_foreachinrange(buildin_getareagid_sub, &center, range, type, st, sd, varname, &count);
	}
	//(方法3)
	else {
		map_foreachinmap(buildin_getareagid_sub, map_id, type, st, sd, varname, &count);
	}
	//返回获取的gid数量
	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}