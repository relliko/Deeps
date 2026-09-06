/**
* Copyright (c) 2011-2014 - Ashita Development Team
*
* Ashita is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Ashita is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Ashita.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Deeps.h"

/**
 * @brief Constructor and Deconstructor
 */
Deeps::Deeps(void)
    : m_AshitaCore(NULL)
    , m_LogManager(NULL)
    , m_PluginId(0)
    , m_Direct3DDevice(NULL)
    , m_Debug(false)
    , m_JobColors(true)
    , m_MaxBars(15)
    , m_PartyOnly(true)
    , m_TVMode(false)
    , m_GUIScale(1)
    , m_CountSkillchains(true)
    , m_Background(NULL)
    , m_CharInfo(0)
    , m_Drag(false)
    , m_LastX(0)
    , m_LastY(0)
    , m_LastRender(0)
    , m_StopReport(false)
    , m_Released(false)
{ }
Deeps::~Deeps(void)
{
    Release();
}

/**
 * @brief Gets the PluginFlags this plugin uses.
 *
 */
uint32_t Deeps::GetFlags(void) const
{
    return (uint32_t)Ashita::PluginFlags::LegacyDirect3D;
}

/**
 * @brief Gets the name of the plugin.
 *
 * @return const char* The name of the plugin
 */
const char* Deeps::GetName(void) const
{
    return "Deeps";
}

double Deeps::GetVersion(void) const
{
    return 1.06f;
}

/**
 * @brief Gets the author of the plugin.
 *
 * @return const char* The authors name.
 */
const char* Deeps::GetAuthor(void) const
{
    return "Relliko, kjLotus";
}

/**
 * @brief Gets the plugin description.
 *
 * @return const char* Plugin description.
 */
const char* Deeps::GetDescription(void) const
{
    return "Damage meters for Ashita v4.";
}

/**
 * @brief Initializes our plugin. This is the main call that happens when your plugin is loaded.
 *
 * @param ashitaCore        The main Ashita Core object interface to interact with Ashita.
 * @param scriptEngine      The main script engine object interface to interact with the script engine.
 * @param dwPluginId        The base address of your plugin. This is used as the ID.
 *
 * @return True on success, false otherwise.
 *
 * @note If your plugin returns false here, it will be unloaded immediately!
 */
bool Deeps::Initialize(IAshitaCore* core, ILogManager* log, uint32_t id)
{
	if (core == NULL || log == NULL)
		return false;
	if (core->GetConfigurationManager() == NULL)
		return false;
	this->m_AshitaCore = core;
	this->m_PluginId = id;
	this->m_LogManager = log;
    this->m_LastRender = clock();
    srand(time(NULL));
    m_CharInfo = 0;
	m_AshitaCore->GetConfigurationManager()->Load("Deeps", "Deeps");

    return true;
}

/**
 * @brief Releases this plugin. This is called when your plugin is unloaded.
 *
 * @note Your plugin should cleanup all its data here before it unloads. Anything such as:
 *          - Font objects.
 *          - Gui objects.
 *          - Bindings to the script engine (if you extended it any).
 */
void Deeps::Release(void)
{
    if (m_Released)
        return;
    m_Released = true;
    StopReport();
    this->Direct3DRelease();
    m_Packets.Clear();
    m_AshitaCore = NULL;
    m_LogManager = NULL;
}

/**
 * @brief Allows a plugin to attempt to handle a game command.
 *
 * @param pszCommand            The command being processed.
 * @param nCommandType          The type of command being processed.
 *
 * @return True on handled, false otherwise.
 */
bool Deeps::HandleCommand(int32_t mode, const char* command, bool injected)
{
    if (m_Released || m_AshitaCore == NULL || command == NULL || m_AshitaCore->GetChatManager() == NULL)
        return false;
    std::vector<std::string> args;
    auto count = Ashita::Commands::GetCommandArgs(command, &args);
    if (count <= 0) return false;
    HANDLECOMMAND("/deeps", "/dps")
    {
        if (count >= 2)
        {
            if (args[1] == "reset")
            {
                m_Entities.clear();
                m_SourceInfo.clear();
                m_CharInfo = 0;
                return true;
            }
            else if (args[1] == "report")
            {
                char mode = 0x00;
                int max = 3;
                if (count > 2)
                {
                    if (std::all_of(args[2].begin(), args[2].end(), ::isdigit))
                    {
                        max = atoi(args[2].c_str());
                    }
                    else
                    {
                        mode = args[2][0];
                        if (count > 3)
                        {
                            if (std::all_of(args[3].begin(), args[3].end(), ::isdigit))
                            {
                                max = atoi(args[3].c_str());
                            }
                        }
                    }
                }

                std::vector<std::string> lines;
                if (m_Background != NULL && mode != 0x00)
                {
                    lines.push_back(m_Background->GetText());
                    for (size_t i = 0; i < m_Bars.size() && i < static_cast<size_t>(std::max(0, max)); ++i)
                    {
                        if (m_Bars[i] != NULL)
                            lines.push_back(m_Bars[i]->GetText());
                    }
                }
                StopReport();
                m_StopReport = false;
                m_ReportThread = std::thread(&Deeps::Report, this, mode, lines);

                return true;
            }
            else if (args[1] == "debug")
            {
                m_Debug = !m_Debug;
                m_AshitaCore->GetChatManager()->Writef(0, false, "%s%s", Ashita::Chat::Header("Deeps").c_str(), Ashita::Chat::Message(m_Debug ? "Debug Enabled" : "Debug Disabled").c_str());
                return true;
            }
            else if (args[1] == "jobcolors")
            {
                m_JobColors = !m_JobColors;
                m_AshitaCore->GetChatManager()->Writef(0, false, "%s%s", Ashita::Chat::Header("Deeps").c_str(), Ashita::Chat::Message(m_JobColors ? "Job Colors Enabled" : "Job Colors Disabled").c_str());
                return true;
            }
            else if (args[1] == "partyonly")
            {
                m_PartyOnly = !m_PartyOnly;
                m_AshitaCore->GetChatManager()->Writef(0, false, "%s%s", Ashita::Chat::Header("Deeps").c_str(), Ashita::Chat::Message(m_PartyOnly ? "Party Only Enabled" : "Party Only Disabled").c_str());
                return true;
            }
            else if (args[1] == "tvmode")
            {
                m_TVMode = !m_TVMode;
                m_GUIScale = m_TVMode ? 1.5 : 1;
                // Wipe deeps to re-render the bars correctly
                 m_Entities.clear();
                m_SourceInfo.clear();
                m_CharInfo = 0;
                m_AshitaCore->GetChatManager()->Writef(0, false, "%s%s", Ashita::Chat::Header("Deeps").c_str(), Ashita::Chat::Message(m_TVMode ? "TV Mode Enabled" : "TV Mode Disabled").c_str());
                return true;
            }
            else if (args[1] == "sc")
            {
                m_CountSkillchains = !m_CountSkillchains;
                m_AshitaCore->GetChatManager()->Writef(0, false, "%s%s", Ashita::Chat::Header("Deeps").c_str(), Ashita::Chat::Message(m_CountSkillchains ? "Skillchain Damage Enabled" : "Skillchain Damage Disabled").c_str());
                return true;
            }
        }
        std::stringstream out;
        out << Ashita::Chat::Header("Deeps");
        out << Ashita::Chat::Error("Invalid command.");
        m_AshitaCore->GetChatManager()->Write(0, false, out.str().c_str());
        out = std::stringstream();
        out << Ashita::Chat::Header("Deeps");
        out << Ashita::Chat::Color2(2, "/dps reset");
        out << Ashita::Chat::Message(" - Reset damage counters.");
        m_AshitaCore->GetChatManager()->Write(0, false, out.str().c_str());
        out = std::stringstream();
        out << Ashita::Chat::Header("Deeps");
        out << Ashita::Chat::Color2(2, "/dps report [s/p/l] [#]");
        out << Ashita::Chat::Message(" - Report damage data to say, party, or linkshell.");
        m_AshitaCore->GetChatManager()->Write(0, false, out.str().c_str());
        out = std::stringstream();
        out << Ashita::Chat::Header("Deeps");
        out << Ashita::Chat::Color2(2, "/dps jobcolors");
        out << Ashita::Chat::Message(" - Toggle job-based color coding.");
        m_AshitaCore->GetChatManager()->Write(0, false, out.str().c_str());
        out = std::stringstream();
        out << Ashita::Chat::Header("Deeps");
        out << Ashita::Chat::Color2(2, "/dps partyonly");
        out << Ashita::Chat::Message(" - Toggle displaying data from non-party members.");
        m_AshitaCore->GetChatManager()->Write(0, false, out.str().c_str());
        out = std::stringstream();
        out << Ashita::Chat::Header("Deeps");
        out << Ashita::Chat::Color2(2, "/dps tvmode");
        out << Ashita::Chat::Message(" - Scales Deeps up to a size that works better on large displays. Note: This resets the current combat data.");
        m_AshitaCore->GetChatManager()->Write(0, false, out.str().c_str());
        out = std::stringstream();
        out << Ashita::Chat::Header("Deeps");
        out << Ashita::Chat::Color2(2, "/dps sc");
        out << Ashita::Chat::Message(" - Toggles skillchain contribution to player damage on or off");
        m_AshitaCore->GetChatManager()->Write(0, false, out.str().c_str());
        return true;
    }
    return false;
}

void Deeps::StopReport(void)
{
    m_StopReport = true;
    m_ReportWake.notify_all();
    if (m_ReportThread.joinable())
        m_ReportThread.join();
}

void Deeps::Report(char mode, std::vector<std::string> lines)
{
    IChatManager* chat = m_AshitaCore != NULL ? m_AshitaCore->GetChatManager() : NULL;
    if (chat == NULL || mode == 0x00)
        return;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        char buff[256];
        if (i != 0)
        {
            std::unique_lock<std::mutex> lock(m_ReportMutex);
            if (m_ReportWake.wait_for(lock, std::chrono::milliseconds(1100), [this]() { return m_StopReport.load(); }))
                return;
        }
        sprintf_s(buff, sizeof(buff), "/%c %s", mode, lines[i].c_str());
        chat->QueueCommand(1, buff);
    }
}

/**
 * @brief Gets the interface version this plugin was compiled with.
 *
 * @note This is a required export, your plugin must implement this!
 */
__declspec(dllexport) double __stdcall expGetInterfaceVersion(void)
{
    return ASHITA_INTERFACE_VERSION;
}

/**
 * @brief Creates an instance of this plugin object.
 *
 * @note This is a required export, your plugin must implement this!
 */
__declspec(dllexport) IPlugin* __stdcall expCreatePlugin(const char* args)
{
    return (IPlugin*)new Deeps();
}

/**
 * @brief Destroys an instance of this plugin object.
 *
 * @note This is a required export for Ashita interface 4.30.
 */
__declspec(dllexport) void __stdcall expDestroyPlugin(IPlugin* instance)
{
    delete instance;
}