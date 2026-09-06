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
    if (data == NULL || size == 0)
        return false;

    if (id != 0x28)
    {
        m_Packets.Remember(data, size);
        return false;
    }

    // Parse the complete variable-length packet before touching damage state or
    // deduplication history. A truncated suffix must not partially apply hits.
    DeepsSafety::ActionPacket packet;
    if (!DeepsSafety::ParseActionPacket(data, size, packet))
        return false;
    if (!m_Packets.Remember(data, size))
        return false;

    auto memory = m_AshitaCore != NULL ? m_AshitaCore->GetMemoryManager() : NULL;
    auto entities = memory != NULL ? memory->GetEntity() : NULL;
    if (entities == NULL)
        return false;
    const uint32_t userID = packet.userID;
    const uint8_t actionType = packet.actionType;
    const uint16_t actionID = packet.actionID;
    const uint16_t index = GetIndexFromId(userID);
    if (userID == 0 || index == 0 || actionType == 0 || actionID == 0)
        return false;

    entitysources_t* entityInfo = NULL;
    auto it = m_Entities.find(userID);
    if (it != m_Entities.end())
    {
        entityInfo = &it->second;
        const uint16_t petIndex = entities->GetPetTargetIndex(index);
        const uint32_t petID = entities->GetServerId(petIndex);
        if (petIndex > 0 && petID > 0)
        {
            entitysources_t newPetInfo;
            auto name = entities->GetName(petIndex);
            newPetInfo.name = name != nullptr ? name : "(Unknown)";
            newPetInfo.color = RandomColors[rand() % RandomColors.size()];
            newPetInfo.id = petID;
            newPetInfo.ownerid = userID;
            m_Entities.insert(std::make_pair(petID, newPetInfo));
        }
    }
    else
    {
        if (userID > 0x1000000)
            return false;
        entitysources_t newInfo;
        auto name = entities->GetName(index);
        newInfo.name = name != nullptr ? name : "(Unknown)";
        newInfo.color = RandomColors[rand() % RandomColors.size()];
        newInfo.id = userID;
        newInfo.ownerid = NULL;
        entityInfo = &m_Entities.emplace(userID, newInfo).first->second;
        if (m_Debug)
        {
            auto chat = m_AshitaCore->GetChatManager();
            if (chat != NULL)
                chat->Writef(-3, false, "Total entities: %u", static_cast<unsigned int>(m_Entities.size()));
        }
    }

    if (entityInfo == NULL)
        return false;
    if (m_Debug && m_AshitaCore->GetChatManager() != NULL)
        m_AshitaCore->GetChatManager()->Writef(-3, false, "Action Type: %d Action ID: %d", actionType, actionID);

    bool isPet = entityInfo->ownerid != NULL;
    if (isPet)
    {
        const auto petOwnerIndex = entities->GetTrustOwnerTargetIndex(index);
        if (petOwnerIndex != 0)
            entityInfo->ownerid = entities->GetServerId(petOwnerIndex);
        auto owner = m_Entities.find(entityInfo->ownerid);
        if (owner == m_Entities.end())
            return false;
        entityInfo = &owner->second;
    }

    if (!IsParsedActionType(actionType))
        return false;

    for (const auto& target : packet.targets)
    {
        for (const auto& action : target.actions)
        {
            source_t* source = GetDamageSource(entityInfo,
                (actionType == ACTIONTYPE_MELEE && action.animation == 4) ? actionType + 1 : actionType,
                actionID, isPet);
            if (source == NULL)
                continue;

            if (m_Debug && m_AshitaCore->GetChatManager() != NULL)
            {
                m_AshitaCore->GetChatManager()->Writef(-3, false, "Reaction: %d Animation: %d", action.reaction, action.animation);
                m_AshitaCore->GetChatManager()->Writef(-3, false, "SpecEffect: %d Param: %d", action.specEffect, action.mainDamage);
            }
            UpdateDamageSource(source, action.messageID, action.mainDamage);

            if (action.hasAdditionalEffect && actionType != ACTIONTYPE_JA)
            {
                const bool isSC = action.additionalMessageID >= 288 && action.additionalMessageID <= 302;
                if (action.additionalMessageID == MSG_ADD_EFFECT_DMG ||
                    action.additionalMessageID == MSG_ADD_EFFECT_DMG2 || (isSC && m_CountSkillchains))
                {
                    const uint32_t key = isSC ? (2 << 8) : (1 << 8);
                    auto sourceIt = entityInfo->sources.find(key);
                    if (sourceIt == entityInfo->sources.end())
                    {
                        source_t newSource;
                        newSource.name = isSC ? "Skillchain" : "Additional Effect";
                        sourceIt = entityInfo->sources.insert(std::make_pair(key, newSource)).first;
                    }
                    damage_t& hit = sourceIt->second.damage["Hit"];
                    hit.count += 1;
                    hit.total += action.additionalDamage;
                    hit.min = action.additionalDamage < hit.min ? action.additionalDamage : hit.min;
                    hit.max = action.additionalDamage > hit.max ? action.additionalDamage : hit.max;
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
    if (m_AshitaCore == NULL || m_AshitaCore->GetMemoryManager() == NULL)
        return 0;
    auto entMgr = m_AshitaCore->GetMemoryManager()->GetEntity();
    if (entMgr == NULL)
        return 0;
    for (int i = 0; i < 0x900; i++)
    {
        if (entMgr->GetServerId(i) == id)
            return i;
    }
    return 0;
}

source_t* Deeps::GetDamageSource(entitysources_t* entityInfo, uint8_t actionType, uint16_t actionID, bool isPet)
{
    if (entityInfo == NULL)
        return NULL;
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
            auto resources = m_AshitaCore != NULL ? m_AshitaCore->GetResourceManager() : NULL;
            auto ability = resources != NULL ? resources->GetAbilityById(actionID) : NULL;
            source->name.append((ability != NULL && ability->Name[2] != NULL) ? ability->Name[2] : "Unknown Ability");
        }
        else if (actionType == ACTIONTYPE_CAST_FINISH)
        {
            auto resources = m_AshitaCore != NULL ? m_AshitaCore->GetResourceManager() : NULL;
            auto spell = resources != NULL ? resources->GetSpellById(actionID) : NULL;
            source->name.append((spell != NULL && spell->Name[2] != NULL) ? spell->Name[2] : "Unknown Spell");
            source->isMagic = true;
        }
        else if (actionType == ACTIONTYPE_JA || actionType == ACTIONTYPE_JA_DNC || actionType == ACTIONTYPE_JA_RUN)
        {
            auto resources = m_AshitaCore != NULL ? m_AshitaCore->GetResourceManager() : NULL;
            auto ability = resources != NULL ? resources->GetAbilityById(actionID + 512) : NULL;
            source->name.append((ability != NULL && ability->Name[2] != NULL) ? ability->Name[2] : "Unknown Ability");
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
    if (source == NULL)
        return false;
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
