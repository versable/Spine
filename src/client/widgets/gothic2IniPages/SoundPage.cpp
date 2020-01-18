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

#include "widgets/gothic2IniPages/SoundPage.h"

#include <QApplication>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

using namespace spine::widgets::g2;

SoundPage::SoundPage(QSettings * iniParser, QWidget * par) : QWidget(par), _iniParser(iniParser) {
	QVBoxLayout * l = new QVBoxLayout();

	{
		QHBoxLayout * hl = new QHBoxLayout();

		{
			QGroupBox * soundBox = new QGroupBox(QApplication::tr("Sound"), this);

			QGridLayout * gl = new QGridLayout();
			gl->setAlignment(Qt::AlignTop);

			QLabel * lbl = new QLabel("soundEnabled", soundBox);
			lbl->setToolTip(QApplication::tr("soundEnabledTooltip"));
			_soundEnabled = new QComboBox(soundBox);
			_soundEnabled->addItems(QStringList() << QApplication::tr("Off") << QApplication::tr("On"));
			gl->addWidget(lbl, 0, 0);
			gl->addWidget(_soundEnabled, 0, 1);

			lbl = new QLabel("musicEnabled", soundBox);
			lbl->setToolTip(QApplication::tr("musicEnabledTooltip"));
			_musicEnabled = new QComboBox(soundBox);
			_musicEnabled->addItems(QStringList() << QApplication::tr("Off") << QApplication::tr("On"));
			gl->addWidget(lbl, 1, 0);
			gl->addWidget(_musicEnabled, 1, 1);

			lbl = new QLabel("zMusic16ChannelsOnly", soundBox);
			lbl->setToolTip(QApplication::tr("zMusic16ChannelsOnlyTooltip"));
			_zMusic16ChannelsOnly = new QComboBox(soundBox);
			_zMusic16ChannelsOnly->addItems(QStringList() << QApplication::tr("Off") << QApplication::tr("On"));
			gl->addWidget(lbl, 2, 0);
			gl->addWidget(_zMusic16ChannelsOnly, 2, 1);

			soundBox->setLayout(gl);

			hl->addWidget(soundBox);
		}

		{
			QGroupBox * volumeBox = new QGroupBox(QApplication::tr("Volume"), this);

			QGridLayout * gl = new QGridLayout();
			gl->setAlignment(Qt::AlignTop);

			QLabel * lbl = new QLabel("soundVolume", volumeBox);
			lbl->setToolTip(QApplication::tr("musicVolumeTooltip"));
			_soundVolume = new QDoubleSpinBox(volumeBox);
			_soundVolume->setMinimum(0.0);
			_soundVolume->setMaximum(1.0);
			gl->addWidget(lbl, 0, 0);
			gl->addWidget(_soundVolume, 0, 1);

			lbl = new QLabel("musicVolume", volumeBox);
			lbl->setToolTip(QApplication::tr("musicVolumeTooltip"));
			_musicVolume = new QDoubleSpinBox(volumeBox);
			_musicVolume->setMinimum(0.0);
			_musicVolume->setMaximum(1.0);
			gl->addWidget(lbl, 1, 0);
			gl->addWidget(_musicVolume, 1, 1);

			volumeBox->setLayout(gl);

			hl->addWidget(volumeBox);
		}

		{
			QGroupBox * controlsAndHotkeysBox = new QGroupBox(QApplication::tr("Reverbation"), this);

			QGridLayout * gl = new QGridLayout();
			gl->setAlignment(Qt::AlignTop);

			QLabel * lbl = new QLabel("soundUseReverb", controlsAndHotkeysBox);
			lbl->setToolTip(QApplication::tr("soundUseReverbTooltip"));
			_soundUseReverb = new QComboBox(controlsAndHotkeysBox);
			_soundUseReverb->addItems(QStringList() << QApplication::tr("Off") << QApplication::tr("On"));
			gl->addWidget(lbl, 0, 0);
			gl->addWidget(_soundUseReverb, 0, 1);

			lbl = new QLabel("useSpeechReverbLevel", controlsAndHotkeysBox);
			lbl->setToolTip(QApplication::tr("useSpeechReverbLevelTooltip"));
			_useSpeechReverbLevel = new QComboBox(controlsAndHotkeysBox);
			_useSpeechReverbLevel->addItems(QStringList() << QApplication::tr("Off") << QApplication::tr("Slight") << QApplication::tr("Full"));
			gl->addWidget(lbl, 1, 0);
			gl->addWidget(_useSpeechReverbLevel, 1, 1);

			controlsAndHotkeysBox->setLayout(gl);

			hl->addWidget(controlsAndHotkeysBox);
		}

		l->addLayout(hl);
	}

	{
		QHBoxLayout * hl = new QHBoxLayout();

		{
			QGroupBox * inventoryBox = new QGroupBox(QApplication::tr("DolbySurroundAndIntelRSX"), this);

			QGridLayout * gl = new QGridLayout();
			gl->setAlignment(Qt::AlignTop);

			QLabel * lbl = new QLabel("extendedProviders", inventoryBox);
			lbl->setToolTip(QApplication::tr("extendedProvidersTooltip"));
			_extendedProviders = new QComboBox(inventoryBox);
			_extendedProviders->addItems(QStringList() << QApplication::tr("Off") << QApplication::tr("On"));
			gl->addWidget(lbl, 0, 0);
			gl->addWidget(_extendedProviders, 0, 1);

			inventoryBox->setLayout(gl);

			hl->addWidget(inventoryBox);
		}

		l->addLayout(hl);
	}

	setLayout(l);

	reject();
}

SoundPage::~SoundPage() {
}

void SoundPage::reject() {
	// Sound
	int idx;
	double d;
	idx = _iniParser->value("SOUND/soundEnabled", 1).toInt();
	_soundEnabled->setCurrentIndex(idx);
	idx = _iniParser->value("SOUND/musicEnabled", 1).toInt();
	_musicEnabled->setCurrentIndex(idx);
	idx = _iniParser->value("ENGINE/zMusic16ChannelsOnly", 0).toInt();
	_zMusic16ChannelsOnly->setCurrentIndex(idx);

	// Volume
	d = _iniParser->value("SOUND/soundVolume", 1.0).toDouble();
	_soundVolume->setValue(d);
	d = _iniParser->value("SOUND/musicVolume", 1.0).toDouble();
	_musicVolume->setValue(d);

	// Reverbation
	idx = _iniParser->value("SOUND/soundUseReverb", 1).toInt();
	_soundUseReverb->setCurrentIndex(idx);
	idx = _iniParser->value("GAME/useSpeechReverbLevel", 0).toInt();
	_useSpeechReverbLevel->setCurrentIndex(idx);

	// Dolby Surround und Intel RSX
	idx = _iniParser->value("SOUND/extendedProviders", 0).toInt();
	_extendedProviders->setCurrentIndex(idx);
}

void SoundPage::accept() {
	// Sound
	_iniParser->setValue("SOUND/soundEnabled", _soundEnabled->currentIndex());
	_iniParser->setValue("SOUND/musicEnabled", _musicEnabled->currentIndex());
	_iniParser->setValue("ENGINE/zMusic16ChannelsOnly", _zMusic16ChannelsOnly->currentIndex());

	// Volume
	_iniParser->setValue("SOUND/soundVolume", _soundVolume->value());
	_iniParser->setValue("SOUND/musicVolume", _musicVolume->value());

	// Reverbation
	_iniParser->setValue("SOUND/soundUseReverb", _soundUseReverb->currentIndex());
	_iniParser->setValue("GAME/useSpeechReverbLevel", _useSpeechReverbLevel->currentIndex());

	// Dolby Surround und Intel RSX
	_iniParser->setValue("SOUND/extendedProviders", _extendedProviders->currentIndex());
}

void SoundPage::updateSettings(QSettings * iniParser) {
	_iniParser = iniParser;

	reject();
}
