#include "oddeyeclient.h"
#include "networkaccessmanager.h"

#include <QJsonDocument>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>

#include "../logger.h"


COddEyeClient::COddEyeClient(QObject *parent)
    : Base(parent)
{}

void COddEyeClient::SendMetrics(const MetricDataList &lstMetrics)
{
    if( !IsReady() )
    {
        LOG_WARNING( "Unable to send metrics: OEClient is not ready" );
        Q_ASSERT(false);
        return;
    }

    if( lstMetrics.isEmpty() )
    {
        LOG_WARNING( "Metrics data list is empty" );
        Q_ASSERT(false);
        return;
    }

    QJsonDocument oNormalMetricsJson;
    QJsonDocument oSpecialMetricsJson;

    ConvertMetricsToJSON( lstMetrics, oNormalMetricsJson, oSpecialMetricsJson );

    // setnd normal metrics
    if( IsValid( oNormalMetricsJson ) )
        Base::SendJsonData( oNormalMetricsJson );
    else
        LOG_WARNING("Invalid json data of collected metrics");

    // send special metrics
    if( IsValid( oSpecialMetricsJson ) )
        Base::SendJsonData( oSpecialMetricsJson );
    else
        LOG_WARNING("Invalid json data of collected special metrics");
}

void COddEyeClient::HandleSendSuccedded(QNetworkReply *pReply, const QJsonDocument &oJsonData)
{
    LOG_INFO( "Metrics successfully sent: " + pReply->readAll() );
    Q_UNUSED( oJsonData );
}

void COddEyeClient::HandleSendError(QNetworkReply *pReply, const QJsonDocument &oJsonData)
{
    Q_ASSERT( pReply );
    QString sError = pReply->errorString();
    LOG_ERROR( sError.toStdString() );

    // store JSON data in cache
    CacheJsonData( oJsonData );

    //m_pNetworkAccessManager->Reset();
}

void COddEyeClient::ConvertMetricsToJSON(const MetricDataList &lstMetrics,
                                         QJsonDocument &oNormalMetricsJson,
                                         QJsonDocument &oSpecialMetricsJson)
{
    QJsonArray oNormalArray;
    QJsonArray oSpecialArray;

    for( MetricDataSPtr pCurrentMetric : lstMetrics)
    {
        if( !pCurrentMetric )
            continue;

         oNormalArray.append( Base::CreateMetricJson( pCurrentMetric ) );

         // Check metric value severity
         if( pCurrentMetric->HasSeverityDescriptor() )
         {
             // setnd error message
             oSpecialArray.append( Base::CreateSpecialMessageJson( pCurrentMetric->GetSeverityDescriptor()) );
         }
    }

    oNormalMetricsJson = QJsonDocument( oNormalArray );
    oSpecialMetricsJson = QJsonDocument( oSpecialArray );
}

bool COddEyeClient::CacheJsonData(const QJsonDocument &oJsonDoc)
{
    if( oJsonDoc.isEmpty() )
    {
        Q_ASSERT(false);
        return false;
    }

    if( m_nMaxCacheCount <= 0 )
        // caching disabled
        return false;

    QDir oCacheDir(m_sCacheDir);
    if( !oCacheDir.exists() )
    {
        LOG_ERROR( "Failed to cache metric data. tmpdir not found!" );
        // TODO: emit appropriate error signal
    }

    // check max cache
    oCacheDir.setNameFilters(QStringList() << "*.json");
    int nCurrentJsonFileCount = oCacheDir.count();

    if( nCurrentJsonFileCount >= m_nMaxCacheCount )
    {
        LOG_INFO( "The maximum number of caches reached. Crrent metric data will be lost" );
        return false;
    }

    QString sJsonFileName = QDateTime::currentDateTime().toString("ddMMyy_HHmmsszzz") + ".json";
    QString sJsonFilePath = oCacheDir.absoluteFilePath( sJsonFileName );

    //
    //  Save JSON data
    //
    QFile oJsonFile(sJsonFilePath);

    if (!oJsonFile.open(QIODevice::WriteOnly))
    {
        LOG_ERROR("Couldn't open json file to save!");
        return false;
    }

    QByteArray aJsonData = oJsonDoc.toJson();
    qint64 nBytesWritten = oJsonFile.write(aJsonData);
    if( nBytesWritten < aJsonData.size() )
        oJsonFile.waitForBytesWritten(100);
    oJsonFile.close();

    LOG_INFO( "Metric data was cached." );
    return true;
}

bool COddEyeClient::IsValid(const QJsonDocument &oJsonDec) const
{
    if( oJsonDec.isEmpty() )
        return false;
    if(oJsonDec.isArray() && oJsonDec.array().isEmpty() )
        return false;
    if( oJsonDec.isObject() && oJsonDec.object().isEmpty() )
        return false;
    // TODO: check existance of Required fields (e.g. "metric")
    return true;
}

