/* Copyright (C) 2006 - 2010 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Azuremyst_Isle
SD%Complete: 75
SDComment: Quest support: 9283, 9528, 9537, 9554(special flight path, proper model for mount missing). Injured Draenei cosmetic only
SDCategory: Azuremyst Isle
EndScriptData */

/* ContentData
npc_draenei_survivor
npc_engineer_spark_overgrind
npc_injured_draenei
npc_magwin
npc_susurrus
EndContentData */

#include "precompiled.h"
#include "escort_ai.h"
#include <cmath>

/*######
## npc_draenei_survivor
######*/

enum
{
    SAY_HEAL1           = -1000176,
    SAY_HEAL2           = -1000177,
    SAY_HEAL3           = -1000178,
    SAY_HEAL4           = -1000179,
    SAY_HELP1           = -1000180,
    SAY_HELP2           = -1000181,
    SAY_HELP3           = -1000182,
    SAY_HELP4           = -1000183,

    SPELL_IRRIDATION    = 35046,
    SPELL_STUNNED       = 28630
};

struct MANGOS_DLL_DECL npc_draenei_survivorAI : public ScriptedAI
{
    npc_draenei_survivorAI(Creature* pCreature) : ScriptedAI(pCreature) {Reset();}

    uint64 m_uiCaster;

    uint32 m_uiSayThanksTimer;
    uint32 m_uiRunAwayTimer;
    uint32 m_uiSayHelpTimer;

    bool m_bCanSayHelp;

    void Reset()
    {
        m_uiCaster = 0;

        m_uiSayThanksTimer = 0;
        m_uiRunAwayTimer = 0;
        m_uiSayHelpTimer = 10000;

        m_bCanSayHelp = true;

        m_creature->CastSpell(m_creature, SPELL_IRRIDATION, true);

        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
        m_creature->SetHealth(int(m_creature->GetMaxHealth()*.1));
        m_creature->SetStandState(UNIT_STAND_STATE_SLEEP);
    }

    void MoveInLineOfSight(Unit* pWho)
    {
        if (m_bCanSayHelp && pWho->GetTypeId() == TYPEID_PLAYER && m_creature->IsFriendlyTo(pWho) &&
            m_creature->IsWithinDistInMap(pWho, 25.0f))
        {
            //Random switch between 4 texts
            switch(urand(0, 3))
            {
                case 0: DoScriptText(SAY_HELP1, m_creature, pWho); break;
                case 1: DoScriptText(SAY_HELP2, m_creature, pWho); break;
                case 2: DoScriptText(SAY_HELP3, m_creature, pWho); break;
                case 3: DoScriptText(SAY_HELP4, m_creature, pWho); break;
            }

            m_uiSayHelpTimer = 20000;
            m_bCanSayHelp = false;
        }
    }

    void SpellHit(Unit* pCaster, const SpellEntry* pSpell)
    {
        if (pSpell->SpellFamilyFlags2 & 0x080000000)
        {
            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            m_creature->SetStandState(UNIT_STAND_STATE_STAND);

            m_creature->CastSpell(m_creature, SPELL_STUNNED, true);

            m_uiCaster = pCaster->GetGUID();

            m_uiSayThanksTimer = 5000;
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (m_uiSayThanksTimer)
        {
            if (m_uiSayThanksTimer <= uiDiff)
            {
                m_creature->RemoveAurasDueToSpell(SPELL_IRRIDATION);

                if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_uiCaster))
                {
                    if (pPlayer->GetTypeId() != TYPEID_PLAYER)
                        return;

                    switch(urand(0, 3))
                    {
                        case 0: DoScriptText(SAY_HEAL1, m_creature, pPlayer); break;
                        case 1: DoScriptText(SAY_HEAL2, m_creature, pPlayer); break;
                        case 2: DoScriptText(SAY_HEAL3, m_creature, pPlayer); break;
                        case 3: DoScriptText(SAY_HEAL4, m_creature, pPlayer); break;
                    }

                    pPlayer->TalkedToCreature(m_creature->GetEntry(),m_creature->GetGUID());
                }

                m_creature->GetMotionMaster()->Clear();
                m_creature->GetMotionMaster()->MovePoint(0, -4115.053711f, -13754.831055f, 73.508949f);

                m_uiRunAwayTimer = 10000;
                m_uiSayThanksTimer = 0;
            }else m_uiSayThanksTimer -= uiDiff;

            return;
        }

        if (m_uiRunAwayTimer)
        {
            if (m_uiRunAwayTimer <= uiDiff)
                m_creature->ForcedDespawn();
            else
                m_uiRunAwayTimer -= uiDiff;

            return;
        }

        if (m_uiSayHelpTimer < uiDiff)
        {
            m_bCanSayHelp = true;
            m_uiSayHelpTimer = 20000;
        }else m_uiSayHelpTimer -= uiDiff;
    }
};

CreatureAI* GetAI_npc_draenei_survivor(Creature* pCreature)
{
    return new npc_draenei_survivorAI(pCreature);
}

/*######
## npc_engineer_spark_overgrind
######*/

enum
{
    SAY_TEXT                = -1000184,
    EMOTE_SHELL             = -1000185,
    SAY_ATTACK              = -1000186,

    AREA_COVE               = 3579,
    AREA_ISLE               = 3639,
    QUEST_GNOMERCY          = 9537,
    FACTION_HOSTILE         = 14,
    SPELL_DYNAMITE          = 7978
};

#define GOSSIP_FIGHT        "Traitor! You will be brought to justice!"

struct MANGOS_DLL_DECL npc_engineer_spark_overgrindAI : public ScriptedAI
{
    npc_engineer_spark_overgrindAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_uiNormFaction = pCreature->getFaction();
        m_uiNpcFlags = pCreature->GetUInt32Value(UNIT_NPC_FLAGS);
        Reset();

        if (pCreature->GetAreaId() == AREA_COVE || pCreature->GetAreaId() == AREA_ISLE)
            m_bIsTreeEvent = true;
    }

    uint32 m_uiNpcFlags;
    uint32 m_uiNormFaction;

    uint32 m_uiDynamiteTimer;
    uint32 m_uiEmoteTimer;

    bool m_bIsTreeEvent;

    void Reset()
    {
        m_creature->setFaction(m_uiNormFaction);
        m_creature->SetUInt32Value(UNIT_NPC_FLAGS, m_uiNpcFlags);

        m_uiDynamiteTimer = 8000;
        m_uiEmoteTimer = urand(120000, 150000);

        m_bIsTreeEvent = false;
    }

    void Aggro(Unit *who)
    {
        DoScriptText(SAY_ATTACK, m_creature, who);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->isInCombat() && !m_bIsTreeEvent)
        {
            if (m_uiEmoteTimer < diff)
            {
                DoScriptText(SAY_TEXT, m_creature);
                DoScriptText(EMOTE_SHELL, m_creature);
                m_uiEmoteTimer = urand(120000, 150000);
            }
            else m_uiEmoteTimer -= diff;
        }
        else if (m_bIsTreeEvent)
        {
            //nothing here yet
            return;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiDynamiteTimer < diff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_DYNAMITE);
            m_uiDynamiteTimer = 8000;
        }
        else m_uiDynamiteTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_engineer_spark_overgrind(Creature* pCreature)
{
    return new npc_engineer_spark_overgrindAI(pCreature);
}

bool GossipHello_npc_engineer_spark_overgrind(Player* pPlayer, Creature* pCreature)
{
    if (pPlayer->GetQuestStatus(QUEST_GNOMERCY) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_FIGHT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

    pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
    return true;
}

bool GossipSelect_npc_engineer_spark_overgrind(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
{
    if (uiAction == GOSSIP_ACTION_INFO_DEF)
    {
        pPlayer->CLOSE_GOSSIP_MENU();
        pCreature->setFaction(FACTION_HOSTILE);
        pCreature->AI()->AttackStart(pPlayer);
    }
    return true;
}

/*######
## npc_injured_draenei
######*/

struct MANGOS_DLL_DECL npc_injured_draeneiAI : public ScriptedAI
{
    npc_injured_draeneiAI(Creature* pCreature) : ScriptedAI(pCreature) {Reset();}

    void Reset()
    {
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
        m_creature->SetHealth(int(m_creature->GetMaxHealth()*.15));
        switch(urand(0, 1))
        {
            case 0: m_creature->SetStandState(UNIT_STAND_STATE_SIT); break;
            case 1: m_creature->SetStandState(UNIT_STAND_STATE_SLEEP); break;
        }
    }

    void MoveInLineOfSight(Unit *who)
    {
        return;                                             //ignore everyone around them (won't aggro anything)
    }

    void UpdateAI(const uint32 diff)
    {
        return;
    }

};
CreatureAI* GetAI_npc_injured_draenei(Creature* pCreature)
{
    return new npc_injured_draeneiAI(pCreature);
}

/*######
## npc_magwin
######*/

#define SAY_START               -1000111
#define SAY_AGGRO               -1000112
#define SAY_PROGRESS            -1000113
#define SAY_END1                -1000114
#define SAY_END2                -1000115
#define EMOTE_HUG               -1000116

#define QUEST_A_CRY_FOR_HELP    9528

struct MANGOS_DLL_DECL npc_magwinAI : public npc_escortAI
{
    npc_magwinAI(Creature* pCreature) : npc_escortAI(pCreature) {Reset();}

    void WaypointReached(uint32 i)
    {
        Player* pPlayer = GetPlayerForEscort();

        if (!pPlayer)
            return;

        switch(i)
        {
            case 0:
                DoScriptText(SAY_START, m_creature, pPlayer);
                break;
            case 17:
                DoScriptText(SAY_PROGRESS, m_creature, pPlayer);
                break;
            case 28:
                DoScriptText(SAY_END1, m_creature, pPlayer);
                break;
            case 29:
                DoScriptText(EMOTE_HUG, m_creature, pPlayer);
                DoScriptText(SAY_END2, m_creature, pPlayer);
                pPlayer->GroupEventHappens(QUEST_A_CRY_FOR_HELP, m_creature);
                break;
        }
    }

    void Aggro(Unit* who)
    {
        DoScriptText(SAY_AGGRO, m_creature, who);
    }

    void Reset() { }
};

bool QuestAccept_npc_magwin(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_A_CRY_FOR_HELP)
    {
        pCreature->setFaction(10);

        if (npc_magwinAI* pEscortAI = dynamic_cast<npc_magwinAI*>(pCreature->AI()))
            pEscortAI->Start(false, pPlayer->GetGUID(), pQuest);
    }
    return true;
}

CreatureAI* GetAI_npc_magwinAI(Creature* pCreature)
{
    return new npc_magwinAI(pCreature);
}

/*######
## npc_susurrus
######*/

enum
{
    ITEM_WHORL_OF_AIR       = 23843,
    SPELL_BUFFETING_WINDS   = 32474,
    TAXI_PATH_ID            = 506
};

#define GOSSIP_ITEM_READY   "I am ready."

bool GossipHello_npc_susurrus(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetGUID());

    if (pPlayer->HasItemCount(ITEM_WHORL_OF_AIR,1,true))
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_READY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

    pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_susurrus(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
{
    if (uiAction == GOSSIP_ACTION_INFO_DEF)
    {
        //spellId is correct, however it gives flight a somewhat funny effect
        pPlayer->CLOSE_GOSSIP_MENU();
        pPlayer->CastSpell(pPlayer,SPELL_BUFFETING_WINDS,true);
    }
    return true;
}

/*######
## npc_ancestor_akida
######*/

enum
{
	QUEST_TOTEM_OF_COO			=	9539,
	NPC_ANCESTOR_AKIDA			=	17379,
	AKIDA_SAY_START				=	-1050010,

	SPELL_APPEAR_ANCESTOR		=	25035,
	SPELL_DISAPPEAR_ANCESTOR	=	30428
};

struct MANGOS_DLL_DECL npc_ancestor_akidaAI : public npc_escortAI
{
	npc_ancestor_akidaAI(Creature* pCreature) : npc_escortAI(pCreature) {Reset();}

	void Reset(){}

	void WaypointReached(uint32 i)
    {
        Player* pPlayer = GetPlayerForEscort();
		m_creature->SetSpeedRate(MOVE_WALK, 1.2f, true);

		if (!pPlayer)
            return;

		if(!(pPlayer->GetQuestStatus(QUEST_TOTEM_OF_COO) == QUEST_STATUS_INCOMPLETE))
		{
			m_creature->StopMoving();
			m_creature->SetDeathState(JUST_DIED);
			m_creature->RemoveCorpse();
			return;
		}

        switch(i)
        {
            case 1:
                DoScriptText(AKIDA_SAY_START, m_creature, pPlayer);
                break;
            case 10:
                pPlayer->GroupEventHappens(QUEST_TOTEM_OF_COO, m_creature);
				m_creature->CastSpell(m_creature,SPELL_DISAPPEAR_ANCESTOR,false);
				m_creature->SetDeathState(JUST_DIED);
				m_creature->RemoveCorpse();
                break;
        }
    }
};

CreatureAI* GetAI_npc_ancestor_akidaAI(Creature* pCreature)
{
    return new npc_ancestor_akidaAI(pCreature);
}

bool QuestAccept_npc_totem_of_akida(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_TOTEM_OF_COO)
    {
		Creature* npcAkida = pCreature->SummonCreature(NPC_ANCESTOR_AKIDA, -4173.75f, -12514.605f, 44.37066f, 0,  TEMPSUMMON_DEAD_DESPAWN, 0);
        if (npc_ancestor_akidaAI* pEscortAI = dynamic_cast<npc_ancestor_akidaAI*>(npcAkida->AI()))
		{
			npcAkida->CastSpell(npcAkida,SPELL_APPEAR_ANCESTOR,false);
            pEscortAI->Start(false, pPlayer->GetGUID(), pQuest);
		}
	}
    return true;
}

/*######
## npc_ancestor_coo
######*/

enum
{
	QUEST_TOTEM_OF_TIKTI	=	9540,
	NPC_ANCESTOR_COO		=	17391,
	SPELL_GHOST_WALK		=	30424,

	COO_SAY_START			=	-1050011,
	COO_SAY_SAY1			=	-1050012,
	COO_SAY_SAY2			=	-1050013
};

struct MANGOS_DLL_DECL npc_ancestor_cooAI : public npc_escortAI
{
	npc_ancestor_cooAI(Creature* pCreature) : npc_escortAI(pCreature) {Reset();}

	bool bReachedTarget;
	uint32 SayTimer;
	uint32 Step;

	void Reset()
	{
		Step = 0;
		bReachedTarget = false;
	}

	void WaypointReached(uint32 i)
    {
        Player* pPlayer = GetPlayerForEscort();
		m_creature->SetSpeedRate(MOVE_WALK, 1.2f, true);

		if (!pPlayer)
            return;

		if(!(pPlayer->GetQuestStatus(QUEST_TOTEM_OF_TIKTI) == QUEST_STATUS_INCOMPLETE))
		{
			m_creature->StopMoving();
			m_creature->SetDeathState(JUST_DIED);
			m_creature->RemoveCorpse();
			return;
		}

        switch(i)
        {
            case 1:
                DoScriptText(COO_SAY_START, m_creature, pPlayer);
                break;
            case 2:
				SayTimer = 1000;
				bReachedTarget = true;
				m_creature->StopMoving();
                break;
        }
    }

	void UpdateAI(const uint32 diff)
	{
		Player* pPlayer = GetPlayerForEscort();

		if(bReachedTarget)
		{
			if (SayTimer <= diff)
			{
		        switch(Step)
		        {
					case 0:
						DoScriptText(COO_SAY_SAY1, m_creature, pPlayer);
						SayTimer = 4000;
						Step++;
						break;
					case 1:
						DoScriptText(COO_SAY_SAY2, m_creature, pPlayer);
						SayTimer = 4000;
						Step++;
						break;
					case 2:
						if (pPlayer)
							pPlayer->GroupEventHappens(QUEST_TOTEM_OF_TIKTI, m_creature);
						m_creature->CastSpell(pPlayer,SPELL_GHOST_WALK,false);
						SayTimer = 3000;
						Step++;
						break;
					case 3:
						m_creature->CastSpell(m_creature,SPELL_DISAPPEAR_ANCESTOR,false);
						SayTimer = 3000;
						Step++;
						break;
					case 4:
						m_creature->SetDeathState(JUST_DIED);
						m_creature->RemoveCorpse();
						break;
				}
			} else SayTimer -= diff;
		}else
			npc_escortAI::UpdateAI(diff);
	}
};

CreatureAI* GetAI_npc_ancestor_cooAI(Creature* pCreature)
{
    return new npc_ancestor_cooAI(pCreature);
}

bool QuestAccept_npc_totem_of_coo(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_TOTEM_OF_TIKTI)
    {
		Creature* npcCoo = pCreature->SummonCreature(NPC_ANCESTOR_COO, -3926.974f, -12752.2852f, 97.6727f, 0,  TEMPSUMMON_DEAD_DESPAWN, 0);
		if (npc_ancestor_cooAI* pEscortAI = dynamic_cast<npc_ancestor_cooAI*>(npcCoo->AI()))
		{
			npcCoo->CastSpell(npcCoo,SPELL_APPEAR_ANCESTOR,false);
            pEscortAI->Start(false, pPlayer->GetGUID(), pQuest);
		}
	}
    return true;
}

/*######
## npc_ancestor_tikti
######*/

enum
{
	QUEST_TOTEM_OF_YOR		=	9541,
	NPC_ANCESTOR_TIKTI		=	17392,
	SPELL_TRANSFORM_TIKTI	=	30431,
	SPELL_EMBRACE_SERPENT	=	30430,

	TIKTI_SAY_START			=	-1050014,
	TIKTI_SAY_SAY1			=	-1050015,
	TIKTI_SAY_SAY2			=	-1050016

};

struct MANGOS_DLL_DECL npc_ancestor_tiktiAI : public npc_escortAI
{
	npc_ancestor_tiktiAI(Creature* pCreature) : npc_escortAI(pCreature) {Reset();}

	bool bReachedTarget;
	uint32 SayTimer;
	uint32 Step;

	void Reset()
	{
		Step = 0;
		bReachedTarget = false;
		SayTimer = 0;
	}

	void WaypointReached(uint32 i)
    {
        Player* pPlayer = GetPlayerForEscort();
		m_creature->SetSpeedRate(MOVE_WALK, 1.2f, true);

		if (!pPlayer)
            return;

		if(!(pPlayer->GetQuestStatus(QUEST_TOTEM_OF_YOR) == QUEST_STATUS_INCOMPLETE))
		{
			m_creature->StopMoving();
			m_creature->SetDeathState(JUST_DIED);
			m_creature->RemoveCorpse();
			return;
		}

        switch(i)
        {
            case 1:
                DoScriptText(TIKTI_SAY_START, m_creature, pPlayer);
                break;
            case 2:
				SayTimer = 1000;
				bReachedTarget = true;
				//m_creature->StopMoving();
				break;
			case 3:
				m_creature->SetDeathState(JUST_DIED);
				m_creature->RemoveCorpse();
				break;
        }
    }

	void UpdateAI(const uint32 diff)
	{
		Player* pPlayer = GetPlayerForEscort();

		if(bReachedTarget)
		{
			if (SayTimer <= diff)
			{
		        switch(Step)
		        {
					case 0:
						DoScriptText(TIKTI_SAY_SAY1, m_creature, pPlayer);
						SayTimer = 4000;
						Step++;
						break;
					case 1:
						m_creature->CastSpell(pPlayer,SPELL_EMBRACE_SERPENT,false);
						DoScriptText(TIKTI_SAY_SAY2, m_creature, pPlayer);
						SayTimer = 4000;
						Step++;
						break;
					case 2:
						if (pPlayer)
							pPlayer->GroupEventHappens(QUEST_TOTEM_OF_YOR, m_creature);
						SayTimer = 3000;
						Step++;
						break;
					case 3:
						m_creature->CastSpell(m_creature,SPELL_TRANSFORM_TIKTI,false);
						SayTimer = 3000;
						Step++;
						break;
					case 4:
						//m_creature->setDeathState(JUST_DIED);
						//m_creature->RemoveCorpse();
						bReachedTarget = false;
						break;
				}
			} else SayTimer -= diff;
		}else
			npc_escortAI::UpdateAI(diff);
	}
};

CreatureAI* GetAI_npc_ancestor_tiktiAI(Creature* pCreature)
{
    return new npc_ancestor_tiktiAI(pCreature);
}

bool QuestAccept_npc_totem_of_tikti(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_TOTEM_OF_YOR)
    {
		Creature* npcTikti = pCreature->SummonCreature(NPC_ANCESTOR_TIKTI, -3875.43f, -13125.012f, 6.82215f, 0,  TEMPSUMMON_DEAD_DESPAWN, 0);
        if (npc_ancestor_tiktiAI* pEscortAI = dynamic_cast<npc_ancestor_tiktiAI*>(npcTikti->AI()))
		{
			npcTikti->CastSpell(npcTikti,SPELL_APPEAR_ANCESTOR,false);
            pEscortAI->Start(false, pPlayer->GetGUID(), pQuest);
		}
	}
    return true;
}

/*######
## npc_ancestor_yor
######*/

enum
{
	QUEST_TOTEM_OF_VARK			=	9542,
	NPC_ANCESTOR_YOR			=	17393,

	SPELL_TRANSFORM_YOR			=	30446,
	SPELL_SHADOW_FOREST			=	30448,


	YOR_SAY_START				=	-1050017,
	YOR_SAY_SAY1				=	-1050018,
	YOR_SAY_SAY2				=	-1050019,

};

struct MANGOS_DLL_DECL npc_ancestor_yorAI : public npc_escortAI
{
	npc_ancestor_yorAI(Creature* pCreature) : npc_escortAI(pCreature) {Reset();}

	bool bReachedTarget;
	uint32 SayTimer;
	uint32 Step;

	void Reset()
	{
		Step = 0;
		bReachedTarget = false;
		SayTimer = 0;
	}

	void WaypointReached(uint32 i)
    {
        Player* pPlayer = GetPlayerForEscort();
		m_creature->SetSpeedRate(MOVE_WALK, 1.2f, true);

		if (!pPlayer)
            return;

		if(!(pPlayer->GetQuestStatus(QUEST_TOTEM_OF_VARK) == QUEST_STATUS_INCOMPLETE))
		{
			m_creature->StopMoving();
			m_creature->SetDeathState(JUST_DIED);
			m_creature->RemoveCorpse();
			return;
		}

        switch(i)
        {
			case 1:
				SayTimer = 500;
				bReachedTarget = true;
				break;
            case 24:
                pPlayer->GroupEventHappens(QUEST_TOTEM_OF_VARK, m_creature);
				m_creature->CastSpell(m_creature,SPELL_DISAPPEAR_ANCESTOR,false);
				m_creature->SetDeathState(JUST_DIED);
				m_creature->RemoveCorpse();
                break;
        }
    }

	void UpdateAI(const uint32 diff)
	{
		Player* pPlayer = GetPlayerForEscort();

		if(bReachedTarget)
		{
			if (SayTimer <= diff)
			{
		        switch(Step)
		        {
					case 0:
						m_creature->CastSpell(m_creature,SPELL_TRANSFORM_YOR,false);
						SayTimer = 4000;
						Step++;
						break;
					case 1:
						DoScriptText(YOR_SAY_SAY1, m_creature, pPlayer);
						SayTimer = 4000;
						Step++;
						break;
					case 2:
						m_creature->CastSpell(m_creature,SPELL_SHADOW_FOREST,false);
						SayTimer = 3000;
						Step++;
						break;
					case 3:
						m_creature->CastSpell(pPlayer,SPELL_SHADOW_FOREST,false);
						SayTimer = 3000;
						Step++;
						break;
					case 4:
						DoScriptText(YOR_SAY_SAY2, m_creature, pPlayer);
						SayTimer = 3000;
						Step++;
						break;
					case 5:
						npc_escortAI::SetRun(true);
						bReachedTarget = false;
						break;
				}
			} else SayTimer -= diff;
		}else
			npc_escortAI::UpdateAI(diff);
	}
};

CreatureAI* GetAI_npc_ancestor_yorAI(Creature* pCreature)
{
    return new npc_ancestor_yorAI(pCreature);
}

bool QuestAccept_npc_totem_of_yor(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_TOTEM_OF_VARK)
    {
		Creature* npcYor = pCreature->SummonCreature(NPC_ANCESTOR_YOR, -4634.25f, -13071.687f, -14.75535f, 0,  TEMPSUMMON_DEAD_DESPAWN, 0);
        if (npc_ancestor_yorAI* pEscortAI = dynamic_cast<npc_ancestor_yorAI*>(npcYor->AI()))
		{
			npcYor->CastSpell(npcYor,SPELL_APPEAR_ANCESTOR,false);
			DoScriptText(YOR_SAY_START, npcYor, pPlayer);
            pEscortAI->Start(false, pPlayer->GetGUID(), pQuest);
		}
	}
    return true;
}

void AddSC_azuremyst_isle()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "npc_draenei_survivor";
    newscript->GetAI = &GetAI_npc_draenei_survivor;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_engineer_spark_overgrind";
    newscript->GetAI = &GetAI_npc_engineer_spark_overgrind;
    newscript->pGossipHello =  &GossipHello_npc_engineer_spark_overgrind;
    newscript->pGossipSelect = &GossipSelect_npc_engineer_spark_overgrind;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_injured_draenei";
    newscript->GetAI = &GetAI_npc_injured_draenei;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_magwin";
    newscript->GetAI = &GetAI_npc_magwinAI;
    newscript->pQuestAccept = &QuestAccept_npc_magwin;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_susurrus";
    newscript->pGossipHello =  &GossipHello_npc_susurrus;
    newscript->pGossipSelect = &GossipSelect_npc_susurrus;
    newscript->RegisterSelf();

	newscript = new Script;
	newscript->Name= "npc_ancestor_akida";
	newscript->GetAI = &GetAI_npc_ancestor_akidaAI;
	newscript->RegisterSelf();

	newscript = new Script;
	newscript->Name= "npc_totem_of_akida";
	newscript->pQuestAccept = &QuestAccept_npc_totem_of_akida;
	newscript->RegisterSelf();

	newscript = new Script;
	newscript->Name= "npc_ancestor_coo";
	newscript->GetAI = &GetAI_npc_ancestor_cooAI;
	newscript->RegisterSelf();

	newscript = new Script;
	newscript->Name= "npc_totem_of_coo";
	newscript->pQuestAccept = &QuestAccept_npc_totem_of_coo;
	newscript->RegisterSelf();

	newscript = new Script;
	newscript->Name= "npc_ancestor_tikti";
	newscript->GetAI = &GetAI_npc_ancestor_tiktiAI;
	newscript->RegisterSelf();

	newscript = new Script;
	newscript->Name= "npc_totem_of_tikti";
	newscript->pQuestAccept = &QuestAccept_npc_totem_of_tikti;
	newscript->RegisterSelf();

	newscript = new Script;
	newscript->Name= "npc_ancestor_yor";
	newscript->GetAI = &GetAI_npc_ancestor_yorAI;
	newscript->RegisterSelf();

	newscript = new Script;
	newscript->Name= "npc_totem_of_yor";
	newscript->pQuestAccept = &QuestAccept_npc_totem_of_yor;
	newscript->RegisterSelf();
}
