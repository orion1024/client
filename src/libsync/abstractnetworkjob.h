/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#pragma once

#include "owncloudlib.h"
#include <QObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPointer>
#include <QElapsedTimer>
#include <QDateTime>
#include <QTimer>
#include "accountfwd.h"

class QUrl;

namespace OCC {

class AbstractSslErrorHandler;

// Constants used to fetch and/or set attributes/properties/headers in network jobs.
const char owncloudCustomSoftErrorStringC[] = "owncloud-custom-soft-error-string";
const char owncloudOCErrorHeaderName[] = "OC-ErrorString";
const char owncloudContentLengthHeaderName[] = "Content-Length";
const char owncloudContentRangeHeaderName[] = "Content-Range";
const char owncloudSTSHeaderName[] = "Strict-Transport-Security";
const char owncloudFileIDHeaderName[] = "OC-FileId";
const char owncloudOCETagHeaderName[] = "OC-ETag";
const char owncloudETagHeaderName[] = "ETag";
const char owncloudOCFinishPollHeaderName[] = "OC-Finish-Poll";
const char owncloudOCMTimeHeaderName[] = "X-OC-MTime";
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 2)
const char owncloudShouldSoftCancelPropertyName[] = "owncloud-should-soft-cancel";
#endif
const char owncloudCheckSumHeaderName[] = "OC-Checksum";

/**
 * @brief The AbstractNetworkJob class
 * @ingroup libsync
 */
class OWNCLOUDSYNC_EXPORT AbstractNetworkJob : public QObject {
    Q_OBJECT
public:
    explicit AbstractNetworkJob(AccountPtr account, const QString &path, QObject* parent = 0);
    virtual ~AbstractNetworkJob();

    virtual void start();

    AccountPtr account() const { return _account; }

    void setPath(const QString &path);
    QString path() const { return _path; }

    void setReply(QNetworkReply *reply);

    void setIgnoreCredentialFailure(bool ignore);
    bool ignoreCredentialFailure() const { return _ignoreCredentialFailure; }

    QByteArray responseTimestamp();
    quint64 duration();

    qint64 timeoutMsec() { return _timer.interval(); }

    // These methods are used to replace the direct calls that were used to fetch the same information.
    // Can and should be overloaded when needed by specific implementation.

    virtual void abortNetworkReply();

    virtual QUrl replyUrl();
    virtual QNetworkReply::NetworkError replyError();
    virtual int replyHttpStatusCode();
    virtual QString replyHttpReasonPhrase();
    virtual QString replyErrorString();
    virtual bool replyHasOCErrorString();
    virtual QByteArray replyOCErrorString();
    virtual  bool replyHasOCFileID();
    virtual QByteArray replyOCFileID();
    virtual QByteArray replyReadAll();
    virtual QString replyContentTypeHeader();
    virtual QUrl replyRedirectionTarget();
    virtual bool replyHasSTS();
    virtual bool replyHasContentRange();
    virtual QByteArray replyContentRange();
    virtual bool replyHasContentLength();
    virtual QByteArray replyContentLength();
    virtual bool replyCustomSoftErrorStringIsValid();
    virtual QString replyCustomSoftErrorString();
    virtual  bool replyHasOCETag();
    virtual QByteArray replyOCETag();
    virtual  bool replyHasETag();
    virtual QByteArray replyETag();
    virtual  bool replyHasOCFinishPoll();
    virtual QByteArray replyOCFinishPoll();
    virtual  bool replyHasOCMTime();
    virtual QByteArray replyOCMTime();
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 2)
    virtual bool replyShouldSoftCancelIsValid();
    virtual QString replyShouldSoftCancel();
#endif
    virtual  bool replyHasOCChecksum();
    virtual QByteArray replyOCChecksum();

public slots:
    void setTimeout(qint64 msec);
    void resetTimeout();
signals:
    void networkError(QNetworkReply *reply);
    void networkActivity();
protected:
    void setupConnections(QNetworkReply *reply);
    QNetworkReply* davRequest(const QByteArray& verb, const QString &relPath,
                              QNetworkRequest req = QNetworkRequest(),
                              QIODevice *data = 0);
    QNetworkReply* davRequest(const QByteArray& verb, const QUrl &url,
                              QNetworkRequest req = QNetworkRequest(),
                              QIODevice *data = 0);
    QNetworkReply* getRequest(const QString &relPath);
    QNetworkReply* getRequest(const QUrl &url);
    QNetworkReply* headRequest(const QString &relPath);
    QNetworkReply* headRequest(const QUrl &url);
    QNetworkReply* deleteRequest(const QUrl &url);

    /*
     * Making this method protected instead of public.
     * We really want external objects not to access the reply
     * in order to abstract network protocol internal workings
     * from propagators classes.
     * For exceptions such as the credentials class, the reply
     * should be sent through signals.
    */
    QNetworkReply* reply() const { return _reply; }

    int maxRedirects() const { return 10; }
    virtual bool finished() = 0;
    QByteArray    _responseTimestamp;
    QElapsedTimer _durationTimer;
    quint64       _duration;
    bool          _timedout;  // set to true when the timeout slot is received

    // Automatically follows redirects. Note that this only works for
    // GET requests that don't set up any HTTP body or other flags.
    bool          _followRedirects;

private slots:
    void slotFinished();
    virtual void slotTimeout();

protected:
    AccountPtr _account;
private:
    QNetworkReply* addTimer(QNetworkReply *reply);
    bool _ignoreCredentialFailure;
    QPointer<QNetworkReply> _reply; // (QPointer because the NetworkManager may be destroyed before the jobs at exit)
    QString _path;
    QTimer _timer;
    int _redirectCount;
};

/**
 * @brief Internal Helper class
 */
class NetworkJobTimeoutPauser {
public:
    NetworkJobTimeoutPauser(QNetworkReply *reply);
    ~NetworkJobTimeoutPauser();
private:
    QPointer<QTimer> _timer;
};


/** Gets the SabreDAV-style error message from an error response.
 *
 * This assumes the response is XML with a 'error' tag that has a
 * 'message' tag that contains the data to extract.
 *
 * Returns a null string if no message was found.
 */
QString OWNCLOUDSYNC_EXPORT extractErrorMessage(const QByteArray& errorResponse);

/** Builds a error message based on the error and the reply body. */
QString OWNCLOUDSYNC_EXPORT errorMessage(const QString& baseError, const QByteArray& body);



} // namespace OCC


