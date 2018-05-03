/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "users.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

    template <> struct FromJson<SearchUserDirectoryJob::User>
    {
        SearchUserDirectoryJob::User operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            SearchUserDirectoryJob::User result;
            result.userId =
                fromJson<QString>(o.value("user_id"));
            result.displayName =
                fromJson<QString>(o.value("display_name"));
            result.avatarUrl =
                fromJson<QString>(o.value("avatar_url"));

            return result;
        }
    };
} // namespace QMatrixClient

class SearchUserDirectoryJob::Private
{
    public:
        QVector<User> results;
        bool limited;
};

SearchUserDirectoryJob::SearchUserDirectoryJob(const QString& searchTerm, int limit)
    : BaseJob(HttpVerb::Post, "SearchUserDirectoryJob",
        basePath % "/user_directory/search")
    , d(new Private)
{
    QJsonObject _data;
    _data.insert("search_term", toJson(searchTerm));
    _data.insert("limit", toJson(limit));
    setRequestData(_data);
}

SearchUserDirectoryJob::~SearchUserDirectoryJob() = default;

const QVector<SearchUserDirectoryJob::User>& SearchUserDirectoryJob::results() const
{
    return d->results;
}

bool SearchUserDirectoryJob::limited() const
{
    return d->limited;
}

BaseJob::Status SearchUserDirectoryJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("results"))
        return { JsonParseError,
            "The key 'results' not found in the response" };
    d->results = fromJson<QVector<User>>(json.value("results"));
    if (!json.contains("limited"))
        return { JsonParseError,
            "The key 'limited' not found in the response" };
    d->limited = fromJson<bool>(json.value("limited"));
    return Success;
}

