/******************************************************************************
 * Copyright (C) 2016 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "syncjob.h"
#include "basejob.h"

#include <QtNetwork/QNetworkReply>

using namespace Quotient;

static size_t jobId = 0;

SyncJob::SyncJob(const QString& since, const QString& filter, int timeout,
                 const QString& presence)
    : BaseJob(HttpVerb::Get, QStringLiteral("SyncJob-%1").arg(++jobId),
              QStringLiteral("_matrix/client/r0/sync/sse"), true)
{
    setLoggingCategory(SYNCJOB);
    QUrlQuery query;
    if (!filter.isEmpty())
        query.addQueryItem(QStringLiteral("filter"), filter);
    if (!presence.isEmpty())
        query.addQueryItem(QStringLiteral("set_presence"), presence);
    if (!since.isEmpty())
        setRequestHeader("Last-Event-Id", since.toUtf8());
    setRequestQuery(query);
    setExpectedContentTypes({ "text/event-stream" });

    setMaxRetries(std::numeric_limits<int>::max());
}

SyncJob::SyncJob(const QString& since, const Filter& filter, int timeout,
                 const QString& presence)
    : SyncJob(since,
              QJsonDocument(toJson(filter)).toJson(QJsonDocument::Compact),
              timeout, presence)
{}

BaseJob::Status SyncJob::parseJson(const QJsonDocument& data)
{
    d.parseJson(data.object());
    if (d.unresolvedRooms().isEmpty())
        return BaseJob::Success;

    qCCritical(MAIN).noquote() << "Incomplete sync response, missing rooms:"
                               << d.unresolvedRooms().join(',');
    return BaseJob::IncorrectResponseError;
}


void SyncJob::onSentRequest(QNetworkReply* reply)
{
    connect(reply, &QIODevice::readyRead, this, [this, reply] {
        if (!status().good())
            return;

        qCInfo(MAIN) << "Incoming SSE data";

        QString rawData;
        do
        {
            const auto line = QString(reply->readLine());
            if (line.trimmed().isEmpty())
                break;
            rawData.append(line);
        } while(true);

        if (rawData.length() < 100)
            qCInfo(MAIN) << rawData;

        const auto parts = rawData.split("\n", QString::SkipEmptyParts);

        QString data, eventType, id;
        for (const auto& part : parts)
        {
            const auto key = part.split(':')[0].trimmed();
            const auto value = part.mid(key.length() + 1).trimmed();

            if (key == "event")
            {
                eventType = value;
            }
            else if (key == "id")
            {
                id = value;
            }
            else if (key == "data")
            {
                if (!data.isEmpty())
                    data.append("\n");
                data.append(value);
            }
        }

        if (eventType == "sync" && !data.isEmpty())
        {
            QJsonParseError error { 0, QJsonParseError::MissingObject };
            const auto& json = QJsonDocument::fromJson(data.toUtf8(), &error);
            if (error.error == QJsonParseError::NoError)
            {
                auto result = parseJson(json);
                d.setNextBatch(id);
                if (result == BaseJob::Success)
                    emit eventReceived(this);
                else
                    ; // TODO: emit something
            }
            else
                qCInfo(MAIN) << "Received broken SSE event" << eventType << "which parsed as" << error.errorString();
        }
        else
            qCInfo(MAIN) << "Received invalid SSE event" << rawData;

    });
}
