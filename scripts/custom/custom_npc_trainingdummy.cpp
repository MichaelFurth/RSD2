#include "precompiled.h"
#define OUT_OF_COMBAT_TIME 5000

struct MANGOS_DLL_DECL custom_npc_trainingdummyAI : public Scripted_NoMovementAI
{
    uint32 combat_timer;

    custom_npc_trainingdummyAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
    {
        Reset();
    }

    void Reset()
    {
        combat_timer = 0;
    }

    void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
    {
        combat_timer = 0;
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        m_creature->ModifyHealth(m_creature->GetMaxHealth());

        combat_timer += diff;
        if (combat_timer > OUT_OF_COMBAT_TIME)
            EnterEvadeMode();
    }
};

CreatureAI* GetAI_custom_npc_trainingdummy(Creature* pCreature)
{
    return new custom_npc_trainingdummyAI(pCreature);
}

void AddSC_custom_npc_trainingdummy()
{
    Script *newscript = new Script;
    newscript->Name = "custom_npc_trainingdummy";
    newscript->GetAI = &GetAI_custom_npc_trainingdummy;
    newscript->RegisterSelf();
}