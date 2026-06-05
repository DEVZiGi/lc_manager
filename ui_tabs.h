#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "lc_manager_database/database.h"
#include "config.h"

using json = nlohmann::json;

void DrawAgentsTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError, DatabaseManager& db);
void DrawInventoryTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError, DatabaseManager& db);
void DrawOrdealTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError);
void DrawExpTrackerTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError);
void DrawDetailedInfoTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError, DatabaseManager& db);
void DrawExtractionTab(const json& liveData, DatabaseManager& db);
void DrawInfoTab();
void DrawSettingsTab(AppSettings& settings, const std::string& configPath);
