#ifndef APPLICATION_H
#define APPLICATION_H

#include <QCoreApplication>



class CApplication : public QCoreApplication
{
    using Base = QCoreApplication;

public:
    CApplication();
    CApplication(int &argc, char **argv);

    // QCoreApplication interface
public:
    bool notify(QObject* pObject, QEvent* pEvent) override;
};

#endif // APPLICATION_H
