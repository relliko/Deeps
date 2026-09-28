#ifndef DEEPS_PACKET_SAFETY_H
#define DEEPS_PACKET_SAFETY_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <limits>
#include <vector>

namespace DeepsSafety
{
class PacketHistory
{
public:
    explicit PacketHistory(std::size_t capacity = 200) : m_Capacity(capacity) {}

    bool Remember(const uint8_t* data, std::size_t size)
    {
        if (data == nullptr || size == 0)
            return false;

        for (const auto& packet : m_Packets)
        {
            if (packet.size() == size && std::memcmp(packet.data(), data, size) == 0)
                return false;
        }

        m_Packets.emplace_back(data, data + size);
        while (m_Packets.size() > m_Capacity)
            m_Packets.pop_front();
        return true;
    }

    void Clear() { m_Packets.clear(); }
    std::size_t Size() const { return m_Packets.size(); }

private:
    std::size_t m_Capacity;
    std::deque<std::vector<uint8_t>> m_Packets;
};

class CheckedBitReader
{
public:
    CheckedBitReader(const uint8_t* data, std::size_t size) : m_Data(data), m_Size(size) {}

    bool Read(std::size_t bitOffset, uint8_t bitCount, uint64_t& value) const
    {
        value = 0;
        if (m_Data == nullptr || bitCount == 0 || bitCount > 64 ||
            bitOffset > std::numeric_limits<std::size_t>::max() - (bitCount - 1) ||
            (bitOffset + bitCount - 1) / 8 >= m_Size)
            return false;

        for (uint8_t i = 0; i < bitCount; ++i)
            value |= static_cast<uint64_t>((m_Data[(bitOffset + i) / 8] >> ((bitOffset + i) % 8)) & 1u) << i;
        return true;
    }

    bool ReadU8(std::size_t offset, uint8_t& value) const
    {
        uint64_t temp = 0;
        if (!Read(offset * 8, 8, temp)) return false;
        value = static_cast<uint8_t>(temp);
        return true;
    }

    bool ReadU32(std::size_t offset, uint32_t& value) const
    {
        uint64_t temp = 0;
        if (!Read(offset * 8, 32, temp)) return false;
        value = static_cast<uint32_t>(temp);
        return true;
    }

private:
    const uint8_t* m_Data;
    std::size_t m_Size;
};

struct ActionRecord
{
    uint8_t reaction;
    uint16_t animation;
    uint8_t specEffect;
    uint32_t mainDamage;
    uint16_t messageID;
    bool hasAdditionalEffect;
    uint16_t additionalDamage;
    uint16_t additionalMessageID;
    bool hasSpikesEffect;
};

struct ActionTarget
{
    std::vector<ActionRecord> actions;
};

struct ActionPacket
{
    uint32_t userID;
    uint8_t actionType;
    uint16_t actionID;
    std::vector<ActionTarget> targets;
};

// Fully validates the variable-length 0x28 layout before exposing records.
// Each target owns its action count and advances the cursor independently.
inline bool ParseActionPacket(const uint8_t* data, std::size_t size, ActionPacket& packet)
{
    CheckedBitReader reader(data, size);
    ActionPacket parsed = {};
    uint8_t targetCount = 0;
    uint64_t value = 0;
    if (!reader.ReadU8(0x09, targetCount) || !reader.ReadU32(0x05, parsed.userID) ||
        !reader.Read(82, 4, value))
        return false;
    parsed.actionType = static_cast<uint8_t>(value);
    if (!reader.Read(86, 10, value))
        return false;
    parsed.actionID = static_cast<uint16_t>(value);

    std::size_t targetStart = 150;
    parsed.targets.reserve(targetCount);
    for (uint8_t targetIndex = 0; targetIndex < targetCount; ++targetIndex)
    {
        if (!reader.Read(targetStart + 32, 4, value))
            return false;
        const uint8_t actionCount = static_cast<uint8_t>(value);
        ActionTarget target;
        target.actions.reserve(actionCount);
        std::size_t actionStart = targetStart + 36;

        for (uint8_t actionIndex = 0; actionIndex < actionCount; ++actionIndex)
        {
            ActionRecord action = {};
            if (!reader.Read(actionStart, 5, value)) return false;
            action.reaction = static_cast<uint8_t>(value);
            if (!reader.Read(actionStart + 5, 12, value)) return false;
            action.animation = static_cast<uint16_t>(value);
            if (!reader.Read(actionStart + 17, 7, value)) return false;
            action.specEffect = static_cast<uint8_t>(value);
            if (!reader.Read(actionStart + 27, 17, value)) return false;
            action.mainDamage = static_cast<uint32_t>(value);
            if (!reader.Read(actionStart + 44, 10, value)) return false;
            action.messageID = static_cast<uint16_t>(value);
            if (!reader.Read(actionStart + 85, 1, value)) return false;
            action.hasAdditionalEffect = value != 0;

            std::size_t suffixStart = actionStart + 86;
            if (action.hasAdditionalEffect)
            {
                if (!reader.Read(actionStart + 96, 16, value)) return false;
                action.additionalDamage = static_cast<uint16_t>(value);
                if (!reader.Read(actionStart + 113, 10, value)) return false;
                action.additionalMessageID = static_cast<uint16_t>(value);
                suffixStart += 37;
            }
            if (!reader.Read(suffixStart, 1, value)) return false;
            action.hasSpikesEffect = value != 0;
            suffixStart += 1;
            if (action.hasSpikesEffect)
            {
                // The spikes payload is not consumed by Deeps, but it is part
                // of the record and must exist for structural validation.
                if (!reader.Read(suffixStart, 34, value)) return false;
                suffixStart += 34;
            }

            actionStart = suffixStart;
            target.actions.push_back(action);
        }

        targetStart = actionStart;
        parsed.targets.push_back(target);
    }

    packet = parsed;
    return true;
}
}

#endif
