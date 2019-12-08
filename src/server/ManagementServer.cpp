/*
	This file is part of Spine.

    Spine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Spine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
// Copyright 2019 Clockwork Origins

#include "ManagementServer.h"

#include "MariaDBWrapper.h"
#include "ServerCommon.h"
#include "SpineServerConfig.h"

#include "common/MessageStructs.h"

#define BOOST_SPIRIT_THREADSAFE
#include "boost/filesystem/operations.hpp"
#include "boost/property_tree/json_parser.hpp"

using namespace boost::property_tree;

namespace spine {
namespace server {

	ManagementServer::ManagementServer() : _server(nullptr), _runner(nullptr) {
	}

	ManagementServer::~ManagementServer() {
		delete _server;
		delete _runner;
	}
	
	int ManagementServer::run() {
		_server = new HttpsServer(SSLCHAINPATH, SSLPRIVKEYNPATH);
		_server->config.port = MANAGEMENTSERVER_PORT;
		
		_server->resource["^/getMods"]["POST"] = std::bind(&ManagementServer::getMods, this, std::placeholders::_1, std::placeholders::_2);
		_server->resource["^/getAchievements"]["POST"] = std::bind(&ManagementServer::getAchievements, this, std::placeholders::_1, std::placeholders::_2);
		_server->resource["^/updateAchievements"]["POST"] = std::bind(&ManagementServer::updateAchievements, this, std::placeholders::_1, std::placeholders::_2);

		_runner = new std::thread([this]() {
			_server->start();
		});
		
		return 0;
	}

	void ManagementServer::stop() {
		_server->stop();
		_runner->join();
	}

	void ManagementServer::getMods(std::shared_ptr<HttpsServer::Response> response, std::shared_ptr<HttpsServer::Request> request) const {
		try {
			const std::string content = ServerCommon::convertString(request->content.string());

			std::stringstream ss(content);

			ptree pt;
			read_json(ss, pt);
		
			const std::string username = pt.get<std::string>("username");
			const std::string password = pt.get<std::string>("password");

			const int userID = ServerCommon::getUserID(username, password);

			if (userID == -1) {
				response->write(SimpleWeb::StatusCode::client_error_unauthorized);
				return;
			}
			
			const std::string language = pt.get<std::string>("language");

			SimpleWeb::StatusCode code = SimpleWeb::StatusCode::success_ok;

			std::stringstream responseStream;
			ptree responseTree;

			do {
				CONNECTTODATABASE(__LINE__)
					
				if (!database.query("PREPARE selectTeamsStmt FROM \"SELECT TeamID FROM teammembers WHERE UserID = ?\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE selectModStmt FROM \"SELECT ModID FROM mods WHERE TeamID = ?\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE selectModNameStmt FROM \"SELECT CAST(Name AS BINARY) FROM modnames WHERE ModID = ? AND Language = CONVERT(? USING BINARY) LIMIT 1\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("SET @paramUserID=" + std::to_string(userID) + ";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					break;
				}
				if (!database.query("SET @paramLanguage='" + language + "';")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					break;
				}
				if (!database.query("EXECUTE selectTeamsStmt USING @paramUserID;")) {
					std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
					break;
				}
				auto lastResults = database.getResults<std::vector<std::string>>();
				
				ptree modNodes;
				for (auto vec : lastResults) {
					if (!database.query("SET @paramTeamID=" + vec[0] + ";")) {
						std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
						break;
					}
					if (!database.query("EXECUTE selectModStmt USING @paramTeamID;")) {
						std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
						break;
					}
					auto results = database.getResults<std::vector<std::string>>();
					for (auto mod : results) {
						if (!database.query("SET @paramModID=" + mod[0] + ";")) {
							std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
							break;
						}
						if (!database.query("EXECUTE selectModNameStmt USING @paramModID, @paramLanguage;")) {
							std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
							break;
						}
						auto nameResults = database.getResults<std::vector<std::string>>();

						if (nameResults.empty()) continue;

						ptree modNode;
						modNode.put("Name", nameResults[0][0]);
						modNode.put("ID", std::stoi(mod[0]));
						modNodes.push_back(std::make_pair("", modNode));
					}
				}
				
				responseTree.add_child("Mods", modNodes);
			} while (false);

			write_json(responseStream, responseTree);

			response->write(code, responseStream.str());
		} catch (...) {
			response->write(SimpleWeb::StatusCode::client_error_bad_request);
		}
	}

	void ManagementServer::getAchievements(std::shared_ptr<HttpsServer::Response> response, std::shared_ptr<HttpsServer::Request> request) const {
		try {
			const std::string content = ServerCommon::convertString(request->content.string());

			std::stringstream ss(content);

			ptree pt;
			read_json(ss, pt);
		
			const std::string username = pt.get<std::string>("username");
			const std::string password = pt.get<std::string>("password");

			const int userID = ServerCommon::getUserID(username, password);

			if (userID == -1) {
				response->write(SimpleWeb::StatusCode::client_error_unauthorized);
				return;
			}

			const int32_t modID = pt.get<int32_t>("modID");

			if (!hasAdminAccessToMod(userID, modID)) {
				response->write(SimpleWeb::StatusCode::client_error_unauthorized);
				return;
			}

			SimpleWeb::StatusCode code = SimpleWeb::StatusCode::success_ok;

			std::stringstream responseStream;
			ptree responseTree;

			do {
				CONNECTTODATABASE(__LINE__)
				
				if (!database.query("PREPARE selectAchievementsStmt FROM \"SELECT Identifier FROM modAchievementList WHERE ModID = ? ORDER BY Identifier ASC\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE selectAchievementNameStmt FROM \"SELECT CAST(Name AS BINARY), CAST(Language AS BINARY) FROM modAchievementNames WHERE ModID = ? AND Identifier = ? LIMIT 1\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE selectAchievementDescriptionStmt FROM \"SELECT CAST(Description AS BINARY), CAST(Language AS BINARY) FROM modAchievementDescriptions WHERE ModID = ? AND Identifier = ? LIMIT 1\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE selectAchievementProgressStmt FROM \"SELECT Max FROM modAchievementProgressMax WHERE ModID = ? AND Identifier = ? LIMIT 1\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE selectAchievementHiddenStmt FROM \"SELECT ModID FROM modAchievementHidden WHERE ModID = ? AND Identifier = ? LIMIT 1\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE selectAchievementIconsStmt FROM \"SELECT LockedIcon, LockedHash, UnlockedIcon, UnlockedHash FROM modAchievementIcons WHERE ModID = ? AND Identifier = ? LIMIT 1\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("SET @paramUserID=" + std::to_string(userID) + ";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					break;
				}
				if (!database.query("SET @paramModID=" + std::to_string(modID) + ";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					break;
				}
				if (!database.query("EXECUTE selectAchievementsStmt USING @paramModID;")) {
					std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
					break;
				}
				const auto results = database.getResults<std::vector<std::string>>();

				ptree achievementNodes;
				for (auto achievement : results) {
					ptree achievementNode;
					
					const int currentID = std::stoi(achievement[0]);

					if (!database.query("SET @paramIdentifier=" + std::to_string(currentID) + ";")) {
						std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
						break;
					}
					
					if (!database.query("EXECUTE selectAchievementNameStmt USING @paramModID, @paramIdentifier;")) {
						std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
						break;
					}
					auto results2 = database.getResults<std::vector<std::string>>();
					if (!results2.empty()) {
						ptree nameNodes;
						for (const auto & vec : results2) {
							ptree nameNode;
							nameNode.put("Text", vec[0]);
							nameNode.put("Language", vec[1]);
							nameNodes.push_back(std::make_pair("", nameNode));
						}
						achievementNode.add_child("Names", nameNodes);
					}
					
					if (!database.query("EXECUTE selectAchievementDescriptionStmt USING @paramModID, @paramIdentifier;")) {
						std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
						break;
					}
					results2 = database.getResults<std::vector<std::string>>();
					if (!results2.empty()) {
						ptree descriptionNodes;
						for (const auto & vec : results2) {
							ptree descriptionNode;
							descriptionNode.put("Text", vec[0]);
							descriptionNode.put("Language", vec[1]);
							descriptionNodes.push_back(std::make_pair("", descriptionNode));
						}
						achievementNode.add_child("Descriptions", descriptionNodes);
					}

					if (!database.query("EXECUTE selectAchievementProgressStmt USING @paramModID, @paramIdentifier;")) {
						std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
						break;
					}
					results2 = database.getResults<std::vector<std::string>>();
					achievementNode.put("MaxProgress", results2.empty() ? 0 : std::stoi(results2[0][0]));

					if (!database.query("EXECUTE selectAchievementHiddenStmt USING @paramModID, @paramIdentifier;")) {
						std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
						break;
					}
					results2 = database.getResults<std::vector<std::string>>();
					achievementNode.put("Hidden", results2.empty());

					if (!database.query("EXECUTE selectAchievementIconsStmt USING @paramModID, @paramIdentifier;")) {
						std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
						break;
					}
					results2 = database.getResults<std::vector<std::string>>();
					if (!results2.empty()) {
						achievementNode.put("LockedImageName", results2[0][0]);
						achievementNode.put("LockedImageHash", results2[0][1]);
						achievementNode.put("UnlockedImageName", results2[0][2]);
						achievementNode.put("UnlockedImageHash", results2[0][3]);
					} else {
						achievementNode.put("LockedImageName", "");
						achievementNode.put("LockedImageHash", "");
						achievementNode.put("UnlockedImageName", "");
						achievementNode.put("UnlockedImageHash", "");
					}
					achievementNodes.push_back(std::make_pair("", achievementNode));
				}
				
				responseTree.add_child("Achievements", achievementNodes);
			} while (false);

			write_json(responseStream, responseTree);

			response->write(code, responseStream.str());
		} catch (...) {
			response->write(SimpleWeb::StatusCode::client_error_bad_request);
		}
	}

	void ManagementServer::updateAchievements(std::shared_ptr<HttpsServer::Response> response, std::shared_ptr<HttpsServer::Request> request) const {
		try {
			const std::string content = ServerCommon::convertString(request->content.string());

			std::stringstream ss(content);

			ptree pt;
			read_json(ss, pt);
		
			const std::string username = pt.get<std::string>("Username");
			const std::string password = pt.get<std::string>("Password");

			const int userID = ServerCommon::getUserID(username, password);

			if (userID == -1) {
				response->write(SimpleWeb::StatusCode::client_error_unauthorized);
				return;
			}

			const int32_t modID = pt.get<int32_t>("ModID");

			if (!hasAdminAccessToMod(userID, modID)) {
				response->write(SimpleWeb::StatusCode::client_error_unauthorized);
				return;
			}

			SimpleWeb::StatusCode code = SimpleWeb::StatusCode::success_ok;

			std::stringstream responseStream;
			ptree responseTree;

			do {
				CONNECTTODATABASE(__LINE__)

				if (!database.query("PREPARE insertAchievementStmt FROM \"INSERT IGNORE INTO modAchievementList (ModID, Identifier) VALUES (?, ?)\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE updateAchievementNameStmt FROM \"INSERT INTO modAchievementNames (ModID, Identifier, Language, Name) VALUES (?, ?, CONVERT(? USING BINARY), CONVERT(? USING BINARY)) ON DUPLICATE KEY UPDATE Name = CONVERT(? USING BINARY)\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE updateAchievementDescriptionStmt FROM \"INSERT INTO modAchievementDescriptions (ModID, Identifier, Language, Description) VALUES (?, ?, CONVERT(? USING BINARY), CONVERT(? USING BINARY)) ON DUPLICATE KEY UPDATE Description = CONVERT(? USING BINARY)\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE updateAchievementProgressStmt FROM \"INSERT INTO modAchievementProgressMax (ModID, Identifier, Max) VALUES (?, ?, ?) ON DUPLICATE KEY UPDATE Max = ?\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE deleteAchievementProgressStmt FROM \"DELETE FROM modAchievementProgressMax WHERE ModID = ? AND Identifier = ? LIMIT 1\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE updateAchievementHiddenStmt FROM \"INSERT IGNORE INTO modAchievementHidden (ModID, Identifier) VALUES (?, ?)\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE deleteAchievementHiddenStmt FROM \"DELETE FROM modAchievementHidden WHERE ModID = ? AND Identifier = ? LIMIT 1\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("PREPARE updateAchievementIconStmt FROM \"INSERT INTO modAchievementIcons (ModID, Identifier, LockedIcon, LockedHash, UnlockedIcon, UnlockedHash) VALUES (?, ?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE LockedIcon = ?, LockedHash = ?, UnlockedIcon = ?, UnlockedHash = ?\";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				if (!database.query("SET @paramModID=" + std::to_string(modID) + ";")) {
					std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
					code = SimpleWeb::StatusCode::client_error_failed_dependency;
					break;
				}
				const auto achievements = pt.get_child("Achievements");

				int id = 0;
				for (const auto achievement : achievements) {
					if (!database.query("SET @paramIdentifier=" + std::to_string(id) + ";")) {
						std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
						code = SimpleWeb::StatusCode::client_error_failed_dependency;
						break;
					}
					if (!database.query("EXECUTE insertAchievementStmt USING @paramModID, @paramIdentifier;")) {
						std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
						code = SimpleWeb::StatusCode::client_error_failed_dependency;
						break;
					}
					for (const auto & tt : achievement.second.get_child("Names")) {
						if (!database.query("SET @paramLanguage='" + tt.second.get<std::string>("Language") + "';")) {
							std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
						if (!database.query("SET @paramName='" + tt.second.get<std::string>("Text") + "';")) {
							std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
						if (!database.query("EXECUTE updateAchievementNameStmt USING @paramModID, @paramIdentifier, @paramLanguage, @paramName, @paramName;")) {
							std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
					}
					for (const auto & tt : achievement.second.get_child("Descriptions")) {
						if (!database.query("SET @paramLanguage='" + tt.second.get<std::string>("Language") + "';")) {
							std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
						if (!database.query("SET @paramDescription='" + tt.second.get<std::string>("Text") + "';")) {
							std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
						if (!database.query("EXECUTE updateAchievementDescriptionStmt USING @paramModID, @paramIdentifier, @paramLanguage, @paramDescription, @paramDescription;")) {
							std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
					}
					const int32_t maxProgress = achievement.second.get<int32_t>("MaxProgress");
					if (maxProgress > 0) {
						if (!database.query("SET @paramProgress=" + std::to_string(maxProgress) + ";")) {
							std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
						if (!database.query("EXECUTE updateAchievementProgressStmt USING @paramModID, @paramIdentifier, @paramProgress, @paramProgress;")) {
							std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
					} else {
						if (!database.query("EXECUTE deleteAchievementProgressStmt USING @paramModID, @paramIdentifier;")) {
							std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
					}
					const bool hidden = achievement.second.get<bool>("Hidden");
					if (hidden) {
						if (!database.query("EXECUTE updateAchievementHiddenStmt USING @paramModID, @paramIdentifier;")) {
							std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
					} else {
						if (!database.query("EXECUTE deleteAchievementHiddenStmt USING @paramModID, @paramIdentifier;")) {
							std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
					}
					
					const std::string lockedImageName = achievement.second.get<std::string>("LockedImageName");
					const std::string lockedImageHash = achievement.second.get<std::string>("LockedImageHash");
					const std::string unlockedImageName = achievement.second.get<std::string>("UnlockedImageName");
					const std::string unlockedImageHash = achievement.second.get<std::string>("UnlockedImageHash");
					
					if (!lockedImageName.empty() || !unlockedImageName.empty()) {
						if (!database.query("SET @paramLockedIcon='" + lockedImageName + "';")) {
							std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
						if (!database.query("SET @paramLockedHash='" + lockedImageHash + "';")) {
							std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
						if (!database.query("SET @paramUnlockedIcon='" + unlockedImageName + "';")) {
							std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
						if (!database.query("SET @paramUnlockedHash='" + unlockedImageHash + "';")) {
							std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
						if (!database.query("EXECUTE updateAchievementIconStmt USING @paramModID, @paramIdentifier, @paramLockedIcon, @paramLockedHash, @paramUnlockedIcon, @paramUnlockedHash, @paramLockedIcon, @paramLockedHash, @paramUnlockedIcon, @paramUnlockedHash;")) {
							std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
							code = SimpleWeb::StatusCode::client_error_failed_dependency;
							break;
						}
					}
					
					id++;
				}
			} while (false);

			response->write(code);
		} catch (...) {
			response->write(SimpleWeb::StatusCode::client_error_bad_request);
		}
	}

	void ManagementServer::uploadAchievementIcons(common::UploadAchievementIconsMessage * msg) const {
		boost::filesystem::create_directories("/var/www/vhosts/clockwork-origins.de/httpdocs/Gothic/downloads/mods/" + std::to_string(msg->modID) + "/achievements");
		
		for (const auto & icon : msg->icons) {
			if (icon.data.empty()) continue; // shouldn't be possible, but we want to be on the save side
			
			std::ofstream out;
			out.open("/var/www/vhosts/clockwork-origins.de/httpdocs/Gothic/downloads/mods/" + std::to_string(msg->modID) + "/achievements/" + icon.name, std::ios::out | std::ios::binary);
			out.write(reinterpret_cast<const char *>(&icon.data[0]), icon.data.size());
			out.close();
		}
	}

	bool ManagementServer::hasAdminAccessToMod(int userID, int modID) const {
		do {
			CONNECTTODATABASE(__LINE__)
				
			if (!database.query("PREPARE selectModStmt FROM \"SELECT TeamID FROM mods WHERE ModID = ? LIMIT 1\";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
			if (!database.query("PREPARE selectMemberStmt FROM \"SELECT UserID FROM teammembers WHERE TeamID = ? AND UserID = ? LIMIT 1\";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
			if (!database.query("SET @paramUserID=" + std::to_string(userID) + ";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
			if (!database.query("SET @paramModID=" + std::to_string(modID) + ";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
			if (!database.query("EXECUTE selectModStmt USING @paramModID;")) {
				std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
				break;
			}
			auto results = database.getResults<std::vector<std::string>>();

			if (results.empty()) break;
			
			if (!database.query("SET @paramTeamID=" + results[0][0] + ";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
			
			if (!database.query("EXECUTE selectMemberStmt USING @paramTeamID, @paramUserID;")) {
				std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
				break;
			}
			results = database.getResults<std::vector<std::string>>();
			
			return !results.empty();
		} while (false);
		return false;
	}

} /* namespace server */
} /* namespace spine */
