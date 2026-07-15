#include "treemapview.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
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

void TreeMapView::setOverlayDate(const QString &date)
{
    m_overlayDate = date;
    update();
}

void TreeMapView::setAdaptiveWindow(bool enabled)
{
    if (m_adaptiveWindow == enabled)
        return;
    m_adaptiveWindow = enabled;
    rebuildLayout();
    update();
}

bool TreeMapView::adaptiveWindow() const
{
    return m_adaptiveWindow;
}

TreeMapView::LayoutSnapshot TreeMapView::captureLayout(ProjectNode *root,
                                                       const QSize &targetSize,
                                                       const VisualSettings &settings,
                                                       bool fitHeight,
                                                       double fixedLineHeightScale) const
{
    TreeMapView renderer;
    renderer.setFont(font());
    renderer.m_settings = settings;
    renderer.m_adaptiveWindow = fitHeight;
    renderer.m_fixedLineHeightScale = fixedLineHeightScale > 0.0
                                          ? fixedLineHeightScale
                                          : m_fixedLineHeightScale;
    renderer.m_lineHeightScale = renderer.m_fixedLineHeightScale;
    renderer.resize(targetSize);
    renderer.setRoot(root);
    const QSize requiredSize(
        std::max(targetSize.width(), static_cast<int>(std::ceil(renderer.m_contentWidth))),
        std::max(targetSize.height(), static_cast<int>(std::ceil(renderer.m_contentHeight))));
    renderer.resize(requiredSize);

    LayoutSnapshot snapshot;
    snapshot.root = root;
    snapshot.size = renderer.size();
    snapshot.items.reserve(renderer.m_items.size());
    for (const LayoutItem &item : std::as_const(renderer.m_items))
        snapshot.items.push_back({item, snapshotKey(root, item)});
    return snapshot;
}

QString TreeMapView::snapshotKey(ProjectNode *root, const LayoutItem &item) const
{
    if (!root || !item.node)
        return {};
    if (item.members.isEmpty())
        return QStringLiteral("node:%1").arg(QDir(root->path).relativeFilePath(item.node->path));

    QStringList paths;
    const QDir base(root->path);
    for (ProjectNode *member : item.members)
        paths.push_back(base.relativeFilePath(member->path));
    paths.sort();
    return QStringLiteral("group:%1").arg(paths.join(QStringLiteral("|")));
}

QRectF TreeMapView::scaledFromLeft(const QRectF &rect, double factor)
{
    return QRectF(rect.left(), rect.top(),
                  rect.width() * std::clamp(factor, 0.0, 1.0), rect.height());
}

QImage TreeMapView::renderTransition(ProjectNode *fromRoot,
                                     ProjectNode *toRoot,
                                     double progress,
                                     const QSize &targetSize) const
{
    return renderTransition(fromRoot, toRoot, progress, targetSize, m_settings,
                            m_adaptiveWindow, m_fixedLineHeightScale);
}

QImage TreeMapView::renderTransition(ProjectNode *fromRoot,
                                     ProjectNode *toRoot,
                                     double progress,
                                     const QSize &targetSize,
                                     const VisualSettings &settings,
                                     bool fitHeight,
                                     double fixedLineHeightScale,
                                     const QString &overlayDate) const
{
    if (!fromRoot && !toRoot)
        return {};

    const double clampedProgress = std::clamp(progress, 0.0, 1.0);
    LayoutSnapshot before = captureLayout(fromRoot ? fromRoot : toRoot, targetSize,
                                          settings, fitHeight, fixedLineHeightScale);
    LayoutSnapshot after = captureLayout(toRoot ? toRoot : fromRoot, targetSize,
                                         settings, fitHeight, fixedLineHeightScale);
    const QSize canvas(std::max({targetSize.width(), before.size.width(), after.size.width()}),
                       std::max({targetSize.height(), before.size.height(), after.size.height()}));
    if (canvas != targetSize) {
        before = captureLayout(fromRoot ? fromRoot : toRoot, canvas,
                               settings, fitHeight, fixedLineHeightScale);
        after = captureLayout(toRoot ? toRoot : fromRoot, canvas,
                              settings, fitHeight, fixedLineHeightScale);
    }

    QHash<QString, int> beforeIndexes;
    for (int index = 0; index < before.items.size(); ++index)
        beforeIndexes.insert(before.items.at(index).key, index);
    QHash<QString, int> afterIndexes;
    for (int index = 0; index < after.items.size(); ++index)
        afterIndexes.insert(after.items.at(index).key, index);

    QVector<LayoutItem> blended;
    blended.reserve(before.items.size() + after.items.size());
    const double appearanceProgress = std::clamp(clampedProgress / 0.5, 0.0, 1.0);

    for (const SnapshotItem &oldItem : std::as_const(before.items)) {
        if (afterIndexes.contains(oldItem.key))
            continue;
        LayoutItem item = oldItem.item;
        // Removed modules collapse toward their left edge. Keeping the left
        // edge fixed avoids the old center-based jump.
        item.rect = scaledFromLeft(item.rect, 1.0 - appearanceProgress);
        blended.push_back(item);
    }

    for (const SnapshotItem &newItem : std::as_const(after.items)) {
        const auto oldIndex = beforeIndexes.constFind(newItem.key);
        if (oldIndex == beforeIndexes.cend()) {
            LayoutItem item = newItem.item;
            // New modules grow from their left edge while retaining the
            // static layout's top and height.
            item.rect = scaledFromLeft(item.rect, appearanceProgress);
            blended.push_back(item);
            continue;
        }

        const LayoutItem &oldItem = before.items.at(oldIndex.value()).item;
        LayoutItem item = newItem.item;
        item.rect = QRectF(oldItem.rect.left() + (newItem.item.rect.left() - oldItem.rect.left()) * clampedProgress,
                           oldItem.rect.top() + (newItem.item.rect.top() - oldItem.rect.top()) * clampedProgress,
                           oldItem.rect.width() + (newItem.item.rect.width() - oldItem.rect.width()) * clampedProgress,
                           oldItem.rect.height() + (newItem.item.rect.height() - oldItem.rect.height()) * clampedProgress);
        item.displayCodeLines = qRound(oldItem.displayCodeLines
                                       + (newItem.item.displayCodeLines - oldItem.displayCodeLines)
                                             * clampedProgress);
        blended.push_back(item);
    }

    TreeMapView renderer;
    renderer.setFont(font());
    renderer.m_settings = settings;
    renderer.m_adaptiveWindow = fitHeight;
    renderer.m_overlayDate = overlayDate;
    QFont renderFont = renderer.font();
    renderFont.setPointSize(settings.fontSize);
    renderer.m_textHeight = QFontMetrics(renderFont).height();
    renderer.m_root = after.root ? after.root : before.root;
    renderer.resize(canvas);
    renderer.m_contentWidth = canvas.width();
    renderer.m_contentHeight = canvas.height();
    // resizeEvent() rebuilds the normal static layout. Assign the blended
    // items after resizing so the temporary interpolated geometry is what is
    // actually rendered.
    renderer.m_items = blended;

    QImage image(canvas, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    renderer.renderScene(painter);
    return image;
}

double TreeMapView::lineHeightScaleForRoots(const QVector<ProjectNode *> &roots,
                                            const QSize &targetSize,
                                            const VisualSettings &settings) const
{
    if (roots.isEmpty())
        return m_fixedLineHeightScale;

    const double targetHeight = std::max(360, targetSize.height());
    const auto maxContentHeightAt = [this, &roots, &targetSize, &settings](double scale) {
        double maximum = 0.0;
        for (ProjectNode *root : roots) {
            TreeMapView renderer;
            renderer.setFont(font());
            renderer.m_settings = settings;
            renderer.m_adaptiveWindow = false;
            renderer.m_fixedLineHeightScale = scale;
            renderer.m_lineHeightScale = scale;
            renderer.resize(targetSize);
            renderer.setRoot(root);
            maximum = std::max(maximum, renderer.m_contentHeight);
        }
        return maximum;
    };

    if (maxContentHeightAt(0.0) > targetHeight)
        return 0.0;

    double low = 0.0;
    double high = std::max(m_fixedLineHeightScale, 0.2);
    while (maxContentHeightAt(high) <= targetHeight && high < 16.0)
        high *= 2.0;
    for (int iteration = 0; iteration < 28; ++iteration) {
        const double middle = (low + high) / 2.0;
        if (maxContentHeightAt(middle) <= targetHeight)
            low = middle;
        else
            high = middle;
    }
    return low;
}

double TreeMapView::contentWidthForRoots(const QVector<ProjectNode *> &roots,
                                         const QSize &targetSize,
                                         const VisualSettings &settings) const
{
    double maximum = 480.0;
    for (ProjectNode *root : roots) {
        TreeMapView renderer;
        renderer.setFont(font());
        renderer.m_settings = settings;
        renderer.m_adaptiveWindow = false;
        renderer.resize(targetSize);
        renderer.setRoot(root);
        maximum = std::max(maximum, renderer.m_contentWidth);
    }
    return maximum;
}

void TreeMapView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    renderScene(painter);
}

void TreeMapView::renderScene(QPainter &painter)
{
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
        const bool hasHover = m_hovered != nullptr;
        const bool hoveredNode = item.node == m_hovered;
        const bool hoveredChild = hasHover && isDescendantOf(item.node, m_hovered);
        const bool selectedNode = !hasHover && item.node == m_selected;
        const bool strongHighlight = hoveredNode || selectedNode;
        const bool softHighlight = hoveredChild || (hasHover && item.node == m_selected);
        QRectF nodeRect = item.rect.adjusted(1.0, 1.0, -1.0, -1.0);
        if (strongHighlight)
            nodeRect = nodeRect.adjusted(-1.5, -1.5, 1.5, 1.5);

        const QColor strongPen = isDarkTheme() ? QColor(QStringLiteral("#ffffff"))
                                               : QColor(QStringLiteral("#17243a"));
        const QColor softPen = isDarkTheme() ? QColor(255, 255, 255, 68)
                                             : QColor(44, 62, 91, 72);
        const QColor normalPen = isDarkTheme() ? QColor(255, 255, 255, 42)
                                               : QColor(44, 62, 91, 44);
        painter.setPen(QPen(strongHighlight ? strongPen : (softHighlight ? softPen : normalPen),
                            strongHighlight ? 2.0 : (softHighlight ? 1.2 : 1.0)));
        QColor fill = colorFor(item.node);
        if (strongHighlight)
            fill = fill.lighter(114);
        else if (softHighlight)
            fill = fill.lighter(106);
        painter.setBrush(fill);
        const double minRadius = std::max(3.0, m_settings.fontSize * 0.18);
        const double maxRadius = std::max(minRadius + 1.0, m_settings.fontSize * 0.75);
        const double radius = std::clamp(std::min(nodeRect.width(), nodeRect.height())
                                             * m_settings.cornerRatio,
                                         minRadius,
                                         maxRadius);
        painter.drawRoundedRect(nodeRect, radius, radius);

        if (item.members.size() > 1) {
            painter.setPen(QPen(isDarkTheme() ? QColor(255, 255, 255, 88)
                                              : QColor(37, 55, 82, 72),
                                  0.8));
            const double dividerX = nodeRect.left()
                                    + std::clamp(nodeRect.width() * 0.16, 52.0, 62.0);
            painter.drawLine(QPointF(dividerX, nodeRect.top() + 5),
                             QPointF(dividerX, nodeRect.bottom() - 5));
        }

        if (nodeRect.width() >= 58.0 && nodeRect.height() >= m_textHeight)
            drawText(painter, item, nodeRect);
    }

    if (!m_overlayDate.isEmpty()) {
        QFont dateFont = painter.font();
        dateFont.setPointSize(m_settings.fontSize);
        painter.setFont(dateFont);
        painter.setPen(Qt::black);
        painter.drawText(rect().adjusted(8, 8, -8, -8),
                         Qt::AlignRight | Qt::AlignBottom, m_overlayDate);
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
        if (area.height() < m_textHeight * 2.0) {
            painter.setPen(textColor);
            painter.setFont(rowNameFont);
            painter.drawText(area.adjusted(4, 0, -4, 0), Qt::AlignCenter,
                             nameMetrics.elidedText(name, Qt::ElideRight,
                                                    std::max(1, static_cast<int>(area.width() - 8))));
            return;
        }
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

    const double leftWidth = std::clamp(rect.width() * 0.16, 52.0, 62.0);
    const QRectF leftArea(rect.left(), rect.top(), leftWidth, rect.height());
    const QRectF rightArea(rect.left() + leftWidth, rect.top(),
                           rect.width() - leftWidth, rect.height());
    const ProjectNode *header = item.members.front();
    const ProjectNode *source = item.members.back();
    const QString headerExtension = QStringLiteral(".%1").arg(QFileInfo(header->name).suffix());
    const QString sourceName = QStringLiteral("%1.%2")
                                   .arg(item.displayName, QFileInfo(source->name).suffix());
    QFont leftNameFont = nameFont;
    leftNameFont.setPointSize(m_settings.fontSize);
    QFont leftLineFont = leftNameFont;
    leftLineFont.setPointSize(m_settings.fontSize);
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
    if (node->isFolder()) {
        node->collapsed = !node->collapsed;
        rebuildLayout();
        update();
    }
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
    QFont font = this->font();
    font.setPointSize(m_settings.fontSize);
    const QFontMetrics metrics(font);
    QVector<ProjectNode *> pending{m_root};
    while (!pending.isEmpty()) {
        ProjectNode *node = pending.takeLast();
        m_maxDepth = std::max(m_maxDepth, node->depth);
        if (!node->collapsed) {
            for (const auto &child : node->children)
                pending.push_back(child.get());
        }
    }
    m_textHeight = metrics.height();
    m_minModuleWidth = std::max(6.0, m_settings.fontSize * 0.375);
    m_minBlockHeight = std::max(1.0, m_textHeight * m_settings.minHeightRatio);
    m_columnGap = std::max(0, m_settings.columnGap);
    m_columnWidths.fill(m_minModuleWidth, m_maxDepth + 1);
    pending = {m_root};
    while (!pending.isEmpty()) {
        ProjectNode *node = pending.takeLast();
        m_columnWidths[node->depth] = std::max(m_columnWidths[node->depth],
                                               widthForLabel(node->name));
        if (!node->collapsed) {
            for (const DisplayGroup &group : displayGroups(node)) {
                const QString label = group.members.size() > 1
                                          ? QStringLiteral("%1.%2")
                                                .arg(group.displayName,
                                                     QFileInfo(group.members.back()->name).suffix())
                                          : group.displayName;
                const double extraWidth = group.members.size() > 1 ? 54.0 : 0.0;
                m_columnWidths[node->depth + 1] = std::max(
                    m_columnWidths[node->depth + 1], widthForLabel(label) + extraWidth);
            }
        }
        for (const auto &child : node->children)
            if (!node->collapsed)
                pending.push_back(child.get());
    }
    int adaptiveRequiredHeight = 0;
    if (m_adaptiveWindow) {
        const double previousScale = m_lineHeightScale;
        const double targetHeight = std::max(360, availableViewportHeight());
        const auto contentHeightAt = [this](double scale) {
            m_lineHeightScale = scale;
            m_items.clear();
            return layoutNode(m_root, 18.0) + 36.0;
        };

        const double minimumContentHeight = contentHeightAt(0.0);
        if (minimumContentHeight <= targetHeight) {
            double low = 0.0;
            double high = std::max(m_fixedLineHeightScale, 0.2);
            while (contentHeightAt(high) <= targetHeight && high < 16.0)
                high *= 2.0;
            for (int iteration = 0; iteration < 28; ++iteration) {
                const double middle = (low + high) / 2.0;
                if (contentHeightAt(middle) <= targetHeight)
                    low = middle;
                else
                    high = middle;
            }
            m_lineHeightScale = low;
        } else {
            // Keep the previous scale when a new minimum-height constraint
            // cannot fit, so expanding names never changes code proportions.
            m_lineHeightScale = previousScale > 0.0 ? previousScale : m_fixedLineHeightScale;
            adaptiveRequiredHeight = static_cast<int>(std::ceil(minimumContentHeight));
        }
    } else {
        m_lineHeightScale = m_fixedLineHeightScale;
    }
    // Adaptive fitting measures the tree repeatedly. Start the real layout
    // from an empty item list so measured rectangles cannot be painted again.
    m_items.clear();
    const double subtreeHeight = layoutNode(m_root, 18.0);

    m_contentWidth = std::max(480.0, columnX(m_maxDepth) + m_columnWidths.last() + 18.0);
    double bottom = 18.0 + subtreeHeight;
    for (const LayoutItem &item : std::as_const(m_items))
        bottom = std::max(bottom, item.rect.bottom());
    m_contentHeight = std::max(360.0, bottom + 18.0);
    setMinimumSize(static_cast<int>(std::ceil(m_contentWidth)),
                   static_cast<int>(std::ceil(m_contentHeight)));

    if (m_adaptiveWindow) {
        if (adaptiveRequiredHeight > 0)
            emit adaptiveFitUnavailable(adaptiveRequiredHeight);
        else
            emit adaptiveFitAvailable();
    }

}

double TreeMapView::layoutNode(ProjectNode *node, double top)
{
    if (!node)
        return 0.0;

    const QVector<DisplayGroup> groups = displayGroups(node);
    const double nodeHeight = heightForCodeLines(node->lines.code);
    const double nodeX = columnX(node->depth);
    const double nodeWidth = m_columnWidths.at(node->depth);
    if (groups.isEmpty() || node->collapsed) {
        m_items.push_back({node,
                           QRectF(nodeX, top, nodeWidth, nodeHeight),
                           {node},
                           node->name,
                           node->lines.code,
                           node->weight()});
        return nodeHeight;
    }

    const double siblingGap = std::max(0, m_settings.siblingGap);
    double childTop = top;
    double subtreeHeight = 0.0;
    for (int groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
        const DisplayGroup &group = groups.at(groupIndex);
        ProjectNode *representative = group.members.front();
        const double groupHeight = group.members.size() == 1 && representative->isFolder()
                                       ? layoutNode(representative, childTop)
                                       : heightForCodeLines(group.codeLines);
        if (group.members.size() != 1 || !representative->isFolder()) {
            m_items.push_back({representative,
                               QRectF(columnX(representative->depth), childTop,
                                      m_columnWidths.at(representative->depth),
                                      groupHeight),
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
                       QRectF(nodeX, nodeTop, nodeWidth, subtreeHeight),
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

double TreeMapView::widthForLabel(const QString &label) const
{
    QFont font = this->font();
    font.setPointSize(m_settings.fontSize);
    const QFontMetrics metrics(font);
    return std::max(m_minModuleWidth, static_cast<double>(metrics.horizontalAdvance(label) + 28));
}

double TreeMapView::columnX(int depth) const
{
    double x = 18.0;
    for (int index = 0; index < depth && index < m_columnWidths.size(); ++index)
        x += m_columnWidths.at(index) + m_columnGap;
    return x;
}

int TreeMapView::availableViewportHeight() const
{
    return parentWidget() ? parentWidget()->height() : height();
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

bool TreeMapView::isDescendantOf(const ProjectNode *node, const ProjectNode *ancestor) const
{
    if (!node || !ancestor || node == ancestor)
        return false;
    for (ProjectNode *parent = node->parent; parent; parent = parent->parent) {
        if (parent == ancestor)
            return true;
    }
    return false;
}

bool TreeMapView::isDarkTheme() const
{
    if (m_settings.theme == ThemeMode::Dark)
        return true;
    if (m_settings.theme == ThemeMode::Light)
        return false;
    return palette().color(QPalette::Window).lightness() < 128;
}
