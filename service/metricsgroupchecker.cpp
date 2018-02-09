#include "metricsgroupchecker.h"
#include "basicmetricchecker.h"

CMetricsGroupChecker::CMetricsGroupChecker(QObject* pParent)
    : Base( pParent )
{
}

MetricDataList CMetricsGroupChecker::CheckMetrics()
{
    MetricDataList lstMetrics;
    for( IMetricCheckerSPtr pCurrentChecker : m_lstMetricCheckers )
    {
        Q_ASSERT(pCurrentChecker);
        if( !pCurrentChecker )
            continue;

        try
        {
            auto pMetricData = pCurrentChecker->CheckMetric();
            lstMetrics.append(pMetricData);
        }
        catch( std::exception const& oErr )
        {
            auto pBasicChecker = dynamic_cast<CBasicMetricChecker*>(pCurrentChecker.get());
            if( pBasicChecker )
            {
                LOG_ERROR( QString("Excpetion: Metric %1 : %2 %3 - check failed: ")
                           .arg( pBasicChecker->GetMetricName() )
                           .arg( pBasicChecker->GetInstanceType() )
                           .arg( pBasicChecker->GetInstanceType() )
                           .toStdString() + oErr.what())
            }
            else
            {
                LOG_ERROR( QString("Excpetion: Metric checking failed: %1").arg( oErr.what() ).toStdString() )
            }
        }
    }

    return lstMetrics;
}

void CMetricsGroupChecker::AddMetricChecker(IMetricCheckerSPtr pMetricChecker)
{
    Q_ASSERT(pMetricChecker);
    if( pMetricChecker )
        m_lstMetricCheckers.append(pMetricChecker);
}

void CMetricsGroupChecker::RemoveMetricChecker(IMetricCheckerSPtr pMetricChecker)
{
    if( pMetricChecker )
        m_lstMetricCheckers.removeAll( pMetricChecker );
}
