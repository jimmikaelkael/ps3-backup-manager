#ifndef FTPFILESYSTEMMODEL_H
#define FTPFILESYSTEMMODEL_H

#include <QAbstractItemModel>

class FtpFileSystemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit FtpFileSystemModel(QObject *parent = 0);

signals:

public slots:

};

#endif // FTPFILESYSTEMMODEL_H
