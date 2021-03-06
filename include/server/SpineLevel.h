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
// Copyright 2020 Clockwork Origins

#pragma once

#include <list>
#include <mutex>

#include "common/MessageStructs.h"

#define BOOST_SPIRIT_THREADSAFE
#include "boost/property_tree/json_parser.hpp"

namespace spine {
namespace server {

	class SpineLevel {
	public:
		static void init();
		
		static common::SendUserLevelMessage getLevel(int userID);
		static void updateLevel(int userID);
		static void clear(const std::vector<int> & userList);

		static void addRanking(boost::property_tree::ptree & json);

	private:
		typedef struct {
			int userID;
			std::string username;
			uint32_t level;
			uint32_t xp;
		} RankingEntry;
		
		static std::mutex _lock;
		static std::map<int, common::SendUserLevelMessage> _levels;
		static std::vector<RankingEntry> _rankings;
		static std::mutex _rankingLock;
		static std::list<int> _updateQueue;
		static std::mutex _updateQueueLock;

		static void cacheLevel(int userID);
	};

} /* namespace server */
} /* namespace spine */
