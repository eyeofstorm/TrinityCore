/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
SDName: instance_stratholme
SD%Complete: 50
SDComment: In progress. Undead side 75% implemented. Save/load not implemented.
SDCategory: Stratholme
EndScriptData */

#include "ScriptMgr.h"
#include "AreaBoundary.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "EventMap.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Player.h"
#include "stratholme.h"

enum InstanceEvents
{
    EVENT_BARON_RUN         = 1,
    EVENT_SLAUGHTER_SQUARE  = 2
};

enum StratholmeMisc
{
    SAY_YSIDA_SAVED         = 0
};

Position const timmyTheCruelSpawnPosition = { 3625.358f, -3188.108f, 130.3985f, 4.834562f };
EllipseBoundary const beforeScarletGate(Position(3671.158f, -3181.79f), 60.0f, 40.0f);

static constexpr DungeonEncounterData Encounters[] =
{
    { BOSS_HEARTHSINGER_FORRESTEN, {{ 473 }} }, // Hearthsinger Forresten
    { BOSS_TIMMY_THE_CRUEL, {{ 474 }} }, // Timmy the Cruel
    { BOSS_COMMANDER_MALOR, {{ 476 }} }, // Commander Malor
    { BOSS_WILLEY_HOPEBREAKER, {{ 475 }} }, // Willey Hopebreaker
    { BOSS_INSTRUCTOR_GALFORD, {{ 477 }} }, // Instructor Galford
    { BOSS_BALNAZZAR, {{ 478 }} }, // Balnazzar
    { BOSS_THE_UNFORGIVEN, {{ 472 }} }, // The Unforgiven
    { BOSS_BARONESS_ANASTARI, {{ 479 }} }, // Baroness Anastari
    { BOSS_NERUB_ENKAN, {{ 480 }} }, // Nerub'enkan
    { BOSS_MALEKI_THE_PALLID, {{ 481 }} }, // Maleki the Pallid
    { BOSS_MAGISTRATE_BARTHILAS, {{ 482 }} }, // Magistrate Barthilas
    { BOSS_RAMSTEIN_THE_GORGER, {{ 483 }} }, // Ramstein the Gorger
    { BOSS_RIVENDARE, {{ 484 }} }, // Lord Aurius Rivendare
    { BOSS_POSTMASTER_MALOWN, {{ 1885 }} } // Postmaster Malown
};

class instance_stratholme : public InstanceMapScript
{
    public:
        instance_stratholme() : InstanceMapScript(StratholmeScriptName, 329) { }

        struct instance_stratholme_InstanceMapScript : public InstanceScript
        {
            instance_stratholme_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(MAX_ENCOUNTER);
                LoadDungeonEncounterData(Encounters);

                for (uint8 i = 0; i < 5; ++i)
                    IsSilverHandDead[i] = false;

                timmySpawned = false;
                scarletsKilled = 0;
                brokenCrystals = 0;
                baronRunState = NOT_STARTED;
            }

            uint8 scarletsKilled;
            int32 brokenCrystals;
            EncounterState baronRunState;

            bool IsSilverHandDead[5];
            bool timmySpawned;

            ObjectGuid serviceEntranceGUID;
            ObjectGuid gauntletGate1GUID;
            ObjectGuid ziggurat1GUID;
            ObjectGuid ziggurat2GUID;
            ObjectGuid ziggurat3GUID;
            ObjectGuid ziggurat4GUID;
            ObjectGuid ziggurat5GUID;
            ObjectGuid portGauntletGUID;
            ObjectGuid portSlaugtherGUID;
            ObjectGuid portElderGUID;
            ObjectGuid ysidaCageGUID;

            ObjectGuid baronGUID;
            ObjectGuid ysidaGUID;
            ObjectGuid ysidaTriggerGUID;
            GuidSet crystalsGUID;
            GuidSet abomnationGUID;
            EventMap events;

            void OnUnitDeath(Unit* who) override
            {
                switch (who->GetEntry())
                {
                    case NPC_CRIMSON_GUARDSMAN:
                    case NPC_CRIMSON_CONJUROR:
                    case NPC_CRIMSON_INITATE:
                    case NPC_CRIMSON_GALLANT:
                    {
                        if (!timmySpawned)
                        {
                            Position pos = who->ToCreature()->GetHomePosition();
                            // check if they're in front of the entrance
                            if (beforeScarletGate.IsWithinBoundary(pos))
                            {
                                if (++scarletsKilled >= TIMMY_THE_CRUEL_CRUSADERS_REQUIRED)
                                {
                                    instance->SummonCreature(NPC_TIMMY_THE_CRUEL, timmyTheCruelSpawnPosition);
                                    timmySpawned = true;
                                }
                            }
                        }
                        break;
                    }
                    case NPC_HEARTHSINGER_FORRESTEN:
                        SetBossState(BOSS_HEARTHSINGER_FORRESTEN, DONE);
                        break;
                    case NPC_COMMANDER_MALOR:
                        SetBossState(BOSS_COMMANDER_MALOR, DONE);
                        break;
                    case NPC_INSTRUCTOR_GALFORD:
                        SetBossState(BOSS_INSTRUCTOR_GALFORD, DONE);
                        break;
                    case NPC_THE_UNFORGIVEN:
                        SetBossState(BOSS_THE_UNFORGIVEN, DONE);
                        break;
                    default:
                        break;
                }
            }

            bool StartSlaugtherSquare()
            {
                if (brokenCrystals >= 3)
                {
                    HandleGameObject(portGauntletGUID, true);
                    HandleGameObject(portSlaugtherGUID, true);
                    return true;
                }

                TC_LOG_DEBUG("scripts", "Instance Stratholme: Cannot open slaugther square yet.");
                return false;
            }

            //if withRestoreTime true, then newState will be ignored and GO should be restored to original state after 10 seconds
            void UpdateGoState(ObjectGuid goGuid, uint32 newState, bool withRestoreTime)
            {
                if (!goGuid)
                    return;

                if (GameObject* go = instance->GetGameObject(goGuid))
                {
                    if (withRestoreTime)
                        go->UseDoorOrButton(10);
                    else
                        go->SetGoState((GOState)newState);
                }
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_BARON:
                        baronGUID = creature->GetGUID();
                        break;
                    case NPC_YSIDA_TRIGGER:
                        ysidaTriggerGUID = creature->GetGUID();
                        break;
                    case NPC_CRYSTAL:
                        crystalsGUID.insert(creature->GetGUID());
                        break;
                    case NPC_ABOM_BILE:
                    case NPC_ABOM_VENOM:
                        abomnationGUID.insert(creature->GetGUID());
                        break;
                    case NPC_YSIDA:
                        ysidaGUID = creature->GetGUID();
                        creature->RemoveNpcFlag(UNIT_NPC_FLAG_QUESTGIVER);
                        break;
                }
            }

            void OnCreatureRemove(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_CRYSTAL:
                        crystalsGUID.erase(creature->GetGUID());
                        break;
                    case NPC_ABOM_BILE:
                    case NPC_ABOM_VENOM:
                        abomnationGUID.erase(creature->GetGUID());
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* go) override
            {
                switch (go->GetEntry())
                {
                    case GO_SERVICE_ENTRANCE:
                        serviceEntranceGUID = go->GetGUID();
                        break;
                    case GO_GAUNTLET_GATE1:
                        //weird, but unless flag is set, client will not respond as expected. DB bug?
                        go->SetFlag(GO_FLAG_LOCKED);
                        gauntletGate1GUID = go->GetGUID();
                        break;
                    case GO_ZIGGURAT1:
                        ziggurat1GUID = go->GetGUID();
                        if (GetBossState(BOSS_BARONESS_ANASTARI) == DONE)
                            HandleGameObject(ObjectGuid::Empty, true, go);
                        break;
                    case GO_ZIGGURAT2:
                        ziggurat2GUID = go->GetGUID();
                        if (GetBossState(BOSS_NERUB_ENKAN) == DONE)
                            HandleGameObject(ObjectGuid::Empty, true, go);
                        break;
                    case GO_ZIGGURAT3:
                        ziggurat3GUID = go->GetGUID();
                        if (GetBossState(BOSS_MALEKI_THE_PALLID) == DONE)
                            HandleGameObject(ObjectGuid::Empty, true, go);
                        break;
                    case GO_ZIGGURAT4:
                        ziggurat4GUID = go->GetGUID();
                        if (GetBossState(BOSS_RIVENDARE) == DONE || GetBossState(BOSS_RAMSTEIN_THE_GORGER) == DONE)
                            HandleGameObject(ObjectGuid::Empty, true, go);
                        break;
                    case GO_ZIGGURAT5:
                        ziggurat5GUID = go->GetGUID();
                        if (GetBossState(BOSS_RIVENDARE) == DONE || GetBossState(BOSS_RAMSTEIN_THE_GORGER) == DONE)
                            HandleGameObject(ObjectGuid::Empty, true, go);
                        break;
                    case GO_PORT_GAUNTLET:
                        portGauntletGUID = go->GetGUID();
                        if (brokenCrystals >= 3)
                            HandleGameObject(ObjectGuid::Empty, true, go);
                        break;
                    case GO_PORT_SLAUGTHER:
                        portSlaugtherGUID = go->GetGUID();
                        if (brokenCrystals >= 3)
                            HandleGameObject(ObjectGuid::Empty, true, go);
                        break;
                    case GO_PORT_ELDERS:
                        portElderGUID = go->GetGUID();
                        break;
                    case GO_YSIDA_CAGE:
                        ysidaCageGUID = go->GetGUID();
                        break;
                }
            }

            bool SetBossState(uint32 id, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(id, state))
                    return false;

                switch (id)
                {
                    case BOSS_BARONESS_ANASTARI:
                        if (state == DONE)
                        {
                            HandleGameObject(ziggurat1GUID, true);

                            //remove when crystals implemented
                            ++brokenCrystals;
                            StartSlaugtherSquare();
                        }
                        break;
                    case BOSS_NERUB_ENKAN:
                        if (state == DONE)
                        {
                            HandleGameObject(ziggurat2GUID, true);

                            //remove when crystals implemented
                            ++brokenCrystals;
                            StartSlaugtherSquare();
                        }
                        break;
                    case BOSS_MALEKI_THE_PALLID:
                        if (state == DONE)
                        {
                            HandleGameObject(ziggurat3GUID, true);

                            //remove when crystals implemented
                            ++brokenCrystals;
                            StartSlaugtherSquare();
                        }
                        break;
                    case BOSS_RAMSTEIN_THE_GORGER:
                        if (state == IN_PROGRESS)
                        {
                            HandleGameObject(portGauntletGUID, false);

                            uint32 count = abomnationGUID.size();
                            for (GuidSet::const_iterator i = abomnationGUID.begin(); i != abomnationGUID.end(); ++i)
                            {
                                if (Creature* pAbom = instance->GetCreature(*i))
                                    if (!pAbom->IsAlive())
                                        --count;
                            }

                            if (!count)
                            {
                                //a bit itchy, it should close the door after 10 secs, but it doesn't. skipping it for now.
                                //UpdateGoState(ziggurat4GUID, 0, true);
                                if (Creature* pBaron = instance->GetCreature(baronGUID))
                                    pBaron->SummonCreature(NPC_RAMSTEIN, 4032.84f, -3390.24f, 119.73f, 4.71f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 30min);
                                TC_LOG_DEBUG("scripts", "Instance Stratholme: Ramstein spawned.");
                            }
                            else
                                TC_LOG_DEBUG("scripts", "Instance Stratholme: {} Abomnation left to kill.", count);
                        }

                        if (state == NOT_STARTED)
                            HandleGameObject(portGauntletGUID, true);

                        if (state == DONE)
                        {
                            events.ScheduleEvent(EVENT_SLAUGHTER_SQUARE, 1min);
                            TC_LOG_DEBUG("scripts", "Instance Stratholme: Slaugther event will continue in 1 minute.");
                        }
                        break;
                    case BOSS_RIVENDARE:
                        HandleGameObject(ziggurat4GUID, GetBossState(BOSS_RAMSTEIN_THE_GORGER) == DONE && state != IN_PROGRESS);
                        HandleGameObject(ziggurat5GUID, GetBossState(BOSS_RAMSTEIN_THE_GORGER) == DONE && state != IN_PROGRESS);
                        if (state == DONE)
                        {
                            HandleGameObject(portGauntletGUID, true);
                            if (GetData(TYPE_BARON_RUN) == IN_PROGRESS)
                                DoRemoveAurasDueToSpellOnPlayers(SPELL_BARON_ULTIMATUM);

                            SetData(TYPE_BARON_RUN, DONE);
                        }
                        break;
                    default:
                        break;
                }

                return true;
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case TYPE_BARON_RUN:
                        switch (data)
                        {
                            case IN_PROGRESS:
                                if (baronRunState == IN_PROGRESS || baronRunState == FAIL)
                                    break;
                                baronRunState = IN_PROGRESS;
                                events.ScheduleEvent(EVENT_BARON_RUN, 45min);
                                TC_LOG_DEBUG("scripts", "Instance Stratholme: Baron run in progress.");
                                break;
                            case FAIL:
                                DoRemoveAurasDueToSpellOnPlayers(SPELL_BARON_ULTIMATUM);
                                if (Creature* ysida = instance->GetCreature(ysidaGUID))
                                    ysida->CastSpell(ysida, SPELL_PERM_FEIGN_DEATH, true);
                                baronRunState = FAIL;
                                break;
                            case DONE:
                                baronRunState = DONE;

                                if (Creature* ysida = instance->GetCreature(ysidaGUID))
                                {
                                    if (GameObject* cage = instance->GetGameObject(ysidaCageGUID))
                                        cage->UseDoorOrButton();

                                    float x, y, z;
                                    //! This spell handles the Dead man's plea quest completion
                                    ysida->CastSpell(nullptr, SPELL_YSIDA_SAVED, true);
                                    ysida->SetWalk(true);
                                    ysida->AI()->Talk(SAY_YSIDA_SAVED);
                                    ysida->SetNpcFlag(UNIT_NPC_FLAG_QUESTGIVER);
                                    ysida->GetClosePoint(x, y, z, ysida->GetObjectScale() / 3, 4.0f);
                                    ysida->GetMotionMaster()->MovePoint(1, x, y, z);

                                    Map::PlayerList const& players = instance->GetPlayers();

                                    for (auto const& i : players)
                                    {
                                        if (Player* player = i.GetSource())
                                        {
                                            if (player->IsGameMaster())
                                                continue;

                                            //! im not quite sure what this one is supposed to do
                                            //! this is server-side spell
                                            player->CastSpell(ysida, SPELL_YSIDA_CREDIT_EFFECT, true);
                                        }
                                    }
                                }
                                events.CancelEvent(EVENT_BARON_RUN);
                                break;
                        }
                        break;
                    case TYPE_SH_AELMAR:
                        IsSilverHandDead[0] = (data) ? true : false;
                        break;
                    case TYPE_SH_CATHELA:
                        IsSilverHandDead[1] = (data) ? true : false;
                        break;
                    case TYPE_SH_GREGOR:
                        IsSilverHandDead[2] = (data) ? true : false;
                        break;
                    case TYPE_SH_NEMAS:
                        IsSilverHandDead[3] = (data) ? true : false;
                        break;
                    case TYPE_SH_VICAR:
                        IsSilverHandDead[4] = (data) ? true : false;
                        break;
                    default:
                        break;
                }
            }

            uint32 GetData(uint32 type) const override
            {
                  switch (type)
                  {
                      case TYPE_SH_QUEST:
                          if (IsSilverHandDead[0] && IsSilverHandDead[1] && IsSilverHandDead[2] && IsSilverHandDead[3] && IsSilverHandDead[4])
                              return 1;
                          return 0;
                      case TYPE_BARON_RUN:
                          return baronRunState;
                      case TYPE_BARONESS:
                          return GetBossState(BOSS_BARONESS_ANASTARI);
                      case TYPE_NERUB:
                          return GetBossState(BOSS_NERUB_ENKAN);
                      case TYPE_PALLID:
                          return GetBossState(BOSS_MALEKI_THE_PALLID);
                      case TYPE_RAMSTEIN:
                          return GetBossState(BOSS_RAMSTEIN_THE_GORGER);
                      case TYPE_BARON:
                          return GetBossState(BOSS_RIVENDARE);
                      default:
                          break;
                  }
                  return 0;
            }

            ObjectGuid GetGuidData(uint32 data) const override
            {
                switch (data)
                {
                    case DATA_BARON:
                        return baronGUID;
                    case DATA_YSIDA_TRIGGER:
                        return ysidaTriggerGUID;
                    case NPC_YSIDA:
                        return ysidaGUID;
                    default:
                        break;
                }
                return ObjectGuid::Empty;
            }

            void Update(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BARON_RUN:
                            if (GetData(TYPE_BARON_RUN) != DONE)
                                SetData(TYPE_BARON_RUN, FAIL);
                            TC_LOG_DEBUG("scripts", "Instance Stratholme: Baron run event reached end. Event has state {}.", GetData(TYPE_BARON_RUN));
                            break;
                        case EVENT_SLAUGHTER_SQUARE:
                            if (Creature* baron = instance->GetCreature(baronGUID))
                            {
                                for (uint8 i = 0; i < 4; ++i)
                                    baron->SummonCreature(NPC_BLACK_GUARD, 4032.84f, -3390.24f, 119.73f, 4.71f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 30min);

                                HandleGameObject(ziggurat4GUID, true);
                                HandleGameObject(ziggurat5GUID, true);
                                TC_LOG_DEBUG("scripts", "Instance Stratholme: Black guard sentries spawned. Opening gates to baron.");
                            }
                            break;
                        default:
                            break;
                    }
                }
            }

            void AfterDataLoad() override
            {
                if (GetBossState(BOSS_BARONESS_ANASTARI) == DONE)
                    ++brokenCrystals;
                if (GetBossState(BOSS_NERUB_ENKAN) == DONE)
                    ++brokenCrystals;
                if (GetBossState(BOSS_MALEKI_THE_PALLID) == DONE)
                    ++brokenCrystals;

                baronRunState = FAIL;
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_stratholme_InstanceMapScript(map);
        }
};

void AddSC_instance_stratholme()
{
    new instance_stratholme();
}
