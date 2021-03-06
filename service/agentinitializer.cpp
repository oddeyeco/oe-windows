#include "agentinitializer.h"
#include "configurationmanager.h"
#include "checkers/scriptsmetricschecker.h"
#include "logger.h"

#include <QCoreApplication>
#include <QDebug>

// static init
QString CAgentInitialzier::s_sDefaultLogDir = "tmp/oddeye_log";

void CAgentInitialzier::InitializeLogger()
{
    // log_dir
    QString sLogsDirPath = ConfMgr.GetMainConfiguration().GetValueAsPath( "SelfConfig/log_dir", s_sDefaultLogDir );
    if( sLogsDirPath.isEmpty() )
    {
        throw CInvalidConfigValueException( "log_dir is empty" );
    }

    Logger::getInstance().setLogsFolderPath( sLogsDirPath );

    // log_rotate_seconds
    qint64 nLogRotateSeconds = ConfMgr.GetMainConfiguration().Value<qint64>("SelfConfig/log_rotate_seconds", 3600);
    Logger::getInstance().setLogRotateSeconds( nLogRotateSeconds );

    // log_rotate_backups
    int nBackupLogFilesCount = ConfMgr.GetMainConfiguration().Value<int>("SelfConfig/log_rotate_backups", 24);
    Logger::getInstance().setBackupFileCount( nBackupLogFilesCount );

    if( nLogRotateSeconds <= 0 || nBackupLogFilesCount <= 0)
        LOG_ERROR("Logging disabled");

    bool bDebugLoggingEnabled = ConfMgr.GetMainConfiguration().Value<bool>( "SelfConfig/debug_log", false );
    Logger::getInstance().SetDebugLoggingEnabled( bDebugLoggingEnabled );
}

void CAgentInitialzier::InitializeEngine(CEngine *pEngine)
{
    if( !pEngine )
    {
        Q_ASSERT(false);
        return;
    }
    LOG_INFO( "-------- OE Engine INIT started --------" );

    // Setup Main Settings
    double dUpdateSecs = ConfMgr.GetMainConfiguration().Value<double>("SelfConfig/check_period_seconds", 1);
    int nMsec = static_cast<int>( dUpdateSecs * 1000 );
    pEngine->SetUpdateInterval( nMsec );

    LOG_INFO( "Check period is: " + QString::number(int(dUpdateSecs)) + " sec" );

    auto lstAllConfigs = ConfMgr.GetAllConfigurations();
    for( ConfigSPtr& pCurrentConfig : lstAllConfigs  )
    {
        Q_ASSERT(pCurrentConfig);
        if( !pCurrentConfig )
            continue;
        // skip if main config
        if( pCurrentConfig.get() == &ConfMgr.GetMainConfiguration() )
            continue;

        // Create checkers for specified sections
        QStringList lstSections = pCurrentConfig->GetAllSectionNames();
        if( lstSections.isEmpty() )
        {
            IMetricsCategoryCheckerSPtr pChecker = CreateCheckerByConfigName( pCurrentConfig->GetName() );
            QString sCheckerName = MakeCheckerName( pCurrentConfig->GetName() );
            if( pChecker )
            {
                LOG_INFO( "Checker loaded: " + sCheckerName );
                pChecker->metaObject()->className();

                // Pass config section to checker
                auto&& oSection = pCurrentConfig->GetRootSection();
                pChecker->SetConfigSection( oSection );
                pEngine->AddChecker(pChecker);
            }
            else
            {
                LOG_INFO( "Checker NOT found: " + sCheckerName );
                qDebug() << "Checker NOT found!";
                // TODO
            }
        }

        for( QString sSectionName : lstSections )
        {
            // Check if checker enabled or not
            QString sEnabledProp = QString("%1/enabled").arg(sSectionName);
            bool bIsEnabled = pCurrentConfig->value( sEnabledProp, QVariant(true) ).toBool();

            if( bIsEnabled )
            {
                IMetricsCategoryCheckerSPtr pChecker = CreateCheckerByConfigName( pCurrentConfig->GetName(),
                                                                                  sSectionName );
                QString sCheckerName = MakeCheckerName( pCurrentConfig->GetName(), sSectionName );
                if( pChecker )
                {
                    LOG_INFO( "Checker loaded: " + sCheckerName );
                    pChecker->metaObject()->className();

                    // Pass config section to checker
                    auto&& oSection = pCurrentConfig->GetSection(sSectionName);
                    pChecker->SetConfigSection( oSection );

                    pEngine->AddChecker(pChecker);
                }
                else
                {
                    LOG_INFO( "Checker NOT found: " + sCheckerName );
                    qDebug() << "Checker NOT found!";
                    // TODO
                }
            }
        }
    }


    //
    //  Check if enabled cripts exists then add CScriptsMetricsChecker
    //
    if( !ConfMgr.GetEnabledScriptsConfigSection().isEmpty() )
    {
        // create scritps metrics checker
        std::shared_ptr<CScriptsMetricsChecker> pScriptsChecker = std::make_shared<CScriptsMetricsChecker>();
        pScriptsChecker->SetConfigSection( ConfMgr.GetEnabledScriptsConfigSection() );
        // Add to engine
        pEngine->AddChecker(pScriptsChecker);

        LOG_INFO( "Script(s) are enabled: " + pScriptsChecker->GetScriptFileNameList().join(", ") );
    }
    else
    {
        LOG_INFO("No enabled scripts");
    }

}

IMetricsCategoryCheckerSPtr CAgentInitialzier::CreateCheckerByConfigName( QString const& sConfigName,
                                                                          QString const& sSectionName )
{
    Q_ASSERT(!sConfigName.isEmpty());
    //Q_ASSERT(!sSectionName.isEmpty());

    // make checker class name from config section name
    QString sCheckerClassName = MakeCheckerName(sConfigName, sSectionName);

    qDebug() << "Checker Name is: " << sCheckerClassName;

    int nTypeID = QMetaType::type(sCheckerClassName.toLatin1());
    if (nTypeID == QMetaType::UnknownType)
        return nullptr;

    void* pCheckerPtr = QMetaType::create(nTypeID);
    if( pCheckerPtr )
    {
        IMetricsCategoryCheckerSPtr spChecker( static_cast<IMetricsCategoryChecker*>(pCheckerPtr) );
        return spChecker;
    }

    return nullptr;
}

//bool CAgentInitialzier::IsMetricsCheckerAvailable(const QString &sDirtyCheckerName)
//{
//    QString sCheckerClassName = MakeCheckerName(sDirtyCheckerName);
//    int nTypeID = QMetaType::type(sCheckerClassName.toLatin1());
//    return nTypeID != QMetaType::UnknownType;
//}

QString CAgentInitialzier::SimplifyName(QString sName)
{
    sName.replace(".", " Dot ");
    sName.replace("-", " ");

    QString sSimplifiedName = sName.simplified().toLower();
    sSimplifiedName.replace(' ', '_');

    return sSimplifiedName;
}

QString CAgentInitialzier::MakeCheckerName(const QString &sConfigName, const QString &sSectionName)
{
    QString sCheckerClassName = SimplifyName(sConfigName)
            + (sSectionName.isEmpty()? "" : "_" + SimplifyName(sSectionName) );
    return ToCamelCase( sCheckerClassName );
}

//QString CAgentInitialzier::MakeCheckerName(const QString &sDirtyName)
//{
//     QString sCheckerClassName = SimplifyName(sDirtyName);
//     return ToCamelCase( sCheckerClassName );
//}

QString CAgentInitialzier::ToCamelCase(const QString &s)
{
    QStringList parts = s.split('_', QString::SkipEmptyParts);
    for (int i=0; i<parts.size(); ++i)
        parts[i].replace(0, 1, parts[i][0].toUpper());

    return parts.join("");
}
