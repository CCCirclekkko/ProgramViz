#include "treemapview.h"

#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

#include <algorithm>
#include <utility>

TreeMapView::TreeMapView(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(480, 360);
    setFocusPolicy(Qt::StrongFocus);
}

void TreeMapView::setRoot(ProjectNode *root)
{
    m_root = root;
    m_hovered = nullptr;
    m_selected = root;
    rebuildLayout();
    update();
    emit nodeSelected(m_selected);
}

ProjectNode *TreeMapView::root() const
{
    return m_root;
}

void TreeMapView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(QStringLiteral("#101827")));

    if (!m_root) {
        painter.setPen(QColor(QStringLiteral("#9aa8bd")));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("选择一个工程目录开始扫描"));
        return;
    }

    for (const LayoutItem &item : std::as_const(m_items)) {
        const bool active = item.node == m_hovered || item.node == m_selected;
        QRectF nodeRect = item.rect.adjusted(1.0, 1.0, -1.0, -1.0);
        if (active)
            nodeRect = nodeRect.adjusted(-1.5, -1.5, 1.5, 1.5);

        painter.setPen(QPen(active ? QColor(QStringLiteral("#f8fafc"))
                                  : QColor(255, 255, 255, 34),
                        active ? 2.0 : 1.0));
        QColor fill = colorFor(item.node);
        if (active)
            fill = fill.lighter(118);
        painter.setBrush(fill);
        painter.drawRoundedRect(nodeRect, 7.0, 7.0);

        if (nodeRect.width() >= 64.0 && nodeRect.height() >= 25.0) {
            painter.setPen(QColor(QStringLiteral("#f8fafc")));
            QFont font = painter.font();
            font.setPointSizeF(std::max(8.0, 10.0 - item.node->depth * 0.35));
            font.setBold(item.node->kind != ProjectNodeKind::File);
            painter.setFont(font);
            const QString label = labelFor(item.node);
            painter.drawText(nodeRect.adjusted(7, 3, -7, -3),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             painter.fontMetrics().elidedText(label, Qt::ElideRight,
                                                              static_cast<int>(nodeRect.width() - 14)));
        }
    }
}

void TreeMapView::mouseMoveEvent(QMouseEvent *event)
{
    ProjectNode *node = nodeAt(event->position());
    if (node == m_hovered)
        return;
    m_hovered = node;
    update();
    emit nodeHovered(node);
}

void TreeMapView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    ProjectNode *node = nodeAt(event->position());
    if (!node)
        return;
    m_selected = node;
    update();
    emit nodeSelected(node);
}

void TreeMapView::leaveEvent(QEvent *)
{
    m_hovered = nullptr;
    update();
    emit nodeHovered(nullptr);
}

void TreeMapView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    rebuildLayout();
}

void TreeMapView::rebuildLayout()
{
    m_items.clear();
    if (!m_root)
        return;

    m_maxDepth = 0;
    QVector<ProjectNode *> pending{m_root};
    while (!pending.isEmpty()) {
        ProjectNode *node = pending.takeLast();
        m_maxDepth = std::max(m_maxDepth, node->depth);
        for (const auto &child : node->children)
            pending.push_back(child.get());
    }
    const double usableHeight = std::max(1.0, height() - 36.0);
    m_rowGap = std::min(9.0, std::max(4.0, usableHeight / (m_maxDepth + 1) / 4.0));
    m_rowHeight = std::max(18.0,
                           (usableHeight - m_rowGap * m_maxDepth) / (m_maxDepth + 1));
    layoutNode(m_root, 18.0, std::max(1.0, width() - 36.0));
}

void TreeMapView::layoutNode(ProjectNode *node, double x, double width)
{
    if (!node)
        return;
    const double y = 18.0 + node->depth * (m_rowHeight + m_rowGap);
    const QRectF rect(x, y, width, m_rowHeight);
    m_items.push_back({node, rect});
    if (node->children.empty())
        return;

    constexpr double siblingGap = 5.0;
    const double availableWidth = std::max(1.0, width - siblingGap * (node->children.size() - 1));
    const qint64 totalWeight = std::max<qint64>(1, node->weight());
    double childX = x;

    for (const auto &child : node->children) {
        double childWidth = availableWidth * static_cast<double>(child->weight())
                            / static_cast<double>(totalWeight);
        if (child.get() == node->children.back().get())
            childWidth = std::max(1.0, x + width - childX);
        layoutNode(child.get(), childX, childWidth);
        childX += childWidth + siblingGap;
    }
}

ProjectNode *TreeMapView::nodeAt(const QPointF &point) const
{
    for (auto it = m_items.crbegin(); it != m_items.crend(); ++it) {
        if (it->rect.contains(point))
            return it->node;
    }
    return nullptr;
}

QColor TreeMapView::colorFor(const ProjectNode *node) const
{
    const int depth = node ? node->depth : 0;
    const int hue = (207 + depth * 11) % 360;
    const int saturation = node && node->kind == ProjectNodeKind::File ? 58 : 64;
    const int lightness = std::clamp(32 + depth * 4, 28, 49);
    return QColor::fromHsl(hue, saturation, lightness);
}

QString TreeMapView::labelFor(const ProjectNode *node) const
{
    if (!node)
        return {};
    if (node->kind == ProjectNodeKind::File)
        return QStringLiteral("%1  ·  %2 行").arg(node->name).arg(node->lines.code);
    return QStringLiteral("%1  ·  %2 行").arg(node->name).arg(node->lines.code);
}
