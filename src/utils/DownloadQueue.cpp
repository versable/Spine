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

#include "utils/DownloadQueue.h"

#include "utils/MultiFileDownloader.h"

#include <QMainWindow>

#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

using namespace spine::utils;

DownloadQueue * DownloadQueue::instance = nullptr;

DownloadQueue::DownloadQueue() : _taskbarButton(nullptr), _taskbarProgress(nullptr), _running(false) {
	instance = this;
	
#ifdef Q_OS_WIN
	_taskbarButton = new QWinTaskbarButton(this);

	_taskbarProgress = _taskbarButton->progress();
	_taskbarProgress->setMinimum(0);
	_taskbarProgress->setMaximum(100);
	_taskbarProgress->setValue(0);
	_taskbarProgress->hide();
#endif
}

DownloadQueue * DownloadQueue::getInstance() {
	return instance;
}

void DownloadQueue::add(MultiFileDownloader * downloader) {
	_totalBytesMap.insert(downloader, 0);
	_downloadedBytesMap.insert(downloader, 0);

	connect(downloader, &MultiFileDownloader::totalBytes, this, &DownloadQueue::updateTotalBytes);
	connect(downloader, &MultiFileDownloader::downloadProgress, this, &DownloadQueue::downloadedBytes);
	connect(downloader, &MultiFileDownloader::downloadSucceeded, this, &DownloadQueue::finishedDownload);
	connect(downloader, &MultiFileDownloader::downloadFailed, this, &DownloadQueue::finishedDownload);

	downloader->querySize();
}

void DownloadQueue::setWindow(QMainWindow * mainWindow) {
#ifdef Q_OS_WIN
	_taskbarButton->setWindow(mainWindow->windowHandle());
#endif
}

void DownloadQueue::checkQueue() {
	if (_queue.isEmpty()) return;

	if (_running) return;

	auto * downloader = _queue.dequeue();

	_running = true;
	
#ifdef Q_OS_WIN
	_taskbarProgress->setMaximum(static_cast<int>(_totalBytesMap[downloader] / 1024 + 1));
	_taskbarProgress->show();
#endif
	
	downloader->startDownload();
}

void DownloadQueue::updateTotalBytes(qint64 bytes) {
	MultiFileDownloader * downloader = dynamic_cast<MultiFileDownloader *>(sender());

	_totalBytesMap[downloader] = bytes;
	
	_queue.enqueue(downloader);

	checkQueue();
}

void DownloadQueue::downloadedBytes(qint64 bytes) {
	const MultiFileDownloader * downloader = dynamic_cast<const MultiFileDownloader *>(sender());

	_downloadedBytesMap[downloader] = bytes;
	
#ifdef Q_OS_WIN
	_taskbarProgress->setValue(static_cast<int>(bytes / 1024));
#endif
}

void DownloadQueue::finishedDownload() {
	const MultiFileDownloader * downloader = dynamic_cast<const MultiFileDownloader *>(sender());

	_totalBytesMap.remove(downloader);
	_downloadedBytesMap.remove(downloader);

	_running = false;

	sender()->deleteLater();
	
#ifdef Q_OS_WIN
	_taskbarProgress->hide();
#endif

	checkQueue();
}
