#include "basicmetricchecker.h"

CBasicMetricChecker::CBasicMetricChecker(const QString &sMetricName,
                                          EMetricDataType eMetricDataType,
                                          const QString &sMetricType,
                                          int nReaction,
                                          double dHighValue,
                                          double dSevereValue,
                                          const QString &sInstanceType,
                                          const QString &sInstanceName )
    : m_sMetricName(sMetricName),
      m_eMetricDataType(eMetricDataType),
      m_sMetricType( sMetricType ),
      m_nReaction( nReaction ),
      m_dHighValue( dHighValue ),
      m_dSevereValue( dSevereValue ),
      m_sInstanceType( sInstanceType ),
      m_sInstanceName( sInstanceName )
{
    Q_ASSERT( !sMetricName.isEmpty() );
    Q_ASSERT( nReaction >= -3 && nReaction <=0);
    Q_ASSERT(  dHighValue == -1 || dSevereValue == -1 || dHighValue < dSevereValue );
    // TODO: Handle the case when dHighValue >= dSevereValue
}

MetricDataSPtr CBasicMetricChecker::CheckMetric()
{
    // Create metric data
    MetricDataSPtr pMetric = std::make_shared<CMetricData>();

    pMetric->SetTime( QDateTime::currentDateTime() );
    pMetric->SetName(m_sMetricName);
    pMetric->SetDataType(m_eMetricDataType);
    pMetric->SetType(m_sMetricType);
    pMetric->SetReaction(m_nReaction);

    if( !m_sInstanceType.isEmpty() && !m_sInstanceName.isEmpty() )
    {
        pMetric->SetInstanceType( m_sInstanceType );
        pMetric->SetInstanceName( m_sInstanceName );
    }

    // Polimorphyc check of metric value
    double dValue = CheckMetricValue();

    pMetric->SetValue( dValue );

    // Set data severity
    EMetricDataSeverity eSeverity = EMetricDataSeverity::Normal;
    if( m_dSevereValue != -1 && dValue >= m_dSevereValue )
        eSeverity = EMetricDataSeverity::Severe;
    else if( m_dHighValue != -1 && dValue > m_dHighValue )
        eSeverity = EMetricDataSeverity::High;
    pMetric->SetDataSeverity( eSeverity );

    return pMetric;
}
