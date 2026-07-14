#pragma once

#include "projectmodel.h"
#include "visualsettings.h"

#include <QColor>
#include <QRectF>
#include <QString>
#include <QVector>
#include <QWidget>

class QPainter;

class TreeMapView final : public QWidget {
    Q_OBJECT

public:
    explicit TreeMapView(QWidget *parent = nullptr);

    void setRoot(ProjectNode *root);
    ProjectNode *root() const;
    void setVisualSettings(const VisualSettings &settings);
    void setAdaptiveWindow(bool enabled);
    bool adaptiveWindow() const;

signals:
    void nodeHovered(ProjectNode *node);
    void nodeSelected(ProjectNode *node);
    void adaptiveFitAvailable();
    void adaptiveFitUnavailable(int requiredHeight);

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
        QString key;
        QString displayName;
        qint64 weight = 1;
        qint64 codeLines = 0;
    };

    void rebuildLayout();
    double layoutNode(ProjectNode *node, double top);
    QVector<DisplayGroup> displayGroups(ProjectNode *node) const;
    ProjectNode *nodeAt(const QPointF &point) const;
    QColor colorFor(const ProjectNode *node) const;
    bool isDarkTheme() const;
    bool isDescendantOf(const ProjectNode *node, const ProjectNode *ancestor) const;
    void drawText(QPainter &painter, const LayoutItem &item, const QRectF &rect);
    double heightForCodeLines(qint64 codeLines) const;
    double widthForLabel(const QString &label) const;
    double columnX(int depth) const;
    int availableViewportHeight() const;

    ProjectNode *m_root = nullptr;
    ProjectNode *m_hovered = nullptr;
    ProjectNode *m_selected = nullptr;
    QVector<LayoutItem> m_items;
    VisualSettings m_settings;
    int m_maxDepth = 0;
    double m_columnGap = 10.0;
    double m_textHeight = 16.0;
    double m_minModuleWidth = 6.0;
    QVector<double> m_columnWidths;
    double m_contentWidth = 480.0;
    double m_contentHeight = 360.0;
    double m_lineHeightScale = 0.2;
    double m_fixedLineHeightScale = 0.2;
    double m_minBlockHeight = 48.0;
    bool m_adaptiveWindow = false;
};
