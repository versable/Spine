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

#include "widgets/ProfileView.h"

#include "SpineConfig.h"

#include "common/MessageStructs.h"

#include "discord/DiscordManager.h"

#include "gui/WaitSpinner.h"

#include "https/Https.h"

#include "utils/Config.h"
#include "utils/Conversion.h"
#include "utils/DownloadQueue.h"
#include "utils/FileDownloader.h"
#include "utils/MultiFileDownloader.h"

#include "widgets/AchievementView.h"
#include "widgets/HiddenAchievementsView.h"
#include "widgets/ProfileModView.h"
#include "widgets/UpdateLanguage.h"

#include "clockUtils/sockets/TcpSocket.h"

#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QtConcurrentRun>
#include <QTableView>
#include <QTabWidget>
#include <QVBoxLayout>

using namespace spine;
using namespace spine::discord;
using namespace spine::gui;
using namespace spine::https;
using namespace spine::utils;
using namespace spine::widgets;

ProfileView::ProfileView(QMainWindow * mainWindow, GeneralSettingsWidget * generalSettingsWidget, QWidget * par) : QWidget(par), _nameLabel(nullptr), _timeLabel(nullptr), _backToProfileButton(nullptr), _modnameLabel(nullptr), _achievmentDescriptionLabel(nullptr), _scrollArea(nullptr), _mainWidget(nullptr), _scrollLayout(nullptr), _achievementsWidget(nullptr), _achievementsLayout(nullptr), _scoresWidget(nullptr), _specialPage(false), _hidePatchesAndToolsBox(nullptr), _mainWindow(mainWindow), _waitSpinner(nullptr), _hiddenAchievementsView(nullptr) {
	QVBoxLayout * l = new QVBoxLayout();
	l->setAlignment(Qt::AlignTop);

	_nameLabel = new QLabel(QApplication::tr("NotLoggedIn"), this);
	connect(generalSettingsWidget, &GeneralSettingsWidget::languageChanged, [this](QString) {
		if (Config::Username.isEmpty()) {
			_nameLabel->setText(QApplication::tr("NotLoggedIn"));
		}
	});
	_nameLabel->setProperty("profileHeader", true);
	_timeLabel = new QLabel("", this);
	_timeLabel->setAlignment(Qt::AlignRight);
	_timeLabel->setProperty("profileHeader", true);
	QHBoxLayout * hbl = new QHBoxLayout();
	hbl->addWidget(_nameLabel, 0, Qt::AlignLeft);
	hbl->addWidget(_timeLabel, 0, Qt::AlignRight);
	l->addLayout(hbl);

	_backToProfileButton = new QPushButton(QApplication::tr("BackToProfile"), this);
	UPDATELANGUAGESETTEXT(_backToProfileButton, "BackToProfile");
	_backToProfileButton->hide();
	connect(_backToProfileButton, &QPushButton::released, this, &ProfileView::reset);
	connect(_backToProfileButton, &QPushButton::released, this, &ProfileView::updateList);

	l->addWidget(_backToProfileButton, 0, Qt::AlignLeft);

	_modnameLabel = new QLabel(this);
	_modnameLabel->setProperty("profileHeader", true);
	_modnameLabel->hide();

	l->addWidget(_modnameLabel, 0, Qt::AlignCenter);

	_scrollArea = new QScrollArea(this);
	_mainWidget = new QWidget(this);
	_scrollLayout = new QVBoxLayout();
	_scrollLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	_mainWidget->setLayout(_scrollLayout);
	_scrollArea->setWidget(_mainWidget);
	_scrollArea->setWidgetResizable(true);

	_scrollArea->hide();

	l->addWidget(_scrollArea);

	_achievmentDescriptionLabel = new QLabel(QApplication::tr("AchievementViewDescription"), this);
	UPDATELANGUAGESETTEXT(_achievmentDescriptionLabel, "AchievementViewDescription");
	l->addWidget(_achievmentDescriptionLabel);
	_achievmentDescriptionLabel->hide();
	_achievmentDescriptionLabel->setWordWrap(true);
	_achievmentDescriptionLabel->setAlignment(Qt::AlignCenter);

	_achievementScrollArea = new QScrollArea(this);
	_achievementsWidget = new QWidget(_achievementScrollArea);
	_achievementsLayout = new QVBoxLayout();
	_achievementsLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	_achievementsWidget->setLayout(_achievementsLayout);
	_achievementScrollArea->setWidget(_achievementsWidget);
	_achievementScrollArea->setWidgetResizable(true);
	_achievementScrollArea->hide();

	l->addWidget(_achievementScrollArea);

	_scoresWidget = new QTabWidget(this);
	_scoresWidget->hide();

	l->addWidget(_scoresWidget);

	QHBoxLayout* hl = new QHBoxLayout();
	_logoutButton = new QPushButton(QApplication::tr("Logout"), this);
	UPDATELANGUAGESETTEXT(_logoutButton, "Logout");
	connect(_logoutButton, &QPushButton::released, this, &ProfileView::triggerLogout);
	_logoutButton->setProperty("logout", true);

	_linkDiscordButton = new QPushButton(QApplication::tr("LinkDiscord"), this);
	UPDATELANGUAGESETTEXT(_linkDiscordButton, "LinkDiscord");
	connect(_linkDiscordButton, &QPushButton::released, this, &ProfileView::linkWithDiscord);
	_linkDiscordButton->hide();

	_unlinkDiscordButton = new QPushButton(QApplication::tr("UnlinkDiscord"), this);
	UPDATELANGUAGESETTEXT(_unlinkDiscordButton, "UnlinkDiscord");
	connect(_unlinkDiscordButton, &QPushButton::released, this, &ProfileView::unlinkFromDiscord);
	_unlinkDiscordButton->hide();

	_hidePatchesAndToolsBox = new QCheckBox(QApplication::tr("HidePatchesAndTools"), this);
	_hidePatchesAndToolsBox->setChecked(true);
	UPDATELANGUAGESETTEXT(_hidePatchesAndToolsBox, "HidePatchesAndTools");
	connect(_hidePatchesAndToolsBox, &QCheckBox::stateChanged, this, &ProfileView::toggledHidePatchesAndTools);

	hl->addWidget(_logoutButton, 0, Qt::AlignLeft);
	hl->addWidget(_linkDiscordButton, 0, Qt::AlignHCenter);
	hl->addWidget(_unlinkDiscordButton, 0, Qt::AlignHCenter);
	hl->addWidget(_hidePatchesAndToolsBox, 0, Qt::AlignRight);

	l->addLayout(hl);

	setLayout(l);

	qRegisterMetaType<int32_t>("int32_t");
	qRegisterMetaType<uint32_t>("uint32_t");
	qRegisterMetaType<std::vector<common::ModStats>>("std::vector<common::ModStats>");
	qRegisterMetaType<std::vector<common::SendAllAchievementStatsMessage::AchievementStats>>("std::vector<common::SendAllAchievementStatsMessage::AchievementStats>");
	qRegisterMetaType<std::vector<common::SendAllScoreStatsMessage::ScoreStats>>("std::vector<common::SendAllScoreStatsMessage::ScoreStats>");
	connect(this, &ProfileView::receivedMods, this, &ProfileView::updateModList);
	connect(this, &ProfileView::receivedAchievementStats, this, &ProfileView::updateAchievements);
	connect(this, &ProfileView::receivedScoreStats, this, &ProfileView::updateScores);
	connect(this, &ProfileView::receivedUserLevel, this, &ProfileView::updateUserLevel);
	
	connect(this, &ProfileView::discordLinkageStateReceived, this, &ProfileView::updateDiscordLinkage);

	connect(DiscordManager::instance(), &DiscordManager::connected, this, &ProfileView::requestDiscordLinkage);

	loginChanged();
}

ProfileView::~ProfileView() {
}

void ProfileView::updateList() {
	if (_specialPage) {
		_specialPage = false;
		return;
	}
	delete _waitSpinner;
	_waitSpinner = new WaitSpinner(QApplication::tr("LoadingProfile"), this);
	_achievmentDescriptionLabel->hide();
	_achievementScrollArea->hide();
	_scoresWidget->hide();
	_modnameLabel->hide();
	_backToProfileButton->hide();
	_nameLabel->show();
	_timeLabel->show();
	_scrollArea->show();
	QtConcurrent::run([this]() {
		clockUtils::sockets::TcpSocket sock;
		clockUtils::ClockError cErr = sock.connectToHostname("clockwork-origins.de", SERVER_PORT, 10000);
		if (clockUtils::ClockError::SUCCESS == cErr) {
			{
				common::RequestUserLevelMessage rulm;
				rulm.username = Config::Username.toStdString();
				rulm.password = Config::Password.toStdString();
				std::string serialized = rulm.SerializePublic();
				sock.writePacket(serialized);
				if (clockUtils::ClockError::SUCCESS == sock.receivePacket(serialized)) {
					try {
						common::Message * m = common::Message::DeserializePublic(serialized);
						if (m) {
							common::SendUserLevelMessage * sulm = dynamic_cast<common::SendUserLevelMessage *>(m);
							emit receivedUserLevel(sulm->level, sulm->currentXP, sulm->nextXP);
						}
						delete m;
					} catch (...) {
						return;
					}
				} else {
					qDebug() << "Error occurred: " << int(cErr);
				}
			}
			{
				common::RequestAllModStatsMessage ramsm;
				ramsm.username = Config::Username.toStdString();
				ramsm.password = Config::Password.toStdString();
				ramsm.language = Config::Language.toStdString();
				std::string serialized = ramsm.SerializePublic();
				sock.writePacket(serialized);
				if (clockUtils::ClockError::SUCCESS == sock.receivePacket(serialized)) {
					try {
						common::Message * m = common::Message::DeserializePublic(serialized);
						if (m) {
							common::SendAllModStatsMessage * samsm = dynamic_cast<common::SendAllModStatsMessage *>(m);
							emit receivedMods(samsm->mods);
						}
						delete m;
					} catch (...) {
						return;
					}
				} else {
					qDebug() << "Error occurred: " << int(cErr);
				}
			}
		}
	});
}

void ProfileView::setGothicDirectory(QString path) {
	_gothicDirectory = path;
}

void ProfileView::setGothic2Directory(QString path) {
	_gothic2Directory = path;
}

void ProfileView::reset() {
	_specialPage = false;
}

void ProfileView::loginChanged() {
	_nameLabel->setText(!Config::Username.isEmpty() ? Config::Username : QApplication::tr("NotLoggedIn"));
	_scrollArea->setVisible(!Config::Username.isEmpty());
	_logoutButton->setVisible(!Config::Username.isEmpty());

	requestDiscordLinkage();
}

void ProfileView::openAchievementView(int32_t modID, QString modName) {
	delete _waitSpinner;
	_waitSpinner = new WaitSpinner(QApplication::tr("LoadingAchievements"), this);
	_specialPage = true;
	_scrollArea->hide();
	_scoresWidget->hide();
	_nameLabel->hide();
	_timeLabel->hide();
	for (AchievementView * av : _achievements) {
		_achievementsLayout->removeWidget(av);
		av->deleteLater();
	}
	_achievements.clear();
	if (_hiddenAchievementsView) {
		_hiddenAchievementsView->deleteLater();
		_hiddenAchievementsView = nullptr;
	}
	_achievmentDescriptionLabel->show();
	_achievementScrollArea->show();
	_modnameLabel->setText(modName);
	_modnameLabel->show();
	_backToProfileButton->show();
	updateAchievements(-1, std::vector<common::SendAllAchievementStatsMessage::AchievementStats>());
	QtConcurrent::run([this, modID]() {
		common::RequestAllAchievementStatsMessage raasm;
		raasm.username = Config::Username.toStdString();
		raasm.password = Config::Password.toStdString();
		raasm.language = Config::Language.toStdString();
		raasm.modID = modID;
		std::string serialized = raasm.SerializePublic();
		clockUtils::sockets::TcpSocket sock;
		clockUtils::ClockError cErr = sock.connectToHostname("clockwork-origins.de", SERVER_PORT, 10000);
		if (clockUtils::ClockError::SUCCESS == cErr) {
			sock.writePacket(serialized);
			if (clockUtils::ClockError::SUCCESS == sock.receivePacket(serialized)) {
				try {
					common::Message * m = common::Message::DeserializePublic(serialized);
					if (m) {
						common::SendAllAchievementStatsMessage * saasm = dynamic_cast<common::SendAllAchievementStatsMessage *>(m);
						emit receivedAchievementStats(modID, saasm->achievements);
					}
					delete m;
				} catch (...) {
					return;
				}
			} else {
				qDebug() << "Error occurred: " << int(cErr);
			}
		}
	});
}

void ProfileView::openScoreView(int32_t modID, QString modName) {
	delete _waitSpinner;
	_waitSpinner = new WaitSpinner(QApplication::tr("LoadingScores"), this);
	_specialPage = true;
	_scrollArea->hide();
	_achievmentDescriptionLabel->hide();
	_nameLabel->hide();
	_timeLabel->hide();
	_achievementScrollArea->hide();
	while (_scoresWidget->tabBar()->count() > 0) {
		_scoresWidget->removeTab(0);
	}
	_scoresWidget->show();
	_modnameLabel->setText(modName);
	_modnameLabel->show();
	_backToProfileButton->show();
	QtConcurrent::run([this, modID]() {
		common::RequestAllScoreStatsMessage rassm;
		rassm.username = Config::Username.toStdString();
		rassm.password = Config::Password.toStdString();
		rassm.language = Config::Language.toStdString();
		rassm.modID = modID;
		std::string serialized = rassm.SerializePublic();
		clockUtils::sockets::TcpSocket sock;
		clockUtils::ClockError cErr = sock.connectToHostname("clockwork-origins.de", SERVER_PORT, 10000);
		if (clockUtils::ClockError::SUCCESS == cErr) {
			sock.writePacket(serialized);
			if (clockUtils::ClockError::SUCCESS == sock.receivePacket(serialized)) {
				try {
					common::Message * m = common::Message::DeserializePublic(serialized);
					if (m) {
						common::SendAllScoreStatsMessage * sassm = dynamic_cast<common::SendAllScoreStatsMessage *>(m);
						emit receivedScoreStats(sassm->scores);
					}
					delete m;
				} catch (...) {
					return;
				}
			} else {
				qDebug() << "Error occurred: " << int(cErr);
			}
		}
	});
}

void ProfileView::updateModList(std::vector<common::ModStats> mods) {
	int32_t overallDuration = 0;

	for (ProfileModView * pmv : _mods) {
		_scrollLayout->removeWidget(pmv);
		pmv->deleteLater();
	}
	_mods.clear();

	for (const common::ModStats & ms : mods) {
		ProfileModView * pmv = new ProfileModView(ms, _gothicDirectory, _gothic2Directory, _mainWidget);
		if (pmv->isPatchOrTool()) {
			pmv->setVisible(_hidePatchesAndToolsBox->checkState() == Qt::Unchecked);
		} else {
			overallDuration += ms.duration;
		}
		_scrollLayout->addWidget(pmv);
		_mods.append(pmv);
		connect(pmv, &ProfileModView::openAchievementView, this, &ProfileView::openAchievementView);
		connect(pmv, &ProfileModView::openScoreView, this, &ProfileView::openScoreView);
	}

	QString timeString;
	if (overallDuration == -1) {
		timeString = "-";
	} else {
		if (overallDuration > 90) {
			timeString = QString::number((overallDuration + 30) / 60) + " " + ((overallDuration >= 90) ? QApplication::tr("Hours") : QApplication::tr("Hour"));
		} else {
			timeString = QString::number(overallDuration) + " " + ((overallDuration > 1 || overallDuration == 0) ? QApplication::tr("Minutes") : QApplication::tr("Minute"));
		}
	}
	_timeLabel->setText(timeString);

	delete _waitSpinner;
	_waitSpinner = nullptr;
}

void ProfileView::updateUserLevel(uint32_t level, uint32_t currentXP, uint32_t nextXP) {
	_nameLabel->setText(Config::Username + " - " + QApplication::tr("Level") + " " + QString::number(level) + " (" + QString::number(currentXP) + " / " + QString::number(nextXP) + ")");
}

void ProfileView::updateAchievements(int32_t modID, std::vector<common::SendAllAchievementStatsMessage::AchievementStats> achievementStats) {
	std::sort(achievementStats.begin(), achievementStats.end(), [](common::SendAllAchievementStatsMessage::AchievementStats a, common::SendAllAchievementStatsMessage::AchievementStats b) {
		return a.unlockedPercent > b.unlockedPercent;
	});
	for (AchievementView * av : _achievements) {
		_achievementsLayout->removeWidget(av);
		av->deleteLater();
	}
	_achievements.clear();
	if (_hiddenAchievementsView) {
		_hiddenAchievementsView->deleteLater();
		_hiddenAchievementsView = nullptr;
	}
	QSet<QPair<QString, QString>> images;
	for (const auto & as : achievementStats) {
		if (!as.iconLocked.empty() && !as.iconLockedHash.empty()) {
			QString filename = QString::fromStdString(as.iconLocked);
			filename.chop(2); // remove .z
			if (!QFileInfo::exists(Config::DOWNLOADDIR + "/achievements/" + QString::number(modID) + "/" + filename)) {
				images.insert(QPair<QString, QString>(QString::fromStdString(as.iconLocked), QString::fromStdString(as.iconLockedHash)));
			}
		}
		if (!as.iconUnlocked.empty() && !as.iconUnlockedHash.empty()) {
			QString filename = QString::fromStdString(as.iconUnlocked);
			filename.chop(2); // remove .z
			if (!QFileInfo::exists(Config::DOWNLOADDIR + "/achievements/" + QString::number(modID) + "/" + filename)) {
				images.insert(QPair<QString, QString>(QString::fromStdString(as.iconUnlocked), QString::fromStdString(as.iconUnlockedHash)));
			}
		}
	}
	if (!images.isEmpty()) {
		MultiFileDownloader * mfd = new MultiFileDownloader(this);
		for (const auto & p : images) {
			QFileInfo fi(p.first);
			FileDownloader * fd = new FileDownloader(QUrl("https://clockwork-origins.de/Gothic/downloads/mods/" + QString::number(modID) + "/achievements/" + p.first), Config::DOWNLOADDIR + "/achievements/" + QString::number(modID) + "/" + fi.path(), fi.fileName(), p.second, mfd);
			mfd->addFileDownloader(fd);
		}

		connect(mfd, &MultiFileDownloader::downloadSucceeded, this, &ProfileView::updateAchievementIcons);

		DownloadQueue::getInstance()->add(mfd);
	}
	int32_t counter = 0;
	for (const auto & as : achievementStats) {
		AchievementView * av = new AchievementView(modID, as, _achievementsWidget);
		av->setVisible(!as.hidden || as.unlocked || as.canSeeHidden);
		counter += (!as.hidden || as.unlocked) ? 0 : 1;
		_achievementsLayout->addWidget(av);
		_achievements.push_back(av);
	}
	if (counter) {
		_hiddenAchievementsView = new HiddenAchievementsView(counter, _achievementsWidget);
		_achievementsLayout->addWidget(_hiddenAchievementsView);
	}

	delete _waitSpinner;
	_waitSpinner = nullptr;
}

void ProfileView::updateScores(std::vector<common::SendAllScoreStatsMessage::ScoreStats> scoreStats) {
	while (_scoresWidget->tabBar()->count() > 0) {
		_scoresWidget->removeTab(0);
	}
	_scoresWidget->show();
	for (const auto & score : scoreStats) {
		QTableView * scoreWidget = new QTableView(_scoresWidget);
		QStandardItemModel * m = new QStandardItemModel(scoreWidget);
		m->setHorizontalHeaderLabels(QStringList() << QApplication::tr("Rank") << QApplication::tr("Name") << QApplication::tr("Score"));
		int rank = 1;
		int lastRank = 1;
		int lastScore = 0;
		int row = 0;
		for (const auto & p : score.scores) {
			if (lastScore != p.second) {
				rank = lastRank;
			}
			m->appendRow(QList<QStandardItem *>() << new QStandardItem(QString::number(rank)) << new QStandardItem(QString::fromLatin1(p.first.c_str())) << new QStandardItem(QString::number(p.second)));
			m->item(row, 0)->setEnabled(QString::fromLatin1(p.first.c_str()) == Config::Username);
			m->item(row, 0)->setTextAlignment(Qt::AlignCenter);
			m->item(row, 1)->setEnabled(QString::fromLatin1(p.first.c_str()) == Config::Username);
			m->item(row, 1)->setTextAlignment(Qt::AlignCenter);
			m->item(row, 2)->setEnabled(QString::fromLatin1(p.first.c_str()) == Config::Username);
			m->item(row, 2)->setTextAlignment(Qt::AlignCenter);
			lastScore = p.second;
			lastRank++;
			row++;
		}
		scoreWidget->setModel(m);
		scoreWidget->verticalHeader()->hide();
		scoreWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
		scoreWidget->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
		scoreWidget->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
		scoreWidget->setProperty("score", true);
		_scoresWidget->addTab(scoreWidget, s2q(score.name));
	}

	delete _waitSpinner;
	_waitSpinner = nullptr;
}

void ProfileView::toggledHidePatchesAndTools() {
	for (ProfileModView * pmv : _mods) {
		if (pmv->isPatchOrTool()) {
			pmv->setVisible(_hidePatchesAndToolsBox->checkState() == Qt::Unchecked);
		}
	}
}

void ProfileView::updateAchievementIcons() {
	for (const auto & av : _achievements) {
		av->updateIcons();
	}
}

void ProfileView::linkWithDiscord() {
	if (Config::Username.isEmpty()) return;
	
	if (Config::Password.isEmpty()) return;

	if (!DiscordManager::instance()->isConnected()) return;

	QJsonObject requestData;
	requestData["username"] = Config::Username;
	requestData["password"] = Config::Password;

	const qint64 discordID = static_cast<qint64>(DiscordManager::instance()->getUserID());
	
	requestData["DiscordID"] = QString::number(discordID);
	
	https::Https::postAsync(19101, "linkToDiscord", QJsonDocument(requestData).toJson(QJsonDocument::Compact), [this](const QJsonObject & json, int statusCode) {
		if (statusCode != 200) return;
		
		if (!json.contains("Linked")) return;
		
		emit discordLinkageStateReceived(json["Linked"].toString() == "1");
	});
}

void ProfileView::unlinkFromDiscord() {
	if (Config::Username.isEmpty()) return;
	
	if (Config::Password.isEmpty()) return;

	if (!DiscordManager::instance()->isConnected()) return;

	QJsonObject requestData;
	requestData["username"] = Config::Username;
	requestData["password"] = Config::Password;
	
	https::Https::postAsync(19101, "unlinkFromDiscord", QJsonDocument(requestData).toJson(QJsonDocument::Compact), [this](const QJsonObject & json, int statusCode) {
		if (statusCode != 200) return;
		
		if (!json.contains("Linked")) return;
		
		emit discordLinkageStateReceived(json["Linked"].toString() == "1");
	});
}

void ProfileView::requestDiscordLinkage() {
	if (Config::Username.isEmpty()) return;
	
	if (Config::Password.isEmpty()) return;

	if (!DiscordManager::instance()->isConnected()) return;

	QJsonObject requestData;
	requestData["username"] = Config::Username;
	requestData["password"] = Config::Password;
	
	https::Https::postAsync(19101, "isLinkedWithDiscord", QJsonDocument(requestData).toJson(QJsonDocument::Compact), [this](const QJsonObject & json, int statusCode) {
		if (statusCode != 200) return;
		
		if (!json.contains("Linked")) return;
		
		emit discordLinkageStateReceived(json["Linked"].toString() == "1");
	});
}

void ProfileView::updateDiscordLinkage(bool linked) {
	_linkDiscordButton->setVisible(!linked);
	_unlinkDiscordButton->setVisible(linked);
}
