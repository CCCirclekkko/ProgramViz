#pragma once

#include "projectmodel.h"
#include "visualsettings.h"

#include <QColor>
#include <QRectF>
#include <QString>
#include <QVector>
#include <QWidget>

class TreeMapView final : public QWidget {
    Q_OBJECT

public:
    explicit TreeMapView(QWidget *parent = nullptr);

    void setRoot(ProjectNode *root);
    ProjectNode *root() const;
    void setVisualSettings(const VisualSettings &settings);

signals:
    void nodeHovered(ProjectNode *node);
    void nodeSelected(ProjectNode *node);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct LayoutItem {
        ProjectNode *node = nullptr;
        QRectF rect;
        QVector<ProjectNode *> members;
        QString displayName;
        qint64 displayCodeLines = 0;
        qint64 displayWeight = 1;
    };

    struct DisplayGroup {
        QVector<ProjectNode *> members;
        QString displayName;
        qint64 weight = 1;
        qint64 codeLines = 0;
    };

    void rebuildLayout();
    void layoutNode(ProjectNode *node, double x, double width);
    QVector<DisplayGroup> displayGroups(ProjectNode *node) const;
    ProjectNode *nodeAt(const QPointF &point) const;
    QColor colorFor(const ProjectNode *node) const;
    QString labelFor(const LayoutItem &item) const;
    bool isDarkTheme() const;

    ProjectNode *m_root = nullptr;
    ProjectNode *m_hovered = nullptr;
    ProjectNode *m_selected = nullptr;
    QVector<LayoutItem> m_items;
    int m_maxDepth = 0;
    double m_rowHeight = 28.0;
    double m_rowGap = 9.0;
    VisualSettings m_settings;
};
