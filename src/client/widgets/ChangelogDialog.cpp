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
// Copyright 2018 Clockwork Origins

#include "widgets/ChangelogDialog.h"

#include <functional>
#include <map>

#include "utils/Config.h"
#include "utils/Conversion.h"
#include "utils/WindowsExtensions.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSettings>
#include <QTextBrowser>
#include <QVBoxLayout>

#include "tinyxml2.h"

#ifdef Q_OS_WIN
	#include "clockUtils/log/Log.h"
#endif

using namespace spine;
using namespace spine::utils;
using namespace spine::widgets;

ChangelogDialog::ChangelogDialog(QWidget * par) : QDialog(par), _changelogBrowser(nullptr) {
	QVBoxLayout * l = new QVBoxLayout();
	l->setAlignment(Qt::AlignTop);

	_changelogBrowser = new QTextBrowser(this);
	l->addWidget(_changelogBrowser);

	const bool dsa = Config::IniParser->value("CHANGELOGDIALOG/DontShowAgain", false).toBool();
	QCheckBox * cb = new QCheckBox(QApplication::tr("DontShowAgain"), this);
	cb->setChecked(dsa);
	l->addWidget(cb);
	connect(cb, &QCheckBox::stateChanged, this, &ChangelogDialog::changedShowState);

	QDialogButtonBox * dbb = new QDialogButtonBox(QDialogButtonBox::StandardButton::Close, Qt::Orientation::Horizontal, this);
	l->addWidget(dbb);

	setLayout(l);

	QPushButton * b = dbb->button(QDialogButtonBox::StandardButton::Close);
	b->setText(QApplication::tr("Close"));

	connect(b, &QPushButton::released, this, &ChangelogDialog::accepted);
	connect(b, &QPushButton::released, this, &ChangelogDialog::accept);
	connect(b, &QPushButton::released, this, &ChangelogDialog::hide);

	setMinimumWidth(500);
	setMinimumHeight(500);

	setWindowTitle(QApplication::tr("Changelog"));
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	restoreSettings();
}

ChangelogDialog::~ChangelogDialog() {
	saveSettings();
}

int ChangelogDialog::execStartup() {
	const bool dsa = Config::IniParser->value("CHANGELOGDIALOG/DontShowAgain", false).toBool();
	if (dsa) {
		return QDialog::Accepted;
	} else {
		return exec();
	}
}

int ChangelogDialog::exec() {
	tinyxml2::XMLDocument doc;

	const tinyxml2::XMLError e = doc.LoadFile((qApp->applicationDirPath().toStdString() + "/changelog.xml").c_str());

	if (e) {
		return QDialog::Rejected;
	}

	QString language = Config::IniParser->value("MISC/language", "English").toString();
	if (language != "Deutsch" && language != "English" && language != "Russian") {
		language = "English";
	}

	std::map<uint32_t, std::pair<QStringList, QStringList>, std::greater<uint32_t>> versions;

	tinyxml2::XMLElement * versionsNode = doc.FirstChildElement("Versions");

	for (tinyxml2::XMLElement * node = versionsNode->FirstChildElement("Version"); node != nullptr; node = node->NextSiblingElement("Version")) {
		if (node->Attribute("majorVersion") == nullptr || node->Attribute("minorVersion") == nullptr || node->Attribute("patchVersion") == nullptr) {
			continue;
		}
		const uint8_t majorVersion = uint8_t(std::stoi(node->Attribute("majorVersion")));
		const uint8_t minorVersion = uint8_t(std::stoi(node->Attribute("minorVersion")));
		const uint8_t patchVersion = uint8_t(std::stoi(node->Attribute("patchVersion")));
		uint32_t version = (majorVersion << 16) + (minorVersion << 8) + patchVersion;
		versions.insert(std::make_pair(version, std::make_pair(QStringList(), QStringList())));
		for (tinyxml2::XMLElement * enhancement = node->FirstChildElement("Enhancement"); enhancement != nullptr; enhancement = enhancement->NextSiblingElement("Enhancement")) {
			if (enhancement->Attribute("language") != nullptr && enhancement->Attribute("language") == language) {
				versions[version].first.push_back(s2q(enhancement->GetText()));
			}
		}
		for (tinyxml2::XMLElement * bug = node->FirstChildElement("Bug"); bug != nullptr; bug = bug->NextSiblingElement("Bug")) {
			if (bug->Attribute("language") != nullptr && bug->Attribute("language") == language) {
				versions[version].second.push_back(s2q(bug->GetText()));
			}
		}
	}

	QString html;

	for (auto & p : versions) {
		const int majorVersion = (p.first / 256 / 256) % 256;
		const int minorVersion = (p.first / 256) % 256;
		const int patchVersion = p.first % 256;
		html += "<h1>Version " + QString::number(majorVersion) + "." + QString::number(minorVersion) + "." + QString::number(patchVersion) + "</h1>";

		if (!p.second.first.empty()) {
			html += "<h2>" + QApplication::tr("Enhancements") + ":</h2><ul>";

			for (const QString & en : p.second.first) {
				html += "<li>" + en;
			}

			html += "</ul>";
		}
		if (!p.second.second.empty()) {
			html += "<h2>" + QApplication::tr("Bugs") + ":</h2><ul>";

			for (const QString & b : p.second.second) {
				html += "<li>" + b;
			}

			html += "</ul>";
		}
	}
	_changelogBrowser->setHtml(html);

	return QDialog::exec();
}

void ChangelogDialog::changedShowState(int state) {
	Config::IniParser->setValue("CHANGELOGDIALOG/DontShowAgain", state == Qt::Checked);
}

void ChangelogDialog::restoreSettings() {
	const QByteArray arr = Config::IniParser->value("WINDOWGEOMETRY/ChangelogDialogGeometry", QByteArray()).toByteArray();
	if (!restoreGeometry(arr)) {
		Config::IniParser->remove("WINDOWGEOMETRY/ChangelogDialogGeometry");
	}
}

void ChangelogDialog::saveSettings() {
	Config::IniParser->setValue("WINDOWGEOMETRY/ChangelogDialogGeometry", saveGeometry());
}
