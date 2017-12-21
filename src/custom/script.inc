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
*		当地图名为 this 时，为玩家所在地图，此时必须有关联玩家。
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
*		返回 0  没找到相关类型的实体
*		返回 >0 为找到的相关数量
*
*		如果找到了相关实体，数组将会保存在 $@areagid 中
*/
static int buildin_getareagid_sub(struct block_list *bl, va_list ap)
{
	//参数要按顺序读取
	//如果使用人物变量,需要将人物的sd传递过来,而不能通过bl获得,此时的bl为循环中的对象
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

/*
* script4each "{<脚本>}",<覆盖范围>{,<参数>...};
* 
* 对指定范围的玩家执行相同的一段脚本
* 
* 全服玩家			- script4each "{<脚本>}",0;
* 指定地图			- script4each "{<脚本>}",1,"地图名";
* 指定区域			- script4each "{<脚本>}",2,"地图名",中心坐标x,中心坐标y,范围;
* 指定玩家所在的队伍	- script4each "{<脚本>}",3,玩家角色编号;
* 指定玩家所在的公会	- script4each "{<脚本>}",4,玩家角色编号;
* 指定区域			- script4each "{<脚本>}",5,"地图名",坐标x0,坐标y0,坐标x1,坐标y1;
* 指定队伍			- script4each "{<脚本>}",6,队伍编号;
* 指定公会			- script4each "{<脚本>}",7,公会编号;
* 
* 警告:
* 	目前 script4each 所执行的脚本中, 禁止出现会导致脚本进入暂停状态的指令.
* 	比如 sleep、sleep2、next、close2、menu、select、prompt、input、progressbar 等, 他们的特点是执行这些指令时, 
* 	都需要等待客户端/玩家返回一定的信息或做出选择才能继续往下执行.
* 
* 返回值:
* 	该指令无论成功失败, 都不会有返回值
*/
static int buildin_script4each_sub(struct block_list *bl, va_list ap)
{
	struct script_code *script = va_arg(ap, struct script_code *);
	run_script(script, 0, bl->id, 0);
	return 0;
}


BUILDIN_FUNC(script4each) {

	struct map_session_data *pl_sd = NULL, *sd = NULL;
	struct s_mapiterator *iter = NULL;
	struct script_code *script = NULL;

	const char *script_str = NULL, *mapname = NULL;
	int type, map_id;

	//获得执行脚本(去空格)
	script_str = skip_space(script_getstr(st, 2));
	//检查脚本格式,其实可以不写,parse_script也会检查格式
	if (script_str == NULL || (script_str[0] && script_str[0] != '{' && script_str[strlen(script_str) - 1] != '}'))
		return SCRIPT_CMD_SUCCESS;

	if ( !( script = parse_script(script_str, "script4each", 0, 0) ) )
		return SCRIPT_CMD_SUCCESS;

	type = script_getnum(st, 3);
	//如果超出0-7的范围则失败

	switch (type) {
	case 0:
		//获得服务器所有人物的迭代器
		iter = mapit_getallusers();
		//遍历迭代器
		for (pl_sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); pl_sd = (TBL_PC*)mapit_next(iter))
			run_script(script, 0, pl_sd->bl.id, 0);
		mapit_free(iter);
		break;
	case 1:
		mapname = script_getstr(st, 4);
		map_id = map_mapname2mapid(mapname);
		if (map_id < 0)
			return SCRIPT_CMD_SUCCESS;
		map_foreachinmap(buildin_script4each_sub, map_id, BL_PC, script);
		break;
	case 2:
		struct block_list center;
		int range;
		mapname = script_getstr(st, 4);
		map_id = map_mapname2mapid(mapname);
		if (map_id < 0)
			return SCRIPT_CMD_SUCCESS;
		center.m = map_id;
		center.x = script_getnum(st, 5);
		center.y = script_getnum(st, 6);
		range = script_getnum(st, 7);

		map_foreachinrange(buildin_script4each_sub, &center, range, BL_PC, script);
		break;
	case 3:
		script_charid2sd(5, sd);
		iter = mapit_getallusers();
		for (pl_sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); pl_sd = (TBL_PC*)mapit_next(iter)) {
			if (pl_sd->status.party_id == sd->status.party_id)
				run_script(script, 0, pl_sd->bl.id, 0);
		}
		mapit_free(iter);
		break;
	case 4:
		script_charid2sd(5, sd);
		iter = mapit_getallusers();
		for (pl_sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); pl_sd = (TBL_PC*)mapit_next(iter)) {
			if (pl_sd->status.guild_id == sd->status.guild_id)
				run_script(script, 0, pl_sd->bl.id, 0);
		}
		mapit_free(iter);
		break;
	case 5:
		int x0, y0, x1, y1;
		mapname = script_getstr(st, 4);
		map_id = map_mapname2mapid(mapname);
		if (map_id < 0) {
			return SCRIPT_CMD_SUCCESS;
		}
		x0 = script_getnum(st, 5);
		y0 = script_getnum(st, 6);
		x1 = script_getnum(st, 7);
		y1 = script_getnum(st, 8);

		map_foreachinarea(buildin_script4each_sub, map_id, x0, y0, x1, y1, BL_PC, script);
		break;
	case 6:
		int party_id;
		party_id = script_getnum(st, 4);
		iter = mapit_getallusers();
		for (pl_sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); pl_sd = (TBL_PC*)mapit_next(iter)) {
			if (pl_sd->status.party_id == party_id)
				run_script(script, 0, pl_sd->bl.id, 0);
		}
		mapit_free(iter);
		break;
	case 7:
		int guild_id;
		guild_id = script_getnum(st, 4);
		iter = mapit_getallusers();
		for (pl_sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); pl_sd = (TBL_PC*)mapit_next(iter)) {
			if (pl_sd->status.guild_id == guild_id)
				run_script(script, 0, pl_sd->bl.id, 0);
		}
		mapit_free(iter);
		break;
	}
	return SCRIPT_CMD_SUCCESS;
}

//delinventory <背包位置序号>,<要移除的数量>{,<char_id>};
BUILDIN_FUNC(delinventory)
{
	TBL_PC *sd;
	int idx, count;
	if (!script_charid2sd(4, sd))
		return SCRIPT_CMD_FAILURE;

	idx = script_getnum(st, 2);
	count = script_getnum(st, 3);

	//pc_delitem中对错误情况进行了判断,所以无需自己判断
	if (pc_delitem(sd, idx, count, 0, 0, LOG_TYPE_OTHER))
		script_pushint(st, 0); //失败返回0
	else
		script_pushint(st, 1); //成功返回1
	return SCRIPT_CMD_SUCCESS;
}

//countinventory <背包位置序号>{,<char_id>};
BUILDIN_FUNC(countinventory)
{
	TBL_PC *sd;
	int idx, count;
	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_FAILURE;

	idx = script_getnum(st, 2);
	if (idx < 0 || idx >= MAX_INVENTORY || sd->inventory.u.items_inventory[idx].nameid <= 0 || sd->inventory_data[idx] == NULL) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	count = sd->inventory.u.items_inventory[idx].amount;
	script_pushint(st, count);
	return SCRIPT_CMD_SUCCESS;
}

//identifybyidx <背包位置序号>{,<char_id>};	
BUILDIN_FUNC(identifybyidx)
{
	TBL_PC *sd;
	int idx;
	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_FAILURE;

	idx = script_getnum(st, 2);

	if (idx < 0 || idx >= MAX_INVENTORY || sd->inventory.u.items_inventory[idx].nameid <= 0 || sd->inventory_data[idx] == NULL) {
		return SCRIPT_CMD_SUCCESS;
	}

	sd->inventory.u.items_inventory[idx].identify = 1;
	clif_item_identified(sd, idx, 0);

	return SCRIPT_CMD_SUCCESS;
}

//equipinventory <背包位置序号>{,<char_id>};
BUILDIN_FUNC(equipinventory)
{
	TBL_PC *sd;
	struct item_data *item_data;
	int idx;

	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_FAILURE;

	idx = script_getnum(st, 2);

	//pc_equipitem其实已经进行了判断,但是要获得item_data->equip,必须对idx进行判断
	if (idx < 0 || idx >= MAX_INVENTORY || sd->inventory.u.items_inventory[idx].nameid <= 0 || sd->inventory_data[idx] == NULL)
		script_pushint(st, 0);
	else if ((item_data = itemdb_exists(sd->inventory.u.items_inventory[idx].nameid))) {
		if (pc_equipitem(sd, idx, item_data->equip))
			script_pushint(st, 1);
		else
			script_pushint(st, 0);
	}
	return SCRIPT_CMD_SUCCESS;
}

//unequipinventory <背包位置序号>{,<char_id>};
BUILDIN_FUNC(unequipinventory)
{
	TBL_PC *sd;
	struct item_data *item_data;
	int idx;

	if (!script_charid2sd(3, sd))
		return SCRIPT_CMD_FAILURE;

	idx = script_getnum(st, 2);

	if (idx < 0 || idx >= MAX_INVENTORY || sd->inventory.u.items_inventory[idx].nameid <= 0 || sd->inventory_data[idx] == NULL)
		script_pushint(st, 0);
	else if ((item_data = itemdb_exists(sd->inventory.u.items_inventory[idx].nameid))) {
		if (pc_unequipitem(sd, idx, item_data->equip))
			script_pushint(st, 1);
		else
			script_pushint(st, 0);
	}
	return SCRIPT_CMD_SUCCESS;
}

//getinventoryinfo <背包位置序号>,<要查看的信息类型>{,<char_id>};
BUILDIN_FUNC(getinventoryinfo)
{
	TBL_PC *sd;

	int idx, type;

	if (!script_charid2sd(4, sd))
		return SCRIPT_CMD_FAILURE;

	idx = script_getnum(st, 2);
	type = script_getnum(st, 3);

	if (idx < 0 || idx >= MAX_INVENTORY || sd->inventory.u.items_inventory[idx].nameid <= 0 || sd->inventory_data[idx] == NULL || type < 0 || type > 11) {
		if (type == 11)
			script_pushstrcopy(st, "-1");
		else
			script_pushint(st, -1);
		return SCRIPT_CMD_SUCCESS;
	}

	switch (type) {
	case 0:
		unsigned short nameid;
		nameid = sd->inventory.u.items_inventory[idx].nameid;
		script_pushint(st, nameid);
		break;
	case 1:
		short amount;
		amount = sd->inventory.u.items_inventory[idx].amount;
		script_pushint(st, amount);
		break;
	case 2:
		unsigned int equip;
		equip = sd->inventory.u.items_inventory[idx].equip;
		script_pushint(st, equip);
		break;
	case 3:
		char refine;
		refine = sd->inventory.u.items_inventory[idx].refine;
		script_pushint(st, refine);
		break;
	case 4:
		char identify;
		identify = sd->inventory.u.items_inventory[idx].identify;
		script_pushint(st, identify);
		break;
	case 5:
		char attribute;
		attribute = sd->inventory.u.items_inventory[idx].attribute;
		script_pushint(st, attribute);
		break;
	case 6:
	case 7:
	case 8:
	case 9:
		unsigned short card;
		card = sd->inventory.u.items_inventory[idx].card[type - 6];
		script_pushint(st, card);
		break;
	case 10:
		unsigned int expire_time;
		expire_time = sd->inventory.u.items_inventory[idx].expire_time;
		script_pushint(st, expire_time);
		break;
	case 11:
		char uid[CHAT_SIZE_MAX];
		safesnprintf(uid, sizeof(uid), "%llu", (unsigned long long)sd->inventory.u.items_inventory[idx].unique_id);
		script_pushstrcopy(st, uid);
		break;
	}
	return SCRIPT_CMD_SUCCESS;
}

//searcharray <数组名>,<搜索的内容(可以是字符串或是数字)>
//成功范围第一个搜索到的索引,失败返回-1
BUILDIN_FUNC(searcharray)
{
	struct script_data* data;
	const char *varname = NULL;
	struct map_session_data *sd = NULL;
	int j = -1; //返回值默认为-1
	unsigned int start, end;

	data = script_getdata(st, 2);
	//判断是否为变量
	if (!data_isreference(data)) {
		ShowError("buildin_searcharray: not a variable\n");
		return SCRIPT_CMD_FAILURE;// not a variable
	}

	//获取变量名
	varname = reference_getname(data);

	//判断是否为人物变量
	if (not_server_variable(*varname)) {
		//人物变量则判断是否关联人物
		if (!script_rid2sd(sd)) {
			return SCRIPT_CMD_FAILURE;
		}
	}
	//判断是否为数组
	if (!script_array_src(st, sd, varname, reference_getref(data))) {
		ShowError("buildin_searcharray: not a array!\n");
		return SCRIPT_CMD_FAILURE;
	}

	//获取数组起始索引
	start = reference_getindex(data);
	//获取数组结束索引
	end = script_array_highest_key(st, sd, varname, reference_getref(data));

	//排除空数组
	if (start < end) {
		int id;
		//获取数组的uid
		id = reference_getid(data);
		for (; start < end; start++) {
			//如果是字符串型数组
			if (is_string_variable(varname)) {
				//取得start索引的的数组字符串元素,放入栈顶
				char *str = (char*)get_val2(st, reference_uid(id, start), reference_getref(data));
				int x = strcmp(str, script_getstr(st, 3));
				//get_val2使用之后必须手动清除栈顶,否则script_pushint将出错
				script_removetop(st, -1, 0);
				if (x == 0) {
					j = start;
					break;
				}
			}
			else {
				//取得start索引的的数组数字元素,放入栈顶
				int32 num = (int32)__64BPRTSIZE(get_val2(st, reference_uid(id, start), reference_getref(data)));
				bool x = (num == script_getnum(st, 3));
				//get_val2使用之后必须手动清除栈顶,否则script_pushint将出错
				script_removetop(st, -1, 0);
				if (x) {
					j = start;
					break;
				}
			}
		}
	}
	script_pushint(st, j);
	return SCRIPT_CMD_SUCCESS;
}