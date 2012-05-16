#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "PK_thread.h"
#include "log.h"
#include "ini_file.h"
#include <algorithm>

int PK_PE::Init(Main_thread *pMain_thread, char *input, int length) {
	m_pMain_thread = pMain_thread;
	short int map_x = 0;
	short int map_y = 0;
	short int role_count = 0;
	if (length < 8) //input必须大于等于8bytes
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "Init length < 8:[%d],length ", length);
		return -1;
	}
	else
	{

		short int current_input_length = 0;
		memcpy(&m_PK_ID, input + current_input_length, 4);
		current_input_length += 4;
		memcpy(&map_id, input + current_input_length, 2);
		current_input_length += 2;
		memcpy(&map_x, input + current_input_length, 2);
		current_input_length += 2;
		memcpy(&map_y, input + current_input_length, 2);
		current_input_length += 2;
		
//LOG(LOG_DEBUG, __FILE__, __LINE__, "PK_ID[%d],map_ID[%d],MAP_X[%d],map_Y[%d]", m_PK_ID, map_id, map_x, map_y);

		map<unsigned int, MapDesc>::iterator pos; //地图指针
		pos = option.map_map.find(map_id);
		if (pos == option.map_map.end())
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, " mapID not found, map_id[%d] ", map_id);
			return -1;
		}
//LOG(LOG_DEBUG, __FILE__, __LINE__, "map_width[%d], map_Height[%d]",			pos->second.Width, pos->second.Height);

		if (pos->second.Height - (map_y - 15) < MAP_SIZE_Y) {
			map_y = pos->second.Height - 1 - MAP_SIZE_Y + 15;
			*(input + 8) = map_y;
		} else if (map_y < 15) {
			map_y = 15;
			*(input + 8) = map_y;
		}
		if (pos->second.Width - (map_x - 25) < MAP_SIZE_X) {
			map_x = pos->second.Width - 1 - MAP_SIZE_X + 25;
			*(input + 6) = map_x;
		} else if (map_x < 25) {
			map_x = 25;
			*(input + 6) = map_x;
		}

//		LOG(LOG_DEBUG, __FILE__, __LINE__, "    01234567890123456789012345678901234567890123456789");
		
		//最上面5行，全部置为0，不让游戏人物跑到最上面
//		for (int i = 0;i<MAP_SIZE_X;i++)
//		pk_map_base[0] = {0};
//		pk_map_base[1] = {0};
		int forbiddenY = 5;
		for (int pk_map_height = forbiddenY, height_loop = map_y - 15; pk_map_height
				< MAP_SIZE_Y; pk_map_height++, height_loop++) {
//			memcpy(
//					pk_map_base[pk_map_height],
//					pos->second.Map + (pos->second.Width * height_loop) + map_x
//							- 25, MAP_SIZE_X);

//			char buf[MAP_SIZE_X + 1] = { 0 };
			for (int x = 0; x < MAP_SIZE_X; x++) {
				//避免前后台的路径错误，强行置成全部可以行走，不管地图是否有不可行走区
				pk_map_base[pk_map_height][x]=1;
//				sprintf(buf + x, "%d ", pk_map_base[pk_map_height][x]);
			}
//			LOG(LOG_DEBUG, __FILE__, __LINE__, "%2d: %s ", pk_map_height, buf);
		}

		memcpy(&role_count, input + current_input_length, 2);
//		LOG(LOG_DEBUG, __FILE__, __LINE__, "role_count[%d]", role_count);
		current_input_length += 2;
		for (short int role_pos = 0; role_pos < role_count; role_pos++) {
			if (length - current_input_length < 61) {
				LOG(LOG_ERROR, __FILE__, __LINE__, "%d ",
						length - current_input_length);
				return -1;
			} else {
				RoleState stRoleState_tmp;
				memcpy(&stRoleState_tmp.ControlID,
						input + current_input_length, 4);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "ControlID[%d]",
//						stRoleState_tmp.ControlID);
				current_input_length += 4;
				memcpy(&stRoleState_tmp.RoleID, input + current_input_length, 4);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "RoleID[%d]",
//						stRoleState_tmp.RoleID);
				current_input_length += 4;
				memcpy(&stRoleState_tmp.Level, input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "Level[%d]",
//						stRoleState_tmp.Level);
				current_input_length += 2;
				//Oppostion 立场阵营
				memcpy(&stRoleState_tmp.Opposition,
						input + current_input_length, 1);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "Opposition[%d]",
//						stRoleState_tmp.Opposition);
				current_input_length += 1;
				memcpy(&stRoleState_tmp.RoleFlag, input + current_input_length,
						1);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "RoleFlag[%d]",
//						stRoleState_tmp.RoleFlag);
				current_input_length += 1;
				memcpy(&stRoleState_tmp.RoleType, input + current_input_length,
						4);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "RoleType[%d]",
//						stRoleState_tmp.RoleType);
				current_input_length += 4;
				//memcpy(&stRoleState_tmp.Origin, input+current_input_length, 1);
				//current_input_length += 1;
				//memcpy(&stRoleState_tmp.Live, input+current_input_length, 1);
				//current_input_length += 1;
				memcpy(&stRoleState_tmp.HP_MAX, input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "HP_MAX[%d]",
//						stRoleState_tmp.HP_MAX);
				current_input_length += 2;

				memcpy(&stRoleState_tmp.MP_MAX, input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "MP_MAX[%d]",
//						stRoleState_tmp.MP_MAX);
				current_input_length += 2;

				memcpy(&stRoleState_tmp.HP, input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "HP[%d]", stRoleState_tmp.HP);
				current_input_length += 2;

				memcpy(&stRoleState_tmp.MP, input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "MP[%d]", stRoleState_tmp.MP);
				current_input_length += 2;
				memcpy(&stRoleState_tmp.MoveAttrb.SpeedMod_Base,
						input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "SpeedMod_Base[%d]",
//						stRoleState_tmp.MoveAttrb.SpeedMod_Base);
				current_input_length += 2;

				memcpy(&stRoleState_tmp.MoveAttrb.CurrentPos.X,
						input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "CurrentPos.X[%d]",
//						stRoleState_tmp.MoveAttrb.CurrentPos.X);
				current_input_length += 2;
				memcpy(&stRoleState_tmp.MoveAttrb.CurrentPos.Y,
						input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "CurrentPos.Y[%d]",
//						stRoleState_tmp.MoveAttrb.CurrentPos.Y);
				current_input_length += 2;

				if ((stRoleState_tmp.MoveAttrb.CurrentPos.Y >= MAP_SIZE_Y)
						|| (stRoleState_tmp.MoveAttrb.CurrentPos.Y < 0)
						|| (stRoleState_tmp.MoveAttrb.CurrentPos.X
								>= MAP_SIZE_X)
						|| (stRoleState_tmp.MoveAttrb.CurrentPos.X < 0)) {
					LOG(LOG_ERROR, __FILE__, __LINE__,
							"%d:[%d,%d] pos error! ", stRoleState_tmp.RoleID,
							stRoleState_tmp.MoveAttrb.CurrentPos.X,
							stRoleState_tmp.MoveAttrb.CurrentPos.Y);
					return -1;
				}

				memcpy(&stRoleState_tmp.MoveAttrb.Direct,
						input + current_input_length, 1);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "Direct[%d]",
//						stRoleState_tmp.MoveAttrb.Direct);
				current_input_length += 1;
				if (pk_map_base[stRoleState_tmp.MoveAttrb.CurrentPos.Y][stRoleState_tmp.MoveAttrb.CurrentPos.X]
						== 0) {
					Pos newPos;
					GetEmptyPos(stRoleState_tmp.MoveAttrb.CurrentPos.X,
							stRoleState_tmp.MoveAttrb.CurrentPos.Y,
							stRoleState_tmp.MoveAttrb.Direct, &newPos);
					stRoleState_tmp.MoveAttrb.CurrentPos.X = newPos.X;
					stRoleState_tmp.MoveAttrb.CurrentPos.Y = newPos.Y;
					*(input + current_input_length - 5) = newPos.X;
					*(input + current_input_length - 3) = newPos.Y;
				}

				memcpy(&stRoleState_tmp.PhysicalAttrib.AttackPowerHign_Base,
						input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "AttackPowerHign_Base[%d]",
//						stRoleState_tmp.PhysicalAttrib.AttackPowerHign_Base);
				current_input_length += 2;

				memcpy(&stRoleState_tmp.PhysicalAttrib.AttackPowerLow_Base,
						input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "AttackPowerLow_Base[%d]",
//						stRoleState_tmp.PhysicalAttrib.AttackPowerLow_Base);
				current_input_length += 2;
				memcpy(&stRoleState_tmp.PhysicalAttrib.AttackArea,
						input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "AttackArea[%d]",
//						stRoleState_tmp.PhysicalAttrib.AttackArea);
				current_input_length += 2;
				memcpy(&stRoleState_tmp.PhysicalAttrib.AttackCDTime_Base,
						input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "AttackCDTime_Base[%d]",
//						stRoleState_tmp.PhysicalAttrib.AttackCDTime_Base);
				current_input_length += 2;
				memcpy(&stRoleState_tmp.PhysicalAttrib.AttackIngTime,
						input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "AttackIngTime[%d]",
//						stRoleState_tmp.PhysicalAttrib.AttackIngTime);
				current_input_length += 2;
				memcpy(&stRoleState_tmp.PhysicalAttrib.AttackBulletMod,
						input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "AttackBulletMod[%d]",
//						stRoleState_tmp.PhysicalAttrib.AttackBulletMod);
				current_input_length += 2;
				memcpy(&stRoleState_tmp.PhysicalAttrib.AttackHit_Base,
						input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "AttackHit_Base[%d]",
//						stRoleState_tmp.PhysicalAttrib.AttackHit_Base);
				current_input_length += 2;
				memcpy(&stRoleState_tmp.PhysicalAttrib.DefenseMiss_Base,
						input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "DefenseMiss_Base[%d]",
//						stRoleState_tmp.PhysicalAttrib.DefenseMiss_Base);
				current_input_length += 2;
				memcpy(&stRoleState_tmp.PhysicalAttrib.AttackCrit_Base,
						input + current_input_length, 2);

				current_input_length += 2;
//				LOG(LOG_DEBUG, __FILE__, __LINE__, "AttackCrit_Base[%d]",
//						stRoleState_tmp.PhysicalAttrib.AttackCrit_Base);

				memcpy(&stRoleState_tmp.PhysicalAttrib.DefenseArmor_Base,
						input + current_input_length, 2);

				current_input_length += 2;
//				LOG(LOG_DEBUG, __FILE__, __LINE__,
//						"RoleID = %d,DefenseArmor_Base = %d",
//						stRoleState_tmp.RoleID,
//						stRoleState_tmp.PhysicalAttrib.DefenseArmor_Base);
				memcpy(&stRoleState_tmp.MagicAttrb.MagicArmor_Base,
						input + current_input_length, 2);
//				LOG(LOG_DEBUG, __FILE__, __LINE__,
//						"MagicAttrb.MagicArmor_Base[%d]",
//						stRoleState_tmp.RoleID,
//						stRoleState_tmp.MagicAttrb.MagicArmor_Base);
				current_input_length += 2;
				memcpy(&stRoleState_tmp.Strength, input + current_input_length,
						2);
				current_input_length += 2;
				memcpy(&stRoleState_tmp.Agile, input + current_input_length, 2);
				current_input_length += 2;
				memcpy(&stRoleState_tmp.Wisdom, input + current_input_length, 2);
				current_input_length += 2;
//				LOG(LOG_DEBUG, __FILE__, __LINE__,
//						"liliang[%d],minjie[%d],zhili[%d]",
//						stRoleState_tmp.Strength, stRoleState_tmp.Agile,
//						stRoleState_tmp.Wisdom);


				//计算真实的攻击速度，按照公式40/(1+sqrt(base)/20)
				stRoleState_tmp.PhysicalAttrib.AttackCDTime_Base
						= 40.0
								/ (1.0
										+ sqrt(
												stRoleState_tmp.PhysicalAttrib.AttackCDTime_Base)
												/ 20.0);

//				LOG(LOG_DEBUG, __FILE__, __LINE__, "11111111AttackCDTime_Base[%d]",
//						stRoleState_tmp.PhysicalAttrib.AttackCDTime_Base);

				int nskill_count = 0;
				memcpy(&nskill_count, input + current_input_length, 2);
				current_input_length += 2;
				for (int nskill_pos = 0; nskill_pos < nskill_count; nskill_pos++) {
					if (length - current_input_length < 3)
					{
						LOG(LOG_ERROR, __FILE__, __LINE__, "length error! length[%d], current_input_length[%d] ", length, current_input_length );
						return -1;
					}
					short int skill_id = 0;
					char skill_level = 0;
					memcpy(&skill_id, input + current_input_length, 2);
					current_input_length += 2;
					skill_level = *(input + current_input_length);
					current_input_length += 1;
					if ((skill_level > 6) || (skill_level < 1))
					{
						LOG(LOG_ERROR, __FILE__, __LINE__, "skill_level error! skill_level[%d] ", skill_level );
						return -1;
					}

					map<short int, SkillDesc>::iterator pos_skill;
					pos_skill = option.skill_map.find(skill_id);
					if (pos_skill == option.skill_map.end())
					{
						LOG(LOG_ERROR, __FILE__, __LINE__, "skill_id[%d] not find!", skill_id);
						return -1;
					}

					SkillAttr SkillAttr_tmp;
					SkillAttr_tmp.SkillLevel = skill_level;
					SkillAttr_tmp.LastUseTime = 0;
					SkillAttr_tmp.Area = pos_skill->second.Area;
					SkillAttr_tmp.DoingTime = pos_skill->second.DoingTime;
					SkillAttr_tmp.MP = pos_skill->second.MP[skill_level - 1];
					SkillAttr_tmp.SkillCD
							= pos_skill->second.SkillCD[skill_level - 1];
					stRoleState_tmp.MagicAttrb.SkillMap.insert(
							make_pair(skill_id, SkillAttr_tmp));
				}
				stRoleState_tmp.Init();
				P_Roles.insert(
						make_pair(stRoleState_tmp.RoleID, stRoleState_tmp));

			}
		}
	}

	if (pipe(pipe_handles) == -1)
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "pipe_handles error! pipe_handles[%d] ", pipe_handles );
		return -1;
	}
	int val = fcntl(pipe_handles[0], F_GETFL, 0);
	val |= O_NONBLOCK;
	fcntl(pipe_handles[0], F_SETFL, val);

	val = fcntl(pipe_handles[1], F_GETFL, 0);
	val |= O_NONBLOCK;
	fcntl(pipe_handles[1], F_SETFL, val);

	PK_State = PK_STATE_READY;

	pthread_t _thrTimer;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if ((pthread_attr_setstacksize(&attr, 1024000)) != 0)
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "pthread_attr_setstacksize error!" );
		return -1;
	}
	if (pthread_create(&_thrTimer, &attr, run_pking, this) != 0)
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "pthread_create error!" );
		return -1;
	}

	char sDebug[64] = { 0 };
	char sDebug_tmp[64] = { 0 };
	PkgHead broadcast_begin_head;
	broadcast_begin_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_begin_head.ucSrvType = SRVTYPE_PK;
	broadcast_begin_head.ucSrvSeq = option.srv_req;
	broadcast_begin_head.usPkgLen = LEN_PKG_HEAD + 4 + 2 + 2 + 1 + length - 6;
	broadcast_begin_head.usMsgType = MSGTYPE_S_C_READY;
	char broadcast_begin[LEN_PKG_HEAD + 4 + 2 + 2 + 1 + 2 + MAX_ROLE_COUNT * 61
			+ 1] = { 0 };
	memcpy(broadcast_begin + LEN_PKG_HEAD, &m_PK_ID, 4);
	memcpy(broadcast_begin + LEN_PKG_HEAD + 4, &map_x, 2);
	memcpy(broadcast_begin + LEN_PKG_HEAD + 4 + 2, &map_y, 2);
	memcpy(broadcast_begin + LEN_PKG_HEAD + 4 + 2 + 2, &option.srv_req, 1);
	memcpy(broadcast_begin + LEN_PKG_HEAD + 4 + 2 + 2 + 1, input + 10,
			length - 10);
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			memset(sDebug_tmp, 0, sizeof(sDebug_tmp));
			strncpy(sDebug_tmp, sDebug, sizeof(sDebug_tmp) - 1);
			snprintf(sDebug, sizeof(sDebug) - 1, "%s,{%d:[%d,%d]}", sDebug_tmp,
					pos_role->second.ControlID,
					pos_role->second.MoveAttrb.CurrentPos.X,
					pos_role->second.MoveAttrb.CurrentPos.Y);
			broadcast_begin_head.uiRoleID = pos_role->second.ControlID;
			broadcast_begin_head.packet(broadcast_begin);
			m_pMain_thread->send_client(broadcast_begin,
					broadcast_begin_head.usPkgLen);
		}
	}
//	LOG(LOG_DEBUG, __FILE__, __LINE__, "[%d: %s]create PK OK!", m_PK_ID, sDebug);
	srand( time(NULL));
	return 0;
}

void* PK_PE::run_pking(void* object) {
	PK_PE *ptrC = (PK_PE*) object;

	ptrC->PK_State = PK_STATE_READY;
	ptrC->current_time.fragment_now = 0;
	int poll_timeout = 1000;
	char event;
	while ((ptrC->PK_State == PK_STATE_PKING) || (ptrC->PK_State
			== PK_STATE_READY)) {
		struct pollfd client;
		client.fd = ptrC->pipe_handles[0];
		client.events = POLLIN;
		client.revents = 0;
		int ready = poll(&client, 1, poll_timeout);
		if (ready > 0) {
			if (read(ptrC->pipe_handles[0], &event, 1) == 1) {
				if (event == 1) {
					ptrC->current_time.fragment_now++;
					if (ptrC->PK_State == PK_STATE_PKING) {
						ptrC->RunTime();
					} else {
						//60秒，如果还是不行，就退出
						if (ptrC->current_time.fragment_now
								> TIME_WAITING_READY * 10) {
							ptrC->PK_State = PK_STATE_PKING;
							ptrC->current_time.fragment_now = 0;
							ptrC->BroadcastBegin();
						}
					}
				} else {
					if (ptrC->PK_State == PK_STATE_PKING)
						ptrC->process_c_s_order();
				}
			} else {
				LOG(LOG_FATAL, __FILE__, __LINE__, "pipe_handles error!");
				break;
			}
		}
	}

	ptrC->m_pMain_thread->kill_thread(ptrC->getPK_ID());
	return NULL;
}

int PK_PE::RunTime() {

	bool bCheckOffline = current_time.fragment_now % 50 == 0;
	for (map<int, RoleState>::iterator pos_role = P_Roles.begin(); pos_role
			!= P_Roles.end(); pos_role++) {
		if (pos_role->second.Live == ROLE_STATUS_DEAD) {
			continue;
		}
		if (bCheckOffline) {
			if ((pos_role->second.RoleFlag == RoleType_Player)
					&& (current_time.fragment_now
							- pos_role->second.CurrentOrder.OrderTime
							> TIME_OFFLINE)) {
				pos_role->second.Live = ROLE_STATUS_OFFLINE;
				CheckOver(pos_role->second.Opposition);
				continue;
			}
		}

		CalculateBuff(&(pos_role->second));
		CalculateBullets();

		if (pos_role->second.Attack_Skill_WaitingTime > 0) {
			pos_role->second.Attack_Skill_WaitingTime--;
			continue;
		}
		RunRole(&(pos_role->second));
	}
	CalculateSkill();

	return 0;
}

int PK_PE::RunRole(RoleState *pRoleState) {
	int return_code = 0;
	switch (pRoleState->CurrentOrder.Type) {
	case ORDER_TYPE_NO:

		if (pRoleState->PhysicalAttrib.LimitCounter <= 0
		//				&& (current_time.fragment_now+10
		//						- pRoleState->PhysicalAttrib.PreAttackTime)
		//						>= pRoleState->PhysicalAttrib.AttackCDTime
		) {

			CalculateAIAttack(pRoleState);
		}
		break;
	case ORDER_TYPE_ATTACK_E:
		return_code = process_c_s_attack(pRoleState);
		if (return_code != ATTACKRESULT_OK_ATTACK && ATTACKRESULT_MOVEING
				!= return_code
				&& ATTACKRESULT_CD_LIMIT != return_code
				) {
			pRoleState->CurrentOrder.Type = ORDER_TYPE_NO;
			if (pRoleState->MoveAttrb.MoveLine.size() > 0) {
				pRoleState->MoveAttrb.MoveLine.clear();
				BrodacastStop(pRoleState->RoleID,
						pRoleState->MoveAttrb.CurrentPos,
						pRoleState->MoveAttrb.CurrentPosMini);
			}
		}
		break;
	case ORDER_TYPE_ATTACK_P:
		break;
	case ORDER_TYPE_MOVE_E:
		break;
	case ORDER_TYPE_CASTSKILL:
		break;
	case ORDER_TYPE_MOVE_P:
		return_code = CalculateMoveP(pRoleState);
		if (return_code == MOVERESULT_ERROR) {
			if ((pk_map_base[pRoleState->CurrentOrder.TargetPos.Y][pRoleState->CurrentOrder.TargetPos.X]
					!= 0) && abs(
					pRoleState->MoveAttrb.CurrentPos.X
							- pRoleState->CurrentOrder.TargetPos.X) <= 3
					&& abs(
							pRoleState->MoveAttrb.CurrentPos.Y
									- pRoleState->CurrentOrder.TargetPos.Y)
							<= 3) {
				pRoleState->CurrentOrder.Type = ORDER_TYPE_NO;
				if (pRoleState->MoveAttrb.MoveLine.size() > 0) {
					pRoleState->MoveAttrb.MoveLine.clear();
					BrodacastStop(pRoleState->RoleID,
							pRoleState->MoveAttrb.CurrentPos,
							pRoleState->MoveAttrb.CurrentPosMini);
				}
			} else {
				if (AStar(pRoleState, pRoleState->MoveAttrb.CurrentPos,
						pRoleState->CurrentOrder.TargetPos,
						&(pRoleState->MoveAttrb.MoveLine),
						&(pRoleState->MoveAttrb.Direct)) != 0) {
//					pRoleState->CurrentOrder.Type = ORDER_TYPE_NO;
//					if (pRoleState->MoveAttrb.MoveLine.size() > 0) {
//						pRoleState->MoveAttrb.MoveLine.clear();
//						BrodacastStop(pRoleState->RoleID,
//								pRoleState->MoveAttrb.CurrentPos,
//								pRoleState->MoveAttrb.CurrentPosMini);
//					}
					//如果发现不能寻路，则直接退出
					BrodacastDead(pRoleState->RoleID);
					pRoleState->Dead();
					CheckOver(pRoleState->Opposition);
				}
			}
		} else if (return_code == MOVERESULT_NO_MOVE) {
			pRoleState->CurrentOrder.Type = ORDER_TYPE_NO;
			if (pRoleState->MoveAttrb.MoveLine.size() > 0) {
				pRoleState->MoveAttrb.MoveLine.clear();
				BrodacastStop(pRoleState->RoleID,
						pRoleState->MoveAttrb.CurrentPos,
						pRoleState->MoveAttrb.CurrentPosMini);
			}
		}
		break;
	case ORDER_TYPE_HOLD:
		break;
	case ORDER_TYPE_STOP:
		break;
	default:
		if (process_c_s_skill(pRoleState) < 0) {
			pRoleState->MoveAttrb.MoveLine.clear();
			pRoleState->CurrentOrder.Type = ORDER_TYPE_NO;
			//BrodacastStop(pRoleState->RoleID, pRoleState->MoveAttrb.CurrentPos, pRoleState->MoveAttrb.CurrentPosMini);
		}
		break;
	}

	return 0;
}

//这个函数是主线程调用的
int PK_PE::process_c_s_ready(int RoleID) {
	if (PK_State != PK_STATE_READY) {
		return -1;
	}

	map<int, RoleState>::iterator pos_role;
	pos_role = P_Roles.find(RoleID);
	if (pos_role == P_Roles.end())
		return -1;

	pos_role->second.PK_State = PK_STATE_READY;

	//返回resp；
	PkgHead stPkgHead_resp;
	stPkgHead_resp.uiRoleID = RoleID;
	stPkgHead_resp.ucDirection = DIRECT_C_S_RESP;
	stPkgHead_resp.ucSrvType = SRVTYPE_PK;
	stPkgHead_resp.usMsgType = MSGTYPE_C_S_READY;
	stPkgHead_resp.usPkgLen = LEN_PKG_HEAD + 4;
	char buffer_resp[LEN_PKG_HEAD + 4 + 1] = { 0 };
	stPkgHead_resp.packet(buffer_resp);
	int nRtn = 0;
	memcpy(buffer_resp + LEN_PKG_HEAD, &nRtn, 4);
	m_pMain_thread->send_client(buffer_resp, stPkgHead_resp.usPkgLen);

	bool bAllReady = true;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player)
			if (pos_role->second.PK_State != PK_STATE_READY)
				bAllReady = false;
	}

	//全部准备好了
	if (bAllReady == true) {
		PK_State = PK_STATE_PKING;
		current_time.fragment_now = 0;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		current_time.tv_start = tv;
		BroadcastBegin();
		//CalculateHatre();
	}

	return 0;
}

int PK_PE::process_c_s_order() {
	// 把用户的输入放到角色状态中
	pthread_mutex_lock(&lock_list);
	list<UserOrder> Orders_tmp = Orders;
	Orders.clear();
	pthread_mutex_unlock(&lock_list);

	list<UserOrder>::iterator pos_list;
	for (pos_list = Orders_tmp.begin(); pos_list != Orders_tmp.end(); ++pos_list) {
		LOG(
				LOG_DEBUG,
				__FILE__,
				__LINE__,
				"RunOnce:Orders[control=%d][role=%d][type=%d][TargetType=%d][pos=%d,%d][TargetID=%d][OrderTime=%d] currenttime=%d",
				pos_list->ControlID, pos_list->RoleID, pos_list->Order.Type,
				pos_list->Order.TargetType, pos_list->Order.TargetPos.X,
				pos_list->Order.TargetPos.Y, pos_list->Order.TargetRoleID,
				pos_list->Order.OrderTime, current_time.fragment_now);
		map<int, RoleState>::iterator pos_map;
		map<int, RoleState>::iterator pos_target;
		pos_map = P_Roles.find(pos_list->RoleID);
		if ((pos_map != P_Roles.end()) && ((pos_map->second.ControlID
				== pos_list->ControlID))) {
			if (pos_map->second.Live == ROLE_STATUS_DEAD)
				continue;
			breakLastSkill(pos_map->second);
			switch (pos_list->Order.Type) {
			case ORDER_TYPE_ATTACK_E:
				pos_target = P_Roles.find(pos_list->Order.TargetRoleID);
				if (pos_target == P_Roles.end()) {
					break;
				}
				if (pos_target->second.Live == ROLE_STATUS_DEAD) {
					break;
				}
				pos_map->second.CurrentOrder = pos_list->Order;
				pos_map->second.PhysicalAttrib.PreTimeStatus = 1;
				break;
			case ORDER_TYPE_MOVE_P:
				Pos tmpPos;
				tmpPos.X = -1;
				tmpPos.Y = -1;
				tmpPos = pos_map->second.MoveAttrb.MoveLine.back();
				if (tmpPos.X == pos_list->Order.TargetPos.X && tmpPos.Y
						== pos_list->Order.TargetPos.Y)
					break;
				LOG(LOG_DEBUG, __FILE__, __LINE__, "Call AStart");
				if (AStar(&(pos_map->second),
						pos_map->second.MoveAttrb.CurrentPos,
						pos_list->Order.TargetPos,
						&(pos_map->second.MoveAttrb.MoveLine),
						&(pos_map->second.MoveAttrb.Direct)) == 0) {

					pos_map->second.CurrentOrder = pos_list->Order;
				}
				break;
			default:
				pos_map->second.CurrentOrder = pos_list->Order;
				break;
			}
		}
	}

	return 0;
}

int PK_PE::process_c_s_skill(RoleState *pRoleState) {

	//检查是否禁止使用技能
	if (pRoleState->MagicAttrb.LimitCounter > 0) {
		return -1;
	}

	//检查是否有该技能
	map<short int, SkillAttr>::iterator pos_skill;
	pos_skill = pRoleState->MagicAttrb.SkillMap.find(
			pRoleState->CurrentOrder.Type);
	if (pos_skill == pRoleState->MagicAttrb.SkillMap.end())
		return -1;

	//检查技能CD时间
	if (pos_skill->second.LastUseTime != 0 && (current_time.fragment_now
			- pos_skill->second.LastUseTime < pos_skill->second.SkillCD)) {
		return -1;
	}

	//检查技能是否有蓝
	if (pRoleState->MP < pos_skill->second.MP) {
		return -1;
	}

	map<int, RoleState>::iterator pos_target;
	if (pRoleState->CurrentOrder.TargetType == 1) {
		if ((pRoleState->CurrentOrder.TargetPos.X > MAP_SIZE_X - 1)
				|| (pRoleState->CurrentOrder.TargetPos.X < 0)
				|| (pRoleState->CurrentOrder.TargetPos.Y > MAP_SIZE_Y - 1)
				|| (pRoleState->CurrentOrder.TargetPos.Y < 0))
			return -1;
	} else {
		pos_target = P_Roles.find(pRoleState->CurrentOrder.TargetRoleID);
		if (pos_target == P_Roles.end()) {
			return -1;
		} else {
			if (pos_target->second.MagicAttrb.ExemptAllCounter > 0)
				return -1;
		}
	}

	if (pos_skill->second.Area > 0) {
		if (pRoleState->CurrentOrder.TargetType == 1) {
			if ((abs(
					pRoleState->MoveAttrb.CurrentPos.X
							- pRoleState->CurrentOrder.TargetPos.X)
					> pos_skill->second.Area) || (abs(
					pRoleState->MoveAttrb.CurrentPos.Y
							- pRoleState->CurrentOrder.TargetPos.Y)
					> pos_skill->second.Area)) {
				if (pRoleState->MoveAttrb.LimitCounter > 0)
					return -1;
				if (CalculateMoveP(pRoleState) != MOVERESULT_OK_MOVE) {
					LOG(LOG_DEBUG, __FILE__, __LINE__, "Call AStart");
					if (AStar(pRoleState, pRoleState->MoveAttrb.CurrentPos,
							pRoleState->CurrentOrder.TargetPos,
							&pRoleState->MoveAttrb.MoveLine,
							&pRoleState->MoveAttrb.Direct) != 0) {
						return -1;
					} else {
						return 0;

					}
				} else {
					return 0;
				}
			}
		} else {
			if ((abs(
					pRoleState->MoveAttrb.CurrentPos.X
							- pos_target->second.MoveAttrb.CurrentPos.X)
					> pos_skill->second.Area) || (abs(
					pRoleState->MoveAttrb.CurrentPos.Y
							- pos_target->second.MoveAttrb.CurrentPos.Y)
					> pos_skill->second.Area)) {
				if (pRoleState->MoveAttrb.LimitCounter > 0)
					return -1;
				if (CalculateMoveP(pRoleState) != MOVERESULT_OK_MOVE) {
					LOG(LOG_DEBUG, __FILE__, __LINE__, "Call AStart");
					if (AStar(pRoleState, pRoleState->MoveAttrb.CurrentPos,
							pos_target->second.MoveAttrb.CurrentPos,
							&pRoleState->MoveAttrb.MoveLine,
							&pRoleState->MoveAttrb.Direct) != 0) {
						return -1;
					} else {
						return 0;
					}
				} else {
					return 0;
				}
			}
		}
	}

	BrodacastStop(pRoleState->RoleID, pRoleState->MoveAttrb.CurrentPos,
			pRoleState->MoveAttrb.CurrentPosMini);
	Skill_Order skill_order_tmp;
	skill_order_tmp.role = pRoleState;
	skill_order_tmp.order = pRoleState->CurrentOrder;
	skill_order_tmp.Start_Time = pos_skill->second.DoingTime;
	skill_order_tmp.SkillLevel = pos_skill->second.SkillLevel;
	skill_order_list.push_back(skill_order_tmp);

	Brodacast_Skill(pRoleState->RoleID, pRoleState->CurrentOrder);
	pRoleState->MP -= pos_skill->second.MP;

	BrodacastMpChange(pRoleState->RoleID, -pos_skill->second.MP, 100);//100表示使用技能，没有时间意义。
	pos_skill->second.LastUseTime = current_time.fragment_now;
	pRoleState->Attack_Skill_WaitingTime = pos_skill->second.DoingTime;
	pRoleState->CurrentOrder.Type = ORDER_TYPE_NO;
	return 0;
}

int PK_PE::process_c_s_attack(RoleState *pRoleState) {
	if (pRoleState->PhysicalAttrib.LimitCounter > 0)
		return ATTACKRESULT_NO_ATTACK;
	//让攻击开始快2秒
	if (current_time.fragment_now+20 - pRoleState->PhysicalAttrib.PreAttackTime
			< pRoleState->PhysicalAttrib.AttackCDTime){
//		LOG (LOG_DEBUG, __FILE__, __LINE__, "PhysicalAttrib.AttackCDTime [%d]", pRoleState->PhysicalAttrib.AttackCDTime);
//		LOG (LOG_DEBUG, __FILE__, __LINE__, "Real CD Time [%d]", current_time.fragment_now - pRoleState->PhysicalAttrib.PreAttackTime);

		return ATTACKRESULT_CD_LIMIT;
	}
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(pRoleState->CurrentOrder.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return ATTACKRESULT_TARGETI_NVALID;
	}

	// 0、判断目标是否已经死亡或者无敌
	if (pos_target->second.Live == ROLE_STATUS_DEAD
			|| pos_target->second.PhysicalAttrib.ExemptCounter > 0) {
		return ATTACKRESULT_TARGETI_NVALID;
	}

	// 1、首先判定是否可以攻击
	if ((abs(
			pRoleState->MoveAttrb.CurrentPos.X
					- pos_target->second.MoveAttrb.CurrentPos.X)
			<= pRoleState->PhysicalAttrib.AttackArea) && (abs(
			pRoleState->MoveAttrb.CurrentPos.Y
					- pos_target->second.MoveAttrb.CurrentPos.Y)
			<= pRoleState->PhysicalAttrib.AttackArea)) {
		if (pRoleState->PhysicalAttrib.PreTimeStatus == 0) {
			pRoleState->PhysicalAttrib.PreTimeStatus = 1;
			pRoleState->MoveAttrb.MoveLine.clear();
		}
		pRoleState->PhysicalAttrib.PreAttackTime = current_time.fragment_now;
		pRoleState->Attack_Skill_WaitingTime
				= pRoleState->PhysicalAttrib.AttackIngTime;
		CalculateAttack(*pRoleState, pos_target->second);
		return ATTACKRESULT_OK_ATTACK;
	}
	if (pRoleState->MoveAttrb.LimitCounter > 0
			|| pRoleState->MoveAttrb.SpeedMod == 0) {
		return ATTACKRESULT_NO_MOVE;
	}
	// 3、判断是否要移动
	if (pRoleState->PhysicalAttrib.PreTimeStatus == 1) {
		pRoleState->PhysicalAttrib.PreTimeStatus = 0;

		if (AStar(pRoleState, pRoleState->MoveAttrb.CurrentPos,
				pos_target->second.MoveAttrb.CurrentPos,
				&pRoleState->MoveAttrb.MoveLine, &pRoleState->MoveAttrb.Direct)
				!= 0) {
			pRoleState->PhysicalAttrib.PreTimeStatus = 1;
			return ATTACKRESULT_ASTART_FAIL;
		}
	} else {
		short int PreD = -1;
		short int NextD = 0;
		if (current_time.fragment_now % 10 == 0) {
			PreD = abs(
					pRoleState->MoveAttrb.CurrentPos.X
							- pos_target->second.MoveAttrb.CurrentPos.X) + abs(
					pRoleState->MoveAttrb.CurrentPos.Y
							- pos_target->second.MoveAttrb.CurrentPos.Y);
		}
		if (CalculateMoveP(pRoleState) == MOVERESULT_OK_MOVE) {
			if (PreD >= 0) {
				NextD
						= abs(
								pRoleState->MoveAttrb.CurrentPos.X
										- pos_target->second.MoveAttrb.CurrentPos.X)
								+ abs(
										pRoleState->MoveAttrb.CurrentPos.Y
												- pos_target->second.MoveAttrb.CurrentPos.Y);
				if ((NextD > PreD)) {
					if (AStar(pRoleState, pRoleState->MoveAttrb.CurrentPos,
							pos_target->second.MoveAttrb.CurrentPos,
							&pRoleState->MoveAttrb.MoveLine,
							&pRoleState->MoveAttrb.Direct) == 0) {
						if (CalculateMoveP(pRoleState) != MOVERESULT_OK_MOVE)
							return ATTACKRESULT_MOVE_FAIL;
					}
				}
			}
		} else {
			if (AStar(pRoleState, pRoleState->MoveAttrb.CurrentPos,
					pos_target->second.MoveAttrb.CurrentPos,
					&pRoleState->MoveAttrb.MoveLine,
					&pRoleState->MoveAttrb.Direct) == 0) {
				if (CalculateMoveP(pRoleState) != MOVERESULT_OK_MOVE)
					return ATTACKRESULT_MOVE_FAIL;
			} else {
				pRoleState->PhysicalAttrib.PreTimeStatus = 1;
				return ATTACKRESULT_ASTART_FAIL;
			}
		}
	}

	return ATTACKRESULT_MOVEING;
}

void PK_PE::CalculateAttack(RoleState &pRoleStateAttack,
		RoleState &pRoleStatePassive) {
	BrodacastAttack(pRoleStateAttack.RoleID, pRoleStatePassive.RoleID);
	Bullet BulletsTmp;
	BulletsTmp.Start_Time = pRoleStateAttack.PhysicalAttrib.AttackIngTime;
	BulletsTmp.Role_Attack = &pRoleStateAttack;
	BulletsTmp.Role_Passive = &pRoleStatePassive;
	BulletsTmp.Speed = pRoleStateAttack.PhysicalAttrib.AttackBulletMod;
	BulletsTmp.CurrentPos.X = pRoleStateAttack.MoveAttrb.CurrentPos.X;
	BulletsTmp.CurrentPos.Y = pRoleStateAttack.MoveAttrb.CurrentPos.Y;
	if (pRoleStateAttack.PhysicalAttrib.AttackBulletMod == 0) {
		BulletsTmp.BulletType = BULLET_TYPE_ATK_CLOSE;
	} else {
		BulletsTmp.BulletType = BULLET_TYPE_ATK_REMOTE;
	}
	Bullets.push_back(BulletsTmp);

	//仇恨关系产生
	HaterInfo tmpHatre;
	tmpHatre.clear();
	double dis;
	short int hp;
	if (pRoleStatePassive.RoleFlag == RoleType_Monster
			|| pRoleStatePassive.RoleFlag == RoleType_NPC) {
		map<int, HaterInfo>::iterator pos;
		pos = pRoleStatePassive.HatreMap.find(pRoleStateAttack.RoleID);
		if (pos == pRoleStatePassive.HatreMap.end()) {
			tmpHatre.Attack = 2;
			dis = Dist(pRoleStatePassive.MoveAttrb.CurrentPos,
					pRoleStateAttack.MoveAttrb.CurrentPos);
			if (dis <= pRoleStatePassive.PhysicalAttrib.AttackArea) {
				tmpHatre.Distance = 8;
			} else {
				dis = 8 - (dis - pRoleStatePassive.PhysicalAttrib.AttackArea);
				tmpHatre.Distance = (short int) (dis < 1 ? 1 : dis);
			}
			hp = 10 - int(
					(pRoleStateAttack.HP / double(pRoleStateAttack.HP_MAX))
							* 10);
			if (hp > 8)
				hp = 8;
			else if (hp < 1)
				hp = 1;
			tmpHatre.Hp = hp;

			pRoleStatePassive.HatreMap.insert(
					make_pair(pRoleStateAttack.RoleID, tmpHatre));
			CalculateAIAttack(&pRoleStatePassive);
		} else {
			pos->second.Attack += 2;
			if (pos->second.Attack > 8)
				pos->second.Attack = 8;
		}

	}
	tmpHatre.clear();
	if (pRoleStateAttack.RoleFlag == RoleType_Monster
			|| pRoleStateAttack.RoleFlag == RoleType_NPC) {
		map<int, HaterInfo>::iterator pos;
		pos = pRoleStateAttack.HatreMap.find(pRoleStatePassive.RoleID);
		if (pos != pRoleStateAttack.HatreMap.end()) {
			pos->second.Hit += 1;
			if (pos->second.Hit > 8)
				pos->second.Hit = 8;
		} else {
			dis = Dist(pRoleStatePassive.MoveAttrb.CurrentPos,
					pRoleStateAttack.MoveAttrb.CurrentPos);
			if (dis <= pRoleStateAttack.PhysicalAttrib.AttackArea) {
				tmpHatre.Distance = 8;
			} else {
				dis = 8 - (dis - pRoleStateAttack.PhysicalAttrib.AttackArea);
				tmpHatre.Distance = (short int) (dis < 1 ? 1 : dis);
			}
			hp = 10 - int(
					(pRoleStatePassive.HP / double(pRoleStatePassive.HP_MAX))
							* 10);
			if (hp > 8)
				hp = 8;
			else if (hp < 1)
				hp = 1;
			tmpHatre.Hp = hp;
			tmpHatre.Hit = 1;

			pRoleStateAttack.HatreMap.insert(
					make_pair(pRoleStatePassive.RoleID, tmpHatre));
			CalculateAIAttack(&pRoleStateAttack);
		}
	}

}

int PK_PE::CalculateMoveP(RoleState *pRoleState) {
	if (pRoleState->MoveAttrb.LimitCounter > 0) {
		return MOVERESULT_NO_MOVE;
	}

	if (pRoleState->MoveAttrb.MoveLine.size() < 2) {
//		LOG(LOG_DEBUG, __FILE__, __LINE__, "CalculateMoveP: MoveLine.size< 2");
		return MOVERESULT_ERROR;
	}
	multimap<short int, Buff>::iterator iter_buff;
	for (iter_buff = pRoleState->multimap_buff.begin(); iter_buff
			!= pRoleState->multimap_buff.end(); iter_buff++) {
		if (iter_buff->second.ParentSkill == SKILL_TYPE_NengLiangLiuShi) {
			iter_buff->second.RunCounter++;
			if (iter_buff->second.RunCounter < 10)
				continue;
			iter_buff->second.RunCounter = 0;
			//short int hpHurt = pRoleState->HP * iter_buff->second.Parametar1 / 100;
			short int mpHurt = pRoleState->MP * iter_buff->second.Parametar2
					/ 100;
			if (mpHurt == 0 && pRoleState->MP > 0)
				mpHurt = 1;
			//if( hpHurt == 0 && pRoleState->HP > 1 )
			//hpHurt = 1;
			//pRoleState->HP -= hpHurt;
			pRoleState->MP -= mpHurt;
			//BrodacastHpChange(pRoleState->RoleID,-hpHurt,HPCHANGE_SOURCE_SKILL,SKILL_TYPE_NengLiangLiuShi );
			BrodacastMpChange(pRoleState->RoleID, -mpHurt,
					SKILL_TYPE_NengLiangLiuShi);
		}
	}

	bool CrossPos = false;
	short int DiffXY = 0;
	short int currMoveSpeed = pRoleState->MoveAttrb.SpeedMod;
	if (currMoveSpeed >= MAP_MAX_LINE)
		currMoveSpeed = MAP_MAX_LINE - 1;
	else if (currMoveSpeed < 0)
		return MOVERESULT_OK_MOVE;
	switch (pRoleState->MoveAttrb.Direct) {
	case DIRECT_NORTH:

		pRoleState->MoveAttrb.CurrentPosMini.Y -= currMoveSpeed;
		if (pRoleState->MoveAttrb.CurrentPosMini.Y < 0) {
			pRoleState->MoveAttrb.CurrentPos.Y--;
			pRoleState->MoveAttrb.CurrentPosMini.Y += LENGTH_STRAIGHT;
			CrossPos = true;
		}

		break;
	case DIRECT_NORTH_EAST:

		DiffXY = pRoleState->MoveAttrb.CurrentPosMini.X - (LENGTH_STRAIGHT
				- pRoleState->MoveAttrb.CurrentPosMini.Y);
		if (DiffXY == 0) {
			pRoleState->MoveAttrb.CurrentPosMini.X
					+= (short int) (currMoveSpeed * MOD_STRAIGHT_DIAGONAL);
			pRoleState->MoveAttrb.CurrentPosMini.Y
					-= (short int) (currMoveSpeed * MOD_STRAIGHT_DIAGONAL);
		} else {
			if (currMoveSpeed > abs(DiffXY)) {
				if (DiffXY > 0)
					pRoleState->MoveAttrb.CurrentPosMini.Y -= DiffXY;
				else
					pRoleState->MoveAttrb.CurrentPosMini.X -= DiffXY;
				pRoleState->MoveAttrb.CurrentPosMini.X
						+= (short int) ((currMoveSpeed - abs(DiffXY))
								* MOD_STRAIGHT_DIAGONAL);
				pRoleState->MoveAttrb.CurrentPosMini.Y
						-= (short int) ((currMoveSpeed - abs(DiffXY))
								* MOD_STRAIGHT_DIAGONAL);
			} else {
				if (DiffXY > 0)
					pRoleState->MoveAttrb.CurrentPosMini.Y -= currMoveSpeed;
				else
					pRoleState->MoveAttrb.CurrentPosMini.X += currMoveSpeed;
			}

		}
		if (pRoleState->MoveAttrb.CurrentPosMini.X >= LENGTH_STRAIGHT) {
			pRoleState->MoveAttrb.CurrentPosMini.X
					= pRoleState->MoveAttrb.CurrentPosMini.X - LENGTH_STRAIGHT;
			pRoleState->MoveAttrb.CurrentPos.X++;
			pRoleState->MoveAttrb.CurrentPosMini.Y = LENGTH_STRAIGHT
					+ pRoleState->MoveAttrb.CurrentPosMini.Y;
			pRoleState->MoveAttrb.CurrentPos.Y--;
			CrossPos = true;
		}

		break;
	case DIRECT_EAST:

		pRoleState->MoveAttrb.CurrentPosMini.X += currMoveSpeed;
		if (pRoleState->MoveAttrb.CurrentPosMini.X >= LENGTH_STRAIGHT) {
			pRoleState->MoveAttrb.CurrentPos.X++;
			pRoleState->MoveAttrb.CurrentPosMini.X
					= pRoleState->MoveAttrb.CurrentPosMini.X - LENGTH_STRAIGHT;
			CrossPos = true;
		}

		break;
	case DIRECT_EAST_SOUTH:

		DiffXY = pRoleState->MoveAttrb.CurrentPosMini.X
				- pRoleState->MoveAttrb.CurrentPosMini.Y;
		if (DiffXY == 0) {
			pRoleState->MoveAttrb.CurrentPosMini.X
					+= (short int) (currMoveSpeed * MOD_STRAIGHT_DIAGONAL);
			pRoleState->MoveAttrb.CurrentPosMini.Y
					+= (short int) (currMoveSpeed * MOD_STRAIGHT_DIAGONAL);
		} else {
			if (currMoveSpeed > abs(DiffXY)) {
				if (DiffXY > 0)
					pRoleState->MoveAttrb.CurrentPosMini.Y += DiffXY;
				else
					pRoleState->MoveAttrb.CurrentPosMini.X -= DiffXY;
				pRoleState->MoveAttrb.CurrentPosMini.X
						+= (short int) ((currMoveSpeed - abs(DiffXY))
								* MOD_STRAIGHT_DIAGONAL);
				pRoleState->MoveAttrb.CurrentPosMini.Y
						+= (short int) ((currMoveSpeed - abs(DiffXY))
								* MOD_STRAIGHT_DIAGONAL);
			} else {
				if (DiffXY > 0)
					pRoleState->MoveAttrb.CurrentPosMini.Y += currMoveSpeed;
				else
					pRoleState->MoveAttrb.CurrentPosMini.X += currMoveSpeed;
			}

		}
		if (pRoleState->MoveAttrb.CurrentPosMini.X >= LENGTH_STRAIGHT) {
			pRoleState->MoveAttrb.CurrentPosMini.X
					= pRoleState->MoveAttrb.CurrentPosMini.X - LENGTH_STRAIGHT;
			pRoleState->MoveAttrb.CurrentPos.X++;
			pRoleState->MoveAttrb.CurrentPosMini.Y
					= pRoleState->MoveAttrb.CurrentPosMini.Y - LENGTH_STRAIGHT;
			pRoleState->MoveAttrb.CurrentPos.Y++;
			CrossPos = true;
		}

		break;
	case DIRECT_SOUTH:

		pRoleState->MoveAttrb.CurrentPosMini.Y += currMoveSpeed;
		if (pRoleState->MoveAttrb.CurrentPosMini.Y >= LENGTH_STRAIGHT) {
			pRoleState->MoveAttrb.CurrentPos.Y++;
			pRoleState->MoveAttrb.CurrentPosMini.Y -= LENGTH_STRAIGHT;
			CrossPos = true;
		}

		break;
	case DIRECT_SOUTH_WEST:

		DiffXY = pRoleState->MoveAttrb.CurrentPosMini.X - (LENGTH_STRAIGHT
				- pRoleState->MoveAttrb.CurrentPosMini.Y);
		if (DiffXY == 0) {
			pRoleState->MoveAttrb.CurrentPosMini.X
					-= (short int) (currMoveSpeed * MOD_STRAIGHT_DIAGONAL);
			pRoleState->MoveAttrb.CurrentPosMini.Y
					+= (short int) (currMoveSpeed * MOD_STRAIGHT_DIAGONAL);
		} else {
			if (currMoveSpeed > abs(DiffXY)) {
				if (DiffXY > 0)
					pRoleState->MoveAttrb.CurrentPosMini.X -= DiffXY;
				else
					pRoleState->MoveAttrb.CurrentPosMini.Y -= DiffXY;
				pRoleState->MoveAttrb.CurrentPosMini.X
						-= (short int) ((currMoveSpeed - abs(DiffXY))
								* MOD_STRAIGHT_DIAGONAL);
				pRoleState->MoveAttrb.CurrentPosMini.Y
						+= (short int) ((currMoveSpeed - abs(DiffXY))
								* MOD_STRAIGHT_DIAGONAL);
			} else {
				if (DiffXY > 0)
					pRoleState->MoveAttrb.CurrentPosMini.X -= currMoveSpeed;
				else
					pRoleState->MoveAttrb.CurrentPosMini.Y += currMoveSpeed;
			}

		}
		if (pRoleState->MoveAttrb.CurrentPosMini.Y >= LENGTH_STRAIGHT) {
			pRoleState->MoveAttrb.CurrentPosMini.Y
					= pRoleState->MoveAttrb.CurrentPosMini.Y - LENGTH_STRAIGHT;
			pRoleState->MoveAttrb.CurrentPos.Y++;
			pRoleState->MoveAttrb.CurrentPosMini.X = LENGTH_STRAIGHT
					+ pRoleState->MoveAttrb.CurrentPosMini.X;
			pRoleState->MoveAttrb.CurrentPos.X--;
			CrossPos = true;
		}

		break;
	case DIRECT_WEST:

		pRoleState->MoveAttrb.CurrentPosMini.X -= currMoveSpeed;
		if (pRoleState->MoveAttrb.CurrentPosMini.X < 0) {
			pRoleState->MoveAttrb.CurrentPos.X--;
			pRoleState->MoveAttrb.CurrentPosMini.X += LENGTH_STRAIGHT;
			CrossPos = true;
		}

		break;
	case DIRECT_WEST_NORTH:
		DiffXY = pRoleState->MoveAttrb.CurrentPosMini.X
				- pRoleState->MoveAttrb.CurrentPosMini.Y;
		if (DiffXY == 0) {
			pRoleState->MoveAttrb.CurrentPosMini.X
					-= (short int) (currMoveSpeed * MOD_STRAIGHT_DIAGONAL);
			pRoleState->MoveAttrb.CurrentPosMini.Y
					-= (short int) (currMoveSpeed * MOD_STRAIGHT_DIAGONAL);
		} else {
			if (currMoveSpeed > abs(DiffXY)) {
				if (DiffXY > 0)
					pRoleState->MoveAttrb.CurrentPosMini.X -= DiffXY;
				else
					pRoleState->MoveAttrb.CurrentPosMini.Y += DiffXY;
				pRoleState->MoveAttrb.CurrentPosMini.X
						-= (short int) ((currMoveSpeed - abs(DiffXY))
								* MOD_STRAIGHT_DIAGONAL);
				pRoleState->MoveAttrb.CurrentPosMini.Y
						-= (short int) ((currMoveSpeed - abs(DiffXY))
								* MOD_STRAIGHT_DIAGONAL);
			} else {
				if (DiffXY > 0)
					pRoleState->MoveAttrb.CurrentPosMini.X -= currMoveSpeed;
				else
					pRoleState->MoveAttrb.CurrentPosMini.Y -= currMoveSpeed;
			}
		}
		if (pRoleState->MoveAttrb.CurrentPosMini.Y < 0) {
			pRoleState->MoveAttrb.CurrentPosMini.Y += LENGTH_STRAIGHT;
			pRoleState->MoveAttrb.CurrentPos.Y--;
			pRoleState->MoveAttrb.CurrentPosMini.X += LENGTH_STRAIGHT;
			pRoleState->MoveAttrb.CurrentPos.X--;
			CrossPos = true;
		}

		break;
	}
	//如果跨过格子了
	if (CrossPos) {
		list<Pos>::iterator pos = pRoleState->MoveAttrb.MoveLine.begin();
		pRoleState->MoveAttrb.MoveLine.erase(pos);
		pos = pRoleState->MoveAttrb.MoveLine.begin();

		//调整目标位置，避免和其他人重叠到一起
		map<int, RoleState>::iterator posIter;
		for (posIter = P_Roles.begin(); posIter != P_Roles.end(); posIter++) {
			if (pRoleState->RoleID == posIter->second.RoleID )
				continue;
			if ((*pos) == (posIter->second.MoveAttrb.CurrentPos)){
				pos->Y+=5; pos->X+=5;
				posIter = P_Roles.begin();
			}
		}


		pRoleState->MoveAttrb.CurrentPos.X = pos->X;
		pRoleState->MoveAttrb.CurrentPos.Y = pos->Y;

		if (pRoleState->MoveAttrb.MoveLine.size() > 1) {
			list<Pos>::iterator pos_next = pos;
			pos_next++;
			pRoleState->MoveAttrb.Direct = getDirect(*pos_next, *pos);
			if (pRoleState->MoveAttrb.Direct == 0) {
				//pRoleState->MoveAttrb.MoveLine.clear();
				//pRoleState->CurrentOrder.Type = ORDER_TYPE_NO;
				LOG(LOG_DEBUG, __FILE__, __LINE__, "Get Direction Error!!");
				return MOVERESULT_ERROR;
			}
		} else if (pRoleState->MoveAttrb.MoveLine.size() == 1) {
			LOG(LOG_DEBUG, __FILE__, __LINE__,
					"CalculateMoveP: [%d]: stop here[%d,%d][%d,%d][%d] !",
					pRoleState->RoleID, pRoleState->MoveAttrb.CurrentPos.X,
					pRoleState->MoveAttrb.CurrentPos.Y,
					pRoleState->MoveAttrb.CurrentPosMini.X,
					pRoleState->MoveAttrb.CurrentPosMini.Y,
					pRoleState->MoveAttrb.Direct);
			return MOVERESULT_OK_MOVE;
		} else {
			//pRoleState->MoveAttrb.MoveLine.clear();
			//pRoleState->CurrentOrder.Type = ORDER_TYPE_NO;
			LOG(LOG_ERROR, __FILE__, __LINE__, "why move line < 1 ?");
			return MOVERESULT_ERROR;
		}
	}

	//LOG (LOG_DEBUG, __FILE__, __LINE__, "CalculateMoveP: %d:[%d,%d]", pRoleState->RoleID,pRoleState->MoveAttrb.CurrentPos.X, pRoleState->MoveAttrb.CurrentPos.Y);

	return MOVERESULT_OK_MOVE;
}

void PK_PE::CalculateBullets() {
	list<Bullet>::iterator pos = Bullets.begin();
	for (; pos != Bullets.end();) {
		if (pos->Role_Passive->Live == ROLE_STATUS_DEAD) {
			pos = Bullets.erase(pos);
			continue;
		}

		if (pos->Start_Time > 0) {
			pos->Start_Time--;
			++pos;
		} else {
			if (pos->BulletType == BULLET_TYPE_ATK_CLOSE) {
				CalculateAttackHurt(*(pos->Role_Attack), *pos->Role_Passive);
				pos = Bullets.erase(pos);
				continue;
			}
			if (abs(
					pos->Role_Passive->MoveAttrb.CurrentPos.X
							- pos->CurrentPos.X) > 3 || abs(
					pos->Role_Passive->MoveAttrb.CurrentPos.Y
							- pos->CurrentPos.Y) > 3) {
				if (pos->Role_Passive->MoveAttrb.CurrentPos.X
						> pos->CurrentPos.X) {
					pos->CurrentPos.X += pos->Speed;
				} else if (pos->Role_Passive->MoveAttrb.CurrentPos.X
						< pos->CurrentPos.X) {
					pos->CurrentPos.X -= pos->Speed;
				}
				if (pos->Role_Passive->MoveAttrb.CurrentPos.Y
						> pos->CurrentPos.Y) {
					pos->CurrentPos.Y += pos->Speed;
				} else if (pos->Role_Passive->MoveAttrb.CurrentPos.Y
						< pos->CurrentPos.Y) {
					pos->CurrentPos.Y -= pos->Speed;
				}
				++pos;
				continue;
			}
			//LOG(LOG_DEBUG, __FILE__, __LINE__, "BulletType[%d]",pos->BulletType);
			if (pos->BulletType == BULLET_TYPE_ATK_REMOTE) {
				CalculateAttackHurt(*(pos->Role_Attack), *(pos->Role_Passive));
			} else {

				switch (pos->BulletType) {
				case SKILL_TYPE_FengBaoZhiChui:
					skill_process_207_effect(
							pos->Role_Passive->MoveAttrb.CurrentPos,
							pos->Role_Attack, pos->Level);
					break;
				case SKILL_TYPE_YouBu:
					skill_process_217_Effect(*pos);
					break;
				case SKILL_TYPE_ZuiJiuYunWu:
					skill_process_223_Effect(
							pos->Role_Passive->MoveAttrb.CurrentPos,
							pos->Role_Attack->Opposition, pos->Level);
					break;
				default:
					break;
				}
			}
			pos = Bullets.erase(pos);
		}
	}
}
void PK_PE::CalculateAttackHurt(RoleState &pRoleAttack, RoleState &pRoleTarget) {
	if ((rand() % 100) > getBHitTarget(pRoleAttack.PhysicalAttrib.AttackHit,
			pRoleTarget.PhysicalAttrib.DefenseMiss)) {
		BrodacastHpChange(pRoleTarget.RoleID, 0, HPCHANGE_SOURCE_ATK,
				ATTACK_EFFECT_MISS);
		return;
	}
	unsigned short int hurt_value = 0;
	int tmpParam = 0;
	hurt_value = pRoleAttack.PhysicalAttrib.AttackPowerLow_Base;
	tmpParam = pRoleAttack.PhysicalAttrib.AttackPowerHign_Base
			- pRoleAttack.PhysicalAttrib.AttackPowerLow_Base;
	if (tmpParam != 0) {
		hurt_value += rand() % tmpParam + 1;
	}

	hurt_value += (short int) (pRoleAttack.PhysicalAttrib.AttackPowerAdd
			* hurt_value / 100);
	tmpParam = (50 + pRoleTarget.Level) + (int)sqrt(
			pRoleTarget.PhysicalAttrib.DefenseArmor);
	if (tmpParam != 0)
		hurt_value = hurt_value * (50 + pRoleAttack.Level) / tmpParam;

	if (hurt_value <= 0)
		hurt_value = 1;
	char currAttackEffect;
	if (pRoleAttack.PhysicalAttrib.AttackCrit == 0
			|| (rand() % 100)
					> (short int) ((pRoleAttack.PhysicalAttrib.AttackCrit
							+ 11.99) / 12)) {
		currAttackEffect = ATTACK_EFFECT_NORMAL;
	} else {
		hurt_value *= 2;
		currAttackEffect = ATTACK_EFFECT_DOUBLE;
	}
	hurt_value += hurt_value * pRoleTarget.PhysicalAttrib.HurtAdd / 100;

	multimap<short int, Buff>::iterator iter_buff;
	for (iter_buff = pRoleTarget.multimap_buff.begin(); iter_buff
			!= pRoleTarget.multimap_buff.end(); iter_buff++) {//检查时否有诅咒技能
		if (SKILL_TYPE_ZuZhou == iter_buff->second.ParentSkill) {
			iter_buff->second.Parametar2 += hurt_value;
		}
	}
	for (iter_buff = pRoleAttack.multimap_buff.begin(); iter_buff
			!= pRoleAttack.multimap_buff.end(); iter_buff++) {
		if (iter_buff->second.ParentSkill == SKILL_TYPE_FeiXueZhiMao) {
			short int tmpHurt = hurt_value;
			hurt_value += iter_buff->second.Parametar1;
			iter_buff->second.Parametar1 += iter_buff->second.Parametar2
					* tmpHurt / 100;
			break;
		}
	}
	for (iter_buff = pRoleTarget.multimap_buff.begin(); iter_buff
			!= pRoleTarget.multimap_buff.end();) {
		if (SKILL_TYPE_WuGuangZhiDun == iter_buff->second.ParentSkill) {
			iter_buff->second.Parametar2 += hurt_value;
			if (iter_buff->second.Parametar2 >= iter_buff->second.Parametar1) {
				hurt_value = iter_buff->second.Parametar2
						- iter_buff->second.Parametar1;
				iter_buff->second.Parametar2 = iter_buff->second.Parametar1;
				skill_process_209_buffer(&pRoleTarget, &iter_buff->second);
				Brodacast_Buffer_Stop(pRoleTarget.RoleID,
						iter_buff->second.Type, iter_buff->second.ParentSkill,
						iter_buff->second.BeginTime);
				pRoleTarget.multimap_buff.erase(iter_buff++);
				continue;
			}
			hurt_value = 0;
		} else if (SKILL_TYPE_FanJiLuoXuan == iter_buff->second.ParentSkill) {
			skill_process_210_buffer(&pRoleTarget, &iter_buff->second);
		}
		++iter_buff;
	}

	if (hurt_value > 0) {
		pRoleTarget.HP -= hurt_value;
		BrodacastHpChange(pRoleTarget.RoleID, -hurt_value, HPCHANGE_SOURCE_ATK,
				currAttackEffect);
		if (pRoleTarget.HP <= 0) {
			onRoleDead(pRoleTarget);
		}
	}
}
void PK_PE::CalculateSkill() {
	list<Skill_Order>::iterator pos = skill_order_list.begin();
	for (; pos != skill_order_list.end();) {
		if (pos->role->Live == ROLE_STATUS_DEAD) {
			pos = skill_order_list.erase(pos);
			continue;
		}

		if (pos->Start_Time > 0) {
			pos->Start_Time--;
			++pos;
		} else {
			switch (pos->order.Type) {
			case SKILL_TYPE_YueShi:
				skill_process_201(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_HuoXueShu:
				skill_process_202(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_YaoShu:
				skill_process_203(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_JianRenFengBao:
				skill_process_204(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_YouLingLang:
				skill_process_205(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_JingXiang:
				skill_process_206(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_FengBaoZhiChui:
				skill_process_207(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_TaoTaiZhiRen:
				skill_process_208(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_WuGuangZhiDun:

				skill_process_209(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_FanJiLuoXuan:
				skill_process_210(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_DiCi:
				skill_process_211(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_DuSheShouWei:
				skill_process_212(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_PaoXiao:
				skill_process_213(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_XueXing:
				skill_process_214(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_QuSan:
				skill_process_215(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_LianSuoShanDian:
				skill_process_216(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_YouBu:
				skill_process_217(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_ZhiLiaoShouWei:
				skill_process_218(pos->role, pos->order, pos->SkillLevel);

				break;
			case SKILL_TYPE_LiuXingHuoYU:
				skill_process_219(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_ZhanZhengJianTa:
				skill_process_220(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_NuHou:
				skill_process_221(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_ChongFeng:
				skill_process_222(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_ZuiJiuYunWu:
				skill_process_223(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_WuDiZhan:
				skill_process_224(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_ZuZhou:
				skill_process_225(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_NengLiangLiuShi:
				skill_process_226(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_QiangLiYiJi:
				skill_process_227(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_ShuFuZhiJian:
				skill_process_228(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_WeiKunZhiJian:
				skill_process_229(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_SheShouTianFu:
				skill_process_230(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_BenTeng:
				skill_process_231(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_FenShen:
				skill_process_232(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_FeiXueZhiMao:
				skill_process_233(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_XiSheng:
				skill_process_234(pos->role, pos->order, pos->SkillLevel);
				break;
			case SKILL_TYPE_ChenMo:
				skill_process_235(pos->role, pos->order, pos->SkillLevel);
				break;
			default:
				break;
			}
			pos = skill_order_list.erase(pos);
		}
	}
}

void PK_PE::CalculateBuff(RoleState *pRoleState) {
	multimap<short int, Buff>::iterator iter_buff =
			pRoleState->multimap_buff.begin();
	while (iter_buff != pRoleState->multimap_buff.end()) {

		if (iter_buff->second.RunTimeLimit > 0) {
			iter_buff->second.RunCounter++;
			if (iter_buff->second.RunCounter >= iter_buff->second.RunTimeLimit) {
				RunBuffer(*pRoleState, iter_buff->second);
				iter_buff->second.RunCounter = 0;
			}
		}
		if (iter_buff->second.EndTime <= current_time.fragment_now) {
			UpdataBuffer(*pRoleState, iter_buff->second, UPDATABUFFER_DEL);
			if (pRoleState->Live == ROLE_STATUS_DEAD)
				return;
			pRoleState->multimap_buff.erase(iter_buff++);
			if (pRoleState->multimap_buff.size() == 0)
				return;
			continue;
		}
		++iter_buff;
	}
}

void PK_PE::breakLastSkill(RoleState & pTargetRole) {
	for (multimap<short int, Buff>::iterator iter =
			pTargetRole.multimap_buff.begin(); iter
			!= pTargetRole.multimap_buff.end();) {
		if (iter->second.Type == BUFFER_TYPE_CASTSKILL
				&& iter->second.bCanBreak) {
			UpdataBuffer(pTargetRole, iter->second, UPDATABUFFER_DEL);
			Brodacast_Buffer_Stop(pTargetRole.RoleID, iter->second.Type,
					iter->second.ParentSkill, iter->second.BeginTime);
			pTargetRole.multimap_buff.erase(iter++);
		} else {
			iter++;
		}
	}
}

void PK_PE::RunBuffer(RoleState & pTargetRole, Buff & tmpBuff) {
	switch (tmpBuff.Type) {
	case BUFFER_TYPE_CASTSKILL:
		switch (tmpBuff.ParentSkill) {
		case SKILL_TYPE_JianRenFengBao:
			skill_process_204_buffer(&pTargetRole, tmpBuff.Parametar1);
			break;
		case SKILL_TYPE_LiuXingHuoYU:
			skill_process_219_Effect(&pTargetRole, tmpBuff);
			break;
		case SKILL_TYPE_WuDiZhan:
			skill_process_224_Effect(&pTargetRole, tmpBuff);
			break;
		case SKILL_TYPE_ZhiLiaoShouWei:
			skill_process_218_Buffer(&pTargetRole, &tmpBuff);
			break;
		default:
			break;
		}
		break;
	case BUFFER_TYPE_BYSKILL:
		switch (tmpBuff.ParentSkill) {
		case SKILL_TYPE_FanJiLuoXuan:
			tmpBuff.Parametar2++;
			break;
		default:
			break;
		}
		break;
	case BUFFER_TYPE_HP:
		if (pTargetRole.Curr_HP_MAX <= pTargetRole.HP)
			break;
		else {
			short int addHP = 0;
			if (tmpBuff.Effecttype == BUFFER_EFFECT_TYPE_RELATIVE)
				addHP = tmpBuff.Parametar1 * pTargetRole.Curr_HP_MAX / 100;
			else if (tmpBuff.Effecttype == BUFFER_EFFECT_TYPE_ABS)
				addHP = tmpBuff.Parametar1;
			if ((pTargetRole.HP + addHP) > pTargetRole.Curr_HP_MAX)
				addHP = pTargetRole.Curr_HP_MAX - pTargetRole.HP;
			pTargetRole.HP += addHP;
			BrodacastHpChange(pTargetRole.RoleID, addHP,
					HPCHANGE_SOURCE_SKILL, tmpBuff.ParentSkill);
		}
		break;
	default:
		break;
	}
}

void PK_PE::UpdataBuffer(RoleState & pTargetRole, Buff & tmpBuff,
		char updataType) {

	bool bCheck = false;
	char checkType = 0;
	multimap<short int, Buff>::iterator checkBuff;
	if (updataType == UPDATABUFFER_ADD && tmpBuff.Type != BUFFER_TYPE_HP
			&& tmpBuff.Type != BUFFER_TYPE_MP) {
		for (checkBuff = pTargetRole.multimap_buff.begin(); checkBuff
				!= pTargetRole.multimap_buff.end(); checkBuff++) {
			if (checkBuff->second.Type == tmpBuff.Type
					&& checkBuff->second.ParentSkill == tmpBuff.ParentSkill) {
				if (checkBuff->second.Level > tmpBuff.Level) {//已经存在更高级的buffer，忽略当前
					return;
				} else if (checkBuff->second.Level < tmpBuff.Level) {//收到更高级的BUffer，覆盖当前的
					checkType = 1;
					checkBuff->second.EndTime = tmpBuff.EndTime;
				} else {//更新当前BUffer 的时间
					checkBuff->second.EndTime = tmpBuff.EndTime;
					checkType = 2;
				}
				bCheck = true;
				break;
			}
		}
	}
	if (tmpBuff.bSendClient) {
		if (updataType == UPDATABUFFER_ADD)
			Brodacast_Skill_Buffer(pTargetRole.RoleID, tmpBuff.Type,
					tmpBuff.ParentSkill, tmpBuff.EndTime - tmpBuff.BeginTime,
					tmpBuff.Parametar1, tmpBuff.Parametar2);
		else if (updataType == UPDATABUFFER_DEL)
			Brodacast_Buffer_Stop(pTargetRole.RoleID, tmpBuff.Type,
					tmpBuff.ParentSkill, tmpBuff.BeginTime);
	}
	if (checkType == 2) {
		return;
	}
	if (updataType == UPDATABUFFER_ADD) {
		if (!bCheck) {
			if (tmpBuff.Type == BUFFER_TYPE_XUANYUN || tmpBuff.Type
					== BUFFER_TYPE_NOMOVE || tmpBuff.Type
					== BUFFER_TYPE_NOSKILL) {
				breakLastSkill(pTargetRole);
			}
			pTargetRole.multimap_buff.insert(make_pair(tmpBuff.Type, tmpBuff));
		}
	}

	switch (tmpBuff.Type) {
	case BUFFER_TYPE_BYSKILL:
		switch (tmpBuff.ParentSkill) {
		case SKILL_TYPE_ZuiJiuYunWu:
			if (bCheck) {
				pTargetRole.MoveAttrb.SpeedMod -= checkBuff->second.Parametar1;
				pTargetRole.PhysicalAttrib.AttackHit
						-= checkBuff->second.Parametar2;
				checkBuff->second.Parametar1 = tmpBuff.Parametar1;
				checkBuff->second.Parametar2 = tmpBuff.Parametar2;
			}
			pTargetRole.MoveAttrb.SpeedMod += tmpBuff.Parametar1 * updataType;
			pTargetRole.PhysicalAttrib.AttackHit += tmpBuff.Parametar2
					* updataType;
			break;
		case SKILL_TYPE_ZuZhou:
			if (bCheck) {
				pTargetRole.PhysicalAttrib.HurtAdd
						-= (short int) (checkBuff->second.Parametar1 / 100);
				checkBuff->second.Parametar1 = tmpBuff.Parametar1;
			}
			pTargetRole.PhysicalAttrib.HurtAdd
					+= (short int) (tmpBuff.Parametar1 / 100) * updataType;
			if (updataType == UPDATABUFFER_ADD)
				break;
			short int hurt;
			hurt = (tmpBuff.Parametar1 % 100) * tmpBuff.Parametar2 / 100;
			BrodacastHpChange(pTargetRole.RoleID, -hurt, HPCHANGE_SOURCE_SKILL,
					SKILL_TYPE_ZuZhou);
			pTargetRole.HP -= hurt;
			if (pTargetRole.HP <= 0) {
				pTargetRole.HP = 0;
				onRoleDead(pTargetRole);
			}
			break;
		case SKILL_TYPE_WuGuangZhiDun:
			if (updataType == UPDATABUFFER_DEL)
				skill_process_209_buffer(&pTargetRole, &tmpBuff);
			else if (updataType == UPDATABUFFER_ADD) {
				if (bCheck) {
					checkBuff->second.Parametar1 = tmpBuff.Parametar1;
				}
			}
			break;
		case SKILL_TYPE_FanJiLuoXuan:
			if (bCheck) {
				checkBuff->second.Parametar1 = tmpBuff.Parametar1;
				checkBuff->second.Parametar2 = tmpBuff.Parametar2;
			}
			break;
		case SKILL_TYPE_YaoShu:
			if (bCheck) {
				pTargetRole.MoveAttrb.SpeedMod -= checkBuff->second.Parametar1;
				checkBuff->second.Parametar1 = tmpBuff.Parametar1;
			} else {
				pTargetRole.PhysicalAttrib.LimitCounter += updataType;
				pTargetRole.MagicAttrb.LimitCounter += updataType;
			}
			pTargetRole.MoveAttrb.SpeedMod += tmpBuff.Parametar1 * updataType;
			break;
		case SKILL_TYPE_SheShouTianFu:
			if (bCheck) {
				pTargetRole.PhysicalAttrib.AttackCrit
						-= checkBuff->second.Parametar1;
				pTargetRole.PhysicalAttrib.AttackHit
						-= checkBuff->second.Parametar2;
				checkBuff->second.Parametar1 = tmpBuff.Parametar1;
				checkBuff->second.Parametar2 = tmpBuff.Parametar2;
			}
			pTargetRole.PhysicalAttrib.AttackCrit += tmpBuff.Parametar1
					* updataType;
			pTargetRole.PhysicalAttrib.AttackHit += tmpBuff.Parametar2
					* updataType;
			break;
		case SKILL_TYPE_FuHuo:

			pTargetRole.MagicAttrb.ExemptAllCounter += updataType;
			pTargetRole.MoveAttrb.LimitCounter += updataType;
			pTargetRole.PhysicalAttrib.LimitCounter += updataType;
			pTargetRole.MagicAttrb.LimitCounter += updataType;
			pTargetRole.PhysicalAttrib.ExemptCounter += updataType;
			if (updataType == UPDATABUFFER_DEL) {
				pTargetRole.HP = (short int) (pTargetRole.Curr_HP_MAX
						* tmpBuff.Parametar1 / 100);
				pTargetRole.MP = (short int) (pTargetRole.Curr_MP_MAX
						* tmpBuff.Parametar2 / 100);
				BrodacastHpChange(pTargetRole.RoleID, pTargetRole.HP,
						HPCHANGE_SOURCE_SKILL, tmpBuff.ParentSkill);
			}
			break;
		default:
			break;
		}
		break;
	case BUFFER_TYPE_CASTSKILL:
		switch (tmpBuff.ParentSkill) {
		case SKILL_TYPE_JianRenFengBao:
			if (bCheck)
				break;
			pTargetRole.PhysicalAttrib.LimitCounter += updataType;
			pTargetRole.MagicAttrb.LimitCounter += updataType;
			pTargetRole.MagicAttrb.ExemptBadCounter += updataType;
			break;
		case SKILL_TYPE_WuDiZhan:
			if (!bCheck) {
				pTargetRole.MagicAttrb.ExemptAllCounter += updataType;
				pTargetRole.MagicAttrb.LimitCounter += updataType;
				pTargetRole.MoveAttrb.LimitCounter += updataType;
				pTargetRole.PhysicalAttrib.LimitCounter += updataType;
				pTargetRole.PhysicalAttrib.ExemptCounter += updataType;
			}
			break;
		case SKILL_TYPE_ChongFeng:
			if (bCheck)
				break;
			pTargetRole.MagicAttrb.ExemptAllCounter += updataType;
			pTargetRole.MagicAttrb.LimitCounter += updataType;
			pTargetRole.MoveAttrb.LimitCounter += updataType;
			pTargetRole.PhysicalAttrib.LimitCounter += updataType;
			pTargetRole.PhysicalAttrib.ExemptCounter += updataType;
			break;
		case SKILL_TYPE_LiuXingHuoYU:
			if (updataType == UPDATABUFFER_ADD)
				break;
			pTargetRole.CurrentOrder.Type = ORDER_TYPE_NO;
			break;
		default:
			break;
		}
		break;
	case BUFFER_TYPE_CALL:
		if (updataType == UPDATABUFFER_ADD)
			break;
		onRoleDead(pTargetRole);
		break;
	case BUFFER_TYPE_XUANYUN:
		if (bCheck)
			break;
		pTargetRole.PhysicalAttrib.LimitCounter += updataType;
		pTargetRole.MoveAttrb.LimitCounter += updataType;
		pTargetRole.MagicAttrb.LimitCounter += updataType;
		break;
	case BUFFER_TYPE_NOSKILL:
		if (bCheck)
			break;
		pTargetRole.MagicAttrb.LimitCounter += updataType;
		break;
	case BUFFER_TYPE_MOVESPEED:
		if (bCheck) {
			pTargetRole.MoveAttrb.SpeedMod -= checkBuff->second.Parametar1;
			checkBuff->second.Parametar1 = tmpBuff.Parametar1;
		}
		pTargetRole.MoveAttrb.SpeedMod += tmpBuff.Parametar1 * updataType;
		break;
	case BUFFER_TYPE_NOMOVE:
		if (bCheck)
			break;
		pTargetRole.MoveAttrb.LimitCounter += updataType;
		break;
	case BUFFER_TYPE_ARMOR:
		if (bCheck) {
			pTargetRole.PhysicalAttrib.DefenseArmor
					-= checkBuff->second.Parametar1;
			checkBuff->second.Parametar1 = tmpBuff.Parametar1;
		}
		pTargetRole.PhysicalAttrib.DefenseArmor += tmpBuff.Parametar1
				* updataType;
		break;
	case BUFFER_TYPE_ATTACKPOWER:
		if (bCheck) {
			pTargetRole.PhysicalAttrib.AttackPowerAdd
					-= checkBuff->second.Parametar1;
			checkBuff->second.Parametar1 = tmpBuff.Parametar1;
		}
		pTargetRole.PhysicalAttrib.AttackPowerAdd += tmpBuff.Parametar1
				* updataType;
		break;
	case BUFFER_TYPE_MAXHP:
		if (bCheck) {
			pTargetRole.Curr_HP_MAX -= checkBuff->second.Parametar1;
			pTargetRole.HP -= checkBuff->second.Parametar1;
			checkBuff->second.Parametar1 = tmpBuff.Parametar1;
		}
		pTargetRole.Curr_HP_MAX += tmpBuff.Parametar1;
		pTargetRole.HP += tmpBuff.Parametar1;
		break;
	case BUFFER_TYPE_MAXMP:
		if (bCheck) {
			pTargetRole.Curr_MP_MAX -= checkBuff->second.Parametar1;
			checkBuff->second.Parametar1 = tmpBuff.Parametar1;
		}
		pTargetRole.Curr_MP_MAX += tmpBuff.Parametar1;
		break;
	default:
		break;
	}

}

void PK_PE::CalculateAIAttack(RoleState *pRoleStatePassive) {

	if (pRoleStatePassive->MoveAttrb.LimitCounter > 0) {

		RoleState *targetRole = NULL;

		short int distance = pRoleStatePassive->PhysicalAttrib.AttackArea;
//			LOG(LOG_DEBUG, __FILE__, __LINE__, "RoleID[%d],distance[%d]",
//					pRoleStatePassive->RoleID, distance);


		short int tempDis = 0;

		for (map<int, RoleState>::iterator tempRole = P_Roles.begin(); tempRole != P_Roles.end(); tempRole++) {
			if ((tempRole->second.Live != ROLE_STATUS_DEAD)
					&& (tempRole->second.Opposition	!= pRoleStatePassive->Opposition)
					&& (tempRole->second.PhysicalAttrib.ExemptCounter == 0)) {

				tempDis = abs(
						tempRole->second.MoveAttrb.CurrentPos.X
								- pRoleStatePassive->MoveAttrb.CurrentPos.X) + abs(
						tempRole->second.MoveAttrb.CurrentPos.Y
								- pRoleStatePassive->MoveAttrb.CurrentPos.Y);
				if (distance > tempDis) {
					distance = tempDis;
					targetRole = &(tempRole->second);
					break;
				}
			}
		}
		if (targetRole) {
			LOG(LOG_DEBUG, __FILE__, __LINE__, "RoleID[%d],targetID[%d]",
					pRoleStatePassive->RoleID, targetRole->RoleID);
			pRoleStatePassive->CurrentOrder.Type = ORDER_TYPE_ATTACK_E;
			pRoleStatePassive->PhysicalAttrib.PreTimeStatus = 1;
			pRoleStatePassive->CurrentOrder.TargetRoleID = targetRole->RoleID;
			pRoleStatePassive->CurrentOrder.TargetType = 2;
			pRoleStatePassive->CurrentOrder.OrderTime
					= current_time.fragment_now;
		} else {
			pRoleStatePassive->CurrentOrder.Type = ORDER_TYPE_NO;
			pRoleStatePassive->CurrentOrder.TargetRoleID = 0;
		}
		return;
	}
	if (current_time.fragment_now % 30 != 10)
		return;
	map<int, RoleState>::iterator currTarget;
	HaterInfo tmpHartre;
	short int dis;
	short int hp;

	short int maxHarte = 0;
	int maxID = 0;
	short int tmpMaxHarte;
	bool bCheck = false;
	for (currTarget = P_Roles.begin(); currTarget != P_Roles.end(); currTarget++) {
		if (currTarget->second.Live == ROLE_STATUS_DEAD) {
			continue;
		}
		if (pRoleStatePassive->Opposition != currTarget->second.Opposition) {
			tmpHartre.clear();
			bCheck = false;
			tmpMaxHarte = 0;
			dis = (short int) Dist(pRoleStatePassive->MoveAttrb.CurrentPos,
					currTarget->second.MoveAttrb.CurrentPos);
//			LOG(LOG_DEBUG, __FILE__, __LINE__,
//					"yuanshi dis[%d],AttackArea[%d]", dis,
//					pRoleStatePassive->PhysicalAttrib.AttackArea);
			if (dis <= pRoleStatePassive->PhysicalAttrib.AttackArea) {
				dis = 8; //8是最大仇恨值
			} else {
				dis = 8 - (dis - pRoleStatePassive->PhysicalAttrib.AttackArea);
				dis = dis < 1 ? 1 : dis;
			}
			hp = 8 - int(
					(currTarget->second.HP / double(
							currTarget->second.Curr_HP_MAX)) * 10);

			if (hp > 8)
				hp = 8;
			else if (hp < 1)
				hp = 1;
			map<int, HaterInfo>::iterator tmpHatreIter;
			for (tmpHatreIter = pRoleStatePassive->HatreMap.begin(); tmpHatreIter
					!= pRoleStatePassive->HatreMap.end(); tmpHatreIter++) {
				if (tmpHatreIter->first == currTarget->second.RoleID) {
					tmpHatreIter->second.Distance = dis;
					tmpHatreIter->second.Hp = hp;
					tmpHatreIter->second.Attack -= 1;
					if (tmpHatreIter->second.Attack < 1)
						tmpHatreIter->second.Attack = 1;
					tmpHatreIter->second.Hit -= 1;
					if (tmpHatreIter->second.Hit < 1)
						tmpHatreIter->second.Hit = 1;
					tmpMaxHarte = tmpHatreIter->second.max();
//					LOG(LOG_DEBUG, __FILE__, __LINE__,
//							"tmpMaxHarte[%d],tmpHatreIter->second.max()[%d]",
//							tmpMaxHarte, tmpHatreIter->second.max());
					bCheck = true;
//					LOG(
//							LOG_DEBUG,
//							__FILE__,
//							__LINE__,
//							"xiugai:monsterID[%d],targetID[%d],dis[%d],hp[%d],Attack[%d],Hit[%d],Hurt[%d]max[%d]",
//							pRoleStatePassive->RoleID,
//							currTarget->second.RoleID, dis, hp,
//							tmpHatreIter->second.Attack,
//							tmpHatreIter->second.Hit,
//							tmpHatreIter->second.Hurt,
//							tmpHatreIter->second.max());
					break;
				}
			}
			if (!bCheck) {

				tmpHartre.Distance = dis;
				tmpHartre.Hp = hp;
				tmpMaxHarte = tmpHartre.max();
//				LOG(LOG_DEBUG, __FILE__, __LINE__,
//						"tmpMaxHarte[%d],tmpHartre.max()[%d]", tmpMaxHarte,
//						tmpHartre.max());
//				LOG(
//						LOG_DEBUG,
//						__FILE__,
//						__LINE__,
//						"insert:monsterID[%d],targetID[%d],dis[%d],hp[%d]max[%d]",
//						pRoleStatePassive->RoleID, currTarget->second.RoleID,
//						dis, hp, tmpHartre.max());
				pRoleStatePassive->HatreMap.insert(
						make_pair(currTarget->second.RoleID, tmpHartre));
			}

			if (maxHarte < tmpMaxHarte) {
				maxHarte = tmpMaxHarte;
				maxID = currTarget->second.RoleID;
//				LOG(LOG_DEBUG, __FILE__, __LINE__,
//						"maxHarte[%d],tmpMaxHarte[%d],currRoleID[%d]maxID[%d]",
//						maxHarte, tmpMaxHarte, currTarget->second.RoleID, maxID);
			}

		}

	}
//	LOG(LOG_DEBUG, __FILE__, __LINE__, "maxHarte[%d],maxID[%d]", maxHarte,
//			maxID);
	if (maxID == 0)
		pRoleStatePassive->CurrentOrder.Type = ORDER_TYPE_NO;
	else {
		if (pRoleStatePassive->CurrentOrder.Type == ORDER_TYPE_NO
				|| pRoleStatePassive->CurrentOrder.TargetRoleID != maxID) {
			pRoleStatePassive->CurrentOrder.Type = ORDER_TYPE_ATTACK_E;
			pRoleStatePassive->PhysicalAttrib.PreTimeStatus = 1;
			pRoleStatePassive->CurrentOrder.TargetRoleID = maxID;
			pRoleStatePassive->CurrentOrder.TargetType = 2;
			pRoleStatePassive->CurrentOrder.OrderTime
					= current_time.fragment_now;
		}
	}
}
void PK_PE::CalculateSkillHurt(RoleState & pRoleStatePassive,
		short int attackLevel, short int hurt_value, short int effect) {
	LOG(LOG_DEBUG, __FILE__, __LINE__,
			"CalculateSkillHurt,yuanshi hurt_value[%d]", hurt_value);
	double tmp = pRoleStatePassive.Level + pRoleStatePassive.Wisdom / 10
			+ pRoleStatePassive.MagicAttrb.MagicArmor;
	if (tmp != 0)
		hurt_value = (short int) (hurt_value * attackLevel / tmp);
	LOG(LOG_DEBUG, __FILE__, __LINE__,
			"CalculateSkillHurt,tmp[%f],attackLevel[%d],hurt_value[%d]", tmp,
			attackLevel, hurt_value);
	pRoleStatePassive.HP -= hurt_value;
	BrodacastHpChange(pRoleStatePassive.RoleID, -hurt_value,
			HPCHANGE_SOURCE_SKILL, effect);
	if (pRoleStatePassive.HP <= 0) {
		onRoleDead(pRoleStatePassive);
	}
}
void PK_PE::CalculateMultiSkillHurt(map<int, short int> & target,
		const short int attackLevel, short int effect) {

	map<int, short int>::iterator tmpInfo = target.begin();
	map<int, RoleState>::iterator pos_target;
	char bRoleDeadOpp = -1;
	for (; tmpInfo != target.end(); tmpInfo++) {
		pos_target = P_Roles.find(tmpInfo->first);
		if (pos_target == P_Roles.end()) {
			continue;
		}
		double tmp = pos_target->second.Level + pos_target->second.Wisdom / 10
				+ pos_target->second.MagicAttrb.MagicArmor;
		if (tmp != 0)
			tmpInfo->second = int(tmpInfo->second * attackLevel / tmp);
		pos_target->second.HP -= tmpInfo->second;
		if (pos_target->second.HP <= 0) {
			pos_target->second.HP = 0;
			pos_target->second.Live = ROLE_STATUS_DEAD;
			BrodacastDead(pos_target->second.RoleID);
			bRoleDeadOpp = pos_target->second.Opposition;
		}
	}
	BrodacastMultiHurt(target, effect);
	if (bRoleDeadOpp != -1)
		CheckOver(bRoleDeadOpp);
}

//反击螺旋的BUFFER处理
void PK_PE::skill_process_210_buffer(RoleState *pRoleState, Buff* p210Buffer) {
	LOG(LOG_DEBUG, __FILE__, __LINE__,
			"skill_process_210_buffer,p210Buffer->Parametar2[%d]",
			p210Buffer->Parametar2);
	//Parameter2 : percent
	if (p210Buffer->Parametar2 >= 10) {
		if ((rand() % 100) <= p210Buffer->Parametar2) {
			Brodacast_Buffer_Run(pRoleState->RoleID, SKILL_TYPE_FanJiLuoXuan,
					p210Buffer->BeginTime);
			p210Buffer->Parametar2 = 0;
			map<int, RoleState>::iterator role_target;
			short int nX = 0;
			short int nY = 0;
			for (role_target = P_Roles.begin(); role_target != P_Roles.end(); role_target++) {
				if (role_target->second.Live == ROLE_STATUS_DEAD)
					continue;
				if (role_target->second.Opposition == pRoleState->Opposition)
					continue;
				if (role_target->second.MagicAttrb.ExemptAllCounter > 0)
					continue;
				if (role_target->second.MagicAttrb.ExemptBadCounter > 0)
					continue;

				nX = pRoleState->MoveAttrb.CurrentPos.X
						- role_target->second.MoveAttrb.CurrentPos.X;
				nY = pRoleState->MoveAttrb.CurrentPos.Y
						- role_target->second.MoveAttrb.CurrentPos.Y;
				LOG(LOG_DEBUG, __FILE__, __LINE__,
						"TargetRoleID[%d],nX[%d],nY[%d]",
						role_target->second.RoleID, nX, nY);
				if ((nX * nX + nY * nY) <= 9) { //默认3范围内
					CalculateSkillHurt(role_target->second, pRoleState->Level,
							p210Buffer->Parametar1, SKILL_TYPE_FanJiLuoXuan);
				}
			}
		}
	}
}

void PK_PE::skill_process_218_Buffer(RoleState * pRoleState, Buff* tmpBuff) {
	map<int, RoleState>::iterator role_target;
	short int nX = 0;
	short int nY = 0;
	for (role_target = P_Roles.begin(); role_target != P_Roles.end(); role_target++) {
		if (role_target->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (role_target->second.Opposition != pRoleState->Opposition)
			continue;
		if (role_target->second.MagicAttrb.ExemptAllCounter > 0)
			continue;
		if (role_target->second.HP >= role_target->second.Curr_HP_MAX)
			continue;

		nX = pRoleState->MoveAttrb.CurrentPos.X
				- role_target->second.MoveAttrb.CurrentPos.X;
		nY = pRoleState->MoveAttrb.CurrentPos.Y
				- role_target->second.MoveAttrb.CurrentPos.Y;

		if ((nX * nX + nY * nY) <= 13*13) {
			short int hp = min(
					role_target->second.Curr_HP_MAX - role_target->second.HP,
					(int) (role_target->second.Curr_HP_MAX
							* tmpBuff->Parametar1 / 100));
			if (hp > 0) {
				role_target->second.HP += hp;
				BrodacastHpChange(role_target->second.RoleID, hp,
						HPCHANGE_SOURCE_SKILL, SKILL_TYPE_ZhiLiaoShouWei);
			}
		}
	}
}

//流星火雨特效
void PK_PE::skill_process_219_Effect(RoleState * pRoleState, Buff & tmpBuff) {
	Pos targetPos;
	targetPos.X = tmpBuff.Parametar2 % MAP_SIZE_X;
	targetPos.Y = (short int) (tmpBuff.Parametar2 / MAP_SIZE_X);
	map<int, RoleState>::iterator tmpTarget;
	map<int, short int> targetRole;

	int radius = 3;
	switch (tmpBuff.Level) {
	case 1:
		radius = 3;
		break;
	case 2:
		radius = 3;
		break;
	case 3:
		radius = 5;
		break;
	case 4:
		radius = 5;
		break;
	case 5:
		radius = 7;
		break;
	case 6:
		radius = 7;
		break;
	default:
		break;
	}

	for (tmpTarget = P_Roles.begin(); tmpTarget != P_Roles.end(); tmpTarget++) {
		if (tmpTarget->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (tmpTarget->second.Opposition == pRoleState->Opposition)
			continue;
		if (tmpTarget->second.MagicAttrb.ExemptAllCounter > 0)
			continue;
		if (tmpTarget->second.MagicAttrb.ExemptBadCounter > 0)
			continue;
		if (abs(tmpTarget->second.MoveAttrb.CurrentPos.X - targetPos.X) > radius
				|| abs(tmpTarget->second.MoveAttrb.CurrentPos.Y - targetPos.Y)
						> 5)
			continue;
		if (Dist(tmpTarget->second.MoveAttrb.CurrentPos, targetPos) <= radius) {
			targetRole.insert(
					make_pair(tmpTarget->second.RoleID, tmpBuff.Parametar1));
		}
	}
	if (targetRole.size() > 0) {
		CalculateMultiSkillHurt(targetRole, pRoleState->Level,
				SKILL_TYPE_LiuXingHuoYU);
	}
}

void PK_PE::skill_process_223_Effect(Pos target_pos, char Opposition,
		char level) {
	LOG(LOG_DEBUG, __FILE__, __LINE__, "skill_process_223_Effect");
	unsigned short int keep_time = 60;
	short int moveParam = -20;
	short int hitParam = -5;
	switch (level) {
	case 1:
		moveParam = -20;
		hitParam = -5;
		break;
	case 2:
		moveParam = -25;
		hitParam = -10;
		break;
	case 3:
		moveParam = -30;
		hitParam = -15;
		break;
	case 4:
		moveParam = -35;
		hitParam = -20;
		break;
	case 5:
		moveParam = -40;
		hitParam = -25;
		break;
	case 6:
		moveParam = -45;
		hitParam = -30;
		break;
	}
	map<int, RoleState>::iterator role_target;
	short int nX = 0;
	short int nY = 0;
	for (role_target = P_Roles.begin(); role_target != P_Roles.end(); role_target++) {
		if (role_target->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (role_target->second.Opposition == Opposition)
			continue;
		if (role_target->second.MagicAttrb.ExemptBadCounter > 0)
			continue;
		if (role_target->second.MagicAttrb.ExemptAllCounter > 0)
			continue;

		nX = target_pos.X - role_target->second.MoveAttrb.CurrentPos.X;
		nY = target_pos.Y - role_target->second.MoveAttrb.CurrentPos.Y;
		LOG(LOG_DEBUG, __FILE__, __LINE__, "RoleID[%d],nX[%d],nY[%d]",
				role_target->second.RoleID, nX, nY);
		if ((nX * nX + nY * nY) <= 9) {//范围3
			Buff Buff_tmp(BUFFER_TYPE_BYSKILL, SKILL_TYPE_ZuiJiuYunWu, level,
					current_time.fragment_now, keep_time, true, true);
			Buff_tmp.Effecttype = BUFFER_EFFECT_TYPE_RELATIVE;
			Buff_tmp.Parametar1 = role_target->second.MoveAttrb.SpeedMod * moveParam/100;
			Buff_tmp.Parametar2 = role_target->second.PhysicalAttrib.AttackHit*hitParam/100;

			UpdataBuffer(role_target->second, Buff_tmp, UPDATABUFFER_ADD);
		}
	}
}

//醉酒云雾
void PK_PE::skill_process_223(RoleState * pRoleState, RoleOrder role_order,
		char level) {

	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}
	Bullet BulletsTmp;
	BulletsTmp.Start_Time = 3;
	BulletsTmp.Role_Attack = pRoleState;
	BulletsTmp.Role_Passive = &pos_target->second;
	BulletsTmp.CurrentPos.X = pRoleState->MoveAttrb.CurrentPos.X;
	BulletsTmp.CurrentPos.Y = pRoleState->MoveAttrb.CurrentPos.Y;
	BulletsTmp.BulletType = SKILL_TYPE_ZuiJiuYunWu;
	BulletsTmp.Speed = 3;
	BulletsTmp.Level = level;
	Bullets.push_back(BulletsTmp);

}

//无敌斩
void PK_PE::skill_process_224_Effect(RoleState * pRoleState, Buff & tmpBuff) {
	LOG(LOG_DEBUG, __FILE__, __LINE__, "skill_process_224_Effect");
	unsigned int uiTime = time(NULL);
	srand(uiTime);

	int targetID = -1;
	short int targetX = -1;
	short int targetY = -1;
	short int roleCounter = P_Roles.size();
	short int randID;
	short int totalLoop = roleCounter * 3;
	map<int, RoleState>::iterator targetRole;
	short int num = 0;
	while (true) {
		num++;
		if (num > totalLoop) {
			break;
		}
		randID = rand() % (roleCounter);
		targetRole = P_Roles.begin();
		for (short int i = 0; i < randID; i++)
			targetRole++;
		if (targetRole->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (pRoleState->Opposition == targetRole->second.Opposition)
			continue;
		if (targetRole->second.PhysicalAttrib.ExemptCounter > 0)
			continue;

		targetID = targetRole->second.RoleID;
		targetX = targetRole->second.MoveAttrb.CurrentPos.X;
		targetY = targetRole->second.MoveAttrb.CurrentPos.Y;
		break;
	}
	if (targetID != -1) {
		short int xRand;
		short int yRand;
		while (true) {
			short int tmpX = 0;
			short int tmpY = 0;
			switch (rand() % 8) {
			case 1:
				xRand = -1;
				yRand = 1;
				break;
			case 2:
				xRand = 0;
				yRand = 1;
				break;
			case 3:
				xRand = 1;
				yRand = 1;
				break;
			case 4:
				xRand = -1;
				yRand = 0;
				break;
			case 5:
				xRand = 1;
				yRand = 0;
				break;
			case 6:
				xRand = -1;
				yRand = -1;
				break;
			case 7:
				xRand = 0;
				yRand = -1;
				break;
			case 8:
				xRand = 1;
				yRand = -1;
				break;
			default:
				xRand = 1;
				yRand = -1;
				break;
			}
			tmpX = targetX + xRand;
			tmpY = targetY + yRand;
			if (tmpX < 0 || tmpX >= MAP_SIZE_X || tmpY < 0 || tmpY
					>= MAP_SIZE_Y)
				continue;
			else
				break;
		}

		pRoleState->MoveAttrb.CurrentPos.X = targetX + xRand;
		pRoleState->MoveAttrb.CurrentPos.Y = targetY + yRand;
		Brodacast_Role_LocationOffset(pRoleState->RoleID,
				pRoleState->MoveAttrb.CurrentPos.X,
				pRoleState->MoveAttrb.CurrentPos.Y, SKILL_TYPE_WuDiZhan);

//		LOG(LOG_DEBUG, __FILE__, __LINE__, "00000000000000  RoleID[%d],nX[%d],nY[%d]",
//				pRoleState->RoleID, pRoleState->MoveAttrb.CurrentPos.X, pRoleState->MoveAttrb.CurrentPos.Y);
		CalculateSkillHurt(targetRole->second, pRoleState->Level,
				tmpBuff.Parametar1, SKILL_TYPE_WuDiZhan);
	}
}

//复活
void PK_PE::skill_process_236(RoleState * pRoleState, RoleOrder role_order,
		char level) {

}

//沉默
void PK_PE::skill_process_235(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	short int keepTime = 20;
	switch (level) {
	case 1:
		keepTime = 20;
		break;
	case 2:
		keepTime = 20;
		break;
	case 3:
		keepTime = 30;
		break;
	case 4:
		keepTime = 30;
		break;
	case 5:
		keepTime = 40;
		break;
	case 6:
		keepTime = 50;
		break;
	default:
		break;
	}
	map<int, RoleState>::iterator targetRole = P_Roles.begin();
	short int nX, nY;
	for (; targetRole != P_Roles.end(); targetRole++) {
		if (targetRole->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (targetRole->second.Opposition == pRoleState->Opposition)
			continue;
		if (targetRole->second.MagicAttrb.ExemptAllCounter > 0)
			continue;
		if (targetRole->second.MagicAttrb.ExemptBadCounter > 0)
			continue;
		nX = role_order.TargetPos.X - targetRole->second.MoveAttrb.CurrentPos.X;
		nY = role_order.TargetPos.Y - targetRole->second.MoveAttrb.CurrentPos.Y;
		if ((nX * nX + nY * nY) <= 16) {
			Buff tmpBuff(BUFFER_TYPE_NOSKILL, SKILL_TYPE_ChenMo, level,
					current_time.fragment_now, keepTime, false, true);
			UpdataBuffer(targetRole->second, tmpBuff, UPDATABUFFER_ADD);
		}
	}

}

//牺牲
void PK_PE::skill_process_234(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}
	short int attackHurt = (short int) (pRoleState->HP * 0.6);
	float targetHurt = pos_target->second.HP;
	switch (level) {
	case 1:
		targetHurt *= 0.4;
		break;
	case 2:
		targetHurt *= 0.45;
		break;
	case 3:
		targetHurt *= 0.5;
		break;
	case 4:
		targetHurt *= 0.55;
		break;
	case 5:
		targetHurt *= 0.6;
		break;
	case 6:
		targetHurt *= 0.65;
		break;
	default:
		break;
	}
	pRoleState->HP -= attackHurt;
	pos_target->second.HP -= (int)targetHurt;
	BrodacastHpChange(pRoleState->RoleID, -attackHurt, HPCHANGE_SOURCE_SKILL,
			SKILL_TYPE_XiSheng);
	BrodacastHpChange(pos_target->second.RoleID, -targetHurt,
			HPCHANGE_SOURCE_SKILL, SKILL_TYPE_XiSheng);
}

//沸血之矛
void PK_PE::skill_process_233(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	short int keepTime = 100;
	short int param2 = 6;
	switch (level) {
	case 1:
		param2 = 6;
		break;
	case 2:
		param2 = 8;
		break;
	case 3:
		param2 = 10;
		break;
	case 4:
		param2 = 12;
		break;
	case 5:
		param2 = 14;
		break;
	case 6:
		param2 = 16;
		keepTime = 150;
		break;
	default:
		break;
	}
	Buff tmpBuff(BUFFER_TYPE_BYSKILL, SKILL_TYPE_FeiXueZhiMao, level,
			current_time.fragment_now, keepTime, true, true);
	tmpBuff.Parametar1 = 0;
	tmpBuff.Parametar2 = param2;
	UpdataBuffer(*pRoleState, tmpBuff, UPDATABUFFER_ADD);
}

//分身
void PK_PE::skill_process_232(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	int loop_id;
	for (loop_id = pRoleState->ControlID + 3000000; loop_id
			< (pRoleState->ControlID + 4000000); loop_id++) {

		map<int, RoleState>::iterator pos_loop_id;
		pos_loop_id = P_Roles.find(loop_id);
		if (pos_loop_id == P_Roles.end()) {
			break;
		}
	}
	LOG(LOG_DEBUG, __FILE__, __LINE__, "new Role id[%d]", loop_id);
	Pos newPos;
	GetEmptyPos(pRoleState->MoveAttrb.CurrentPos.X,
			pRoleState->MoveAttrb.CurrentPos.Y, pRoleState->MoveAttrb.Direct,
			&newPos);

	unsigned short int keepTime = 100;
	short int AttackPowerAdd = 30-100;
	unsigned short int HurtAdd = 450-100;

	switch (level) {
	case 1:
		AttackPowerAdd = 30-100;
		HurtAdd = 450-100;
		break;
	case 2:
		AttackPowerAdd = 35-100;
		HurtAdd = 400-100;
		break;
	case 3:
		AttackPowerAdd = 40-100;
		HurtAdd = 350-100;
		break;
	case 4:
		AttackPowerAdd = 45-100;
		HurtAdd = 300-100;
		break;
	case 5:
		AttackPowerAdd = 50-100;
		HurtAdd = 250-100;
		break;
	case 6:
		AttackPowerAdd = 60-100;
		HurtAdd = 200-100;
		break;
	default:
		break;
	}

	RoleState stRoleState_tmp;
	stRoleState_tmp.ControlID = pRoleState->ControlID;
	stRoleState_tmp.RoleID = loop_id;
	stRoleState_tmp.CurrentOrder.Type = ORDER_TYPE_NO;
	stRoleState_tmp.HP_MAX = pRoleState->HP_MAX;
	stRoleState_tmp.HP = pRoleState->HP;
	stRoleState_tmp.Level = pRoleState->Level;
	stRoleState_tmp.Live = ROLE_STATUS_LIVE;
	stRoleState_tmp.MagicAttrb.MagicArmor = 0;
	stRoleState_tmp.MagicAttrb.MagicArmor_Base = 0;

	stRoleState_tmp.MoveAttrb.CurrentPosMini.X = 500;
	stRoleState_tmp.MoveAttrb.CurrentPosMini.Y = 500;
	stRoleState_tmp.MoveAttrb.Direct = pRoleState->MoveAttrb.Direct;

	stRoleState_tmp.MoveAttrb.CurrentPos.X = newPos.X;
	stRoleState_tmp.MoveAttrb.CurrentPos.Y = newPos.Y;
	stRoleState_tmp.MoveAttrb.SpeedMod = pRoleState->MoveAttrb.SpeedMod_Base;
	stRoleState_tmp.MoveAttrb.SpeedMod_Base
			= pRoleState->MoveAttrb.SpeedMod_Base;
	stRoleState_tmp.MagicAttrb.MagicArmor = 0;
	stRoleState_tmp.MagicAttrb.MagicArmor_Base = 0;
	stRoleState_tmp.Attack_Skill_WaitingTime = 0;

	stRoleState_tmp.MP = 0;
	stRoleState_tmp.MP_MAX = 0;
	stRoleState_tmp.Opposition = pRoleState->Opposition;
	//stRoleState_tmp.Origin = ORIGIN_CALL;

	stRoleState_tmp.PhysicalAttrib = pRoleState->PhysicalAttrib;
	stRoleState_tmp.PhysicalAttrib.PreAttackTime = 0;
	stRoleState_tmp.PhysicalAttrib.PreTimeStatus = 0;
	stRoleState_tmp.PhysicalAttrib.AttackPowerAdd = AttackPowerAdd;
	stRoleState_tmp.PhysicalAttrib.HurtAdd = HurtAdd;
	stRoleState_tmp.PK_State = PK_STATE_PKING;
	stRoleState_tmp.RoleFlag = RoleType_Call;
	stRoleState_tmp.RoleType = pRoleState->RoleType;

	Buff Buff_tmp(BUFFER_TYPE_CALL, SKILL_TYPE_FenShen, level,
			current_time.fragment_now, keepTime, false, false);
	UpdataBuffer(stRoleState_tmp, Buff_tmp, UPDATABUFFER_ADD);
	P_Roles.insert(make_pair(stRoleState_tmp.RoleID, stRoleState_tmp));

	char new_role_msg[39 + 1] = { 0 };
	int pos_len = 0;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleID, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &current_time.fragment_now, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &keepTime, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.ControlID, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleFlag, 1);
	pos_len += 1;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleType, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &pRoleState->RoleID, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len,
			&stRoleState_tmp.PhysicalAttrib.AttackBulletMod, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.HP_MAX, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MP_MAX, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.HP, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MP, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.SpeedMod_Base, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.CurrentPos.X, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.CurrentPos.Y, 2);
	pos_len += 2;
	short int effect = SKILL_TYPE_FenShen;
	memcpy(new_role_msg + pos_len, &effect, 2);
	pos_len += 2;
	Brodacast_Skill_Newrole(new_role_msg);
}

//奔腾
void PK_PE::skill_process_231(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}
	short int moveParam = 5;
	short int keepTime = 30;
	switch (level) {
	case 1:
		moveParam = 5;
		break;
	case 2:
		moveParam = 10;
		break;
	case 3:
		moveParam = 15;
		break;
	case 4:
		moveParam = 20;
		break;
	case 5:
		moveParam = 25;
		break;
	case 6:
		moveParam = 30;
		keepTime = 50;
		break;
	default:
		break;
	}
	Buff tmpBuff(BUFFER_TYPE_MOVESPEED, SKILL_TYPE_BenTeng, level,
			current_time.fragment_now, keepTime, true, true);
	tmpBuff.Parametar1 = moveParam * pos_target->second.MoveAttrb.SpeedMod_Base
			/ 100;
	UpdataBuffer(pos_target->second, tmpBuff, UPDATABUFFER_ADD);
}

//射手天赋
void PK_PE::skill_process_230(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	short int critParam = 2;
	short int hitParam = 2;
	short int keepTime = 80;
	switch (level) {
	case 1:
		critParam = 2;
		hitParam = 2;
		break;
	case 2:
		critParam = 4;
		hitParam = 4;
		break;
	case 3:
		critParam = 6;
		hitParam = 6;
		break;
	case 4:
		critParam = 8;
		hitParam = 8;
		break;
	case 5:
		critParam = 10;
		hitParam = 10;
		break;
	case 6:
		critParam = 15;
		hitParam = 15;
		keepTime = 100;
		break;
	default:
		break;
	}
	Buff tmpBuff(BUFFER_TYPE_BYSKILL, SKILL_TYPE_SheShouTianFu, level,
			current_time.fragment_now, keepTime, true, true);
	tmpBuff.Parametar1 = critParam * pRoleState->PhysicalAttrib.AttackCrit_Base
			/ 100;
	tmpBuff.Parametar2 = hitParam * pRoleState->PhysicalAttrib.AttackHit_Base
			/ 100;
	UpdataBuffer(*pRoleState, tmpBuff, UPDATABUFFER_ADD);
}

//围困之箭
void PK_PE::skill_process_229(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	short int hurt = 100;
	short int radius = 3;
	switch (level) {
	case 1:
		hurt = 100;
		radius = 3;
		break;
	case 2:
		hurt = 200;
		radius = 3;
		break;
	case 3:
		hurt = 250;
		radius = 5;
		break;
	case 4:
		hurt = 350;
		radius = 5;
		break;
	case 5:
		hurt = 400;
		radius = 7;
		break;
	case 6:
		hurt = 600;
		radius = 7;
		break;
	default:
		break;
	}
	map<int, RoleState>::iterator targetRole = P_Roles.begin();
	short int nX, nY;
	for (; targetRole != P_Roles.end(); targetRole++) {
		if (targetRole->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (targetRole->second.Opposition == pRoleState->Opposition)
			continue;
		if (targetRole->second.MagicAttrb.ExemptAllCounter > 0)
			continue;
		if (targetRole->second.MagicAttrb.ExemptBadCounter > 0)
			continue;
		nX = pRoleState->MoveAttrb.CurrentPos.X
				- targetRole->second.MoveAttrb.CurrentPos.X;
		nY = pRoleState->MoveAttrb.CurrentPos.Y
				- targetRole->second.MoveAttrb.CurrentPos.Y;
		if ((nX * nX + nY * nY) <= radius) {
			CalculateSkillHurt(targetRole->second, pRoleState->Level, hurt,
					SKILL_TYPE_WeiKunZhiJian);
		}
	}
}

//束缚之箭
void PK_PE::skill_process_228(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}
	//short int hurt = 0;
	short int lastTime = 20;
	short int radius = 3;
	switch (level) {
	case 1:
		lastTime = 20;
		radius = 3;
		break;
	case 2:
		lastTime = 30;
		radius = 3;
		break;
	case 3:
		lastTime = 30;
		radius = 5;
		break;
	case 4:
		lastTime = 40;
		radius = 5;
		break;
	case 5:
		lastTime = 40;
		radius = 7;
		break;
	case 6:
		lastTime = 50;
		radius = 7;
		break;
	default:
		break;
	}
	map<int, RoleState>::iterator targetRole = P_Roles.begin();
	short int nX, nY;
	for (; targetRole != P_Roles.end(); targetRole++) {
		if (targetRole->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (targetRole->second.Opposition == pRoleState->Opposition)
			continue;
		if (targetRole->second.MagicAttrb.ExemptAllCounter > 0)
			continue;
		if (targetRole->second.MagicAttrb.ExemptBadCounter > 0)
			continue;
		nX = pos_target->second.MoveAttrb.CurrentPos.X
				- targetRole->second.MoveAttrb.CurrentPos.X;
		nY = pos_target->second.MoveAttrb.CurrentPos.Y
				- targetRole->second.MoveAttrb.CurrentPos.Y;
		if ((nX * nX + nY * nY) <= radius*radius) {
			Buff tmpBuff(BUFFER_TYPE_XUANYUN, SKILL_TYPE_ShuFuZhiJian, level,
					current_time.fragment_now, lastTime, false, true);
			UpdataBuffer(targetRole->second, tmpBuff, UPDATABUFFER_ADD);
		}
	}
}

//强力一击
void PK_PE::skill_process_227(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	int hurt = 150;
	int len = 5;
	switch (level) {
	case 1:
		hurt = 150;
		len = 5;
		break;
	case 2:
		hurt = 200;
		len = 7;
		break;
	case 3:
		hurt = 250;
		len = 9;
		break;
	case 4:
		hurt = 300;
		len = 11;
		break;
	case 5:
		hurt = 350;
		len = 13;
		break;
	case 6:
		hurt = 500;
		len = 15;
		break;
	default:
		break;
	}
	Pos startPos = pRoleState->MoveAttrb.CurrentPos;
	Pos endPos, tmpPos;
	if (role_order.TargetType == 1) {
		tmpPos = role_order.TargetPos;
	} else {
		map<int, RoleState>::iterator pos_target;
		pos_target = P_Roles.find(role_order.TargetRoleID);
		if (pos_target == P_Roles.end()) {
			return;
		}
		tmpPos = pos_target->second.MoveAttrb.CurrentPos;
	}

	int xDist = tmpPos.X - startPos.X;
	int yDist = tmpPos.Y - startPos.Y;
	int dist = (int)sqrt(xDist * xDist + yDist * yDist);

	if(dist==0){
		dist=1; //避免除0错误。
	}
	endPos.X = startPos.X + len * xDist / dist;
	endPos.Y = startPos.Y + len * yDist / dist;
	if (endPos.X >= MAP_SIZE_X || endPos.X < 0 || endPos.Y > MAP_SIZE_Y
			|| endPos.Y < 0) {
		int xTotalDist;
		int yTotalDist;
		if (xDist > 0) {
			xTotalDist = MAP_SIZE_X - startPos.X - 1;
		} else {
			xTotalDist = -startPos.X;
		}
		if (yDist > 0) {
			yTotalDist = MAP_SIZE_Y - startPos.Y - 1;
		} else {
			yTotalDist = -startPos.Y;
		}
		Pos endPosByX;
		endPosByX.X = xTotalDist + startPos.X;
		endPosByX.Y = startPos.Y + yDist * xTotalDist / xDist;
		if (endPosByX.Y >= 0 && endPosByX.Y < MAP_SIZE_Y) {
			endPos.X = endPosByX.X;
			endPos.Y = endPosByX.Y;
		} else {
			endPos.X = startPos.X + xDist * yTotalDist / yDist;
			endPos.Y = yTotalDist + startPos.Y;
		}
	}

//	LOG(
//			LOG_DEBUG,
//			__FILE__,
//			__LINE__,
//			"1111111 [%d]: startPos.X[%d] startPos.Y[%d] endPos.X[%d] endPos.Y[%d]!",
//			pRoleState->RoleID, startPos.X, startPos.Y, endPos.X, endPos.Y);

	map<int, RoleState>::iterator targetRole = P_Roles.begin();
	for (; targetRole != P_Roles.end(); targetRole++) {
		if (targetRole->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (targetRole->second.Opposition == pRoleState->Opposition)
			continue;
		if (targetRole->second.MagicAttrb.ExemptAllCounter > 0)
			continue;
		if (targetRole->second.MagicAttrb.ExemptBadCounter > 0)
			continue;
		if (Pnt2SegmentDist(startPos, endPos,
				targetRole->second.MoveAttrb.CurrentPos) < 3) {
			CalculateSkillHurt(targetRole->second, hurt, pRoleState->Level,
					SKILL_TYPE_QiangLiYiJi);
		}
	}
}

//能量流失
void PK_PE::skill_process_226(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}
	unsigned short int keepTime = 50;
	//int param1 = 1;
	int param2 = 1;
	switch (level) {
	case 1:
		//param1 = 1;
		param2 = 1;
		break;
	case 2:
		//param1 = 2;
		param2 = 2;
		break;
	case 3:
		//param1 = 3;
		param2 = 3;
		break;
	case 4:
		//param1 = 4;
		param2 = 4;
		break;
	case 5:
		//param1 = 5;
		param2 = 5;
		break;
	case 6:
		//param1 = 6;
		param2 = 6;
		break;
	default:
		break;
	}
	Buff Buff_tmp(BUFFER_TYPE_BYSKILL, SKILL_TYPE_NengLiangLiuShi, level,
			current_time.fragment_now, keepTime, true, true);
	//Buff_tmp.Parametar1 = param1;
	Buff_tmp.Parametar2 = param2;

	Buff_tmp.Effecttype = BUFFER_EFFECT_TYPE_RELATIVE;
	UpdataBuffer(pos_target->second, Buff_tmp, UPDATABUFFER_ADD);
}

//诅咒
void PK_PE::skill_process_225(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}
	short int firstAdd = 5;
	short int secondAdd = 5;
	unsigned short int keepTime = 40;
	switch (level) {
	case 1:
		keepTime = 40;
		firstAdd = 5;
		secondAdd = 5;
		break;
	case 2:
		keepTime = 40;
		firstAdd = 6;
		secondAdd = 6;
		break;
	case 3:
		keepTime = 60;
		firstAdd = 6;
		secondAdd = 6;
		break;
	case 4:
		keepTime = 60;
		firstAdd = 7;
		secondAdd = 7;
		break;
	case 5:
		keepTime = 80;
		firstAdd = 8;
		secondAdd = 8;
		break;
	case 6:
		keepTime = 80;
		firstAdd = 10;
		secondAdd = 10;
		break;
	default:
		break;
	}
	Buff Buff_tmp(BUFFER_TYPE_BYSKILL, SKILL_TYPE_ZuZhou, level,
			current_time.fragment_now, keepTime, true, true);
	Buff_tmp.Parametar1 = firstAdd * 100 + secondAdd;
	UpdataBuffer(pos_target->second, Buff_tmp, UPDATABUFFER_ADD);
}

//无敌斩
void PK_PE::skill_process_224(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	unsigned short int keepTime = 40;
	short int hurt = 80;
	switch (level) {
	case 1:
		keepTime = 40;
		hurt = 80;
		break;
	case 2:
		keepTime = 60;
		hurt = 80;
		break;
	case 3:
		keepTime = 80;
		hurt = 120;
		break;
	case 4:
		keepTime = 100;
		hurt = 120;
		break;
	case 5:
		keepTime = 120;
		hurt = 160;
		break;
	case 6:
		keepTime = 140;
		hurt = 200;
		break;
	default:
		break;
	}
	Buff Buff_tmp(BUFFER_TYPE_CASTSKILL, SKILL_TYPE_WuDiZhan, level,
			current_time.fragment_now, keepTime, false, true);
	Buff_tmp.RunTimeLimit = 5;
	Buff_tmp.Parametar1 = hurt;
	UpdataBuffer(*pRoleState, Buff_tmp, UPDATABUFFER_ADD);
}

//冲锋   ，对一条线上的敌人伤害，并移动
void PK_PE::skill_process_222(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	short int hurt = 100;
	short int range = 5;
	switch (level) {
	case 1:
		hurt = 100;
		range = 5;
		break;
	case 2:
		hurt = 200;
		range = 7;
		break;
	case 3:
		hurt = 300;
		range = 7;
		break;
	case 4:
		hurt = 400;
		range = 9;
		break;
	case 5:
		hurt = 500;
		range = 9;
		break;
	case 6:
		hurt = 600;
		range = 11;
		break;
	default:
		break;
	}
	Pos startPos = pRoleState->MoveAttrb.CurrentPos;
	Pos tempPos, endPos;
	if (role_order.TargetType == 1) {
		tempPos = role_order.TargetPos;
	} else {
		map<int, RoleState>::iterator pos_target;
		pos_target = P_Roles.find(role_order.TargetRoleID);
		if (pos_target == P_Roles.end()) {
			return;
		}
		tempPos = pos_target->second.MoveAttrb.CurrentPos;
	}
	short int dist = (short int) Dist(tempPos, startPos);
	short int xDist = tempPos.X - startPos.X;
	short int yDist = tempPos.Y - startPos.Y;

	if(0==dist)
		dist =1;

	endPos.X = startPos.X + range * xDist / dist;
	endPos.Y = startPos.Y + range * yDist / dist;
	if (endPos.X >= MAP_SIZE_X || endPos.X < 0 || endPos.Y >= MAP_SIZE_Y
			|| endPos.Y < 0) {
		short int xTotalDist;
		short int yTotalDist;
		if (xDist > 0) {
			xTotalDist = MAP_SIZE_X - 1 - startPos.X;
		} else {
			xTotalDist = -startPos.X;
		}
		if (yDist > 0) {
			yTotalDist = MAP_SIZE_Y - 1 - startPos.Y;
		} else {
			yTotalDist = -startPos.Y;
		}

		Pos endPosByX;
		Pos endPosByY;

		endPosByX.X = xTotalDist + startPos.X;
		endPosByX.Y = startPos.Y + yDist * xTotalDist / xDist;

		endPosByY.X = startPos.X + xDist * yTotalDist / yDist;
		endPosByY.Y = yTotalDist + startPos.Y;

		if (endPosByX.Y >= 0 && endPosByX.Y < MAP_SIZE_Y) {
			endPos.X = endPosByX.X;
			endPos.Y = endPosByX.Y;
		} else if (endPosByY.X >= 0 && endPosByY.X < MAP_SIZE_X) {
			endPos.X = endPosByY.X;
			endPos.Y = endPosByY.Y;
		}
	}

	if (pk_map_base[endPos.Y][endPos.X] == 0) {
		short int circle = 0;
		bool bFind = false;
		for (circle = 1; circle <= range; circle++) {
			for (int xloop = endPos.X - circle; xloop < endPos.X + circle + 1; xloop++) {
				for (int yloop = endPos.Y - circle; yloop < endPos.Y + circle
						+ 1; yloop++) {
					if (xloop >= 0 && xloop < MAP_SIZE_X && yloop >= 0 && yloop
							< MAP_SIZE_Y) {
						if (pk_map_base[yloop][xloop] != 0) {
							endPos.X = xloop;
							endPos.Y = yloop;
							bFind = true;
							break;
						}
					}
				}
				if (bFind)
					break;
			}
			if (bFind)
				break;
		}
		if (!bFind) {
			endPos.X = pRoleState->MoveAttrb.CurrentPos.X;
			endPos.Y = pRoleState->MoveAttrb.CurrentPos.Y;
		}
	}

	pRoleState->MoveAttrb.CurrentPos.X = endPos.X;
	pRoleState->MoveAttrb.CurrentPos.Y = endPos.Y;
	pRoleState->MoveAttrb.CurrentPosMini.X = 500;
	pRoleState->MoveAttrb.CurrentPosMini.Y = 500;

	Brodacast_Role_LocationOffset(pRoleState->RoleID, endPos.X, endPos.Y,
			SKILL_TYPE_ChongFeng);
	Buff Buff_tmp(BUFFER_TYPE_CASTSKILL, SKILL_TYPE_ChongFeng, level,
			current_time.fragment_now, 20, false, true);
	UpdataBuffer(*pRoleState, Buff_tmp, UPDATABUFFER_ADD);
	map<int, RoleState>::iterator targetRole = P_Roles.begin();
	for (; targetRole != P_Roles.end(); targetRole++) {

		if (targetRole->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (targetRole->second.Opposition == pRoleState->Opposition)
			continue;
		if (targetRole->second.PhysicalAttrib.ExemptCounter > 0)
			continue;

		double tmpDist = Pnt2SegmentDist(startPos, endPos,
				targetRole->second.MoveAttrb.CurrentPos);

		if (tmpDist <= 3) { //3表示这条线上的容许范围，不必要一定在一条直线上
			CalculateSkillHurt(targetRole->second, pRoleState->Level, hurt,
					SKILL_TYPE_ChongFeng);
		}
	}
}

//怒吼，增加护甲
void PK_PE::skill_process_221(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	short int param1 = 5;
	unsigned short int keepTime = 100;
	switch (level) {
	case 1:
		param1 = 5;
		break;
	case 2:
		param1 = 10;
		break;
	case 3:
		param1 = 15;
		break;
	case 4:
		param1 = 20;
		break;
	case 5:
		param1 = 25;
		break;
	case 6:
		param1 = 30;
		break;
	default:
		break;
	}

	map<int, RoleState>::iterator pos_target;
	for (pos_target = P_Roles.begin(); pos_target != P_Roles.end(); pos_target++) {
		if (pos_target->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (pos_target->second.Opposition != pRoleState->Opposition)
			continue;

		Buff tmpBuff(BUFFER_TYPE_ARMOR, SKILL_TYPE_NuHou, level,
				current_time.fragment_now, keepTime, true, true);
		tmpBuff.Effecttype = BUFFER_EFFECT_TYPE_RELATIVE;
		tmpBuff.Parametar1 = pos_target->second.PhysicalAttrib.DefenseArmor_Base * param1
				/ 100;
		UpdataBuffer(*pRoleState, tmpBuff, UPDATABUFFER_ADD);
	}
}

//战争践踏
void PK_PE::skill_process_220(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	short int hurt=50, lastTime=20, range=3;
	switch (level) {
	case 1:
		hurt = 50;
		lastTime = 20;
		range = 3;
		break;
	case 2:
		hurt = 50;
		lastTime = 20;
		range = 5;
		break;
	case 3:
		hurt = 50;
		lastTime = 30;
		range = 5;
		break;
	case 4:
		hurt = 50;
		lastTime = 30;
		range = 7;
		break;
	case 5:
		hurt = 50;
		lastTime = 40;
		range = 7;
		break;
	case 6:
		hurt = 500;
		lastTime = 40;
		range = 7;
		break;
	default:
		break;
	}

	map<int, RoleState>::iterator pos_target;
	short int nX = 0;
	short int nY = 0;
	for (pos_target = P_Roles.begin(); pos_target != P_Roles.end(); pos_target++) {
		if (pos_target->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (pos_target->second.Opposition == pRoleState->Opposition)
			continue;
		if (pos_target->second.MagicAttrb.ExemptAllCounter > 0)
			continue;
		if (pos_target->second.MagicAttrb.ExemptBadCounter > 0)
			continue;
		nX = pRoleState->MoveAttrb.CurrentPos.X
				- pos_target->second.MoveAttrb.CurrentPos.X;
		nY = pRoleState->MoveAttrb.CurrentPos.Y
				- pos_target->second.MoveAttrb.CurrentPos.Y;
		if ((nX * nX + nY * nY) <= range*range) {
			Buff tmpBuff(BUFFER_TYPE_XUANYUN, SKILL_TYPE_ZhanZhengJianTa,
					level, current_time.fragment_now, lastTime, false, true);
			UpdataBuffer(pos_target->second, tmpBuff, UPDATABUFFER_ADD);
			CalculateSkillHurt(pos_target->second, pRoleState->Level, hurt,
					SKILL_TYPE_ZhanZhengJianTa);
		}
	}
}

//流星火雨
void PK_PE::skill_process_219(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	short int targetPosX, targetPosY;
	if (role_order.TargetType == 1) {
		targetPosX = role_order.TargetPos.X;
		targetPosY = role_order.TargetPos.Y;
	} else if (role_order.TargetType == 2) {
		map<int, RoleState>::iterator pos_target;
		pos_target = P_Roles.find(role_order.TargetRoleID);
		if (pos_target == P_Roles.end()) {
			return;
		}
		targetPosX = pos_target->second.MoveAttrb.CurrentPos.X;
		targetPosY = pos_target->second.MoveAttrb.CurrentPos.Y;
	} else {
		return;
	}
	short int hurt = 150;
	short int lastTime = 50;
	switch (level) {
	case 1:
		hurt = 150;
		lastTime = 50;
		break;
	case 2:
		hurt = 200;
		lastTime = 50;
		break;
	case 3:
		hurt = 250;
		lastTime = 70;
		break;
	case 4:
		hurt = 300;
		lastTime = 70;
		break;
	case 5:
		hurt = 350;
		lastTime = 70;
		break;
	case 6:
		hurt = 500;
		lastTime = 70;
		break;
	default:
		break;

	}
	Buff tmpBuff(BUFFER_TYPE_CASTSKILL, SKILL_TYPE_LiuXingHuoYU, level,
			current_time.fragment_now, lastTime, true, true);
	tmpBuff.Parametar1 = hurt;
	tmpBuff.RunTimeLimit = 10;
	tmpBuff.Parametar2 = targetPosY * MAP_SIZE_X + targetPosX;
	UpdataBuffer(*pRoleState, tmpBuff, UPDATABUFFER_ADD);
	pRoleState->CurrentOrder.Type = ORDER_TYPE_CASTSKILL;
}

//治疗守卫
void PK_PE::skill_process_218(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	RoleState stRoleState_tmp;
	int loop_id = 0;
	for (loop_id = pRoleState->ControlID + 3000000; loop_id
			< (pRoleState->ControlID + 4000000); loop_id++) {
		map<int, RoleState>::iterator pos_loop_id;
		pos_loop_id = P_Roles.find(loop_id);
		if (pos_loop_id == P_Roles.end()) {

			break;
		}
	}

	unsigned short int keepTime = 50;
	int param1 = 1;
	int HP = 100;
	int AttackArea = 13;
	switch (level) {
	case 1:
		keepTime = 50;
		param1 = 1;
		HP = 100;
		break;
	case 2:
		keepTime = 50;
		param1 = 2;
		HP = 200;
		break;
	case 3:
		keepTime = 50;
		param1 = 3;
		HP = 300;
		break;
	case 4:
		keepTime = 70;
		param1 = 4;
		HP = 400;
		break;
	case 5:
		keepTime = 70;
		param1 = 5;
		HP = 500;
		break;
	case 6:
		keepTime = 70;
		param1 = 8;
		HP = 600;
		break;
	default:
		break;
	}

	stRoleState_tmp.ControlID = pRoleState->ControlID;
	stRoleState_tmp.RoleID = loop_id;
	stRoleState_tmp.CurrentOrder.Type = ORDER_TYPE_NO;

	stRoleState_tmp.HP_MAX = HP;
	stRoleState_tmp.HP = HP;
	stRoleState_tmp.Level = pRoleState->Level;
	stRoleState_tmp.Live = ROLE_STATUS_LIVE;

	if (pk_map_base[role_order.TargetPos.Y][role_order.TargetPos.X] == 0) {
		Pos newPos;
		GetEmptyPos(pRoleState->MoveAttrb.CurrentPos.X,
				pRoleState->MoveAttrb.CurrentPos.Y,
				pRoleState->MoveAttrb.Direct, &newPos);
		stRoleState_tmp.MoveAttrb.CurrentPos.X = newPos.X;
		stRoleState_tmp.MoveAttrb.CurrentPos.Y = newPos.Y;
	} else {
		stRoleState_tmp.MoveAttrb.CurrentPos.X = role_order.TargetPos.X;
		stRoleState_tmp.MoveAttrb.CurrentPos.Y = role_order.TargetPos.Y;
	}

	stRoleState_tmp.MoveAttrb.CurrentPosMini.X = 500;
	stRoleState_tmp.MoveAttrb.CurrentPosMini.Y = 500;
	stRoleState_tmp.MoveAttrb.Direct = pRoleState->MoveAttrb.Direct;

	stRoleState_tmp.MoveAttrb.SpeedMod = 0;
	stRoleState_tmp.MoveAttrb.SpeedMod_Base = 0;
	stRoleState_tmp.MoveAttrb.LimitCounter = 1;
	stRoleState_tmp.MagicAttrb.MagicArmor = 0;
	stRoleState_tmp.MagicAttrb.MagicArmor_Base = 0;
	stRoleState_tmp.MagicAttrb.ExemptAllCounter++;
	stRoleState_tmp.MagicAttrb.ExemptAllCounter = 1;
	stRoleState_tmp.MP = 0;
	stRoleState_tmp.MP_MAX = 0;
	stRoleState_tmp.Opposition = pRoleState->Opposition;
	//stRoleState_tmp.Origin = ORIGIN_CALL;
	stRoleState_tmp.Attack_Skill_WaitingTime = 0;
	stRoleState_tmp.PhysicalAttrib.AttackPowerHign_Base = 0;
	stRoleState_tmp.PhysicalAttrib.AttackPowerLow_Base = 0;
	stRoleState_tmp.PhysicalAttrib.AttackBulletMod = 0;
	stRoleState_tmp.PhysicalAttrib.LimitCounter = 1;
	stRoleState_tmp.PhysicalAttrib.AttackCDTime = 0;
	stRoleState_tmp.PhysicalAttrib.AttackCDTime_Base = 0;
	stRoleState_tmp.PhysicalAttrib.AttackHit = 0;
	stRoleState_tmp.PhysicalAttrib.AttackHit_Base = 0;
	stRoleState_tmp.PhysicalAttrib.AttackIngTime = 0;
	stRoleState_tmp.PhysicalAttrib.PreAttackTime = 0;
	stRoleState_tmp.PhysicalAttrib.PreTimeStatus = 0;
	stRoleState_tmp.PhysicalAttrib.AttackArea = AttackArea;
	stRoleState_tmp.PhysicalAttrib.DefenseArmor = 10;
	stRoleState_tmp.PhysicalAttrib.DefenseArmor_Base = 10;
	stRoleState_tmp.PK_State = PK_STATE_PKING;
	stRoleState_tmp.RoleFlag = RoleType_Call;
	stRoleState_tmp.RoleType = 10002; //蛇杖

	Buff Buff_tmp(BUFFER_TYPE_CALL, SKILL_TYPE_ZhiLiaoShouWei, level,
			current_time.fragment_now, keepTime, false, false);
	Buff Buff_tmp1(BUFFER_TYPE_CASTSKILL, SKILL_TYPE_ZhiLiaoShouWei, level,
			current_time.fragment_now, keepTime, false, false);
	Buff_tmp1.Parametar1 = param1;
	Buff_tmp1.RunTimeLimit = 10;
	Buff_tmp1.Effecttype = BUFFER_EFFECT_TYPE_RELATIVE;
	UpdataBuffer(stRoleState_tmp, Buff_tmp, UPDATABUFFER_ADD);
	UpdataBuffer(stRoleState_tmp, Buff_tmp1, UPDATABUFFER_ADD);
	char new_role_msg[39 + 1] = { 0 };
	int pos_len = 0;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleID, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &current_time.fragment_now, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &keepTime, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.ControlID, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleFlag, 1);
	pos_len += 1;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleType, 4);
	pos_len += 4;
	int copd_id = 0;
	memcpy(new_role_msg + pos_len, &copd_id, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len,
			&stRoleState_tmp.PhysicalAttrib.AttackBulletMod, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.HP_MAX, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MP_MAX, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.HP, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MP, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.SpeedMod_Base, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.CurrentPos.X, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.CurrentPos.Y, 2);
	pos_len += 2;
	short int effect = SKILL_TYPE_ZhiLiaoShouWei;
	memcpy(new_role_msg + pos_len, &effect, 2);
	pos_len += 2;
	Brodacast_Skill_Newrole(new_role_msg);
	P_Roles.insert(make_pair(stRoleState_tmp.RoleID, stRoleState_tmp));

}
void PK_PE::skill_process_217_Effect(Bullet & bullet) {

	if (bullet.Role_Attack->Opposition != bullet.Role_Passive->Opposition) {
		short int sheep_time = 20;
		switch (bullet.Level) {
		case 1:
			sheep_time = 20;
			break;
		case 2:
			sheep_time = 25;
			break;
		case 3:
			sheep_time = 30;
			break;
		case 4:
			sheep_time = 35;
			break;
		case 5:
			sheep_time = 40;
			break;
		case 6:
			sheep_time = 60;
			break;
		default:
			break;
		}
		Buff Buff_tmp(BUFFER_TYPE_NOMOVE, SKILL_TYPE_YouBu, bullet.Level,
				current_time.fragment_now, sheep_time, false, true);
		UpdataBuffer(*bullet.Role_Passive, Buff_tmp, UPDATABUFFER_ADD);
	}
}

//诱捕
void PK_PE::skill_process_217(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}
	Bullet BulletsTmp;
	BulletsTmp.Start_Time = 3;
	BulletsTmp.Role_Attack = pRoleState;
	BulletsTmp.Role_Passive = &pos_target->second;
	BulletsTmp.CurrentPos.X = pRoleState->MoveAttrb.CurrentPos.X;
	BulletsTmp.CurrentPos.Y = pRoleState->MoveAttrb.CurrentPos.Y;
	BulletsTmp.BulletType = SKILL_TYPE_YouBu;
	BulletsTmp.Speed = 3;
	BulletsTmp.Level = level;
	Bullets.push_back(BulletsTmp);
}

//连锁闪电
void PK_PE::skill_process_216(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}
	char roleNum = 3;
	short int hurtValue = 100;
	switch (level) {
	case 1:
		hurtValue = 100;
		roleNum = 3;
		break;
	case 2:
		hurtValue = 100;
		roleNum = 5;
		break;
	case 3:
		hurtValue = 160;
		roleNum = 5;
		break;
	case 4:
		hurtValue = 160;
		roleNum = 7;
		break;
	case 5:
		hurtValue = 220;
		roleNum = 7;
		break;
	case 6:
		hurtValue = 300;
		roleNum = 7;
		break;
	default:
		break;
	}

	map<int, short int> targetRole;
	targetRole.insert(make_pair(pRoleState->RoleID, 0));
	targetRole.insert(make_pair(pos_target->second.RoleID, hurtValue));
	hurtValue = (short int) (hurtValue * 0.8);
	//roleNum -= 1;
	RoleState * tmpMaxRole = NULL;
	for (short int i = 0; i < roleNum; i++) {
		map<int, RoleState>::iterator role_target;
		tmpMaxRole = NULL;
		for (role_target = P_Roles.begin(); role_target != P_Roles.end(); role_target++) {
			if (role_target->second.Live == ROLE_STATUS_DEAD)
				continue;
			if (role_target->second.Opposition == pRoleState->Opposition)
				continue;
			if (role_target->second.MagicAttrb.ExemptBadCounter > 0)
				continue;
			if (role_target->second.MagicAttrb.ExemptAllCounter > 0)
				continue;
			if (targetRole.find(role_target->second.RoleID) != targetRole.end())
				continue;

			tmpMaxRole = &role_target->second;
		}
		if (tmpMaxRole != NULL) {
			targetRole.insert(make_pair(tmpMaxRole->RoleID, hurtValue));
			hurtValue = (short int) (hurtValue * 0.85);//每次跳跃损失15%伤害
		} else {
			break;
		}
	}

//	map<int, short int>::iterator iter;
//	int i=0;
//	for (iter = targetRole.begin(); iter != targetRole.end(); iter++, i++) {
//		LOG (LOG_DEBUG, __FILE__, __LINE__, "888888888 [%d]: roleid[%d] addValue[%d]!", i, iter->first, iter->second);
//	}


	CalculateMultiSkillHurt(targetRole, pRoleState->Level,
			SKILL_TYPE_LianSuoShanDian);
}

//驱散
void PK_PE::skill_process_215(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}
	short int hurtValue = 0;
	short int keepTime = 20;

	short int decreaseSpeed = 50;

	if (pos_target->second.RoleFlag == RoleType_Call) {
		if (pos_target->second.HP <= hurtValue) {
			CalculateSkillHurt(pos_target->second, pRoleState->Level,
					hurtValue, SKILL_TYPE_QuSan);
			return;
		}
		CalculateSkillHurt(pos_target->second, pRoleState->Level, hurtValue,
				SKILL_TYPE_QuSan);
	}

	multimap<short int, Buff>::iterator tmpBuff;
	for (tmpBuff = pos_target->second.multimap_buff.begin(); tmpBuff
			!= pos_target->second.multimap_buff.end();) {
		if (tmpBuff->second.bCanBreak && tmpBuff->second.Type
				!= BUFFER_TYPE_CASTSKILL) {
			UpdataBuffer(pos_target->second, tmpBuff->second, UPDATABUFFER_DEL);
			pos_target->second.multimap_buff.erase(tmpBuff++);
		} else {
			++tmpBuff;
		}
	}

	if (pRoleState->Opposition != pos_target->second.Opposition) {
		Buff tempBuff(BUFFER_TYPE_MOVESPEED, SKILL_TYPE_QuSan, level,
				current_time.fragment_now, keepTime, false, true);
		tempBuff.Effecttype = BUFFER_EFFECT_TYPE_RELATIVE;
		tempBuff.Parametar1 = -(decreaseSpeed * pos_target->second.MoveAttrb.SpeedMod_Base
				/ 100);

		UpdataBuffer(pos_target->second, tempBuff, UPDATABUFFER_ADD);
	}
}

//血性
void PK_PE::skill_process_214( RoleState * pRoleState,RoleOrder role_order,char level )
{
	short int addHp = 5,nTime = 100;
	switch( level )
	{
		case 1:
			addHp = 5;
			break;
		case 2:
			addHp = 7;
			break;
		case 3:
			addHp = 10;
			break;
		case 4:
			addHp = 12;
			break;
		case 5:
			addHp = 15;
			break;
		case 6:
			addHp = 20;
			break;
		default:
			break;
	}

	map<int, RoleState>::iterator pos_target;
	for (pos_target = P_Roles.begin(); pos_target != P_Roles.end(); pos_target++) {
		if (pos_target->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (pos_target->second.Opposition != pRoleState->Opposition)
			continue;

		Buff currBuff(BUFFER_TYPE_MAXHP, SKILL_TYPE_XueXing, level,
				current_time.fragment_now, nTime, true, true);

		currBuff.Effecttype = BUFFER_EFFECT_TYPE_RELATIVE;
		currBuff.Parametar1 = pos_target->second.HP_MAX * addHp / 100;

		UpdataBuffer(pos_target->second, currBuff, UPDATABUFFER_ADD);
	}
}

//咆哮
void PK_PE::skill_process_213(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	short int addValue=5, lastTime=100;
	switch (level) {
	case 1:
		addValue = 5;
		break;
	case 2:
		addValue = 10;
		break;
	case 3:
		addValue = 15;
		break;
	case 4:
		addValue = 20;
		break;
	case 5:
		addValue = 25;
		break;
	case 6:
		addValue = 30;
		break;
	default:
		break;

	}

	map<int, RoleState>::iterator pos_target;
	for (pos_target = P_Roles.begin(); pos_target != P_Roles.end(); pos_target++) {
		if (pos_target->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (pos_target->second.Opposition != pRoleState->Opposition)
			continue;

		Buff tempBuff(BUFFER_TYPE_ATTACKPOWER, SKILL_TYPE_PaoXiao, level,
				current_time.fragment_now, lastTime, true, true);
		tempBuff.Effecttype = BUFFER_EFFECT_TYPE_RELATIVE;
		tempBuff.Parametar1 = pos_target->second.PhysicalAttrib.AttackPowerHign_Base * addValue / 100;;

//		LOG (LOG_DEBUG, __FILE__, __LINE__, "111111111111: roleid[%d] AttackPowerHign_Base[%d] AttackPowerLow_Base[%d] addValue[%d]!", pos_target->second.RoleID, pos_target->second.PhysicalAttrib.AttackPowerHign_Base, pos_target->second.PhysicalAttrib.AttackPowerLow_Base, addValue);
//		LOG (LOG_DEBUG, __FILE__, __LINE__, "222222222222: roleid[%d] addPoint[%d]!", pos_target->second.RoleID, tempBuff.Parametar1);
		UpdataBuffer(pos_target->second, tempBuff, UPDATABUFFER_ADD);
	}
}

//毒蛇守卫
void PK_PE::skill_process_212(RoleState * pRoleState, RoleOrder role_order,
		char level) {
	RoleState stRoleState_tmp;
	int loop_id = 0;
	for (loop_id = pRoleState->ControlID + 3000000; loop_id
			< (pRoleState->ControlID + 4000000); loop_id++) {
		map<int, RoleState>::iterator pos_loop_id;
		pos_loop_id = P_Roles.find(loop_id);
		if (pos_loop_id == P_Roles.end()) {

			break;
		}
	}

	unsigned short int keepTime = 50;
	unsigned short int HP = 100;
	unsigned short int AttackPower = 50;
	switch (level) {
	case 1:
		keepTime = 50;
		HP = 100;
		AttackPower = 50;
		break;
	case 2:
		keepTime = 50;
		HP = 200;
		AttackPower = 100;
		break;
	case 3:
		keepTime = 70;
		HP = 300;
		AttackPower = 150;
		break;
	case 4:
		keepTime = 70;
		HP = 400;
		AttackPower = 200;
		break;
	case 5:
		keepTime = 90;
		HP = 500;
		AttackPower = 250;
		break;
	case 6:
		keepTime = 120;
		HP = 600;
		AttackPower = 300;
		break;
	default:
		break;
	}

	stRoleState_tmp.ControlID = pRoleState->ControlID;
	stRoleState_tmp.RoleID = loop_id;
	stRoleState_tmp.CurrentOrder.Type = ORDER_TYPE_NO;

	stRoleState_tmp.HP_MAX = HP;
	stRoleState_tmp.HP = HP;
	stRoleState_tmp.Level = pRoleState->Level;
	stRoleState_tmp.Live = ROLE_STATUS_LIVE;

	if (pk_map_base[role_order.TargetPos.Y][role_order.TargetPos.X] == 0) {
		Pos newPos;
		GetEmptyPos(pRoleState->MoveAttrb.CurrentPos.X,
				pRoleState->MoveAttrb.CurrentPos.Y,
				pRoleState->MoveAttrb.Direct, &newPos);
		stRoleState_tmp.MoveAttrb.CurrentPos.X = newPos.X;
		stRoleState_tmp.MoveAttrb.CurrentPos.Y = newPos.Y;
	} else {
		stRoleState_tmp.MoveAttrb.CurrentPos.X = role_order.TargetPos.X;
		stRoleState_tmp.MoveAttrb.CurrentPos.Y = role_order.TargetPos.Y;
	}

	stRoleState_tmp.MoveAttrb.CurrentPosMini.X = 500;
	stRoleState_tmp.MoveAttrb.CurrentPosMini.Y = 500;
	stRoleState_tmp.MoveAttrb.Direct = pRoleState->MoveAttrb.Direct;
	stRoleState_tmp.MoveAttrb.SpeedMod_Base = 0;
	stRoleState_tmp.MoveAttrb.LimitCounter = 1;
	stRoleState_tmp.MagicAttrb.MagicArmor = 0;
	stRoleState_tmp.MagicAttrb.MagicArmor_Base = 0;
	stRoleState_tmp.MagicAttrb.ExemptAllCounter = 1;

	stRoleState_tmp.MP = 0;
	stRoleState_tmp.MP_MAX = 0;
	stRoleState_tmp.Opposition = pRoleState->Opposition;
	//stRoleState_tmp.Origin = ORIGIN_CALL;
	stRoleState_tmp.PhysicalAttrib.AttackPowerHign_Base = AttackPower;
	stRoleState_tmp.PhysicalAttrib.AttackPowerLow_Base = AttackPower;
	stRoleState_tmp.PhysicalAttrib.AttackBulletMod = 2;
	stRoleState_tmp.Attack_Skill_WaitingTime = 0;

	stRoleState_tmp.PhysicalAttrib.AttackCDTime_Base
			= pRoleState->PhysicalAttrib.AttackCDTime_Base-10; //攻击速度加快1秒

	if(stRoleState_tmp.PhysicalAttrib.AttackCDTime_Base <=0)
		stRoleState_tmp.PhysicalAttrib.AttackCDTime_Base
				= pRoleState->PhysicalAttrib.AttackCDTime_Base;

	stRoleState_tmp.PhysicalAttrib.AttackHit_Base
			= pRoleState->PhysicalAttrib.AttackHit_Base;
	stRoleState_tmp.PhysicalAttrib.AttackIngTime
			= pRoleState->PhysicalAttrib.AttackIngTime;

	stRoleState_tmp.PhysicalAttrib.PreAttackTime = 0;
	stRoleState_tmp.PhysicalAttrib.PreTimeStatus = 0;
	stRoleState_tmp.PhysicalAttrib.AttackArea = 10;
	stRoleState_tmp.PhysicalAttrib.DefenseArmor_Base = 10;
	stRoleState_tmp.Init();
	stRoleState_tmp.PK_State = PK_STATE_PKING;
	stRoleState_tmp.RoleFlag = RoleType_Call;
	stRoleState_tmp.RoleType = 10001; //蛇杖


	Buff Buff_tmp(BUFFER_TYPE_CALL, SKILL_TYPE_DuSheShouWei, level,
			current_time.fragment_now, keepTime, false, false);

	UpdataBuffer(stRoleState_tmp, Buff_tmp, UPDATABUFFER_ADD);

	P_Roles.insert(make_pair(stRoleState_tmp.RoleID, stRoleState_tmp));

	char new_role_msg[39 + 1] = { 0 };
	int pos_len = 0;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleID, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &current_time.fragment_now, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &keepTime, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.ControlID, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleFlag, 1);
	pos_len += 1;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleType, 4);
	pos_len += 4;
	int copd_id = 0;
	memcpy(new_role_msg + pos_len, &copd_id, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len,
			&stRoleState_tmp.PhysicalAttrib.AttackBulletMod, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.HP_MAX, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MP_MAX, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.HP, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MP, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.SpeedMod_Base, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.CurrentPos.X, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.CurrentPos.Y, 2);
	pos_len += 2;
	short int effect = SKILL_TYPE_DuSheShouWei;
	memcpy(new_role_msg + pos_len, &effect, 2);
	pos_len += 2;
	Brodacast_Skill_Newrole(new_role_msg);
}

//地刺
void PK_PE::skill_process_211(RoleState *pRoleState, RoleOrder role_order,
		char level) {
	short int hurt, noallTime, len;
	hurt = 100;
	noallTime = 10;
	len = 3;

	switch (level) {
	case 1:
		hurt = 100;
		noallTime = 10;
		len = 3;
		break;
	case 2:
		hurt = 150;
		noallTime = 15;
		len = 3;
		break;
	case 3:
		hurt = 200;
		noallTime = 20;
		len = 5;
		break;
	case 4:
		hurt = 250;
		noallTime = 25;
		len = 5;
		break;
	case 5:
		hurt = 300;
		noallTime = 30;
		len = 7;
		break;
	case 6:
		hurt = 500;
		noallTime = 40;
		len = 7;
		break;
	default:
		break;
	}
	Pos startPos = pRoleState->MoveAttrb.CurrentPos;
	Pos tempPos, endPos;
	if (role_order.TargetType == 1) {
		tempPos = role_order.TargetPos;
	} else {
		map<int, RoleState>::iterator pos_target;
		pos_target = P_Roles.find(role_order.TargetRoleID);
		if (pos_target == P_Roles.end()) {
			return;
		}
		tempPos = pos_target->second.MoveAttrb.CurrentPos;
	}
	short int dist = (short int) Dist(tempPos, startPos);
	short int tempDist = tempPos.X - startPos.X;
	if (tempDist == 0)
		endPos.X = startPos.X;
	else
		endPos.X = startPos.X + (len * tempDist) / dist;
	tempDist = tempPos.Y - startPos.Y;
	if (tempDist == 0)
		endPos.Y = startPos.Y;
	else
		endPos.Y = startPos.Y + (len * tempDist) / dist;

	map<int, RoleState>::iterator targetRole = P_Roles.begin();
	while (targetRole != P_Roles.end()) {
		if (targetRole->second.Opposition != pRoleState->Opposition
				&& targetRole->second.MagicAttrb.ExemptBadCounter <= 0
				&& targetRole->second.MagicAttrb.ExemptAllCounter <= 0
				&& Pnt2SegmentDist(startPos, endPos,
						targetRole->second.MoveAttrb.CurrentPos) < 2) {
			Buff Buff_tmp(BUFFER_TYPE_XUANYUN, SKILL_TYPE_DiCi, level,
					current_time.fragment_now, noallTime, false, true);

			UpdataBuffer(targetRole->second, Buff_tmp, UPDATABUFFER_ADD);
			CalculateSkillHurt(targetRole->second, pRoleState->Level, hurt,
					SKILL_TYPE_DiCi);
		}
		targetRole++;
	}
}

//反击螺旋
void PK_PE::skill_process_210(RoleState *pRoleState, RoleOrder role_order,
		char level) {
	unsigned short int keepTime = 100;
	int hurt = 200;
	int percent = 15;
	switch (level) {
	case 1:
		hurt = 200;
		percent = 15;
		break;
	case 2:
		hurt = 300;
		percent = 16;
		break;
	case 3:
		hurt = 400;
		percent = 17;
		break;
	case 4:
		hurt = 500;
		percent = 18;
		break;
	case 5:
		hurt = 600;
		percent = 19;
		break;
	case 6:
		hurt = 700;
		percent = 20;
		break;
	default:
		break;
	}
	Buff currBuff(BUFFER_TYPE_BYSKILL, SKILL_TYPE_FanJiLuoXuan, level,
			current_time.fragment_now, keepTime, true, true);
	currBuff.RunTimeLimit = 1;
	currBuff.Parametar1 = hurt;
	currBuff.Parametar2 = percent;
	UpdataBuffer(*pRoleState, currBuff, UPDATABUFFER_ADD);
}

//风暴之锤
void PK_PE::skill_process_207_effect(Pos target_pos, RoleState *pRoleState,
		char Level) {
	unsigned short int hurt_value = 100;
	unsigned short int keep_time = 20;
	switch (Level) {
	case 1:
		hurt_value = 100;
		keep_time = 20;
		break;
	case 2:
		hurt_value = 200;
		keep_time = 25;
		break;
	case 3:
		hurt_value = 300;
		keep_time = 30;
		break;
	case 4:
		hurt_value = 400;
		keep_time = 35;
		break;
	case 5:
		hurt_value = 500;
		keep_time = 40;
		break;
	case 6:
		hurt_value = 600;
		keep_time = 50;
		break;
	}
	map<int, RoleState>::iterator role_target;
	short int nX = 0;
	short int nY = 0;
	for (role_target = P_Roles.begin(); role_target != P_Roles.end(); role_target++) {
		if (role_target->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (role_target->second.Opposition == pRoleState->Opposition)
			continue;
		if (role_target->second.MagicAttrb.ExemptBadCounter > 0)
			continue;
		if (role_target->second.MagicAttrb.ExemptAllCounter > 0)
			continue;
		nX = target_pos.X - role_target->second.MoveAttrb.CurrentPos.X;
		nY = target_pos.Y - role_target->second.MoveAttrb.CurrentPos.Y;
		if ((nX * nX + nY * nY) <= 9) {
			Buff Buff_tmp(BUFFER_TYPE_XUANYUN, SKILL_TYPE_FengBaoZhiChui,
					Level, current_time.fragment_now, keep_time, false, true);

			UpdataBuffer(role_target->second, Buff_tmp, UPDATABUFFER_ADD);
			CalculateSkillHurt(role_target->second, pRoleState->Level,
					hurt_value, SKILL_TYPE_FengBaoZhiChui);
		}
	}
}

//无光之盾
void PK_PE::skill_process_209(RoleState *pRoleState, RoleOrder role_order,
		char level) {
	RoleState * targetRole = pRoleState;
	if (role_order.TargetRoleID != pRoleState->RoleID) {
		map<int, RoleState>::iterator pos_target;
		pos_target = P_Roles.find(role_order.TargetRoleID);
		if (pos_target == P_Roles.end()) {
			return;
		}
		targetRole = &pos_target->second;
	}
	unsigned short int keepTime = 50;
	int hurt = 250;
	switch (level) {
	case 1:
		hurt = 250;
		break;
	case 2:
		hurt = 350;
		break;
	case 3:
		hurt = 450;
		break;
	case 4:
		hurt = 550;
		break;
	case 5:
		hurt = 650;
		break;
	case 6:
		hurt = 800;
		break;
	default:
		break;
	}
	Buff currBuff(BUFFER_TYPE_BYSKILL, SKILL_TYPE_WuGuangZhiDun, level,
			current_time.fragment_now, keepTime, true, true);
	currBuff.Parametar1 = hurt;
	UpdataBuffer(*targetRole, currBuff, UPDATABUFFER_ADD);
}
void PK_PE::skill_process_209_buffer(RoleState *pRoleState, Buff* p209Buffer) {
	unsigned short int hurt_value = p209Buffer->Parametar2;
	map<int, RoleState>::iterator pos_target;
	short int nX = 0;
	short int nY = 0;

	int radius = 3;
	switch (p209Buffer->Level) {
	case 1:
		radius = 3;
		break;
	case 2:
		radius = 3;
		break;
	case 3:
		radius = 5;
		break;
	case 4:
		radius = 5;
		break;
	case 5:
		radius = 7;
		break;
	case 6:
		radius = 7;
		break;
	default:
		break;
	}

	for (pos_target = P_Roles.begin(); pos_target != P_Roles.end(); pos_target++) {
		if (pos_target->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (pos_target->second.Opposition == pRoleState->Opposition)
			continue;
		nX = pRoleState->MoveAttrb.CurrentPos.X
				- pos_target->second.MoveAttrb.CurrentPos.X;
		nY = pRoleState->MoveAttrb.CurrentPos.Y
				- pos_target->second.MoveAttrb.CurrentPos.Y;
		if ((nX * nX + nY * nY) <= radius) {
			CalculateSkillHurt(pos_target->second, pRoleState->Level,
					hurt_value, SKILL_TYPE_WuGuangZhiDun);
		}
	}
}

//淘汰之刃
void PK_PE::skill_process_208(RoleState *pRoleState, RoleOrder role_order,
		char level) {

	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}
	short int hurtHp = 100;
	short int killHp = 6;
	switch (level) {
	case 1:
		hurtHp = 100;
		killHp = 6;
		break;
	case 2:
		hurtHp = 150;
		killHp = 8;
		break;
	case 3:
		hurtHp = 200;
		killHp = 10;
		break;
	case 4:
		hurtHp = 250;
		killHp = 12;
		break;
	case 5:
		hurtHp = 300;
		killHp = 14;
		break;
	case 6:
		hurtHp = 350;
		killHp = 16;
		break;
	default:
		break;
	}
	if (pos_target->second.HP < pos_target->second.Curr_HP_MAX * killHp / 100) {
		hurtHp = pos_target->second.HP;
	}
	CalculateSkillHurt(pos_target->second, pRoleState->Level, hurtHp,
			SKILL_TYPE_TaoTaiZhiRen);

}

//风暴之锤
void PK_PE::skill_process_207(RoleState *pRoleState, RoleOrder role_order,
		char level) {
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}

	Bullet BulletsTmp;
	BulletsTmp.Start_Time = 3;
	BulletsTmp.Role_Attack = pRoleState;
	BulletsTmp.Role_Passive = &pos_target->second;
	BulletsTmp.CurrentPos.X = pRoleState->MoveAttrb.CurrentPos.X;
	BulletsTmp.CurrentPos.Y = pRoleState->MoveAttrb.CurrentPos.Y;
	BulletsTmp.BulletType = SKILL_TYPE_FengBaoZhiChui;
	BulletsTmp.Speed = 3;
	BulletsTmp.Level = level;
	Bullets.push_back(BulletsTmp);
}

//镜像
void PK_PE::skill_process_206(RoleState *pRoleState, RoleOrder role_order,
		char level) {
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}

	int loop_id;
	for (loop_id = pRoleState->ControlID + 3000000; loop_id
			< (pRoleState->ControlID + 4000000); loop_id++) {
		map<int, RoleState>::iterator pos_loop_id;
		pos_loop_id = P_Roles.find(loop_id);
		if (pos_loop_id == P_Roles.end()) {
			break;
		}
	}

	unsigned short int keepTime = 80;
	short int AttackPowerAdd = -60;
	unsigned short int HurtAdd = 300;

	switch (level) {
	case 1:
		AttackPowerAdd = -60;
		HurtAdd = 300;
		break;
	case 2:
		AttackPowerAdd = -50;
		HurtAdd = 250;
		break;
	case 3:
		AttackPowerAdd = -40;
		HurtAdd = 200;
		break;
	case 4:
		AttackPowerAdd = -30;
		HurtAdd = 150;
		break;
	case 5:
		AttackPowerAdd = -20;
		HurtAdd = 100;
		break;
	case 6:
		AttackPowerAdd = 0;
		HurtAdd = 50;
		break;
	default:
		break;
	}

	Pos newPos;
	GetEmptyPos(pos_target->second.MoveAttrb.CurrentPos.X,
			pos_target->second.MoveAttrb.CurrentPos.Y,
			pRoleState->MoveAttrb.Direct, &newPos);

	RoleState stRoleState_tmp;
	stRoleState_tmp.ControlID = pRoleState->ControlID;
	stRoleState_tmp.RoleID = loop_id;
	stRoleState_tmp.CurrentOrder.Type = ORDER_TYPE_NO;
	stRoleState_tmp.HP_MAX = pos_target->second.HP_MAX;
	stRoleState_tmp.HP = pos_target->second.HP;
	stRoleState_tmp.Level = pos_target->second.Level;
	stRoleState_tmp.Live = ROLE_STATUS_LIVE;
	stRoleState_tmp.MagicAttrb.MagicArmor = 0;
	stRoleState_tmp.MagicAttrb.MagicArmor_Base = 0;

	stRoleState_tmp.MoveAttrb.CurrentPosMini.X = 500;
	stRoleState_tmp.MoveAttrb.CurrentPosMini.Y = 500;
	stRoleState_tmp.MoveAttrb.Direct = pRoleState->MoveAttrb.Direct;

	stRoleState_tmp.MoveAttrb.CurrentPos.X = newPos.X;
	stRoleState_tmp.MoveAttrb.CurrentPos.Y = newPos.Y;
	stRoleState_tmp.MoveAttrb.SpeedMod
			= pos_target->second.MoveAttrb.SpeedMod_Base;
	stRoleState_tmp.MoveAttrb.SpeedMod_Base
			= pos_target->second.MoveAttrb.SpeedMod_Base;
	stRoleState_tmp.MagicAttrb.MagicArmor = 0;
	stRoleState_tmp.MagicAttrb.MagicArmor_Base = 0;

	stRoleState_tmp.MP = 0;
	stRoleState_tmp.MP_MAX = 0;
	stRoleState_tmp.Opposition = pRoleState->Opposition;
	//stRoleState_tmp.Origin = ORIGIN_CALL;

	stRoleState_tmp.PhysicalAttrib = pos_target->second.PhysicalAttrib;
	stRoleState_tmp.PhysicalAttrib.PreAttackTime = 0;
	stRoleState_tmp.PhysicalAttrib.PreTimeStatus = 0;
	stRoleState_tmp.PhysicalAttrib.AttackPowerAdd = AttackPowerAdd;
	stRoleState_tmp.PhysicalAttrib.HurtAdd = HurtAdd;
	stRoleState_tmp.PK_State = PK_STATE_PKING;
	stRoleState_tmp.RoleFlag = RoleType_Call;
	stRoleState_tmp.RoleType = pos_target->second.RoleType;
	stRoleState_tmp.Attack_Skill_WaitingTime = 0;

	Buff Buff_tmp(BUFFER_TYPE_CALL, SKILL_TYPE_JingXiang, level,
			current_time.fragment_now, keepTime, false, false);
	UpdataBuffer(stRoleState_tmp, Buff_tmp, UPDATABUFFER_ADD);
	P_Roles.insert(make_pair(stRoleState_tmp.RoleID, stRoleState_tmp));

	char new_role_msg[39 + 1] = { 0 };
	int pos_len = 0;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleID, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &current_time.fragment_now, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &keepTime, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.ControlID, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleFlag, 1);
	pos_len += 1;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleType, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &pos_target->second.RoleID, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len,
			&stRoleState_tmp.PhysicalAttrib.AttackBulletMod, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.HP_MAX, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MP_MAX, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.HP, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MP, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.SpeedMod_Base, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.CurrentPos.X, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.CurrentPos.Y, 2);
	pos_len += 2;
	short int effect = SKILL_TYPE_JingXiang;
	memcpy(new_role_msg + pos_len, &effect, 2);
	pos_len += 2;
	Brodacast_Skill_Newrole(new_role_msg);
}

//幽灵狼
void PK_PE::skill_process_205(RoleState *pRoleState, RoleOrder role_order,
		char level) {

	int loop_id;
	for (loop_id = pRoleState->ControlID + 3000000; loop_id
			< (pRoleState->ControlID + 4000000); loop_id++) {
		map<int, RoleState>::iterator pos_loop_id;
		pos_loop_id = P_Roles.find(loop_id);
		if (pos_loop_id == P_Roles.end()) {
			break;
		}
	}

	unsigned short int keepTime = 200;
	int HP = 200;
	int AttckPower = 80;
	switch (level) {
	case 1:
		HP = 200;
		AttckPower = 80;
		break;
	case 2:
		HP = 300;
		AttckPower = 160;
		break;
	case 3:
		HP = 400;
		AttckPower = 240;
		break;
	case 4:
		HP = 500;
		AttckPower = 320;
		break;
	case 5:
		HP = 600;
		AttckPower = 400;
		break;
	case 6:
		HP = 800;
		AttckPower = 480;
		break;
	default:
		break;

	}

	Pos newPos;
	GetEmptyPos(pRoleState->MoveAttrb.CurrentPos.X,
			pRoleState->MoveAttrb.CurrentPos.Y, pRoleState->MoveAttrb.Direct,
			&newPos);

	RoleState stRoleState_tmp;
	stRoleState_tmp.ControlID = pRoleState->ControlID;

	stRoleState_tmp.CurrentOrder.Type = ORDER_TYPE_NO;
	stRoleState_tmp.RoleID = loop_id;
	stRoleState_tmp.HP = HP;
	stRoleState_tmp.HP_MAX = HP;
	stRoleState_tmp.Level = 1;
	stRoleState_tmp.Live = ROLE_STATUS_LIVE;
	stRoleState_tmp.MagicAttrb.MagicArmor = 0;
	stRoleState_tmp.MagicAttrb.MagicArmor_Base = 0;

	stRoleState_tmp.MoveAttrb.CurrentPosMini.X = 500;
	stRoleState_tmp.MoveAttrb.CurrentPosMini.Y = 500;
	stRoleState_tmp.MoveAttrb.Direct = 1;

	stRoleState_tmp.MoveAttrb.SpeedMod = 300;
	stRoleState_tmp.MoveAttrb.SpeedMod_Base = 300;
	stRoleState_tmp.MoveAttrb.CurrentPos.X = newPos.X;
	stRoleState_tmp.MoveAttrb.CurrentPos.Y = newPos.Y;
	stRoleState_tmp.MagicAttrb.MagicArmor = 0;
	stRoleState_tmp.MagicAttrb.MagicArmor_Base = 0;

	stRoleState_tmp.MP = 0;
	stRoleState_tmp.MP_MAX = 0;
	stRoleState_tmp.Opposition = pRoleState->Opposition;
	//stRoleState_tmp.Origin = ORIGIN_CALL;

	stRoleState_tmp.PhysicalAttrib.AttackArea = 2;
	stRoleState_tmp.PhysicalAttrib.AttackBulletMod
			= pRoleState->PhysicalAttrib.AttackBulletMod;
	stRoleState_tmp.PhysicalAttrib.AttackCDTime = 15;
	stRoleState_tmp.PhysicalAttrib.AttackCDTime_Base = 15;
	stRoleState_tmp.PhysicalAttrib.AttackIngTime = 3;
	stRoleState_tmp.PhysicalAttrib.AttackHit = 100;
	stRoleState_tmp.PhysicalAttrib.AttackHit_Base = 100;

	stRoleState_tmp.PhysicalAttrib.AttackPowerHign_Base = AttckPower;
	stRoleState_tmp.PhysicalAttrib.AttackPowerLow_Base = AttckPower;
	stRoleState_tmp.PhysicalAttrib.DefenseArmor = 10;
	stRoleState_tmp.PhysicalAttrib.DefenseArmor_Base = 10;
	stRoleState_tmp.PhysicalAttrib.DefenseMiss = 0;
	stRoleState_tmp.PhysicalAttrib.DefenseMiss_Base = 0;
	stRoleState_tmp.PhysicalAttrib.PreAttackTime = 0;
	stRoleState_tmp.PhysicalAttrib.PreTimeStatus = 0;
	stRoleState_tmp.Attack_Skill_WaitingTime = 0;
	stRoleState_tmp.PK_State = PK_STATE_PKING;
	stRoleState_tmp.RoleFlag = RoleType_Call;
	stRoleState_tmp.RoleType = 125;

	Buff Buff_tmp(BUFFER_TYPE_CALL, SKILL_TYPE_YouLingLang, level,
			current_time.fragment_now, keepTime, false, false);

	UpdataBuffer(stRoleState_tmp, Buff_tmp, UPDATABUFFER_ADD);

	P_Roles.insert(make_pair(stRoleState_tmp.RoleID, stRoleState_tmp));

	char new_role_msg[40] = { 0 };
	int pos_len = 0;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleID, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &current_time.fragment_now, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &keepTime, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.ControlID, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleFlag, 1);
	pos_len += 1;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.RoleType, 4);
	pos_len += 4;
	int copd_id = 0;
	memcpy(new_role_msg + pos_len, &copd_id, 4);
	pos_len += 4;
	memcpy(new_role_msg + pos_len,
			&stRoleState_tmp.PhysicalAttrib.AttackBulletMod, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.HP_MAX, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MP_MAX, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.HP, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MP, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.SpeedMod_Base, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.CurrentPos.X, 2);
	pos_len += 2;
	memcpy(new_role_msg + pos_len, &stRoleState_tmp.MoveAttrb.CurrentPos.Y, 2);
	pos_len += 2;
	short int effect = SKILL_TYPE_YouLingLang;
	memcpy(new_role_msg + pos_len, &effect, 2);
	pos_len += 2;
	Brodacast_Skill_Newrole(new_role_msg);
}

void PK_PE::skill_process_204_buffer(RoleState *pRoleState,
		short int hurt_value) {
	map<int, RoleState>::iterator pos_target;
	short int nX = 0;
	short int nY = 0;
	for (pos_target = P_Roles.begin(); pos_target != P_Roles.end(); pos_target++) {
		if (pos_target->second.Live == ROLE_STATUS_DEAD)
			continue;
		if (pos_target->second.Opposition == pRoleState->Opposition)
			continue;
		if (pos_target->second.PhysicalAttrib.ExemptCounter > 0)
			continue;
		nX = pRoleState->MoveAttrb.CurrentPos.X
				- pos_target->second.MoveAttrb.CurrentPos.X;
		nY = pRoleState->MoveAttrb.CurrentPos.Y
				- pos_target->second.MoveAttrb.CurrentPos.Y;
		if ((nX * nX + nY * nY) <= 9) {
			CalculateSkillHurt(pos_target->second, pRoleState->Level,
					hurt_value, SKILL_TYPE_JianRenFengBao);
		}
	}
}

//剑刃风暴
void PK_PE::skill_process_204(RoleState *pRoleState, RoleOrder role_order,
		char level) {
	short int hurt_second = 150;
	short int keepTime = 40;
	switch (level) {
	case 1:
		hurt_second = 150;
		keepTime = 40;
		break;
	case 2:
		hurt_second = 150;
		keepTime = 40;
		break;
	case 3:
		hurt_second = 200;
		keepTime = 40;
		break;
	case 4:
		hurt_second = 200;
		keepTime = 40;
		break;
	case 5:
		hurt_second = 250;
		keepTime = 40;
		break;
	case 6:
		hurt_second = 350;
		keepTime = 50;
		break;
	default:
		break;
	}

	Buff Buff_tmp(BUFFER_TYPE_CASTSKILL, SKILL_TYPE_JianRenFengBao, level,
			current_time.fragment_now, keepTime, false, true);
	Buff_tmp.Effecttype = BUFFER_EFFECT_TYPE_ABS;
	Buff_tmp.Parametar1 = hurt_second;
	Buff_tmp.RunTimeLimit = 10;

	UpdataBuffer(*pRoleState, Buff_tmp, UPDATABUFFER_ADD);
}

//妖术
void PK_PE::skill_process_203(RoleState *pRoleState, RoleOrder role_order,
		char level) {

	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}

	short int sheep_time = 20;
	short int param1 = 10;
	switch (level) {
	case 1:
		sheep_time = 20;
		param1 = 10;
		break;
	case 2:
		sheep_time = 25;
		param1 = 15;
		break;
	case 3:
		sheep_time = 30;
		param1 = 20;
		break;
	case 4:
		sheep_time = 35;
		param1 = 25;
		break;
	case 5:
		sheep_time = 40;
		param1 = 30;
		break;
	case 6:
		sheep_time = 50;
		param1 = 35;
		break;
	default:
		break;
	}
	Buff Buff_tmp(BUFFER_TYPE_BYSKILL, SKILL_TYPE_YaoShu, level,
			current_time.fragment_now, sheep_time, true, true);
	Buff_tmp.Effecttype = BUFFER_EFFECT_TYPE_RELATIVE;
	Buff_tmp.Parametar1 = -(pos_target->second.MoveAttrb.SpeedMod_Base * param1
			/ 100);

	UpdataBuffer(pos_target->second, Buff_tmp, UPDATABUFFER_ADD);

}

//活血术
void PK_PE::skill_process_202(RoleState *pRoleState, RoleOrder role_order,
		char level) {
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}
	unsigned short int keepTime = 30;
	int param1 = 5;
	switch (level) {
	case 1:
		param1 = 5;
		break;
	case 2:
		param1 = 6;
		break;
	case 3:
		param1 = 7;
		break;
	case 4:
		param1 = 8;
		break;
	case 5:
		param1 = 9;
		break;
	case 6:
		keepTime = 50;
		param1 = 10;
		break;
	default:
		break;
	}
	Buff Buff_tmp(BUFFER_TYPE_HP, SKILL_TYPE_HuoXueShu, level,
			current_time.fragment_now, keepTime, true, true);
	Buff_tmp.Parametar1 = param1;
	Buff_tmp.RunTimeLimit = 10;
	Buff_tmp.Effecttype = BUFFER_EFFECT_TYPE_RELATIVE;
	UpdataBuffer(pos_target->second, Buff_tmp, UPDATABUFFER_ADD);
}

//月蚀
void PK_PE::skill_process_201(RoleState *pRoleState, RoleOrder role_order,
		char level) {
	map<int, RoleState>::iterator pos_target;
	pos_target = P_Roles.find(role_order.TargetRoleID);
	if (pos_target == P_Roles.end()) {
		return;
	}
	short int skill_hurt = 150;
	unsigned short int keepTime = 0;
	short int param1 = 5;
	switch (level) {
	case 1:
		skill_hurt = 150;
		param1 = 5;
		break;
	case 2:
		skill_hurt = 250;
		param1 = 10;
		break;
	case 3:
		skill_hurt = 350;
		param1 = 15;
		break;
	case 4:
		skill_hurt = 450;
		param1 = 20;
		break;
	case 5:
		skill_hurt = 550;
		param1 = 20;
		break;
	case 6:
		skill_hurt = 650;
		param1 = 25;
		break;
	default:
		break;
	}
	CalculateSkillHurt(pos_target->second, pRoleState->Level, skill_hurt,
			SKILL_TYPE_YueShi);
	if (pos_target->second.Live != ROLE_STATUS_DEAD) {
		Buff Buff_tmp(BUFFER_TYPE_ARMOR, SKILL_TYPE_YueShi, level,
				current_time.fragment_now, keepTime, true, true);
		Buff_tmp.Parametar1
				= -(pos_target->second.PhysicalAttrib.DefenseArmor_Base
						* param1 / 100);

		UpdataBuffer(pos_target->second, Buff_tmp, UPDATABUFFER_ADD);
	}
}

void PK_PE::onRoleDead(RoleState & pTargetRole) {
	BrodacastDead(pTargetRole.RoleID);
	map<short int, SkillAttr>::iterator pos_skill;
	pos_skill = pTargetRole.MagicAttrb.SkillMap.find(SKILL_TYPE_FuHuo);

	if (pos_skill != pTargetRole.MagicAttrb.SkillMap.end()) {

		if (pos_skill->second.LastUseTime == 0 || ((current_time.fragment_now
				- pos_skill->second.LastUseTime) >= pos_skill->second.SkillCD)) {
			pTargetRole.multimap_buff.clear();
			pTargetRole.Init();
			Buff tmpBuff(BUFFER_TYPE_BYSKILL, SKILL_TYPE_FuHuo,
					pos_skill->second.SkillLevel, current_time.fragment_now,
					20, false, false);
			switch (pos_skill->second.SkillLevel) {
			case 1:
				tmpBuff.Parametar1 = 10;
				tmpBuff.Parametar2 = 10;
				break;
			case 2:
				tmpBuff.Parametar1 = 20;
				tmpBuff.Parametar2 = 20;
				break;
			case 3:
				tmpBuff.Parametar1 = 30;
				tmpBuff.Parametar2 = 30;
				break;
			case 4:
				tmpBuff.Parametar1 = 40;
				tmpBuff.Parametar2 = 40;
				break;
			case 5:
				tmpBuff.Parametar1 = 50;
				tmpBuff.Parametar2 = 50;
				break;
			case 6:
				tmpBuff.Parametar1 = 60;
				tmpBuff.Parametar2 = 60;
				break;
			default:
				break;
			}

			UpdataBuffer(pTargetRole, tmpBuff, UPDATABUFFER_ADD);
			RoleOrder tmpOrder;
			tmpOrder.OrderTime = current_time.fragment_now;
			tmpOrder.Type = SKILL_TYPE_FuHuo;
			tmpOrder.TargetType = 2;
			tmpOrder.TargetRoleID = pTargetRole.RoleID;
			Brodacast_Skill(pTargetRole.RoleID, tmpOrder);
			pos_skill->second.LastUseTime = current_time.fragment_now;
			pTargetRole.CurrentOrder.Type = ORDER_TYPE_NO;
			return;
		}
	}
	pTargetRole.Dead();
	CheckOver(pTargetRole.Opposition);
}

void PK_PE::BroadcastBegin() {
	PkgHead broadcast_head;
	char broadcast_begin_pos[LEN_PKG_HEAD] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD;
	broadcast_head.usMsgType = MSGTYPE_S_C_BEGIN;
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_begin_pos);
			m_pMain_thread->send_client(broadcast_begin_pos,
					broadcast_head.usPkgLen);
		}
	}
}

void PK_PE::BrodacastAttack(int role_id, int role_passive) {
//	LOG(LOG_DEBUG, __FILE__, __LINE__,"BrodacastAttack:attackID[%d],targetID[%d]", role_id, role_passive);
	PkgHead broadcast_head;
	char broadcast_attack[LEN_PKG_HEAD + 4 + 4 + 4] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usMsgType = MSGTYPE_S_C_ATTACK;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 4 + 4 + 4;
	memcpy(broadcast_attack + LEN_PKG_HEAD, &role_id, 4);
	memcpy(broadcast_attack + LEN_PKG_HEAD + 4, &current_time.fragment_now, 4);
	memcpy(broadcast_attack + LEN_PKG_HEAD + 4 + 4, &role_passive, 4);
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_attack);
			m_pMain_thread->send_client(broadcast_attack,
					broadcast_head.usPkgLen);
		}
	}
}

void PK_PE::BrodacastMultiHurt(map<int, short int> & target, short int effect) {
	PkgHead broadcast_head;
	short int roleNum = target.size();
	if (roleNum == 0)
		return;

	char broadcast_multiHurt[LEN_PKG_HEAD + 2 + MAX_ROLE_COUNT * 6 + 2 + 4] = {
			0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 2 + roleNum * 6 + 2 + 4;
	broadcast_head.usMsgType = MSGTYPE_S_C_MULTIHURT;
	map<int, short int>::iterator tmpTarget = target.begin();
	memcpy(broadcast_multiHurt + LEN_PKG_HEAD, &roleNum, 2);
	short int magLen = 2;
	for (; tmpTarget != target.end(); tmpTarget++) {
		memcpy(broadcast_multiHurt + LEN_PKG_HEAD + magLen, &tmpTarget->first,
				4);
		magLen += 4;
		memcpy(broadcast_multiHurt + LEN_PKG_HEAD + magLen, &tmpTarget->second,
				2);
		magLen += 2;
	}

	memcpy(broadcast_multiHurt + LEN_PKG_HEAD + magLen,
			&current_time.fragment_now, 4);
	magLen += 4;
	memcpy(broadcast_multiHurt + LEN_PKG_HEAD + magLen, &effect, 2);
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_multiHurt);
			m_pMain_thread->send_client(broadcast_multiHurt,
					broadcast_head.usPkgLen);
		}

	}

}

void PK_PE::BrodacastHpChange(int role_id, short int hp, char source,
		short int effectID) {
	PkgHead broadcast_head;
	char broadcast_hurt[LEN_PKG_HEAD + 13] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 13;
	broadcast_head.usMsgType = MSGTYPE_S_C_HPCHANGE;
	memcpy(broadcast_hurt + LEN_PKG_HEAD, &role_id, 4);
	memcpy(broadcast_hurt + LEN_PKG_HEAD + 4, &current_time.fragment_now, 4);
	memcpy(broadcast_hurt + LEN_PKG_HEAD + 4 + 4, &hp, 2);
	memcpy(broadcast_hurt + LEN_PKG_HEAD + 4 + 4 + 2, &source, 1);
	memcpy(broadcast_hurt + LEN_PKG_HEAD + 4 + 4 + 2 + 1, &effectID, 2);
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_hurt);
			m_pMain_thread->send_client(broadcast_hurt, broadcast_head.usPkgLen);
		}
	}
}

void PK_PE::BrodacastMpChange(int role_id, short int mp, short int effectID) {
	PkgHead broadcast_head;
	char broadcast_hurt[LEN_PKG_HEAD + 12] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 12;
	broadcast_head.usMsgType = MSGTYPE_S_C_MPCHANGE;
	memcpy(broadcast_hurt + LEN_PKG_HEAD, &role_id, 4);
	memcpy(broadcast_hurt + LEN_PKG_HEAD + 4, &current_time.fragment_now, 4);
	memcpy(broadcast_hurt + LEN_PKG_HEAD + 4 + 4, &mp, 2);
	memcpy(broadcast_hurt + LEN_PKG_HEAD + 4 + 4 + 2, &effectID, 2);
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_hurt);
			m_pMain_thread->send_client(broadcast_hurt, broadcast_head.usPkgLen);
		}
	}
}

void PK_PE::BrodacastDead(int role_id) {
	PkgHead broadcast_head;
	char broadcast_dead[LEN_PKG_HEAD + 4] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 4;
	broadcast_head.usMsgType = MSGTYPE_S_C_DEAD;
	memcpy(broadcast_dead + LEN_PKG_HEAD, &role_id, 4);
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_dead);
			m_pMain_thread->send_client(broadcast_dead, broadcast_head.usPkgLen);
		}
	}
}

void PK_PE::BrodacastStop(int role_id, Pos current_pos, Pos current_pos_mini) {
	PkgHead broadcast_head;
	char broadcast_dead[LEN_PKG_HEAD + 4 + 4 + 4 + 4] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 4 + 4 + 4 + 4;
	broadcast_head.usMsgType = MSGTYPE_S_C_STOP;
	memcpy(broadcast_dead + LEN_PKG_HEAD, &role_id, 4);
	memcpy(broadcast_dead + LEN_PKG_HEAD + 4, &current_time.fragment_now, 4);
	memcpy(broadcast_dead + LEN_PKG_HEAD + 4 + 4, &current_pos.X, 2);
	memcpy(broadcast_dead + LEN_PKG_HEAD + 4 + 4 + 2, &current_pos.Y, 2);
	memcpy(broadcast_dead + LEN_PKG_HEAD + 4 + 4 + 4, &current_pos_mini.X, 2);
	memcpy(broadcast_dead + LEN_PKG_HEAD + 4 + 4 + 4 + 2, &current_pos_mini.Y,
			2);
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_dead);
			m_pMain_thread->send_client(broadcast_dead, broadcast_head.usPkgLen);
		}
	}
}
void PK_PE::Brodacast_Skill(int role_id, RoleOrder & order) {
	PkgHead broadcast_head;
	char broadcast_skill[LEN_PKG_HEAD + 4 + 4 + 2 + 1 + 4] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 4 + 4 + 2 + 1 + 4;
	broadcast_head.usMsgType = MSGTYPE_S_C_SKILL;
	memcpy(broadcast_skill + LEN_PKG_HEAD, &role_id, 4);
	memcpy(broadcast_skill + LEN_PKG_HEAD + 4, &current_time.fragment_now, 4);
	memcpy(broadcast_skill + LEN_PKG_HEAD + 4 + 4, &order.Type, 2);
	memcpy(broadcast_skill + LEN_PKG_HEAD + 4 + 4 + 2, &order.TargetType, 1);
	if (order.TargetType == 1) {
		memcpy(broadcast_skill + LEN_PKG_HEAD + 4 + 4 + 2 + 1,
				&order.TargetPos.X, 2);
		memcpy(broadcast_skill + LEN_PKG_HEAD + 4 + 4 + 2 + 1 + 2,
				&order.TargetPos.Y, 2);
	} else {
		memcpy(broadcast_skill + LEN_PKG_HEAD + 4 + 4 + 2 + 1,
				&order.TargetRoleID, 4);
	}
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_skill);
			m_pMain_thread->send_client(broadcast_skill,
					broadcast_head.usPkgLen);
		}
	}
}

void PK_PE::Brodacast_Skill_Buffer(int role_id, short int type,
		short int parentSkillID, short int keepTime, int param1, int param2) {
	PkgHead broadcast_head;
	char broadcast_skill_buffer[LEN_PKG_HEAD + 22] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 22;
	broadcast_head.usMsgType = MSGTYPE_S_C_SKILL_BUFFER;
	memcpy(broadcast_skill_buffer + LEN_PKG_HEAD, &role_id, 4);
	memcpy(broadcast_skill_buffer + LEN_PKG_HEAD + 4, &type, 2);
	memcpy(broadcast_skill_buffer + LEN_PKG_HEAD + 4 + 2, &parentSkillID, 2);
	memcpy(broadcast_skill_buffer + LEN_PKG_HEAD + 4 + 2 + 2,
			&current_time.fragment_now, 4);
	memcpy(broadcast_skill_buffer + LEN_PKG_HEAD + 4 + 2 + 4 + 2, &keepTime, 2);
	memcpy(broadcast_skill_buffer + LEN_PKG_HEAD + 4 + 2 + 4 + 2 + 2, &param1,
			4);
	memcpy(broadcast_skill_buffer + LEN_PKG_HEAD + 4 + 2 + 4 + 2 + 4 + 2,
			&param2, 4);
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_skill_buffer);
			m_pMain_thread->send_client(broadcast_skill_buffer,
					broadcast_head.usPkgLen);
		}
	}
}

void PK_PE::Brodacast_Skill_Newrole(char *new_role) {
	PkgHead broadcast_head;
	char broadcast_skill[LEN_PKG_HEAD + 41] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 41;
	broadcast_head.usMsgType = MSGTYPE_S_C_SKILL_NEWROLE;
	memcpy(broadcast_skill + LEN_PKG_HEAD, new_role, 41);
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_skill);
			m_pMain_thread->send_client(broadcast_skill,
					broadcast_head.usPkgLen);
		}
	}

}

void PK_PE::Brodacast_Buffer_Run(int roleID, short int skillType, int beginTime) {
	PkgHead broadcast_head;
	char broadcast_BufferRun[LEN_PKG_HEAD + 14] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 14;
	broadcast_head.usMsgType = MSGTYPE_S_C_BUFFER_RUN;
	memcpy(broadcast_BufferRun + LEN_PKG_HEAD, &roleID, 4);
	memcpy(broadcast_BufferRun + LEN_PKG_HEAD + 4, &skillType, 2);
	memcpy(broadcast_BufferRun + LEN_PKG_HEAD + 6, &current_time.fragment_now,
			4);
	memcpy(broadcast_BufferRun + LEN_PKG_HEAD + 10, &beginTime, 4);
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_BufferRun);
			m_pMain_thread->send_client(broadcast_BufferRun,
					broadcast_head.usPkgLen);
		}
	}
}
void PK_PE::Brodacast_Buffer_Stop(int roleID, short int bufferID,
		short int skillID, int beginTime) {
	PkgHead broadcast_head;
	char broadcast_BufferStop[LEN_PKG_HEAD + 16] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 16;
	broadcast_head.usMsgType = MSGTYPE_S_C_BUFFER_STOP;

	memcpy(broadcast_BufferStop + LEN_PKG_HEAD, &roleID, 4);
	memcpy(broadcast_BufferStop + LEN_PKG_HEAD + 4, &bufferID, 2);
	memcpy(broadcast_BufferStop + LEN_PKG_HEAD + 6, &skillID, 2);
	memcpy(broadcast_BufferStop + LEN_PKG_HEAD + 8, &beginTime, 4);
	memcpy(broadcast_BufferStop + LEN_PKG_HEAD + 12,
			&current_time.fragment_now, 4);
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_BufferStop);
			m_pMain_thread->send_client(broadcast_BufferStop,
					broadcast_head.usPkgLen);
		}
	}
}

void PK_PE::Brodacast_Role_LocationOffset(int roleID, short int targetX,
		short int targetY, short int parentSkill) {
	PkgHead broadcast_head;
	char broadcast_Offset[LEN_PKG_HEAD + 4 + 4 + 2 + 2 + 2] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 14;
	broadcast_head.usMsgType = MSGTYPE_S_C_LOCATAINOFFSET;
	memcpy(broadcast_Offset + LEN_PKG_HEAD, &roleID, 4);
	memcpy(broadcast_Offset + LEN_PKG_HEAD + 4, &current_time.fragment_now, 4);
	memcpy(broadcast_Offset + LEN_PKG_HEAD + 4 + 4, &targetX, 2);
	memcpy(broadcast_Offset + LEN_PKG_HEAD + 4 + 4 + 2, &targetY, 2);
	memcpy(broadcast_Offset + LEN_PKG_HEAD + 4 + 4 + 2 + 2, &parentSkill, 2);
	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_Offset);
			m_pMain_thread->send_client(broadcast_Offset,
					broadcast_head.usPkgLen);
		}
	}
}

int PK_PE::EndGame(char WinOpposition) {
	PkgHead broadcast_head;
	char broadcast_FightEnd[LEN_PKG_HEAD + 4] = { 0 };
	broadcast_head.ucDirection = DIRECT_S_C_REQ;
	broadcast_head.ucSrvType = SRVTYPE_PK;
	broadcast_head.ucSrvSeq = option.srv_req;
	broadcast_head.usPkgLen = LEN_PKG_HEAD + 4;
	broadcast_head.usMsgType = MSGTYPE_S_C_FIGHTEND;
	memcpy(broadcast_FightEnd + LEN_PKG_HEAD, &current_time.fragment_now, 4);
	map<int, RoleState>::iterator pos_role;

	int sockfd = 0;
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(option.GEPort);
	inet_pton(AF_INET, option.GEIP, &servaddr.sin_addr);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr))
			!= 0) {
		LOG(LOG_ERROR, __FILE__, __LINE__,
				"connect exchange[%s][%d] fail! error[%s]", option.GWIP,
				option.GWPort, strerror(errno));
		return -1;
	}

	struct timeval timeout = { 3, 0 };
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout,
			sizeof(struct timeval));

	PkgHead stPkgHead;
	stPkgHead.ucDirection = DIRECT_SC_S_REQ;
	stPkgHead.ucSrvType = SRVTYPE_GE;
	stPkgHead.ucSrvSeq = 1;
	stPkgHead.usMsgType = MSGTYPE_S_S_END_GAME;

	char sBuffer[LEN_PKG_HEAD + 4 + 2 + 1 + 2 + MAX_ROLE_COUNT * 12] = { 0 };
	int buffer_pos = LEN_PKG_HEAD + 2 + 1 + 2 + 4;
	short int role_count = 0;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if ((pos_role->second.RoleFlag == RoleType_Player)
				|| (pos_role->second.RoleFlag == RoleType_Pet)) {
			role_count++;
			memcpy(sBuffer + buffer_pos, &pos_role->second.RoleFlag, 1);
			buffer_pos += 1;
			memcpy(sBuffer + buffer_pos, &pos_role->second.RoleType, 4);
			buffer_pos += 4;
			memcpy(sBuffer + buffer_pos, &pos_role->second.RoleID, 4);
			buffer_pos += 4;
			memcpy(sBuffer + buffer_pos, &pos_role->second.Opposition, 1);
			buffer_pos += 1;
			memcpy(sBuffer + buffer_pos, &pos_role->second.Live, 1);
			buffer_pos += 1;
			memcpy(sBuffer + buffer_pos, &pos_role->second.HP, 2);
			buffer_pos += 2;
			memcpy(sBuffer + buffer_pos, &pos_role->second.MP, 2);
			buffer_pos += 2;
		}
		if (pos_role->second.RoleFlag == RoleType_Player) {
			LOG(LOG_DEBUG, __FILE__, __LINE__, "Fight End RoleID[%d]",
					pos_role->second.RoleID);
			broadcast_head.uiRoleID = pos_role->second.ControlID;
			broadcast_head.packet(broadcast_FightEnd);
			m_pMain_thread->send_client(broadcast_FightEnd,
					broadcast_head.usPkgLen);
		}
	}
	memcpy(sBuffer + LEN_PKG_HEAD, &m_PK_ID, 4);
	memcpy(sBuffer + LEN_PKG_HEAD + 4, &map_id, 2);
	memcpy(sBuffer + LEN_PKG_HEAD + 2 + 4, &WinOpposition, 1);
	memcpy(sBuffer + LEN_PKG_HEAD + 2 + 1 + 4, &role_count, 2);
	stPkgHead.usPkgLen = buffer_pos;
	stPkgHead.packet(sBuffer);

	if (buffer_pos != send(sockfd, sBuffer, buffer_pos, 0)) {
		LOG(LOG_ERROR, __FILE__, __LINE__, "send end game to GE[%s:%d] fail!",
				option.GEIP, option.GEPort);
		close(sockfd);
		return -1;
	}

	buffer_pos = LEN_PKG_HEAD + 4;
	memset(sBuffer, 0, sizeof(sBuffer));
	if (buffer_pos != recv(sockfd, sBuffer, buffer_pos, 0)) {
		LOG(LOG_ERROR, __FILE__, __LINE__,
				"recv end game from GE[%s:%d] fail!", option.GEIP,
				option.GEPort);
		close(sockfd);
		return -1;
	}

	close(sockfd);
	return 0;
}

int PK_PE::CheckOver(const char opposition) {
	if (PK_State == PK_STATE_OVER)
		return 0;
	char WinOpposition = 0;
	map<int, RoleState>::iterator it = P_Roles.begin();
	for (; it != P_Roles.end(); ++it) {
		if (it->second.Opposition == opposition) {
			if (it->second.Live == ROLE_STATUS_LIVE)
				return 0;
		} else {
			WinOpposition = it->second.Opposition;
		}
	}
	EndGame(WinOpposition);
	PK_State = PK_STATE_OVER;
	LOG(LOG_DEBUG, __FILE__, __LINE__, "PK[%d] over", m_PK_ID);
	return 0;
}

int PK_PE::AStar(RoleState *role, Pos StartPos, Pos EndPos,
		list<Pos> *MoveLine, char *FirstDirect) {
	list<PosA> WaitingList;
	list<PosA> DoneList;
	list<PosA>::iterator pos;
	list<PosA>::iterator pos_min;

	if ((EndPos.X >= MAP_SIZE_X) || (EndPos.Y >= MAP_SIZE_Y) || (EndPos.X < 0)
			|| (EndPos.Y < 0)) {
		LOG(LOG_ERROR, __FILE__, __LINE__, "AStar:end point [%d, %d] error!",
				EndPos.X, EndPos.Y);
		return -1;
	}
	if (pk_map_base[EndPos.Y][EndPos.X] == 0) {
		//LOG (LOG_ERROR, __FILE__, __LINE__, "AStar:end point [%d, %d] can not move to!", EndPos.X, EndPos.Y);
		GetEmptyPos(EndPos.X, EndPos.Y, role->MoveAttrb.Direct, &EndPos);
	}

	PosA CurrentNode;
	CurrentNode.Node.X = StartPos.X;
	CurrentNode.Node.Y = StartPos.Y;
	CurrentNode.NodeParent.X = StartPos.X;
	CurrentNode.NodeParent.Y = StartPos.Y;
	CurrentNode.F = 0;
	CurrentNode.G = 0;
	WaitingList.push_back(CurrentNode);

	while (1) {
		//如果下一个要处理的节点是目标节点，退出
		if ((CurrentNode.Node.X == EndPos.X)
				&& (CurrentNode.Node.Y == EndPos.Y)) {
			DoneList.push_back(CurrentNode);
			break;
		}

		// 1、把当前处理节点的周边所有格子放到列表中
		// 1.1	检查当前格子的周边格子是否已经在列表中
		int P1 = 0;
		int P2 = 0;
		int P3 = 0;
		int P4 = 0;
		int P5 = 0;
		int P6 = 0;
		int P7 = 0;
		int P8 = 0;
		for (pos = WaitingList.begin(); pos != WaitingList.end(); ++pos) {
			if ((pos->Node.X == CurrentNode.Node.X) && (pos->Node.Y
					== CurrentNode.Node.Y - 1))
				P1 = 1;
			if ((pos->Node.X == CurrentNode.Node.X + 1) && (pos->Node.Y
					== CurrentNode.Node.Y - 1))
				P2 = 1;
			if ((pos->Node.X == CurrentNode.Node.X + 1) && (pos->Node.Y
					== CurrentNode.Node.Y))
				P3 = 1;
			if ((pos->Node.X == CurrentNode.Node.X + 1) && (pos->Node.Y
					== CurrentNode.Node.Y + 1))
				P4 = 1;
			if ((pos->Node.X == CurrentNode.Node.X) && (pos->Node.Y
					== CurrentNode.Node.Y + 1))
				P5 = 1;
			if ((pos->Node.X == CurrentNode.Node.X - 1) && (pos->Node.Y
					== CurrentNode.Node.Y + 1))
				P6 = 1;
			if ((pos->Node.X == CurrentNode.Node.X - 1) && (pos->Node.Y
					== CurrentNode.Node.Y))
				P7 = 1;
			if ((pos->Node.X == CurrentNode.Node.X - 1) && (pos->Node.Y
					== CurrentNode.Node.Y - 1))
				P8 = 1;
		}
		for (pos = DoneList.begin(); pos != DoneList.end(); ++pos) {
			if ((pos->Node.X == CurrentNode.Node.X) && (pos->Node.Y
					== CurrentNode.Node.Y - 1))
				P1 = 1;
			if ((pos->Node.X == CurrentNode.Node.X + 1) && (pos->Node.Y
					== CurrentNode.Node.Y - 1))
				P2 = 1;
			if ((pos->Node.X == CurrentNode.Node.X + 1) && (pos->Node.Y
					== CurrentNode.Node.Y))
				P3 = 1;
			if ((pos->Node.X == CurrentNode.Node.X + 1) && (pos->Node.Y
					== CurrentNode.Node.Y + 1))
				P4 = 1;
			if ((pos->Node.X == CurrentNode.Node.X) && (pos->Node.Y
					== CurrentNode.Node.Y + 1))
				P5 = 1;
			if ((pos->Node.X == CurrentNode.Node.X - 1) && (pos->Node.Y
					== CurrentNode.Node.Y + 1))
				P6 = 1;
			if ((pos->Node.X == CurrentNode.Node.X - 1) && (pos->Node.Y
					== CurrentNode.Node.Y))
				P7 = 1;
			if ((pos->Node.X == CurrentNode.Node.X - 1) && (pos->Node.Y
					== CurrentNode.Node.Y - 1))
				P8 = 1;
		}

		//计算一个格子周边的的全部预估值
		if ((P1 == 0) && ((CurrentNode.Node.Y - 1) >= 0)
				&& (pk_map_base[CurrentNode.Node.Y - 1][CurrentNode.Node.X]
						!= 0)) {
			PosA PosATmp;
			PosATmp.Node.X = CurrentNode.Node.X;
			PosATmp.Node.Y = CurrentNode.Node.Y - 1;
			PosATmp.F = 10 + CurrentNode.F;
			PosATmp.G = PosATmp.F + abs(EndPos.X - CurrentNode.Node.X) * 10
					+ abs(EndPos.Y - (CurrentNode.Node.Y - 1)) * 10;
			PosATmp.NodeParent.X = CurrentNode.Node.X;
			PosATmp.NodeParent.Y = CurrentNode.Node.Y;
			WaitingList.push_back(PosATmp);
		}
		if ((P2 == 0) && ((CurrentNode.Node.X + 1) < MAP_SIZE_X)
				&& ((CurrentNode.Node.Y - 1) >= 0)
				&& (pk_map_base[CurrentNode.Node.Y - 1][CurrentNode.Node.X + 1]
						!= 0)) {
			PosA PosATmp;
			PosATmp.Node.X = CurrentNode.Node.X + 1;
			PosATmp.Node.Y = CurrentNode.Node.Y - 1;
			PosATmp.F = 14 + CurrentNode.F;
			PosATmp.G = PosATmp.F + abs(EndPos.X - (CurrentNode.Node.X + 1))
					* 10 + abs(EndPos.Y - (CurrentNode.Node.Y - 1)) * 10;
			PosATmp.NodeParent.X = CurrentNode.Node.X;
			PosATmp.NodeParent.Y = CurrentNode.Node.Y;
			WaitingList.push_back(PosATmp);

		}
		if ((P3 == 0) && ((CurrentNode.Node.X + 1) < MAP_SIZE_X)
				&& (pk_map_base[CurrentNode.Node.Y][CurrentNode.Node.X + 1]
						!= 0)) {
			PosA PosATmp;
			PosATmp.Node.X = CurrentNode.Node.X + 1;
			PosATmp.Node.Y = CurrentNode.Node.Y;
			PosATmp.F = 10 + CurrentNode.F;
			PosATmp.G = PosATmp.F + abs(EndPos.X - (CurrentNode.Node.X + 1))
					* 10 + abs(EndPos.Y - (CurrentNode.Node.Y)) * 10;
			PosATmp.NodeParent.X = CurrentNode.Node.X;
			PosATmp.NodeParent.Y = CurrentNode.Node.Y;
			WaitingList.push_back(PosATmp);
		}
		if ((P4 == 0) && ((CurrentNode.Node.X + 1) < MAP_SIZE_X)
				&& ((CurrentNode.Node.Y + 1) < MAP_SIZE_Y)
				&& (pk_map_base[CurrentNode.Node.Y + 1][CurrentNode.Node.X + 1]
						!= 0)) {
			PosA PosATmp;
			PosATmp.Node.X = CurrentNode.Node.X + 1;
			PosATmp.Node.Y = CurrentNode.Node.Y + 1;
			PosATmp.F = 14 + CurrentNode.F;
			PosATmp.G = PosATmp.F + abs(EndPos.X - (CurrentNode.Node.X + 1))
					* 10 + abs(EndPos.Y - (CurrentNode.Node.Y + 1)) * 10;
			PosATmp.NodeParent.X = CurrentNode.Node.X;
			PosATmp.NodeParent.Y = CurrentNode.Node.Y;
			WaitingList.push_back(PosATmp);
		}
		if ((P5 == 0) && ((CurrentNode.Node.Y + 1) < MAP_SIZE_Y)
				&& (pk_map_base[CurrentNode.Node.Y + 1][CurrentNode.Node.X]
						!= 0)) {
			PosA PosATmp;
			PosATmp.Node.X = CurrentNode.Node.X;
			PosATmp.Node.Y = CurrentNode.Node.Y + 1;
			PosATmp.F = 10 + CurrentNode.F;
			PosATmp.G = PosATmp.F + abs(EndPos.X - CurrentNode.Node.X) * 10
					+ abs(EndPos.Y - (CurrentNode.Node.Y + 1)) * 10;
			PosATmp.NodeParent.X = CurrentNode.Node.X;
			PosATmp.NodeParent.Y = CurrentNode.Node.Y;
			WaitingList.push_back(PosATmp);
		}
		if ((P6 == 0) && ((CurrentNode.Node.X - 1) >= 0)
				&& ((CurrentNode.Node.Y + 1) < MAP_SIZE_Y)
				&& (pk_map_base[CurrentNode.Node.Y + 1][CurrentNode.Node.X - 1]
						!= 0)) {
			PosA PosATmp;
			PosATmp.Node.X = CurrentNode.Node.X - 1;
			PosATmp.Node.Y = CurrentNode.Node.Y + 1;
			PosATmp.F = 14 + CurrentNode.F;
			PosATmp.G = PosATmp.F + abs(EndPos.X - (CurrentNode.Node.X - 1))
					* 10 + abs(EndPos.Y - (CurrentNode.Node.Y + 1)) * 10;
			PosATmp.NodeParent.X = CurrentNode.Node.X;
			PosATmp.NodeParent.Y = CurrentNode.Node.Y;
			WaitingList.push_back(PosATmp);
		}
		if ((P7 == 0) && ((CurrentNode.Node.X - 1) >= 0)
				&& (pk_map_base[CurrentNode.Node.Y][CurrentNode.Node.X - 1]
						!= 0)) {
			PosA PosATmp;
			PosATmp.Node.X = CurrentNode.Node.X - 1;
			PosATmp.Node.Y = CurrentNode.Node.Y;
			PosATmp.F = 10 + CurrentNode.F;
			PosATmp.G = PosATmp.F + abs(EndPos.X - (CurrentNode.Node.X - 1))
					* 10 + abs(EndPos.Y - (CurrentNode.Node.Y)) * 10;
			PosATmp.NodeParent.X = CurrentNode.Node.X;
			PosATmp.NodeParent.Y = CurrentNode.Node.Y;
			WaitingList.push_back(PosATmp);
		}
		if ((P8 == 0) && ((CurrentNode.Node.X - 1) >= 0)
				&& ((CurrentNode.Node.Y - 1) >= 0)
				&& (pk_map_base[CurrentNode.Node.Y - 1][CurrentNode.Node.X - 1]
						!= 0)) {
			PosA PosATmp;
			PosATmp.Node.X = CurrentNode.Node.X - 1;
			PosATmp.Node.Y = CurrentNode.Node.Y - 1;
			PosATmp.F = 14 + CurrentNode.F;
			PosATmp.G = PosATmp.F + abs(EndPos.X - (CurrentNode.Node.X - 1))
					* 10 + abs(EndPos.Y - (CurrentNode.Node.Y - 1)) * 10;
			PosATmp.NodeParent.X = CurrentNode.Node.X;
			PosATmp.NodeParent.Y = CurrentNode.Node.Y;
			WaitingList.push_back(PosATmp);
		}

		//把当前点从WaitingList删掉，在donelist中增加
		for (pos = WaitingList.begin(); pos != WaitingList.end(); ++pos) {
			if ((pos->Node.X == CurrentNode.Node.X) && (pos->Node.Y
					== CurrentNode.Node.Y)) {
				DoneList.push_back(*pos);
				WaitingList.erase(pos);
				break;
			}
		}

		//如果WaitingList为空，表示搜索失败
		if (WaitingList.size() == 0) {
			LOG(LOG_ERROR, __FILE__, __LINE__,
					"AStar: can not find the way[%d,%d] to[%d,%d]", StartPos.X,
					StartPos.Y, EndPos.X, EndPos.Y);
			return -1;
		} else {
			pos = WaitingList.begin();
			int MinLine = pos->G;
			for (; pos != WaitingList.end(); ++pos) {
				if (pos->G <= MinLine) {
					MinLine = pos->G;
					CurrentNode = *pos;
				}
			}
			if (MinLine >= MAP_MAX_LINE) {
				LOG(LOG_ERROR, __FILE__, __LINE__,
						"AStar: way[%d,%d] to[%d,%d] more than[%d]",
						StartPos.X, StartPos.Y, EndPos.X, EndPos.Y,
						MAP_MAX_LINE);
				return -1;
			}
		}
	}

	//倒推得出路径
	pos = DoneList.end();
	pos--;
	MoveLine->clear();
	MoveLine->push_front(pos->Node);
	for (; pos != DoneList.begin();) {
		list<PosA>::iterator PrePos = pos;
		for (;;) {
			PrePos--;
			if (PrePos == DoneList.begin()) {
				*FirstDirect = getDirect(pos->Node, PrePos->Node);
				if (*FirstDirect == 0)
					return -1;
			}
			if (((pos->NodeParent.X == PrePos->Node.X) && (pos->NodeParent.Y
					== PrePos->Node.Y)) || (PrePos == DoneList.begin())) {
				MoveLine->push_front(PrePos->Node);
				pos = PrePos;
				break;
			}
		}
	}

	//下发路径给客户端
	char sRespBuff[256];
	PkgHead RespPkgHead;
	RespPkgHead.ucDirection = DIRECT_S_C_REQ;
	RespPkgHead.ucSrvType = SRVTYPE_PK;
	RespPkgHead.ucSrvSeq = option.srv_req;
	RespPkgHead.usMsgType = MSGTYPE_S_C_MOVE;
	memcpy(sRespBuff + LEN_PKG_HEAD, &role->RoleID, 4);
	memcpy(sRespBuff + LEN_PKG_HEAD + 4, &current_time.fragment_now, 4);
	memcpy(sRespBuff + LEN_PKG_HEAD + 4 + 4, &role->MoveAttrb.CurrentPosMini.X,
			2);
	memcpy(sRespBuff + LEN_PKG_HEAD + 4 + 4 + 2,
			&role->MoveAttrb.CurrentPosMini.Y, 2);

	list<Pos>::iterator pos_line_pre;
	list<Pos>::iterator pos_line_current;
	pos_line_pre = MoveLine->begin();
	char direct_current = *FirstDirect;
	short int pos_len = LEN_PKG_HEAD + 4 + 4 + 4 + 2;
	memcpy(sRespBuff + pos_len, &(pos_line_pre->X), 2);
	pos_len += 2;
	memcpy(sRespBuff + pos_len, &(pos_line_pre->Y), 2);
	pos_len += 2;
	pos_line_current = pos_line_pre;
	pos_line_current++;
	short int turn_point = 1;

	for (; pos_line_current != MoveLine->end(); ++pos_line_current, ++pos_line_pre) {
		if (pos_line_current->X > pos_line_pre->X) {
			if (pos_line_current->Y > pos_line_pre->Y) {
				if (direct_current == DIRECT_EAST_SOUTH)
					continue;
				else
					direct_current = DIRECT_EAST_SOUTH;
			} else if (pos_line_current->Y < pos_line_pre->Y) {
				if (direct_current == DIRECT_NORTH_EAST)
					continue;
				else
					direct_current = DIRECT_NORTH_EAST;
			} else {
				if (direct_current == DIRECT_EAST)
					continue;
				else
					direct_current = DIRECT_EAST;
			}
		} else if (pos_line_current->X < pos_line_pre->X) {
			if (pos_line_current->Y > pos_line_pre->Y) {
				if (direct_current == DIRECT_SOUTH_WEST)
					continue;
				else
					direct_current = DIRECT_SOUTH_WEST;
			} else if (pos_line_current->Y < pos_line_pre->Y) {
				if (direct_current == DIRECT_WEST_NORTH)
					continue;
				else
					direct_current = DIRECT_WEST_NORTH;
			} else {
				if (direct_current == DIRECT_WEST)
					continue;
				else
					direct_current = DIRECT_WEST;
			}
		} else {
			if (pos_line_current->Y > pos_line_pre->Y) {
				if (direct_current == DIRECT_SOUTH)
					continue;
				else
					direct_current = DIRECT_SOUTH;
			} else if (pos_line_current->Y < pos_line_pre->Y) {
				if (direct_current == DIRECT_NORTH)
					continue;
				else
					direct_current = DIRECT_NORTH;
			} else {
				return -1;
			}
		}

		memcpy(sRespBuff + pos_len, &(pos_line_pre->X), 2);
		pos_len += 2;
		memcpy(sRespBuff + pos_len, &(pos_line_pre->Y), 2);
		pos_len += 2;
		turn_point++;
	}

	memcpy(sRespBuff + pos_len, &(pos_line_pre->X), 2);
	pos_len += 2;
	memcpy(sRespBuff + pos_len, &(pos_line_pre->Y), 2);
	pos_len += 2;
	turn_point++;

	memcpy(sRespBuff + LEN_PKG_HEAD + 4 + 4 + 4, &turn_point, 2);
	RespPkgHead.usPkgLen = pos_len;

	map<int, RoleState>::iterator pos_role;
	for (pos_role = P_Roles.begin(); pos_role != P_Roles.end(); ++pos_role) {
		if (pos_role->second.RoleFlag == RoleType_Player) {
//			LOG(LOG_DEBUG, __FILE__, __LINE__, "Send Move Msg:roleID[%d]",
//					pos_role->second.RoleID);
			RespPkgHead.uiRoleID = pos_role->second.ControlID;
			RespPkgHead.packet(sRespBuff);
			m_pMain_thread->send_client(sRespBuff, RespPkgHead.usPkgLen);
//			LOG(LOG_DEBUG, __FILE__, __LINE__, " Move Msg Length:[%d]",
//					RespPkgHead.usPkgLen);
		}
	}
	return 0;
}

void PK_PE::GetEmptyPos(short int currX, short int currY, char direct,
		Pos* result) {
	short int offsetX = 0;
	short int offsetY = 0;
	switch (direct) {
	case DIRECT_EAST://右
		offsetX++;
		break;
	case DIRECT_EAST_SOUTH://右下
		offsetX++;
		offsetY++;
		break;
	case DIRECT_NORTH://上
		offsetY--;
		break;
	case DIRECT_NORTH_EAST://右上
		offsetX++;
		offsetY--;
		break;
	case DIRECT_SOUTH://下
		offsetY++;
		break;
	case DIRECT_SOUTH_WEST://左下
		offsetX--;
		offsetY++;
		break;
	case DIRECT_WEST://左
		offsetX--;
		break;
	case DIRECT_WEST_NORTH://左上
		offsetX--;
		offsetY--;
		break;
	default:
		break;
	}
	short int loopCounter = 0;
	result->X = currX;
	result->Y = currY;

	loopCounter = 3;
	short int i = 0;

	//搜索前方
	for (i = 0; i < loopCounter; i++) {
		result->X += offsetX;
		result->Y += offsetY;
		if (result->X < 0 || result->X >= MAP_SIZE_X || result->Y < 0
				|| result->Y >= MAP_SIZE_Y) {
			break;
		}
		if (pk_map_base[result->Y][result->X] != 0) {
			return;
		}
	}

	for (short int circle = 1; circle < 5; circle++) {
		for (short int xloop = currX - circle; xloop < currX + circle + 1; xloop++) {
			for (short int yloop = currY - circle; yloop < currY + circle + 1; yloop++) {
				if (xloop == currX && yloop == currY)
					continue;
				if (xloop >= 0 && xloop < MAP_SIZE_X && yloop >= 0 && yloop
						< MAP_SIZE_Y) {
					if (pk_map_base[yloop][xloop] != 0) {
						result->X = xloop;
						result->Y = yloop;
						return;
					}
				}
			}
		}
	}
	result->X = currX;
	result->Y = currY;
	return;
}

