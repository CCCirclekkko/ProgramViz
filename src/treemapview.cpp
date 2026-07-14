#include "treemapview.h"

#include <QFileInfo>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

#include <algorithm>
#include <cmath>
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

bool isHeaderSourcePair(const ProjectNode *first, const ProjectNode *second)
{
    if (!first || !second || first->kind != ProjectNodeKind::File
        || second->kind != ProjectNodeKind::File) {
        return false;
    }
    const QString firstExtension = QFileInfo(first->name).suffix();
    const QString secondExtension = QFileInfo(second->name).suffix();
    return (isHeaderExtension(firstExtension) && isSourceExtension(secondExtension))
           || (isSourceExtension(firstExtension) && isHeaderExtension(secondExtension));
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
    rebuildLayout();
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

    const QColor connectorColor = isDarkTheme() ? QColor(255, 255, 255, 40)
                                                : QColor(37, 55, 82, 48);
    painter.setPen(QPen(connectorColor, 1.0));
    for (const LayoutItem &item : std::as_const(m_items)) {
        if (!item.node || !item.node->parent)
            continue;
        const LayoutItem *parentItem = nullptr;
        for (const LayoutItem &candidate : std::as_const(m_items)) {
            if (candidate.node == item.node->parent) {
                parentItem = &candidate;
                break;
            }
        }
        if (!parentItem)
            continue;
        const QPointF start(parentItem->rect.right(), parentItem->rect.center().y());
        const QPointF end(item.rect.left(), item.rect.center().y());
        const double middleX = (start.x() + end.x()) / 2.0;
        painter.drawLine(start, QPointF(middleX, start.y()));
        painter.drawLine(QPointF(middleX, start.y()), QPointF(middleX, end.y()));
        painter.drawLine(QPointF(middleX, end.y()), end);
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
            fill = fill.lighter(114);
        painter.setBrush(fill);
        painter.drawRoundedRect(nodeRect, 8.0, 8.0);

        if (item.members.size() > 1) {
            painter.setPen(QPen(isDarkTheme() ? QColor(255, 255, 255, 88)
                                              : QColor(37, 55, 82, 72),
                                  0.8));
            const double dividerX = nodeRect.left() + std::clamp(nodeRect.width() * 0.23,
                                                                  44.0,
                                                                  72.0);
            painter.drawLine(QPointF(dividerX, nodeRect.top() + 5),
                             QPointF(dividerX, nodeRect.bottom() - 5));
        }

        if (nodeRect.width() >= 58.0 && nodeRect.height() >= m_minBlockHeight - 4.0)
            drawText(painter, item, nodeRect);
    }
}

void TreeMapView::drawText(QPainter &painter, const LayoutItem &item, const QRectF &rect)
{
    const QColor textColor = isDarkTheme() ? QColor(QStringLiteral("#f8fafc"))
                                           : QColor(QStringLiteral("#18263c"));
    QFont nameFont = painter.font();
    nameFont.setPointSize(m_settings.fontSize);
    nameFont.setBold(item.node->kind != ProjectNodeKind::File);
    QFont lineFont = nameFont;
    lineFont.setBold(false);
    const int rowGap = 2;

    const auto drawRows = [&](const QRectF &area,
                              const QString &name,
                              qint64 codeLines,
                              QFont rowNameFont,
                              QFont rowLineFont) {
        const QFontMetrics nameMetrics(rowNameFont);
        const QFontMetrics lineMetrics(rowLineFont);
        const double rowHeight = std::max(nameMetrics.height(), lineMetrics.height());
        const double pairHeight = rowHeight * 2.0 + rowGap;
        const double top = area.center().y() - pairHeight / 2.0;
        const QRectF nameArea(area.left() + 4, top, area.width() - 8, rowHeight);
        const QRectF linesArea(area.left() + 4, top + rowHeight + rowGap,
                               area.width() - 8, rowHeight);
        painter.setPen(textColor);
        painter.setFont(rowNameFont);
        painter.drawText(nameArea, Qt::AlignCenter,
                         nameMetrics.elidedText(name, Qt::ElideRight,
                                                std::max(1, static_cast<int>(nameArea.width()))));
        painter.setFont(rowLineFont);
        painter.setOpacity(0.82);
        painter.drawText(linesArea, Qt::AlignCenter,
                         QStringLiteral("%1 行").arg(codeLines));
        painter.setOpacity(1.0);
    };

    if (item.members.size() == 1) {
        drawRows(rect, item.displayName, item.displayCodeLines, nameFont, lineFont);
        return;
    }

    const double leftWidth = std::clamp(rect.width() * 0.115, 36.0, 44.0);
    const QRectF leftArea(rect.left(), rect.top(), leftWidth, rect.height());
    const QRectF rightArea(rect.left() + leftWidth, rect.top(),
                           rect.width() - leftWidth, rect.height());
    const ProjectNode *header = item.members.front();
    const ProjectNode *source = item.members.back();
    const QString headerExtension = QStringLiteral(".%1").arg(QFileInfo(header->name).suffix());
    const QString sourceName = QStringLiteral("%1.%2")
                                   .arg(item.displayName, QFileInfo(source->name).suffix());
    QFont leftNameFont = nameFont;
    leftNameFont.setPointSize(std::max(11, m_settings.fontSize - 4));
    QFont leftLineFont = leftNameFont;
    leftLineFont.setPointSize(std::max(10, m_settings.fontSize - 6));
    drawRows(leftArea, headerExtension, header->lines.code, leftNameFont, leftLineFont);
    drawRows(rightArea, sourceName, source->lines.code, nameFont, lineFont);
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
    if (!m_root) {
        m_contentWidth = 480.0;
        m_contentHeight = 360.0;
        setMinimumSize(480, 360);
        return;
    }

    m_maxDepth = 0;
    int maxNameWidth = 0;
    QFont font = this->font();
    font.setPointSize(m_settings.fontSize);
    const QFontMetrics metrics(font);
    QVector<ProjectNode *> pending{m_root};
    while (!pending.isEmpty()) {
        ProjectNode *node = pending.takeLast();
        m_maxDepth = std::max(m_maxDepth, node->depth);
        maxNameWidth = std::max(maxNameWidth, metrics.horizontalAdvance(node->name));
        for (const auto &child : node->children)
            pending.push_back(child.get());
    }
    m_minBlockHeight = std::max(44.0, metrics.height() * 2.0 + 8.0);
    m_columnWidth = std::max(210.0, static_cast<double>(maxNameWidth + 50));
    m_lineHeightScale = std::max(0.08,
                                 (height() - 36.0) * 0.62
                                     / static_cast<double>(std::max<qint64>(1, m_root->lines.code)));
    const double subtreeHeight = layoutNode(m_root, 18.0, 18.0);

    m_contentWidth = std::max(480.0,
                              36.0 + (m_maxDepth + 1) * m_columnWidth
                                  + m_maxDepth * m_columnGap);
    double bottom = 18.0 + subtreeHeight;
    for (const LayoutItem &item : std::as_const(m_items))
        bottom = std::max(bottom, item.rect.bottom());
    m_contentHeight = std::max(360.0, bottom + 18.0);
    setMinimumSize(static_cast<int>(std::ceil(m_contentWidth)),
                   static_cast<int>(std::ceil(m_contentHeight)));
}

double TreeMapView::layoutNode(ProjectNode *node, double x, double top)
{
    if (!node)
        return 0.0;

    const QVector<DisplayGroup> groups = displayGroups(node);
    const double nodeHeight = heightForCodeLines(node->lines.code);
    if (groups.isEmpty()) {
        m_items.push_back({node,
                           QRectF(x, top, m_columnWidth, nodeHeight),
                           {node},
                           node->name,
                           node->lines.code,
                           node->weight()});
        return nodeHeight;
    }

    constexpr double siblingGap = 6.0;
    const double childX = x + m_columnWidth + m_columnGap;
    double childTop = top;
    double subtreeHeight = 0.0;
    for (int groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
        const DisplayGroup &group = groups.at(groupIndex);
        ProjectNode *representative = group.members.front();
        const double groupHeight = group.members.size() == 1 && representative->isFolder()
                                       ? layoutNode(representative, childX, childTop)
                                       : heightForCodeLines(group.codeLines);
        if (group.members.size() != 1 || !representative->isFolder()) {
            m_items.push_back({representative,
                               QRectF(childX, childTop, m_columnWidth, groupHeight),
                               group.members,
                               group.displayName,
                               group.codeLines,
                               group.weight});
        }
        childTop += groupHeight;
        if (groupIndex != groups.size() - 1)
            childTop += siblingGap;
        subtreeHeight = childTop - top;
    }

    const double nodeTop = top;
    m_items.push_back({node,
                       QRectF(x, nodeTop, m_columnWidth, subtreeHeight),
                       {node},
                       node->name,
                       node->lines.code,
                       node->weight()});
    return subtreeHeight;
}

double TreeMapView::heightForCodeLines(qint64 codeLines) const
{
    return std::max(m_minBlockHeight,
                    static_cast<double>(std::max<qint64>(1, codeLines)) * m_lineHeightScale);
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
                if (group.key == stem && group.members.size() == 1
                    && isHeaderSourcePair(group.members.front(), candidate)) {
                    existingIndex = index;
                    break;
                }
            }
        }
        if (existingIndex >= 0) {
            DisplayGroup &group = groups[existingIndex];
            if (isHeaderExtension(QFileInfo(candidate->name).suffix()))
                group.members.prepend(candidate);
            else
                group.members.push_back(candidate);
            group.displayName = stem;
            group.weight += candidate->weight();
            group.codeLines += candidate->lines.code;
        } else {
            DisplayGroup group;
            group.members.push_back(candidate);
            group.key = candidate->kind == ProjectNodeKind::File ? stem : candidate->name.toLower();
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

bool TreeMapView::isDarkTheme() const
{
    if (m_settings.theme == ThemeMode::Dark)
        return true;
    if (m_settings.theme == ThemeMode::Light)
        return false;
    return palette().color(QPalette::Window).lightness() < 128;
}
