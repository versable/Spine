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

#include "FileDownloader.h"

#include "utils/Compression.h"
#include "utils/Config.h"
#include "utils/Conversion.h"
#include "utils/Hashing.h"

#include "utils/ErrorReporting.h"

#include "boost/iostreams/filter/zlib.hpp"

#include "clockUtils/log/Log.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcess>
#include <QSettings>
#include <QtConcurrentRun>

#include "zipper/unzipper.h"

#ifdef Q_OS_WIN
	#include <Windows.h>
	#include <shellapi.h>
#endif

using namespace spine::utils;

FileDownloader::FileDownloader(QUrl url, QString targetDirectory, QString fileName, QString hash, QObject * par) : QObject(par), _webAccessManager(new QNetworkAccessManager(this)), _url(url), _targetDirectory(targetDirectory), _fileName(fileName), _hash(hash), _filesize(-1), _outputFile(nullptr), _finished(false) {
}

FileDownloader::~FileDownloader() {
	delete _outputFile;
}

void FileDownloader::requestFileSize() {
	const QNetworkRequest request(_url);
	QNetworkReply * reply = _webAccessManager->head(request);
	reply->setReadBufferSize(Config::downloadRate * 8);
	connect(reply, &QNetworkReply::finished, this, &FileDownloader::determineFileSize);
	connect(this, &FileDownloader::abort, reply, &QNetworkReply::abort);
}

QString FileDownloader::getFileName() const {
	return _fileName;
}

void FileDownloader::startDownload() {
	if (Config::extendedLogging) {
		LOGINFO("Starting Download of file " << _fileName.toStdString() << " from " << _url.toString().toStdString());
	}
	QDir dir(_targetDirectory);
	if (!dir.exists()) {
		bool b = dir.mkpath(dir.absolutePath());
		if (!b) {
			emit downloadFinished();
			emit fileFailed(DownloadError::UnknownError);
			return;
		}
	}
	QString realName = _fileName;
	if (QFileInfo(realName).suffix() == "z") {
		realName.chop(2);
	}
	if (QFileInfo::exists(_targetDirectory + "/" + realName)) {
		QEventLoop hashLoop;
		QFutureWatcher<bool> watcher;
		connect(&watcher, &QFutureWatcher<bool>::finished, &hashLoop, &QEventLoop::quit);
		QFuture<bool> f = QtConcurrent::run([&]() {
			return utils::Hashing::checkHash(_targetDirectory + "/" + realName, _hash);
		});
		watcher.setFuture(f);
		hashLoop.exec();
		
		if (f.result()) {
			if (Config::extendedLogging) {
				LOGINFO("Skipping file as it already exists");
			}
			if (_filesize == -1) {
				_finished = true;
				
				QEventLoop loop;
				connect(this, &FileDownloader::fileSizeDetermined, &loop, &QEventLoop::quit);
				requestFileSize();
				loop.exec();
			}
			emit downloadProgress(_filesize);

			emit downloadFinished();
			emit fileSucceeded();
			return;
		}
	}

	if (_fileName.contains("directx_Jun2010_redist.exe", Qt::CaseInsensitive) && Config::IniParser->value("INSTALLATION/DirectX", true).toBool()) {
		emit downloadProgress(_filesize);

		emit downloadFinished();
		emit fileSucceeded();
		return;
	}
	_outputFile = new QFile(_targetDirectory + "/" + _fileName);
	if (!_outputFile->open(QIODevice::WriteOnly)) {
		if (Config::extendedLogging) {
			LOGINFO("Can't open file for output");
		}

		emit downloadFinished();
		emit fileFailed(DownloadError::UnknownError);
		return;
	}
	const QNetworkRequest request(_url);
	QNetworkReply * reply = _webAccessManager->get(request);
	reply->setReadBufferSize(Config::downloadRate * 8);
	connect(reply, &QNetworkReply::downloadProgress, this, &FileDownloader::updateDownloadProgress);
	connect(reply, &QNetworkReply::readyRead, this, &FileDownloader::writeToFile);
	connect(reply, &QNetworkReply::finished, this, &FileDownloader::fileDownloaded);
	connect(reply, &QNetworkReply::errorOccurred, this, &FileDownloader::networkError);
	connect(reply, &QNetworkReply::sslErrors, this, &FileDownloader::sslErrors);
	connect(this, &FileDownloader::abort, reply, &QNetworkReply::abort);
	emit startedDownload(_fileName);
}

void FileDownloader::updateDownloadProgress(qint64 bytesReceived, qint64) {
	emit downloadProgress(bytesReceived);
}

void FileDownloader::fileDownloaded() {
	QNetworkReply * reply = dynamic_cast<QNetworkReply *>(sender());

	emit downloadFinished();
	
	if (reply->error() == QNetworkReply::NetworkError::NoError) {
		if (Config::extendedLogging) {
			LOGINFO("Uncompressing file");
		}
		const QByteArray data = reply->readAll(); // the rest
		_outputFile->write(data);
		_outputFile->close();

		uncompressAndHash();		
	} else {
		_outputFile->close();
		_outputFile->remove();
		LOGERROR("Unknown Error: " << reply->error() << ", " << q2s(reply->errorString()));
		if (reply->error() != QNetworkReply::OperationCanceledError) {
			utils::ErrorReporting::report(QString("Unknown Error during download: %1, %2 (%3)").arg(reply->error()).arg(reply->errorString()).arg(_url.toString()));
		}
		emit fileFailed(DownloadError::UnknownError);
	}
	reply->deleteLater();
}

void FileDownloader::determineFileSize() {
	const qlonglong filesize = dynamic_cast<QNetworkReply *>(sender())->header(QNetworkRequest::ContentLengthHeader).toLongLong();
	sender()->deleteLater();
	_filesize = filesize;

	if (_finished) {
		emit fileSizeDetermined();
	} else {
		emit totalBytes(_filesize);
	}
}

void FileDownloader::writeToFile() {
	QNetworkReply * reply = dynamic_cast<QNetworkReply *>(sender());
	const QByteArray data = reply->readAll();
	_outputFile->write(data);
	const QFileDevice::FileError err = _outputFile->error();
	if (err != QFileDevice::NoError) {
		reply->abort();
		emit downloadFinished();
		emit fileFailed((err == QFileDevice::ResizeError || err == QFileDevice::ResourceError) ? DownloadError::DiskSpaceError : DownloadError::UnknownError);
	}
}

void FileDownloader::networkError(QNetworkReply::NetworkError err) {
	emit downloadFinished();
	
	if (err == QNetworkReply::NetworkError::OperationCanceledError) {
		emit fileFailed(DownloadError::CanceledError);
	} else {
		emit fileFailed(DownloadError::NetworkError);
	}
}

void FileDownloader::sslErrors(const QList<QSslError> & errors) {
	for (const auto & err : errors) {
		LOGINFO(err.error() << " - " << q2s(err.errorString()));
	}
}

void FileDownloader::uncompressAndHash() {
	QtConcurrent::run([this]() {
		QFileInfo fi(_fileName);
		const QString fileNameBackup = _fileName;

		auto suffix = fi.suffix();
		
		// compressed files always end with .z
		// in this case, uncompress, drop file extension and proceeed		
		if (suffix.compare("z", Qt::CaseInsensitive) == 0) {
			try {
				utils::Compression::uncompress(_targetDirectory + "/" + _fileName, true); // remove compressed download now
			} catch (boost::iostreams::zlib_error & e) {
				LOGERROR("Exception: " << e.what());
				utils::ErrorReporting::report(QString("Uncompressing of %1 failed: %2 (%3)").arg(_fileName).arg(e.what()).arg(_url.toString()));
			}
			_fileName.chop(2);
		}

		fi = QFileInfo(_fileName);
		suffix = fi.suffix();

		// zip case
		if (suffix.compare("zip", Qt::CaseInsensitive) == 0) {
			handleZip();
			
			return;
		}
		
		const bool b = utils::Hashing::checkHash(_targetDirectory + "/" + _fileName, _hash);
		if (b) {
			if (_fileName.startsWith("vc") && _fileName.endsWith(".exe")) {
				handleVcRedist();
			} else if (_fileName == "directx_Jun2010_redist.exe") {
				handleDirectX();
			} else {
				emit fileSucceeded();
			}
		} else {
			LOGERROR("Hash invalid: " << _fileName.toStdString());
			emit fileFailed(DownloadError::HashError);
			utils::ErrorReporting::report(QString("Hash invalid: %1 (%2)").arg(_fileName).arg(_url.toString()));
		}
	});
}

void FileDownloader::handleZip() {
	const auto fullPath = _targetDirectory + "/" + _fileName;
	{
		const bool b = utils::Hashing::checkHash(fullPath, _hash);
		if (!b) {
			LOGERROR("Hash invalid: " << _fileName.toStdString());
			emit fileFailed(DownloadError::HashError);
			utils::ErrorReporting::report(QString("Hash invalid: %1 (%2)").arg(_fileName).arg(_url.toString()));
			return;
		}
	}

	{
		zipper::Unzipper unzipper(q2s(fullPath));
		const bool b = unzipper.extract(q2s(_targetDirectory));
		if (!b) {
			LOGERROR("Unzipping failed: " << _fileName.toStdString());
			emit fileFailed(DownloadError::UnknownError);
			utils::ErrorReporting::report(QString("Unzipping failed: %1 (%2)").arg(_fileName).arg(_url.toString()));
			return;
		}				
	}

	QFile(fullPath).remove();

	if (!QFileInfo::exists(_targetDirectory + "/.manifest")) {
		LOGERROR("Archive doesn't contain manifest: " << _fileName.toStdString());
		emit fileFailed(DownloadError::UnknownError);
		utils::ErrorReporting::report(QString("Archive doesn't contain manifest: %1 (%2)").arg(_fileName).arg(_url.toString()));
		return;
	}

	QFile manifest(_targetDirectory + "/.manifest");
	if (!manifest.open(QIODevice::ReadOnly)) {
		LOGERROR("Manifest can't be opened: " << _fileName.toStdString());
		emit fileFailed(DownloadError::UnknownError);
		return;
	}

	QList<QPair<QString, QString>> files;
	
	QTextStream ts(&manifest);

	while (!ts.atEnd()) {
		const QString line = ts.readLine();
		
		if (line.isEmpty()) continue;

		const auto split = line.split(";");

		if (split.count() != 2) continue;

		files.append(qMakePair(split[0], split[1]));
	}
	manifest.close();

	QFile(_targetDirectory + "/.manifest").remove();

	emit unzippedArchive(_fileName + ".z", files);
	emit fileSucceeded();
}

void FileDownloader::handleVcRedist() {
#ifdef Q_OS_WIN
	if (Config::extendedLogging) {
		LOGINFO("Starting Visual Studio Redistributable");
	}
	SHELLEXECUTEINFO shExecInfo = { 0 };
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExecInfo.hwnd = nullptr;
	shExecInfo.lpVerb = "runas";
	char file[1024];
	QString qf = _targetDirectory + "/" + _fileName;
	qf = qf.replace("\0", "");
	strcpy(file, qf.toUtf8().constData());
	shExecInfo.lpFile = file;
	shExecInfo.lpParameters = "/q /norestart";
	char directory[1024];
	strcpy(directory, _targetDirectory.replace("\0", "").toUtf8().constData());
	shExecInfo.lpDirectory = directory;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = nullptr;
	ShellExecuteEx(&shExecInfo);
	const int result = WaitForSingleObject(shExecInfo.hProcess, INFINITE);
	if (result != 0) {
		LOGERROR("Execute failed: " << _fileName.toStdString());
		emit fileFailed(DownloadError::UnknownError);
	} else {
		if (Config::extendedLogging) {
			LOGINFO("Download succeeded");
		}
		emit fileSucceeded();
	}
#endif
}

void FileDownloader::handleDirectX() {
#ifdef Q_OS_WIN
	if (Config::extendedLogging) {
		LOGINFO("Starting DirectX Redistributable");
	}
	bool dxSuccess = true;
	{
		SHELLEXECUTEINFO shExecInfo = { 0 };
		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		shExecInfo.hwnd = nullptr;
		shExecInfo.lpVerb = "runas";
		char file[1024];
		QString qf = _targetDirectory + "/" + _fileName;
		qf = qf.replace("\0", "");
		strcpy(file, qf.toUtf8().constData());
		shExecInfo.lpFile = file;
		char parameters[1024];
		qf = ("/Q /T:\"" + _targetDirectory + "/directX\"");
		qf = qf.replace("\0", "");
		strcpy(parameters, qf.toUtf8().constData());
		shExecInfo.lpParameters = parameters;
		char directory[1024];
		strcpy(directory, _targetDirectory.replace("\0", "").toUtf8().constData());
		shExecInfo.lpDirectory = directory;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = nullptr;
		ShellExecuteEx(&shExecInfo);
		const int result = WaitForSingleObject(shExecInfo.hProcess, INFINITE);
		if (result != 0) {
			dxSuccess = false;
			LOGERROR("Execute failed: " << _fileName.toStdString());
			emit fileFailed(DownloadError::UnknownError);
		}
	}
	if (dxSuccess) {
		SHELLEXECUTEINFO shExecInfo = { 0 };
		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		shExecInfo.hwnd = nullptr;
		shExecInfo.lpVerb = "runas";
		char file[1024];
		QString qf = (_targetDirectory + "/directX/DXSETUP.exe");
		qf = qf.replace("\0", "");
		strcpy(file, qf.toUtf8().constData());
		shExecInfo.lpFile = file;
		shExecInfo.lpParameters = "/silent";
		char directory[1024];
		qf = (_targetDirectory + "/directX");
		qf = qf.replace("\0", "");
		strcpy(directory, qf.toUtf8().constData());
		shExecInfo.lpDirectory = directory;
		shExecInfo.nShow = SW_SHOWNORMAL;
		shExecInfo.hInstApp = nullptr;
		ShellExecuteEx(&shExecInfo);
		const int result = WaitForSingleObject(shExecInfo.hProcess, INFINITE);
		if (result != 0) {
			dxSuccess = false;
			LOGERROR("Execute failed: " << _fileName.toStdString());
			emit fileFailed(DownloadError::UnknownError);
		}
	}
	if (dxSuccess) {
		if (Config::extendedLogging) {
			LOGINFO("Download succeeded");
		}
		emit fileSucceeded();
	}
	QDir(_targetDirectory + "/directX/").removeRecursively();
	Config::IniParser->setValue("INSTALLATION/DirectX", true);
#endif
}
