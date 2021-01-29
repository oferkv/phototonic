#pragma once
#include <QFileIconProvider>
#include <QMimeDatabase>

class IconProvider : public QFileIconProvider
{
public:
    IconProvider() : QFileIconProvider(),
        m_dirIcon(QIcon::fromTheme(QLatin1String("inode-directory"))),
        m_unknownIcon(QIcon::fromTheme("unknown"))
    {}

    //icon(QFileIconProvider::IconType type) const override;
    QIcon icon(const QFileInfo &info) const override;

private:
    QIcon m_dirIcon;
    QIcon m_unknownIcon;
    mutable QHash<QString, QIcon> m_cache;
    QMimeDatabase m_mimeDb;
};

