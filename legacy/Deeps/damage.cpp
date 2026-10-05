#include "Deeps.h"

/**
 * @brief Allows a plugin to attempt to handle an incoming packet.
 *
 * @param uiPacketId            The id of the packet.
 * @param uiPacketSize          The size of the packet.
 * @param lpRawData             The raw packet data.
 *
 * @return True on handled, false otherwise.
 *
 * @note    Returning true on this will block the packet from being handled! This can
 *          have undesired effects! Use with caution as this can get you banned!
 */
bool Deeps::HandleIncomingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked)
{
    for (std::list<void*>::iterator it = m_Packets.begin(); it != m_Packets.end(); it++)
    {
        if (memcmp(data, (*it), size) == 0)
        {
            return false;
        }
    }

    void* packet = malloc(1024);
    memset(packet, 0, 1024);
    memcpy(packet, data, size);
    m_Packets.push_back(packet);
    while (m_Packets.size() > 200)
    {
        free(*m_Packets.begin());
        m_Packets.pop_front();
    }

    entitysources_t* entityInfo = NULL;

    if (id == 0x28) //action
    {
        uint8_t targetNum  = Read8(data, 0x09);
        uint8_t actionType = (uint8_t)(Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, 82, 4));
        uint16_t actionID = (uint16_t)(Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, 86, 10));
        uint8_t actionNum  = (uint8_t)(Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, 182, 4));
        uint32_t userID   = Read32(data, 0x05);
        uint16_t startBit = 150;
        uint16_t damage   = 0;
        uint16_t index = GetIndexFromId(userID);

        if (userID == 0 || index == 0 || actionType == 0 || actionID == 0)
        {
            return false;
        }

        auto it = m_Entities.find(userID);

        if (it != m_Entities.end())
        {
            entityInfo = &it->second;
            // Check this entity for a pet and creates an entitysource_t for it if necessary
            uint16_t petIndex = m_AshitaCore->GetMemoryManager()->GetEntity()->GetPetTargetIndex(index);
            uint32_t petID = m_AshitaCore->GetMemoryManager()->GetEntity()->GetServerId(petIndex);

            if (petIndex > 0 && petID > 0)
            {
                entitysources_t newPetInfo;
                auto name = m_AshitaCore->GetMemoryManager()->GetEntity()->GetName(petIndex);
                newPetInfo.name        = name != nullptr ? name : "(Unknown)";
                newPetInfo.color       = RandomColors[rand() % RandomColors.size()];
                newPetInfo.id          = petID;
                newPetInfo.ownerid     = userID;
                m_Entities.insert(std::make_pair(petID, newPetInfo)).first->second;
            }
        }
        else // new entity being stored
        {
            // Ignoring NPCs
            if (userID > 0x1000000)
            {
                return false;
            }
            entitysources_t newInfo;
            auto name = m_AshitaCore->GetMemoryManager()->GetEntity()->GetName(index);
            newInfo.name        = name != nullptr ? name : "(Unknown)";
            newInfo.color       = RandomColors[rand() % RandomColors.size()];
            newInfo.id          = userID;
            newInfo.ownerid     = NULL;
            entityInfo          = &m_Entities.insert(std::make_pair(userID, newInfo)).first->second;

            if (m_Debug)
            {
                m_AshitaCore->GetChatManager()->Writef(-3, false, "Total entities: %d", m_Entities.size());
            }

        }

        if (entityInfo)
        {
            if (m_Debug)
            {
                m_AshitaCore->GetChatManager()->Writef(-3, false, "Action Type: %d Action ID: %d", actionType, actionID);
            }

            bool isPet = false;
            if (entityInfo->ownerid != NULL) // Only a pet entity should have data in this field
            {
                isPet = true;
            }

            // When the entity is a pet, swap the entityInfo with its owner to count its damage towards them.
            if (isPet)
            {
                // Checking/updating the owner of this pet regularly as the ID can end up on another player if two players resummon pets.
                auto petOwnerIndex = m_AshitaCore->GetMemoryManager()->GetEntity()->GetTrustOwnerTargetIndex(index);
                if (petOwnerIndex != 0)
                {
                    entityInfo->ownerid = m_AshitaCore->GetMemoryManager()->GetEntity()->GetServerId(petOwnerIndex);
                }
                // Swapping this pets entityInfo out for its owners to count the damage towards them instead.
                auto it = m_Entities.find(entityInfo->ownerid);
                if (it != m_Entities.end())
                {
                    entityInfo = &it->second;
                }
                else
                {
                    return false;
                }
            }

            if (IsParsedActionType(actionType))
            {
                if (actionID == 0)
                    return false;
                source_t* source   = GetDamageSource(entityInfo, actionType, actionID, isPet);

                uint32_t addEffectDamage = 0;
                uint8_t addEffectCount   = 0;
                uint16_t addMessageID    = 0;

                for (int i = 0; i < targetNum; i++)
                {
                    for (int j = 0; j < actionNum; j++)
                    {
                        // Unpacking an action packet
                        uint8_t reaction       = (uint8_t)(Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, startBit + 36, 5));
                        uint16_t animation     = (uint16_t)(Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, startBit + 41, 12));
                        uint8_t specEffect     = (uint8_t)(Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, startBit + 53, 7));
                        // uint8_t knockback      = (uint8_t)(Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, startBit + 60, 3));
                        uint32_t mainDamage    = (uint32_t)(Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, startBit + 63, 17));
                        uint16_t messageID     = (uint16_t)(Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, startBit + 80, 10));

                        uint8_t hasAdditionalEffect = Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, startBit + 121, 1) & 0x1;
                        uint8_t hasSpikesEffect = Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, startBit + 121, 1) & 0x1;

                        if (m_Debug)
                        {
                            m_AshitaCore->GetChatManager()->Writef(-3, false, "Reaction: %d Animation: %d", reaction, animation);
                            m_AshitaCore->GetChatManager()->Writef(-3, false, "SpecEffect: %d Param: %d", specEffect, mainDamage);
                        }

                        //Daken (ranged attack on attack)
                        if (actionType == ACTIONTYPE_MELEE && animation == 4)
                            source = GetDamageSource(entityInfo, actionType + 1, actionID, isPet);

                        if (!UpdateDamageSource(source, messageID, mainDamage))
                            return false;

                        // BEGIN additional effect and skillchain damage
                        if (hasAdditionalEffect && actionType != ACTIONTYPE_JA)
                        {
                            addEffectDamage = (uint16_t)(Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, startBit + 132, 16));
                            addMessageID = (uint16_t)(Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, startBit + 149, 10));
                            bool isSC = (addMessageID >= 288 && addMessageID <= 302); // 288-302 are skillchain messages

                            if (addMessageID == MSG_ADD_EFFECT_DMG || addMessageID == MSG_ADD_EFFECT_DMG2 || (isSC && m_CountSkillchains))
                            {

                                uint32_t key    = 0;
                                if (addMessageID == MSG_ADD_EFFECT_DMG || addMessageID == MSG_ADD_EFFECT_DMG2)
                                {
                                    key = 1 << 8; // additional effect key
                                }
                                else
                                {
                                    key = 2 << 8; // skillchain key
                                }

                                source_t* source;
                                auto sourcesIt = entityInfo->sources.find(key);
                                if (sourcesIt != entityInfo->sources.end())
                                {
                                    source = &sourcesIt->second;
                                }
                                else
                                {
                                    source_t newsource;
                                    if (key == 1 << 8)
                                    {
                                        newsource.name.append("Additional Effect");
                                    }
                                    else // key == 2 << 8
                                    {
                                        newsource.name.append("Skillchain");
                                    }

                                    sourcesIt = entityInfo->sources.insert(std::make_pair(key, newsource)).first;
                                    source    = &sourcesIt->second;

                                }
                                source->damage["Hit"].count += 1;
                                source->damage["Hit"].total += addEffectDamage;
                                source->damage["Hit"].min = (addEffectDamage < source->damage["Hit"].min ? addEffectDamage : source->damage["Hit"].min);
                                source->damage["Hit"].max = (addEffectDamage > source->damage["Hit"].max ? addEffectDamage : source->damage["Hit"].max);
                            }

                            startBit += 37;
                        }
                        // END additional effect and skillchain damage

                        startBit += 1;
                        if (hasSpikesEffect)
                        {
                            startBit += 34;
                        }
                        startBit += 86;
                    }
                    startBit += 36;
                }
            }
        }
    }
    return false;
}


// Returns true if given actionType matches one of the ones we're parsing
bool Deeps::IsParsedActionType(uint8_t actionType)
{
    return ((actionType == ACTIONTYPE_MELEE)            ||
            (actionType == ACTIONTYPE_RA_FINISH)        ||
            (actionType == ACTIONTYPE_WS_FINISH)        ||
            (actionType == ACTIONTYPE_CAST_FINISH)      ||
            (actionType == ACTIONTYPE_JA)               ||
            (actionType == ACTIONTYPE_NPC_TP_FINISH)    ||
            (actionType == ACTIONTYPE_AVATAR_BP_FINISH) ||
            (actionType == ACTIONTYPE_JA_DNC)           ||
            (actionType == ACTIONTYPE_JA_RUN));
}


uint16_t Deeps::GetIndexFromId(int id)
{
    auto entMgr = m_AshitaCore->GetMemoryManager()->GetEntity();
    for (int i = 0; i < 0x900; i++)
    {
        if (entMgr->GetServerId(i) == id)
            return i;
    }
    return 0;
}

source_t* Deeps::GetDamageSource(entitysources_t* entityInfo, uint8_t actionType, uint16_t actionID, bool isPet)
{
    uint32_t key;
    if (isPet) // All pet attacks are going into a "Pet" damage source
    {
        key = 0xBADC0DE;
    }
    else
    {
        key = (actionID << 8) + actionType;
    }
    auto sourcesIt = entityInfo->sources.find(key);

    source_t* source;

    if (sourcesIt != entityInfo->sources.end())
    {
        source = &sourcesIt->second;
    }
    else
    {
        source_t newsource;

        sourcesIt = entityInfo->sources.insert(std::make_pair(key, newsource)).first;

        source = &sourcesIt->second;

        if (isPet)
        {
            source->name.append("Pet");
        }
        else if (actionType == ACTIONTYPE_MELEE)
        {
            source->name.append("Attack");
        }
        else if (actionType == ACTIONTYPE_RA_FINISH)
        {
            source->name.append("Ranged Attack");
        }
        else if (actionType == ACTIONTYPE_WS_FINISH || actionType == ACTIONTYPE_NPC_TP_FINISH)
        {
            source->name.append(m_AshitaCore->GetResourceManager()->GetAbilityById(actionID)->Name[2]);
        }
        else if (actionType == ACTIONTYPE_CAST_FINISH)
        {
            source->name.append(m_AshitaCore->GetResourceManager()->GetSpellById(actionID)->Name[2]);
            source->isMagic = true;
        }
        else if (actionType == ACTIONTYPE_JA || actionType == ACTIONTYPE_JA_DNC || actionType == ACTIONTYPE_JA_RUN)
        {
            source->name.append(m_AshitaCore->GetResourceManager()->GetAbilityById(actionID + 512)->Name[2]);
        }
    }
    return source;
}

/**
 * @brief Updates the total, count, and min/max values for a damage source.
 *
 * @param source The source_t to update
 * @param message The message ID from an incoming action packet
 * @param damage The damage value from an incoming action packet
 * @return true
 * @return false
 */
bool Deeps::UpdateDamageSource(source_t* source, uint16_t message, uint32_t damage)
{
    damage_t* type = NULL;
    bool val       = false;
    if (std::find(hitMessages.begin(), hitMessages.end(), message) != hitMessages.end())
    {
        type = &source->damage["Hit"];
        val  = true;
    }
    else if (std::find(critMessages.begin(), critMessages.end(), message) != critMessages.end())
    {
        type = &source->damage["Crit"];
        val  = true;
    }
    else if (std::find(missMessages.begin(), missMessages.end(), message) != missMessages.end())
    {
        type = &source->damage["Miss"];
    }
    else if (std::find(evadeMessages.begin(), evadeMessages.end(), message) != evadeMessages.end())
    {
        type = &source->damage["Evade"];
    }
    else if (std::find(parryMessages.begin(), parryMessages.end(), message) != parryMessages.end())
    {
        type = &source->damage["Parry"];
    }
    if (type)
    {
        damage = val ? damage : 0;
        type->total += damage;
        type->count++;
        type->min = (damage < type->min ? damage : type->min);
        type->max = (damage > type->max ? damage : type->max);
        return true;
    }
    return false;
}