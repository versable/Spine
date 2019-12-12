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

#pragma once

#include <cstdint>
#include <functional>
#include <memory>

class QJsonObject;
class QString;

namespace spine {
namespace https {

	class Https {
	public:
		static void post(uint16_t port, const QString & f, const QString & content, const std::function<void(const QJsonObject &, int statusCode)> & callback);
		static void postAsync(uint16_t port, const QString & f, const QString & content, const std::function<void(const QJsonObject &, int statusCode)> & callback);
	};

} /* namespace https */
} /* namespace spine */
