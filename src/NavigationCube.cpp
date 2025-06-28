#include "CADViewer.h"
#include <QPainter>
#include <QMouseEvent>

namespace HybridCAD {

NavigationCube::NavigationCube(QWidget *parent)
    : QWidget(parent)
    , m_isHovered(false)
{
    setFixedSize(80, 80);
    setupFaces();
}

NavigationCube::~NavigationCube() = default;

void NavigationCube::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw cube background
    painter.fillRect(rect(), QColor(40, 40, 40));

    // Draw faces
    for (const auto& faceName : m_faceNames) {
        QRect faceRect = m_faceRects[faceName];
        if (m_isHovered && faceName == m_hoveredFace) {
            painter.fillRect(faceRect, QColor(100, 100, 100));
        } else {
            painter.fillRect(faceRect, QColor(60, 60, 60));
        }
        painter.setPen(Qt::white);
        painter.drawText(faceRect, Qt::AlignCenter, faceName);
    }
}

void NavigationCube::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QString face = getFaceFromPosition(event->pos());
        if (!face.isEmpty()) {
            emit viewChanged(face);
        }
    }
}

void NavigationCube::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    m_isHovered = true;
    update();
}

void NavigationCube::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_isHovered = false;
    m_hoveredFace = "";
    update();
}

void NavigationCube::setupFaces()
{
    m_faceNames << "Front" << "Back" << "Left" << "Right" << "Top" << "Bottom";
    
    m_faceRects["Top"] = QRect(25, 0, 30, 20);
    m_faceRects["Bottom"] = QRect(25, 60, 30, 20);
    m_faceRects["Left"] = QRect(0, 25, 20, 30);
    m_faceRects["Right"] = QRect(60, 25, 20, 30);
    m_faceRects["Front"] = QRect(25, 25, 30, 30);
    m_faceRects["Back"] = QRect(25, 25, 30, 30); // Not visible, but for completeness
}

QString NavigationCube::getFaceFromPosition(const QPoint& pos)
{
    for (auto it = m_faceRects.begin(); it != m_faceRects.end(); ++it) {
        if (it.value().contains(pos)) {
            return it.key();
        }
    }
    return "";
}

} // namespace HybridCAD
