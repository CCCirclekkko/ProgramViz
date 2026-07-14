#include "treemapview.h"

#include <QFileInfo>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

#include <algorithm>
#include <utility>

namespace {

bool isHeaderExtension(const QString &extension)
{
    const QString ext = extension.toLower();
    return ext == QStringLiteral("h") || ext == QStringLiteral("hh")
           || ext == QStringLiteral("hpp") || ext == QStringLiteral("hxx");
}

bool isSourceExtension(const QString &extension)
{
    const QString ext = extension.toLower();
    return ext == QStringLiteral("c") || ext == QStringLiteral("cc")
           || ext == QStringLiteral("cpp") || ext == QStringLiteral("cxx");
}

} // namespace

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

void TreeMapView::setVisualSettings(const VisualSettings &settings)
{
    m_settings = settings;
    update();
}

void TreeMapView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), isDarkTheme() ? QColor(QStringLiteral("#111827"))
                                           : QColor(QStringLiteral("#f3f6fb")));

    if (!m_root) {
        painter.setPen(isDarkTheme() ? QColor(QStringLiteral("#a8b6ca"))
                                     : QColor(QStringLiteral("#5e6f86")));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("选择一个工程目录开始扫描"));
        return;
    }

    for (const LayoutItem &item : std::as_const(m_items)) {
        const bool active = item.node == m_hovered || item.node == m_selected;
        QRectF nodeRect = item.rect.adjusted(1.0, 1.0, -1.0, -1.0);
        if (active)
            nodeRect = nodeRect.adjusted(-1.5, -1.5, 1.5, 1.5);

        painter.setPen(QPen(active ? (isDarkTheme() ? QColor(QStringLiteral("#ffffff"))
                                                    : QColor(QStringLiteral("#17243a")))
                                  : (isDarkTheme() ? QColor(255, 255, 255, 42)
                                                   : QColor(44, 62, 91, 44)),
                        active ? 2.0 : 1.0));
        QColor fill = colorFor(item.node);
        if (active)
            fill = fill.lighter(118);
        painter.setBrush(fill);
        painter.drawRoundedRect(nodeRect, 7.0, 7.0);

        if (nodeRect.width() >= 64.0 && nodeRect.height() >= 37.0) {
            painter.setPen(isDarkTheme() ? QColor(QStringLiteral("#f8fafc"))
                                         : QColor(QStringLiteral("#18263c")));
            QFont font = painter.font();
            font.setPointSizeF(std::max(8.0, static_cast<double>(m_settings.fontSize)
                                             - item.node->depth * 0.35));
            font.setBold(item.node->kind != ProjectNodeKind::File);
            painter.setFont(font);
            const QString name = painter.fontMetrics().elidedText(
                item.displayName, Qt::ElideRight, static_cast<int>(nodeRect.width() - 14));
            const QString lineCount = QStringLiteral("%1 行").arg(item.displayCodeLines);
            const QRectF nameRect = nodeRect.adjusted(7, 4, -7, -nodeRect.height() / 2.0);
            const QRectF linesRect = nodeRect.adjusted(7, nodeRect.height() / 2.0, -7, -4);
            painter.drawText(nameRect, Qt::AlignCenter, name);
            painter.setOpacity(0.78);
            painter.drawText(linesRect, Qt::AlignCenter, lineCount);
            painter.setOpacity(1.0);
        }

        if (item.members.size() > 1) {
            painter.setPen(QPen(isDarkTheme() ? QColor(255, 255, 255, 74)
                                              : QColor(37, 55, 82, 62),
                                  0.7));
            double dividerX = nodeRect.left();
            for (int memberIndex = 0; memberIndex < item.members.size() - 1; ++memberIndex) {
                dividerX += nodeRect.width() * static_cast<double>(item.members.at(memberIndex)->weight())
                            / static_cast<double>(item.displayWeight);
                painter.drawLine(QPointF(dividerX, nodeRect.top() + 4),
                                 QPointF(dividerX, nodeRect.bottom() - 4));
            }
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
    m_items.push_back({node, rect, {node}, node->name, node->lines.code, node->weight()});
    if (node->children.empty())
        return;

    constexpr double siblingGap = 5.0;
    const QVector<DisplayGroup> groups = displayGroups(node);
    const double availableWidth = std::max(1.0, width - siblingGap * (groups.size() - 1));
    qint64 totalWeight = 0;
    for (const DisplayGroup &group : groups)
        totalWeight += group.weight;
    totalWeight = std::max<qint64>(1, totalWeight);
    double childX = x;

    for (int groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
        const DisplayGroup &group = groups.at(groupIndex);
        double childWidth = availableWidth * static_cast<double>(group.weight)
                            / static_cast<double>(totalWeight);
        if (groupIndex == groups.size() - 1)
            childWidth = std::max(1.0, x + width - childX);
        ProjectNode *representative = group.members.front();
        if (group.members.size() == 1 && representative->isFolder()) {
            layoutNode(representative, childX, childWidth);
        } else {
            const double y = 18.0 + representative->depth * (m_rowHeight + m_rowGap);
            m_items.push_back({representative,
                               QRectF(childX, y, childWidth, m_rowHeight),
                               group.members,
                               group.displayName,
                               group.codeLines,
                               group.weight});
        }
        childX += childWidth + siblingGap;
    }
}

QVector<TreeMapView::DisplayGroup> TreeMapView::displayGroups(ProjectNode *node) const
{
    QVector<DisplayGroup> groups;
    for (const auto &child : node->children) {
        ProjectNode *candidate = child.get();
        const bool pairCandidate = candidate->kind == ProjectNodeKind::File
                                    && (isHeaderExtension(QFileInfo(candidate->name).suffix())
                                        || isSourceExtension(QFileInfo(candidate->name).suffix()));
        const QString stem = QFileInfo(candidate->name).completeBaseName().toLower();
        int existingIndex = -1;
        if (pairCandidate) {
            for (int index = 0; index < groups.size(); ++index) {
                const DisplayGroup &group = groups.at(index);
                if (group.displayName.toLower() == stem && group.members.size() == 1
                    && group.members.front()->kind == ProjectNodeKind::File
                    && ((isHeaderExtension(QFileInfo(group.members.front()->name).suffix())
                         && isSourceExtension(QFileInfo(candidate->name).suffix()))
                        || (isSourceExtension(QFileInfo(group.members.front()->name).suffix())
                            && isHeaderExtension(QFileInfo(candidate->name).suffix())))) {
                    existingIndex = index;
                    break;
                }
            }
        }
        if (existingIndex >= 0) {
            DisplayGroup &group = groups[existingIndex];
            group.members.push_back(candidate);
            group.displayName = stem;
            group.weight += candidate->weight();
            group.codeLines += candidate->lines.code;
        } else {
            DisplayGroup group;
            group.members.push_back(candidate);
            group.displayName = candidate->name;
            group.weight = candidate->weight();
            group.codeLines = candidate->lines.code;
            groups.push_back(group);
        }
    }
    return groups;
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
    const int hue = m_settings.colorScheme == ColorScheme::DifferentHueByLevel
                        ? (m_settings.hue + depth * 43) % 360
                        : m_settings.hue;
    const int saturation = std::max(18, m_settings.saturation - depth * 9);
    const int intensity = std::clamp(m_settings.intensity - (isDarkTheme() ? depth * 2 : depth * 3),
                                     36, 96);
    return colorFromHsi(hue, saturation, intensity);
}

QString TreeMapView::labelFor(const LayoutItem &item) const
{
    if (!item.node)
        return {};
    return QStringLiteral("%1\n%2 行").arg(item.displayName).arg(item.displayCodeLines);
}

bool TreeMapView::isDarkTheme() const
{
    if (m_settings.theme == ThemeMode::Dark)
        return true;
    if (m_settings.theme == ThemeMode::Light)
        return false;
    return palette().color(QPalette::Window).lightness() < 128;
}
