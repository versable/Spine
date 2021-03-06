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
    along with Spine.  If not, see <http://www.gnu.org/licenses/>.
 */
// Copyright 2019 Clockwork Origins

#pragma once

#include "simple-web-server/server_https.hpp"

using HttpsServer = SimpleWeb::Server<SimpleWeb::HTTPS>;

namespace spine {
namespace server {

	class DatabaseServer {
	public:
		DatabaseServer();
		~DatabaseServer();

		int run();
		void stop();

	private:
		HttpsServer * _server;
		std::thread * _runner;

		void getModnameForIDs(std::shared_ptr<HttpsServer::Response> response, std::shared_ptr<HttpsServer::Request> request) const;
		void getUserID(std::shared_ptr<HttpsServer::Response> response, std::shared_ptr<HttpsServer::Request> request) const;
		void getOwnRating(std::shared_ptr<HttpsServer::Response> response, std::shared_ptr<HttpsServer::Request> request) const;
		void getWeightedRating(std::shared_ptr<HttpsServer::Response> response, std::shared_ptr<HttpsServer::Request> request) const;
		void getRatings(std::shared_ptr<HttpsServer::Response> response, std::shared_ptr<HttpsServer::Request> request) const;
		void unlockAchievement(std::shared_ptr<HttpsServer::Response> response, std::shared_ptr<HttpsServer::Request> request) const;
		void getUserIDForDiscordID(std::shared_ptr<HttpsServer::Response> response, std::shared_ptr<HttpsServer::Request> request) const;
		void getSpineLevelRanking(std::shared_ptr<HttpsServer::Response> response, std::shared_ptr<HttpsServer::Request> request) const;
	};

} /* namespace server */
} /* namespace spine */
