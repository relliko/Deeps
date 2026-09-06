#include "../Deeps/PacketSafety.h"

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

static void PutBits(std::vector<uint8_t>& bytes, std::size_t offset, uint8_t count, uint64_t value)
{
    const std::size_t required = (offset + count + 7) / 8;
    if (bytes.size() < required)
        bytes.resize(required, 0);
    for (uint8_t bit = 0; bit < count; ++bit)
    {
        const uint8_t mask = static_cast<uint8_t>(1u << ((offset + bit) % 8));
        if ((value >> bit) & 1u)
            bytes[(offset + bit) / 8] |= mask;
        else
            bytes[(offset + bit) / 8] &= static_cast<uint8_t>(~mask);
    }
}

struct FixtureAction
{
    uint16_t animation;
    uint32_t damage;
    uint16_t message;
    bool additional;
    uint16_t additionalDamage;
    uint16_t additionalMessage;
    bool spikes;
};

static std::vector<uint8_t> MakeActionPacket(const std::vector<std::vector<FixtureAction>>& targets)
{
    std::vector<uint8_t> bytes(19, 0);
    PutBits(bytes, 0x05 * 8, 32, 0x123456);
    PutBits(bytes, 0x09 * 8, 8, targets.size());
    PutBits(bytes, 82, 4, 1);
    PutBits(bytes, 86, 10, 7);

    std::size_t targetStart = 150;
    for (const auto& target : targets)
    {
        PutBits(bytes, targetStart, 32, 0x01020304);
        PutBits(bytes, targetStart + 32, 4, target.size());
        std::size_t actionStart = targetStart + 36;
        for (const auto& action : target)
        {
            PutBits(bytes, actionStart, 5, 3);
            PutBits(bytes, actionStart + 5, 12, action.animation);
            PutBits(bytes, actionStart + 17, 7, 9);
            PutBits(bytes, actionStart + 27, 17, action.damage);
            PutBits(bytes, actionStart + 44, 10, action.message);
            PutBits(bytes, actionStart + 85, 1, action.additional);
            std::size_t suffixStart = actionStart + 86;
            if (action.additional)
            {
                PutBits(bytes, actionStart + 96, 16, action.additionalDamage);
                PutBits(bytes, actionStart + 113, 10, action.additionalMessage);
                suffixStart += 37;
            }
            PutBits(bytes, suffixStart, 1, action.spikes);
            suffixStart += 1;
            if (action.spikes)
            {
                PutBits(bytes, suffixStart, 34, 0x155555555ULL);
                suffixStart += 34;
            }
            actionStart = suffixStart;
        }
        targetStart = actionStart;
    }
    return bytes;
}

int main()
{
    using namespace DeepsSafety;

    PacketHistory history(2);
    const uint8_t a[] = {1, 2};
    const uint8_t samePrefix[] = {1, 2, 3};
    const uint8_t b[] = {4};
    assert(!history.Remember(nullptr, 1));
    assert(!history.Remember(a, 0));
    assert(history.Remember(a, sizeof(a)));
    assert(!history.Remember(a, sizeof(a)));
    assert(history.Remember(samePrefix, sizeof(samePrefix)));
    assert(history.Remember(b, sizeof(b)));
    assert(history.Size() == 2);
    assert(history.Remember(a, sizeof(a)));
    history.Clear();
    assert(history.Size() == 0);

    const uint8_t bits[] = {0xAC, 0x03, 0x78, 0x56, 0x34, 0x12};
    CheckedBitReader reader(bits, sizeof(bits));
    uint64_t value = 99;
    assert(reader.Read(2, 5, value) && value == 11);
    assert(reader.Read(7, 3, value) && value == 7);
    assert(!reader.Read(47, 2, value) && value == 0);
    assert(!reader.Read(0, 0, value));
    assert(!reader.Read(0, 65, value));
    assert(!reader.Read(std::numeric_limits<std::size_t>::max(), 2, value));
    CheckedBitReader nullReader(nullptr, 10);
    assert(!nullReader.Read(0, 1, value));

    uint8_t u8 = 0;
    uint32_t u32 = 0;
    assert(reader.ReadU8(0, u8) && u8 == 0xAC);
    assert(reader.ReadU32(2, u32) && u32 == 0x12345678u);
    assert(!reader.ReadU32(3, u32));

    // Target counts differ (1 then 2), and optional suffixes force three
    // distinct cursor lengths: additional-only, spikes-only, and neither.
    const std::vector<uint8_t> packetBytes = MakeActionPacket({
        {{4, 111, 1, true, 33, 163, false}},
        {{8, 222, 2, false, 0, 0, true}, {12, 333, 3, false, 0, 0, false}}
    });
    ActionPacket packet;
    assert(ParseActionPacket(packetBytes.data(), packetBytes.size(), packet));
    assert(packet.userID == 0x123456u && packet.actionType == 1 && packet.actionID == 7);
    assert(packet.targets.size() == 2);
    assert(packet.targets[0].actions.size() == 1);
    assert(packet.targets[1].actions.size() == 2);
    assert(packet.targets[0].actions[0].additionalDamage == 33);
    assert(packet.targets[0].actions[0].additionalMessageID == 163);
    assert(packet.targets[1].actions[0].hasSpikesEffect);
    assert(packet.targets[1].actions[0].mainDamage == 222);
    assert(packet.targets[1].actions[1].animation == 12);
    assert(packet.targets[1].actions[1].mainDamage == 333);

    // Removing the final byte invalidates the late third action. The parser
    // commits no partially parsed result because assignment occurs at the end.
    ActionPacket unchanged;
    unchanged.userID = 99;
    assert(!ParseActionPacket(packetBytes.data(), packetBytes.size() - 1, unchanged));
    assert(unchanged.userID == 99 && unchanged.targets.empty());
    PacketHistory validatedHistory;
    ActionPacket candidate;
    if (ParseActionPacket(packetBytes.data(), packetBytes.size() - 1, candidate))
        validatedHistory.Remember(packetBytes.data(), packetBytes.size() - 1);
    assert(validatedHistory.Size() == 0);
    if (ParseActionPacket(packetBytes.data(), packetBytes.size(), candidate))
        validatedHistory.Remember(packetBytes.data(), packetBytes.size());
    assert(validatedHistory.Size() == 1);

    // A second target that advertises more actions than are present must not
    // accidentally reuse the first target's count or accept the short packet.
    std::vector<uint8_t> badCount = packetBytes;
    std::size_t secondTargetStart = 150 + 36 + 86 + 37 + 1;
    PutBits(badCount, secondTargetStart + 32, 4, 3);
    assert(!ParseActionPacket(badCount.data(), badCount.size(), unchanged));
    return 0;
}
