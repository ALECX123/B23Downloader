// Created by voidzero <vooidzero.github@qq.com>

#include "DownloadTask.h"
#include "Extractor.h"
#include "Network.h"
#include "utils.h"
#include "Flv.h"
#include <QApplication>
#include <QtNetwork>

// 127: 8K 超高清
// 126: 杜比视界
// 125: HDR 真彩
static QMap<int, QString> videoQnDescMap{
	{120, "4K 超清"},
	{116, "1080P 60帧"},
	{112, "1080P 高码率"},
	{80, "1080P 高清"},
	{74, "720P 60帧"},
	{64, "720P 高清"},
	{32, "480P 清晰"},
	{16, "360P 流畅"},
};

static QMap<int, QString> liveQnDescMap{
	{10000, "原画"},
	{400, "蓝光"},
	{250, "超清"},
	{150, "高清"},
	{80, "流畅"}
};


inline bool jsonValue2Bool(const QJsonValue& val, bool defaultVal = false) {
	if (val.isNull() || val.isUndefined()) {
		return defaultVal;
	}
	if (val.isBool()) {
		return val.toBool();
	}
	if (val.isDouble()) {
		return static_cast<bool>(val.toInt());
	}
	if (val.isString()) {
		auto str = val.toString().toLower();
		if (str == "1" || str == "true") {
			return true;
		}
		if (str == "0" || str == "false") {
			return false;
		}
	}
	return defaultVal;
}


AbstractDownloadTask::~AbstractDownloadTask()
{
	if (httpReply != nullptr) {
		httpReply->abort();
	}
}

QString AbstractDownloadTask::getTitle() const
{
	return QFileInfo(path).baseName();
}

AbstractDownloadTask* AbstractDownloadTask::fromJsonObj(const QJsonObject& json)
{
	int type = json["type"].toInt(-1);
	switch (type) {
	case static_cast<int>(ContentType::PGC):
		return new PgcDownloadTask(json);
	case static_cast<int>(ContentType::PUGV):
		return new PugvDownloadTask(json);
	case static_cast<int>(ContentType::UGC):
		return new UgcDownloadTask(json);
	case static_cast<int>(ContentType::Comic):
		return new ComicDownloadTask(json);
	}
	return nullptr;
}

QJsonValue AbstractDownloadTask::getReplyJson(const QString& dataKey)
{
	auto reply = this->httpReply;
	this->httpReply = nullptr;
	reply->deleteLater();

	// abort() is called.
	if (reply->error() == QNetworkReply::OperationCanceledError) {
		return QJsonValue();
	}

	const auto [json, errorString] = Network::Bili::parseReply(reply, dataKey);

	if (!errorString.isNull()) {
		emit errorOccurred(errorString);
		return QJsonValue();
	}

	if (dataKey.isEmpty()) {
		return json;
	}
	else {
		return json[dataKey];
	}
}



AbstractVideoDownloadTask::~AbstractVideoDownloadTask() = default;

qint64 AbstractVideoDownloadTask::getDownloadedBytesCnt() const
{
	return VideoDownloadedBytesCnt + AudioDownloadedBytesCnt;
}

void AbstractVideoDownloadTask::startDownload()
{
	httpReply = getPlayUrlInfo();
	connect(httpReply, &QNetworkReply::finished, this, [this] {
		auto data = getReplyJson(getPlayUrlInfoDataKey()).toObject();
		if (data.isEmpty()) {
			return;
		}
		parsePlayUrlInfo(data);
		});
}

void AbstractVideoDownloadTask::stopDownload()
{
	if (httpReply != nullptr) {
		httpReply->abort();
	}
}



QJsonObject VideoDownloadTask::toJsonObj() const
{
	return QJsonObject{
		{"path", path},
		{"qn", qn},
		{"video_bytes", VideoDownloadedBytesCnt},
		{"audio_bytes", AudioDownloadedBytesCnt},

		{"video_total", VideoTotalBytesCnt},
		{"audio_total", AudioTotalBytesCnt}
	};
}

VideoDownloadTask::VideoDownloadTask(const QJsonObject& json)
	: AbstractVideoDownloadTask(json["path"].toString(), json["qn"].toInt())
{
	VideoDownloadedBytesCnt = json["video_bytes"].toInteger(0);
	AudioDownloadedBytesCnt = json["audio_bytes"].toInteger(0);

	VideoTotalBytesCnt = json["video_total"].toInteger(0);
	AudioTotalBytesCnt = json["audio_total"].toInteger(0);
	connect(this, &VideoDownloadTask::Merge2Mp4Signal, this, &VideoDownloadTask::Merge2Mp4);
}

void VideoDownloadTask::removeFile()
{
	QFile::remove(path);
}

int VideoDownloadTask::estimateRemainingSeconds(qint64 downBytesPerSec) const
{
	if (downBytesPerSec == 0 || VideoTotalBytesCnt == 0 || AudioTotalBytesCnt == 0) {
		return -1;
	}
	qint64 ret = (VideoTotalBytesCnt + AudioTotalBytesCnt - VideoDownloadedBytesCnt - AudioDownloadedBytesCnt) / downBytesPerSec;
	return (ret > INT32_MAX ? -1 : static_cast<int>(ret));
}

double VideoDownloadTask::getProgress() const
{
	if (VideoTotalBytesCnt == 0 || AudioTotalBytesCnt == 0) {
		return 0;
	}
	return static_cast<double>(VideoDownloadedBytesCnt + AudioDownloadedBytesCnt) / (VideoTotalBytesCnt + AudioTotalBytesCnt);
}

QString VideoDownloadTask::getProgressStr() const
{
	if (VideoTotalBytesCnt == 0 || AudioDownloadedBytesCnt == 0) {
		return QString();
	}
	return QStringLiteral("%1/%2").arg(
		Utils::formattedDataSize(VideoDownloadedBytesCnt + AudioDownloadedBytesCnt),
		Utils::formattedDataSize(VideoTotalBytesCnt + AudioTotalBytesCnt)
	);
}

QnList VideoDownloadTask::getAllPossibleQn()
{
	return videoQnDescMap.keys();
}

QString VideoDownloadTask::getQnDescription(int qn)
{
	return videoQnDescMap.value(qn);
}

QString VideoDownloadTask::getQnDescription() const
{
	return getQnDescription(qn);
}

QnInfo VideoDownloadTask::getQnInfoFromPlayUrlInfo(const QJsonObject& data)
{
	QnInfo qnInfo;
	for (auto&& fmtValR : data["support_formats"].toArray()) {
		auto fmtObj = fmtValR.toObject();
		auto qn = fmtObj["quality"].toInt();
		auto desc = fmtObj["new_description"].toString();
		qnInfo.qnList.append(qn);
		if (videoQnDescMap.value(qn) != desc) {
			videoQnDescMap.insert(qn, desc);
		}
	}
	qnInfo.currentQn = data["quality"].toInt();
	return qnInfo;
}

bool VideoDownloadTask::checkQn(int qnFromReply)
{
	if (qnFromReply != qn) {
		if (VideoDownloadedBytesCnt == 0 || AudioDownloadedBytesCnt == 0) {
			qn = qnFromReply;
		}
		else {
			emit errorOccurred("获取到画质与已下载部分不同. 请确定登录/会员状态");
			return false;
		}
	}

	return true;
}

namespace {

	struct SegmentBase
	{
		QString initialization;
		QString indexRange;
	};

	struct VideoBaseUrl
	{
		int id = 0;
		QUrl baseUrl;
		int codecId = 0;
		SegmentBase segmentBase;
	};

	struct AudioBaseUrl
	{
		int id = 0;
		QUrl baseUrl;
		SegmentBase segmentBase;
	};

} // anonymous namespace

void VideoDownloadTask::parsePlayUrlInfo(const QJsonObject& data)
{
	if (jsonValue2Bool(data["is_preview"], false)) {
		if (!jsonValue2Bool(data["has_paid"], true)) {
			emit errorOccurred("该视频需要大会员/付费");
			return;
		}
	}

	auto qnInfo = getQnInfoFromPlayUrlInfo(data);
	if (!checkQn(qnInfo.currentQn)) {
		return;
	}

	auto video = data["dash"]["video"].toArray();
	auto audio = data["dash"]["audio"].toArray();
	//QJsonDocument document;
	//document.setObject(data);
	//QByteArray byteArray = document.toJson(QJsonDocument::Compact);
	//QString strJson(byteArray);
	//qDebug() << strJson;

	/*视频编码代码
		值	含义	备注
		7	AVC 编码	8K 视频不支持该格式
		12	HEVC 编码
		13	AV1 编码

	视频伴音音质代码
		值	含义
		30216	64K
		30232	132K
		30280	192K
		30250	杜比全景声
		30251	Hi - Res无损*/
	QList<VideoBaseUrl> videoBaseUrlList;
	QList<AudioBaseUrl> audioBaseUrlList;

	for (const auto& valueRef : video) {
		const auto value = valueRef.toObject();
		VideoBaseUrl vbu;
		vbu.id = value["id"].toInt();
		vbu.codecId = value["codecid"].toInt();
		if (vbu.codecId == 12) // skip HEVC
			continue;
		vbu.baseUrl = value["baseUrl"].toString();
		const auto sbObj = value["SegmentBase"].toObject();
		vbu.segmentBase.indexRange = sbObj["indexRange"].toString();
		vbu.segmentBase.initialization = sbObj["Initialization"].toString();
		videoBaseUrlList.append(vbu);
	}
	for (const auto& valueRef : audio) {
		const auto value = valueRef.toObject();
		AudioBaseUrl vbu;
		vbu.id = value["id"].toInt();
		vbu.baseUrl = value["baseUrl"].toString();
		const auto sbObj = value["SegmentBase"].toObject();
		vbu.segmentBase.indexRange = sbObj["indexRange"].toString();
		vbu.segmentBase.initialization = sbObj["Initialization"].toString();
		audioBaseUrlList.append(vbu);
	}

	if (video.isEmpty()) {
		emit errorOccurred("请求错误: video 为空");
		return;
	}

	QUrl videoStreamUrl;
	QUrl audioStreamUrl;
	for (const auto& entry : videoBaseUrlList) {
		if (entry.id == qnInfo.currentQn) {
			videoStreamUrl = entry.baseUrl;
			break;
		}
	}
	for (const auto& entry : audioBaseUrlList) {
		if (entry.id == qnInfo.currentQn + 30200) {
			audioStreamUrl = entry.baseUrl;
			break;
		}
	}
	if (videoStreamUrl.isEmpty() || audioStreamUrl.isEmpty()) {
		emit errorOccurred("url empty");
		return;
	}

	VideoDownloadedBytesCnt = 0;
	AudioDownloadedBytesCnt = 0;
	startDownloadStream(videoStreamUrl, false);
	startDownloadStream(audioStreamUrl, true);
}

std::unique_ptr<QFile> VideoDownloadTask::openFileForWrite(QString filePath)
{
	auto file = openFileForWriteImpl(filePath);
	if (!file)
		return nullptr;
	return file;
}

std::unique_ptr<QFile> VideoDownloadTask::openFileForWriteImpl(const QString& filePath)
{
	auto dir = QFileInfo(filePath).absolutePath();
	qDebug() << filePath;
	if (!QFileInfo::exists(dir)) {
		if (!QDir().mkpath(dir)) {
			qDebug() << dir;
			emit errorOccurred("创建目录失败");
			return nullptr;
		}
	}

	auto file = std::make_unique<QFile>(filePath);
	if (!file->open(QIODevice::ReadWrite)) {
		emit errorOccurred("打开文件失败");
		return nullptr;
	}
	return file;
}

void VideoDownloadTask::startDownloadStream(const QUrl& url, bool audio)
{
	emit getUrlInfoFinished();

	// check extension of filename
	auto ext = Utils::fileExtension(url.fileName());
	if (audio) {
		if (AudioDownloadedBytesCnt == 0 && !m4sAudioPath.endsWith(ext, Qt::CaseInsensitive))
		{
			m4sAudioPath = path + "Audio";
			m4sAudioPath.append(ext);
		}
		m4sAudiofile = openFileForWrite(m4sAudioPath);
		if (!m4sAudiofile)
			return;
	}
	else
	{
		if (VideoDownloadedBytesCnt == 0 && !m4sVideoPath.endsWith(ext, Qt::CaseInsensitive))
		{
			m4sVideoPath = path + "Video";
			m4sVideoPath.append(ext);
		}
		m4sVideofile = openFileForWrite(m4sVideoPath);
		if (!m4sVideofile)
			return;
	}
	auto request = Network::Bili::Request(url);

	if (audio)
	{
		audiohttpReply = Network::accessManager()->get(request);
		connect(audiohttpReply, &QNetworkReply::readyRead, this, &VideoDownloadTask::onAudioStreamReadyRead);
		connect(audiohttpReply, &QNetworkReply::finished, this, &VideoDownloadTask::onAudioStreamFinished);
	}
	else
	{
		videohttpReply = Network::accessManager()->get(request);
		connect(videohttpReply, &QNetworkReply::readyRead, this, &VideoDownloadTask::onVideoStreamReadyRead);
		connect(videohttpReply, &QNetworkReply::finished, this, &VideoDownloadTask::onVideoStreamFinished);
	}
}
void VideoDownloadTask::Merge2Mp4()
{
	if (!videodownload || !audiodownload)
	{
		return;
	}

	m4sAudiofile.reset();
	m4sVideofile.reset();
	QString wz = " -i \"" + m4sAudioPath + "\" -i \"" + m4sVideoPath + "\" -c:v copy -c:a copy -f mp4 -y \"" + path + ".mp4\"";
	QProcess* process = new QProcess(this);
	connect(process, &QProcess::readyReadStandardOutput, this, [process]() {
		QString s = process->readAllStandardOutput();
		qDebug() << s;
		});
	connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
		QFile::remove(m4sAudioPath);
		QFile::remove(m4sVideoPath);
		emit downloadFinished();
		process->deleteLater();
		});

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment(); //获取系统完整环境变量
	env.insert("PATH", env.value("PATH") + "; " + QCoreApplication::applicationDirPath()); //在已有的环境变量中附加新的绝对路径，注意分号“；”
	process->setProcessEnvironment(env);
	process->setWorkingDirectory(QCoreApplication::applicationDirPath());
	QString str = QApplication::applicationDirPath();
	str += "/ffmpeg.exe";
	str += wz;
	process->setProcessChannelMode(QProcess::MergedChannels);
	qDebug() << str;
	process->start(str);
	if (!process->waitForStarted()) {
		emit errorOccurred(QString("Merge2Mp4 failed:%1").arg(process->errorString()));
		qDebug() << "start failed:" << process->errorString();
	}
}

void VideoDownloadTask::onVideoStreamFinished()
{
	auto reply = videohttpReply;
	videohttpReply->deleteLater();
	videohttpReply = nullptr;

	if (reply->error() == QNetworkReply::OperationCanceledError) {
		return;
	}

	if (reply->error() != QNetworkReply::NoError) {
		emit errorOccurred("网络请求错误");
		return;
	}
	videodownload = true;
	emit Merge2Mp4Signal();
}

void VideoDownloadTask::onAudioStreamFinished()
{
	auto reply = audiohttpReply;
	audiohttpReply->deleteLater();
	audiohttpReply = nullptr;

	if (reply->error() == QNetworkReply::OperationCanceledError) {
		return;
	}

	if (reply->error() != QNetworkReply::NoError) {
		emit errorOccurred("网络请求错误");
		return;
	}
	audiodownload = true;
	emit Merge2Mp4Signal();
}

void VideoDownloadTask::onVideoStreamReadyRead()
{
	auto tmp = VideoDownloadedBytesCnt + videohttpReply->bytesAvailable();
	VideoTotalBytesCnt = videohttpReply->header(QNetworkRequest::ContentLengthHeader).toInt();
	Q_ASSERT(m4sVideofile != nullptr);
	if (-1 == m4sVideofile->write(videohttpReply->readAll())) {
		emit errorOccurred("文件写入失败: " + m4sVideofile->errorString());
		videohttpReply->abort();
	}
	else {
		VideoDownloadedBytesCnt = tmp;
	}
}

void VideoDownloadTask::onAudioStreamReadyRead()
{
	auto tmp = AudioDownloadedBytesCnt + audiohttpReply->bytesAvailable();
	AudioTotalBytesCnt = audiohttpReply->header(QNetworkRequest::ContentLengthHeader).toInt();
	Q_ASSERT(m4sAudiofile != nullptr);
	if (-1 == m4sAudiofile->write(audiohttpReply->readAll())) {
		emit errorOccurred("文件写入失败: " + m4sAudiofile->errorString());
		audiohttpReply->abort();
	}
	else {
		AudioDownloadedBytesCnt = tmp;
	}
}


QJsonObject PgcDownloadTask::toJsonObj() const
{
	auto json = VideoDownloadTask::toJsonObj();
	json.insert("type", static_cast<int>(ContentType::PGC));
	json.insert("ssid", ssId);
	json.insert("epid", epId);
	return json;
}

PgcDownloadTask::PgcDownloadTask(const QJsonObject& json)
	: VideoDownloadTask(json),
	ssId(json["ssid"].toInteger()),
	epId(json["epid"].toInteger())
{
}

QNetworkReply* PgcDownloadTask::getPlayUrlInfo(qint64 epId, int qn)
{
	auto api = "https://api.bilibili.com/pgc/player/web/playurl";
	auto query = QString("?ep_id=%1&qn=%2&fourk=1").arg(epId).arg(qn);
	return Network::Bili::get(api + query);
}

QNetworkReply* PgcDownloadTask::getPlayUrlInfo() const
{
	return getPlayUrlInfo(epId, qn);
}

const QString PgcDownloadTask::playUrlInfoDataKey = "result";

QString PgcDownloadTask::getPlayUrlInfoDataKey() const
{
	return playUrlInfoDataKey;
}



QJsonObject PugvDownloadTask::toJsonObj() const
{
	auto json = VideoDownloadTask::toJsonObj();
	json.insert("type", static_cast<int>(ContentType::PUGV));
	json.insert("ssid", ssId);
	json.insert("epid", epId);
	return json;
}

PugvDownloadTask::PugvDownloadTask(const QJsonObject& json)
	: VideoDownloadTask(json),
	ssId(json["ssid"].toInteger()),
	epId(json["epid"].toInteger())
{
}

QNetworkReply* PugvDownloadTask::getPlayUrlInfo(qint64 epId, int qn)
{
	auto api = "https://api.bilibili.com/pugv/player/web/playurl";
	auto query = QString("?ep_id=%1&qn=%2&fourk=1").arg(epId).arg(qn);
	return Network::Bili::get(api + query);
}

QNetworkReply* PugvDownloadTask::getPlayUrlInfo() const
{
	return getPlayUrlInfo(epId, qn);
}

const QString PugvDownloadTask::playUrlInfoDataKey = "data";

QString PugvDownloadTask::getPlayUrlInfoDataKey() const
{
	return playUrlInfoDataKey;
}



QJsonObject UgcDownloadTask::toJsonObj() const
{
	auto json = VideoDownloadTask::toJsonObj();
	json.insert("type", static_cast<int>(ContentType::UGC));
	json.insert("aid", aid);
	json.insert("cid", cid);
	return json;
}

UgcDownloadTask::UgcDownloadTask(const QJsonObject& json)
	: VideoDownloadTask(json),
	aid(json["aid"].toInteger()),
	cid(json["cid"].toInteger())
{
}

QNetworkReply* UgcDownloadTask::getPlayUrlInfo(qint64 aid, qint64 cid, int qn)
{
	auto api = "https://api.bilibili.com/x/player/wbi/playurl";
	auto query = QString("?avid=%1&cid=%2&qn=%3&fnver=0&fnval=80&fourk=1").arg(aid).arg(cid).arg(qn);
	return Network::Bili::get(api + query);
}

QNetworkReply* UgcDownloadTask::getPlayUrlInfo() const
{
	return getPlayUrlInfo(aid, cid, qn);
}

const QString UgcDownloadTask::playUrlInfoDataKey = "data";

QString UgcDownloadTask::getPlayUrlInfoDataKey() const
{
	return playUrlInfoDataKey;
}



QNetworkReply* LiveDownloadTask::getPlayUrlInfo(qint64 roomId, int qn)
{
	auto api = "https://api.live.bilibili.com/xlive/web-room/v2/index/getRoomPlayInfo";
	auto query = QString("?protocol=0,1&format=0,1,2&codec=0,1&room_id=%1&qn=%2").arg(roomId).arg(qn);
	return Network::Bili::get(api + query);
}

QNetworkReply* LiveDownloadTask::getPlayUrlInfo() const
{
	return getPlayUrlInfo(roomId, qn);
}

const QString LiveDownloadTask::playUrlInfoDataKey = "data";

QString LiveDownloadTask::getPlayUrlInfoDataKey() const
{
	return playUrlInfoDataKey;
}

LiveDownloadTask::LiveDownloadTask(qint64 roomId, int qn, const QString& path)
	: AbstractVideoDownloadTask(QString(), qn), basePath(path), roomId(roomId)
{
}

LiveDownloadTask::~LiveDownloadTask() = default;

QJsonObject LiveDownloadTask::toJsonObj() const
{
	return QJsonObject();
}

QString LiveDownloadTask::getTitle() const
{
	return QFileInfo(basePath).baseName();
}

void LiveDownloadTask::removeFile()
{
	// don't delete
}

int LiveDownloadTask::estimateRemainingSeconds(qint64 downBytesPerSec) const
{
	Q_UNUSED(downBytesPerSec)
		// return duration of downloaded video instead
		return (dldDelegate == nullptr ? 0 : dldDelegate->getDurationInMSec() / 1000);
}

QString LiveDownloadTask::getProgressStr() const
{
	if (VideoDownloadedBytesCnt == 0 || AudioDownloadedBytesCnt == 0) {
		return QString();
	}
	return Utils::formattedDataSize(VideoDownloadedBytesCnt + AudioDownloadedBytesCnt);
}

QnList LiveDownloadTask::getAllPossibleQn()
{
	return liveQnDescMap.keys();
}

QString LiveDownloadTask::getQnDescription(int qn)
{
	return liveQnDescMap.value(qn);
}

QString LiveDownloadTask::getQnDescription() const
{
	return getQnDescription(qn);
}

QnInfo LiveDownloadTask::getQnInfoFromPlayUrlInfo(const QJsonObject& data)
{
	QnInfo qnInfo;
	auto infoObj = data["playurl_info"].toObject()["playurl"].toObject();

	QMap<int, QString> m;
	for (auto&& qnDescValR : infoObj["g_qn_desc"].toArray()) {
		auto qnDescObj = qnDescValR.toObject();
		auto qn = qnDescObj["qn"].toInt();
		auto desc = qnDescObj["desc"].toString();
		m[qn] = desc;
	}
	auto obj = infoObj["stream"].toArray().first()
		["format"].toArray().first()
		["codec"].toArray().first();
	qnInfo.currentQn = obj["current_qn"].toInt();
	for (auto&& qnValR : obj["accept_qn"].toArray()) {
		auto qn = qnValR.toInt();
		qnInfo.qnList.append(qn);
		if (liveQnDescMap.value(qn) != m[qn]) {
			liveQnDescMap.insert(qn, m[qn]);
		}
	}
	return qnInfo;
}

QString LiveDownloadTask::getPlayUrlFromPlayUrlInfo(const QJsonObject& data)
{
	auto urlObj = data["playurl_info"].toObject()
		["playurl"].toObject()
		["stream"].toArray().first()
		["format"].toArray().first()
		["codec"].toArray().first();
	auto baseUrl = urlObj["base_url"].toString();
	auto obj = urlObj["url_info"].toArray().first();
	auto host = obj["host"].toString();
	auto extra = obj["extra"].toString();
	return host + baseUrl + extra;
}

void LiveDownloadTask::parsePlayUrlInfo(const QJsonObject& data)
{
	if (data["live_status"].toInt() != 1) {
		emit errorOccurred("未开播或正在轮播");
		return;
	}
	qn = getQnInfoFromPlayUrlInfo(data).currentQn;
	auto url = getPlayUrlFromPlayUrlInfo(data);
	auto ext = Utils::fileExtension(QUrl(url).fileName());
	if (ext != ".flv") {
		emit errorOccurred("非FLV");
		return;
	}

	emit getUrlInfoFinished();

	downloadedBytesCnt = 0;

	httpReply = Network::Bili::get(url);
	dldDelegate = std::make_unique<FlvLiveDownloadDelegate>(*httpReply, [this]() {
		auto dateStr = QDateTime::currentDateTime().toString("[yyyy.MM.dd] hh.mm.ss");
		auto path = basePath + " " + dateStr + ".flv";
		auto file = std::make_unique<QFile>(path);
		if (file->open(QIODevice::WriteOnly)) {
			this->path = std::move(path);
			return file;
		}
		else {
			return decltype(file)();
		}
		});

	connect(httpReply, &QNetworkReply::readyRead, this, [this]() {
		auto ret = dldDelegate->newDataArrived();
		if (!ret) {
			httpReply->abort();
			emit errorOccurred(dldDelegate->errorString());
		}
		downloadedBytesCnt = dldDelegate->getReadBytesCnt() + httpReply->bytesAvailable();
		});

	connect(httpReply, &QNetworkReply::finished, this, [this]() {
		auto reply = httpReply;
		httpReply = nullptr;
		reply->deleteLater();
		dldDelegate.reset();

		if (reply->error() == QNetworkReply::OperationCanceledError) {
			return;
		}
		else if (reply->error() != QNetworkReply::NoError) {
			emit errorOccurred("网络请求错误");
		}
		else {
			emit errorOccurred("已结束或下载速度过慢");
		}
		});
}


ComicDownloadTask::~ComicDownloadTask() = default;

QJsonObject ComicDownloadTask::toJsonObj() const
{
	return QJsonObject{
		{"type", static_cast<int>(ContentType::Comic)},
		{"path", path},
		{"id", comicId},
		{"epid", epId},
		{"imgs", finishedImgCnt},
		{"bytes", bytesCntTillLastImg},
		{"total", totalImgCnt},
	};
}

ComicDownloadTask::ComicDownloadTask(const QJsonObject& json)
	: AbstractDownloadTask(json["path"].toString()),
	comicId(json["id"].toInteger()),
	epId(json["epid"].toInteger()),
	totalImgCnt(json["total"].toInt(0)),
	finishedImgCnt(json["imgs"].toInt(0)),
	bytesCntTillLastImg(json["bytes"].toInteger(0))
{
}

void ComicDownloadTask::startDownload()
{
	auto getImgPathsUrl = "https://manga.bilibili.com/twirp/comic.v1.Comic/GetImageIndex?device=pc&platform=web";
	httpReply = Network::Bili::postJson(getImgPathsUrl, QJsonObject({ {"ep_id", epId} }));
	connect(httpReply, &QNetworkReply::finished, this, &ComicDownloadTask::getImgInfoFinished);
}

void ComicDownloadTask::stopDownload()
{
	if (httpReply != nullptr) {
		httpReply->abort();
	}
}

void ComicDownloadTask::removeFile()
{
	// simple but may delete innocent files !!!
	QDir(path).removeRecursively();
}

void ComicDownloadTask::getImgInfoFinished()
{
	auto data = getReplyJson("data").toObject();
	if (data.isEmpty()) {
		return;
	}

	// assert: imgRqstPaths.isEmpty()

	auto images = data["images"].toArray();
	totalImgCnt = images.size();
	imgRqstPaths.reserve(totalImgCnt);
	for (auto&& imgObjRef : images) {
		auto imgObj = imgObjRef.toObject();
		imgRqstPaths.append(imgObj["path"].toString());
	}
	emit getUrlInfoFinished();
	downloadNextImg();
}

void ComicDownloadTask::downloadNextImg()
{
	if (finishedImgCnt == totalImgCnt) {
		emit downloadFinished();
		return;
	}
	auto getTokenUrl = "https://manga.bilibili.com/twirp/comic.v1.Comic/ImageToken?device=pc&platform=web";
	auto postData = R"({"urls":"[\")" + imgRqstPaths[finishedImgCnt].toUtf8() + R"(\"]"})";
	httpReply = Network::Bili::postJson(getTokenUrl, postData);

	connect(httpReply, &QNetworkReply::finished, this, &ComicDownloadTask::getImgTokenFinished);
}

void ComicDownloadTask::getImgTokenFinished()
{
	auto data = getReplyJson("data");
	if (data.isNull() || data.isUndefined()) {
		return;
	}
	auto obj = data.toArray().first();
	auto token = obj["token"].toString();
	auto url = obj["url"].toString();
	auto index = Utils::paddedNum(finishedImgCnt + 1, Utils::numberOfDigit(totalImgCnt));
	auto fileName = index + Utils::fileExtension(url);
	file = openFileForWrite(fileName);
	if (!file) {
		return;
	}
	httpReply = Network::Bili::get(url + "?token=" + token);
	connect(httpReply, &QNetworkReply::readyRead, this, &ComicDownloadTask::onImgReadyRead);
	connect(httpReply, &QNetworkReply::finished, this, &ComicDownloadTask::downloadImgFinished);
}

std::unique_ptr<QSaveFile> ComicDownloadTask::openFileForWrite(const QString& fileName)
{
	if (!QFileInfo::exists(path)) {
		if (!QDir().mkpath(path)) {
			emit errorOccurred("创建目录失败");
			return nullptr;
		}
	}

	auto f = std::make_unique<QSaveFile>(QDir(path).filePath(fileName));
	if (!f->open(QIODevice::WriteOnly)) {
		emit errorOccurred("打开文件失败");
		return nullptr;
	}

	return f;
}

void ComicDownloadTask::onImgReadyRead()
{
	if (curImgTotalBytesCnt == 0) {
		curImgRecvBytesCnt = httpReply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
	}
	auto size = httpReply->bytesAvailable();
	if (-1 == file->write(httpReply->readAll())) {
		emit errorOccurred("文件写入失败: " + file->errorString());
		abortCurrentImg();
		httpReply->abort();
	}
	else {
		curImgRecvBytesCnt += size;
	}
}

void ComicDownloadTask::abortCurrentImg()
{
	curImgRecvBytesCnt = 0;
	curImgTotalBytesCnt = 0;
	file.reset();
}

void ComicDownloadTask::downloadImgFinished()
{
	auto imgSize = curImgRecvBytesCnt;
	curImgRecvBytesCnt = 0;
	curImgTotalBytesCnt = 0;

	auto httpReply = this->httpReply;
	this->httpReply->deleteLater();
	this->httpReply = nullptr;

	auto file = std::move(this->file);

	auto error = httpReply->error();
	if (error != QNetworkReply::NoError) {
		if (error != QNetworkReply::OperationCanceledError) {
			emit errorOccurred("网络错误");
		}
		return;
	}

	if (!file->commit()) {
		emit errorOccurred("保存文件失败");
		return;
	}

	finishedImgCnt++;
	bytesCntTillLastImg += imgSize;
	downloadNextImg();
}

qint64 ComicDownloadTask::getDownloadedBytesCnt() const
{
	return bytesCntTillLastImg + curImgRecvBytesCnt;
}

int ComicDownloadTask::estimateRemainingSeconds(qint64 downBytesPerSec) const
{
	if (downBytesPerSec == 0) {
		return Unknown;
	}

	if (finishedImgCnt == totalImgCnt - 1 && curImgTotalBytesCnt != 0) {
		// last image, remaining bytes count is known
		return (curImgTotalBytesCnt - curImgRecvBytesCnt) / downBytesPerSec;
	}
	else if (finishedImgCnt == 0) {
		if (curImgTotalBytesCnt == 0) {
			return Unknown;
		}
		else {
			return (curImgTotalBytesCnt * totalImgCnt - curImgRecvBytesCnt) / downBytesPerSec;
		}
	}
	else {
		int estimateTotalBytes = bytesCntTillLastImg * totalImgCnt / finishedImgCnt;
		return (estimateTotalBytes - bytesCntTillLastImg - curImgRecvBytesCnt) / downBytesPerSec;
	}
}

double ComicDownloadTask::getProgress() const
{
	if (totalImgCnt == 0) {
		return 0;
	}
	return static_cast<double>(finishedImgCnt) / totalImgCnt;
}

QString ComicDownloadTask::getProgressStr() const
{
	if (totalImgCnt == 0) {
		return QString();
	}
	else {
		return QStringLiteral("%1页/%2页").arg(finishedImgCnt).arg(totalImgCnt);
	}
}

QString ComicDownloadTask::getQnDescription() const
{
	return QString();
}
