#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <memory>

#include "CADTypes.h"

namespace HybridCAD {

class GeometryManager;
class MeshManager;

class CADViewer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit CADViewer(QWidget *parent = nullptr);
    ~CADViewer();
    
    // Camera control
    void resetView();
    void setView(const QVector3D& eye, const QVector3D& center, const QVector3D& up);
    void frontView();
    void backView();
    void leftView();
    void rightView();
    void topView();
    void bottomView();
    void isometricView();
    
    // View settings
    void setWireframeMode(bool enabled);
    void setGridVisible(bool visible);
    void setAxesVisible(bool visible);
    void setBackgroundColor(const QColor& color);
    
    // Object management
    void addObject(CADObjectPtr object);
    void removeObject(CADObjectPtr object);
    void clearObjects();
    const CADObjectList& getObjects() const { return m_objects; }
    
    // Selection
    void selectObject(CADObjectPtr object);
    void deselectAll();
    CADObjectPtr getSelectedObject() const;
    const std::vector<CADObjectPtr>& getSelectedObjects() const { return m_selectedObjects; }
    
    // Coordinate conversion
    QVector3D screenToWorld(const QPoint& screenPos, float depth = 0.0f) const;
    QPoint worldToScreen(const QVector3D& worldPos) const;
    
    // View state
    bool isWireframeMode() const { return m_wireframeMode; }
    bool isGridVisible() const { return m_showGrid; }
    bool isAxesVisible() const { return m_showAxes; }

signals:
    void objectSelected(CADObjectPtr object);
    void objectDeselected(CADObjectPtr object);
    void selectionChanged();
    void coordinatesChanged(const QVector3D& worldPos);

protected:
    // OpenGL overrides
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    
    // Event handling
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void animate();

private:
    void setupShaders();
    void setupGeometry();
    void updateMatrices();
    
    // Rendering methods
    void renderGrid();
    void renderAxes();
    void renderObjects();
    void renderSelectionOutline();
    
    // Camera methods
    void updateCameraPosition();
    void panCamera(float deltaX, float deltaY);
    void rotateCamera(float deltaX, float deltaY);
    void zoomCamera(float delta);
    
    // Selection methods
    CADObjectPtr pickObject(const QPoint& screenPos);
    bool rayIntersectsObject(const QVector3D& rayOrigin, const QVector3D& rayDirection, 
                           CADObjectPtr object, float& distance);
    
    // Geometry rendering
    void renderBox(const Point3D& min, const Point3D& max);
    void renderCylinder(float radius, float height, int segments = 32);
    void renderSphere(float radius, int segments = 32);
    void renderCone(float bottomRadius, float topRadius, float height, int segments = 32);
    
    // Camera state
    QVector3D m_cameraPosition;
    QVector3D m_cameraTarget;
    QVector3D m_cameraUp;
    float m_cameraDistance;
    float m_cameraRotationX;
    float m_cameraRotationY;
    
    // View matrices
    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_modelMatrix;
    
    // OpenGL resources
    std::unique_ptr<QOpenGLShaderProgram> m_shaderProgram;
    std::unique_ptr<QOpenGLShaderProgram> m_gridShaderProgram;
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLBuffer m_indexBuffer;
    QOpenGLVertexArrayObject m_vao;
    
    // Scene objects
    CADObjectList m_objects;
    std::vector<CADObjectPtr> m_selectedObjects;
    GeometryManager* m_geometryManager;
    MeshManager* m_meshManager;
    
    // View settings
    ViewSettings m_viewSettings;
    bool m_wireframeMode;
    bool m_showGrid;
    bool m_showAxes;
    QColor m_backgroundColor;
    
    // Interaction state
    bool m_isDragging;
    bool m_isRotating;
    bool m_isPanning;
    QPoint m_lastMousePos;
    Qt::MouseButton m_dragButton;
    
    // Animation
    QTimer* m_animationTimer;
    
    // Constants
    static constexpr float CAMERA_DISTANCE_MIN = 0.1f;
    static constexpr float CAMERA_DISTANCE_MAX = 1000.0f;
    static constexpr float CAMERA_ZOOM_SPEED = 0.1f;
    static constexpr float CAMERA_ROTATION_SPEED = 0.5f;
    static constexpr float CAMERA_PAN_SPEED = 0.01f;
};

} // namespace HybridCAD 