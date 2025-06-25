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
#include <QSettings>
#include <QMap>
#include <memory>
#include <array>

#include "CADTypes.h"

namespace HybridCAD {

class GeometryManager;
class MeshManager;

// Navigation cube widget for viewport navigation like Blender
class NavigationCube : public QWidget
{
    Q_OBJECT

public:
    explicit NavigationCube(QWidget *parent = nullptr);
    ~NavigationCube();

signals:
    void viewChanged(const QString& viewName);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void setupFaces();
    QString getFaceFromPosition(const QPoint& pos);
    
    QStringList m_faceNames;
    QMap<QString, QRect> m_faceRects;
    QString m_hoveredFace;
    bool m_isHovered;
};

// Grid plane enumeration
enum class GridPlane {
    XY_PLANE,
    XZ_PLANE,
    YZ_PLANE
};

// Shape placement states
enum class PlacementState {
    NONE,
    SELECTING_SHAPE,
    SETTING_START_POINT,
    SETTING_END_POINT,
    PLACING,
    DRAGGING_TO_SIZE
};

// Tool states for shape creation and editing
enum class ActiveTool {
    SELECT,
    PLACE_SHAPE,
    EXTRUDE_2D,
    ERASER,
    MEASURE,
    SKETCH_LINE,
    SKETCH_RECTANGLE,
    SKETCH_CIRCLE
};

// Camera movement states
enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// Keybinding actions
enum class KeyAction {
    TOGGLE_GRID,
    TOGGLE_WIREFRAME,
    TOGGLE_AXES,
    TOGGLE_GRID_XY,
    TOGGLE_GRID_XZ,
    TOGGLE_GRID_YZ,
    TOGGLE_MULTI_PLANE_GRID,
    RESET_VIEW,
    FRONT_VIEW,
    BACK_VIEW,
    LEFT_VIEW,
    RIGHT_VIEW,
    TOP_VIEW,
    BOTTOM_VIEW,
    ISOMETRIC_VIEW,
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    PLACE_SHAPE,
    DELETE_SELECTED,
    SELECT_ALL,
    DESELECT_ALL,
    SKETCH_LINE,
    SKETCH_RECTANGLE,
    SKETCH_CIRCLE,
    CANCEL_CURRENT_ACTION
};

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
    
    // Camera movement
    void moveCamera(CameraMovement direction, float deltaTime = 0.016f);
    void setCameraSpeed(float speed) { m_cameraSpeed = speed; }
    float getCameraSpeed() const { return m_cameraSpeed; }
    
    // View settings
    void setWireframeMode(bool enabled);
    void setGridVisible(bool visible);
    void setAxesVisible(bool visible);
    void setBackgroundColor(const QColor& color);
    
    // Grid control
    void setGridPlane(GridPlane plane);
    GridPlane getGridPlane() const { return m_gridPlane; }
    void setGridSize(float size);
    float getGridSize() const { return m_gridSize; }
    
    // Multi-plane grid support
    void setMultiPlaneGridVisible(bool visible);
    bool isMultiPlaneGridVisible() const { return m_showMultiPlaneGrid; }
    void toggleGridPlane(GridPlane plane);
    bool isGridPlaneVisible(GridPlane plane) const;
    
    // Snap functionality
    void setSnapToGrid(bool enabled);
    bool isSnapToGrid() const { return m_snapToGrid; }
    QVector3D snapToGrid(const QVector3D& position) const;
    
    // Shape placement workflow
    void setActiveTool(ActiveTool tool);
    ActiveTool getActiveTool() const { return m_activeTool; }
    void setShapeToPlace(ObjectType shapeType);
    void startShapePlacement();
    void cancelShapePlacement();
    
    // Sketching functionality
    void startLineSketch();
    void startRectangleSketch();
    void startCircleSketch();
    void finishCurrentSketch();
    void cancelCurrentSketch();
    
    // Extrusion functionality
    void startExtrusionMode(CADObjectPtr object);
    void setExtrusionDistance(float distance);
    void finishExtrusion();
    
    // Eraser functionality
    void setEraserMode(bool enabled);
    bool isEraserMode() const { return m_eraserMode; }
    void setEraserShape(ObjectType shapeType);
    
    // Object management
    void addObject(CADObjectPtr object);
    void removeObject(CADObjectPtr object);
    void clearObjects();
    const CADObjectList& getObjects() const { return m_objects; }
    
    // Selection
    void selectObject(CADObjectPtr object);
    void deselectAll();
    void selectAll();
    void deleteSelected();
    CADObjectPtr getSelectedObject() const;
    const std::vector<CADObjectPtr>& getSelectedObjects() const { return m_selectedObjects; }
    
    // Coordinate conversion
    QVector3D screenToWorld(const QPoint& screenPos, float depth = 0.0f);
    QPoint worldToScreen(const QVector3D& worldPos);
    
    // View state
    bool isWireframeMode() const { return m_wireframeMode; }
    bool isGridVisible() const { return m_showGrid; }
    bool isAxesVisible() const { return m_showAxes; }
    
    // Keybinding management
    void setKeyBinding(KeyAction action, const QKeySequence& keySequence);
    QKeySequence getKeyBinding(KeyAction action) const;
    void loadKeyBindings();
    void saveKeyBindings();
    void resetKeyBindingsToDefault();
    QMap<KeyAction, QKeySequence> getDefaultKeyBindings() const;

signals:
    void objectSelected(CADObjectPtr object);
    void objectDeselected(CADObjectPtr object);
    void selectionChanged();
    void coordinatesChanged(const QVector3D& worldPos);
    void shapePlacementStarted(ObjectType shapeType);
    void shapePlacementFinished(CADObjectPtr object);
    void extrusionStarted(CADObjectPtr object);
    void extrusionFinished(CADObjectPtr object);
    void sketchStarted(ActiveTool sketchType);
    void sketchFinished(CADObjectPtr object);
    void gridToggled(bool visible);
    void wireframeToggled(bool enabled);
    void axesToggled(bool visible);

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
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void animate();

private:
    void setupShaders();
    void setupGeometry();
    void updateMatrices();
    void setupNavigationCube();
    void setupDefaultKeyBindings();
    
    // Rendering methods
    void renderGrid();
    void renderMultiPlaneGrid();
    void renderAxes();
    void renderObjects();
    void renderSelectionOutline();
    void renderPlacementPreview();
    void renderExtrusionPreview();
    void renderEraserPreview();
    void renderSketchPreview();
    void renderSizeRuler();
    
    // Camera methods
    void updateCameraPosition();
    void panCamera(float deltaX, float deltaY);
    void rotateCamera(float deltaX, float deltaY);
    void zoomCamera(float delta);
    void processKeyboardInput();
    
    // Selection methods
    CADObjectPtr pickObject(const QPoint& screenPos);
    bool rayIntersectsObject(const QVector3D& rayOrigin, const QVector3D& rayDirection, 
                           CADObjectPtr object, float& distance);
    
    // Shape placement methods
    void handleShapePlacementClick(const QPoint& screenPos);
    void updatePlacementPreview(const QPoint& screenPos);
    CADObjectPtr createShapeAtPoints(ObjectType shapeType, const QVector3D& startPoint, const QVector3D& endPoint);
    
    // Sketching methods
    void handleSketchClick(const QPoint& screenPos);
    void updateSketchPreview(const QPoint& screenPos);
    CADObjectPtr createLineFromPoints(const QVector3D& startPoint, const QVector3D& endPoint);
    CADObjectPtr createRectangleFromPoints(const QVector3D& startPoint, const QVector3D& endPoint);
    CADObjectPtr createCircleFromPoints(const QVector3D& centerPoint, const QVector3D& edgePoint);
    
    // Extrusion methods
    void handleExtrusionInteraction(const QPoint& screenPos);
    void updateExtrusionPreview(const QPoint& screenPos);
    
    // Eraser methods
    void handleEraserClick(const QPoint& screenPos);
    void performBooleanSubtraction(CADObjectPtr target, CADObjectPtr eraser);
    
    // Grid rendering
    void renderGridPlane(GridPlane plane);
    QVector3D getGridPlaneNormal(GridPlane plane) const;
    QVector3D projectToGridPlane(const QVector3D& point, GridPlane plane) const;
    
    // Geometry rendering
    void renderBox(const Point3D& min, const Point3D& max);
    void renderCylinder(float radius, float height, int segments = 32);
    void renderSphere(float radius, int segments = 32);
    void renderCone(float bottomRadius, float topRadius, float height, int segments = 32);
    void renderLine(const QVector3D& start, const QVector3D& end, float thickness = 1.0f);
    
    // Key action handling
    void executeKeyAction(KeyAction action);
    KeyAction getKeyActionFromEvent(QKeyEvent* event) const;

    // Camera state
    QVector3D m_cameraPosition;
    QVector3D m_cameraTarget;
    QVector3D m_cameraUp;
    float m_cameraDistance;
    float m_cameraRotationX;
    float m_cameraRotationY;
    float m_cameraSpeed;
    
    // Keyboard state for continuous movement
    QSet<int> m_pressedKeys;
    QTimer* m_keyUpdateTimer;
    
    // View state
    bool m_wireframeMode;
    bool m_showGrid;
    bool m_showAxes;
    QColor m_backgroundColor;
    
    // Grid settings
    GridPlane m_gridPlane;
    float m_gridSize;
    bool m_snapToGrid;
    bool m_showMultiPlaneGrid;
    std::array<bool, 3> m_visibleGridPlanes;
    
    // Tool state
    ActiveTool m_activeTool;
    PlacementState m_placementState;
    ObjectType m_shapeToPlace;
    QVector3D m_placementStartPoint;
    QVector3D m_placementEndPoint;
    QVector3D m_currentDragPoint;
    bool m_isDraggingShape;
    
    // Sketching state
    std::vector<QVector3D> m_sketchPoints;
    bool m_isSketchingActive;
    
    // Extrusion state
    CADObjectPtr m_extrusionObject;
    float m_extrusionDistance;
    
    // Eraser state
    bool m_eraserMode;
    ObjectType m_eraserShape;
    
    // Mouse state
    bool m_isDragging;
    bool m_isRotating;
    bool m_isPanning;
    QPoint m_lastMousePos;
    Qt::MouseButton m_dragButton;
    
    // Objects and selection
    CADObjectList m_objects;
    std::vector<CADObjectPtr> m_selectedObjects;
    
    // OpenGL resources
    std::unique_ptr<QOpenGLShaderProgram> m_shaderProgram;
    std::unique_ptr<QOpenGLShaderProgram> m_gridShaderProgram;
    std::unique_ptr<QOpenGLShaderProgram> m_lineShaderProgram;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLBuffer m_indexBuffer;
    
    // Matrices
    QMatrix4x4 m_modelMatrix;
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_projectionMatrix;
    
    // Animation
    QTimer* m_animationTimer;
    
    // Navigation cube
    NavigationCube* m_navigationCube;
    
    // Managers
    GeometryManager* m_geometryManager;
    MeshManager* m_meshManager;
    
    // View settings storage
    struct ViewSettings {
        float gridSize = 1.0f;
        int gridDivisions = 20;
    } m_viewSettings;
    
    // Keybinding system
    QMap<KeyAction, QKeySequence> m_keyBindings;
    QSettings* m_settings;
    
    // Constants
    static constexpr float DEFAULT_GRID_SIZE = 1.0f;
    static constexpr float CAMERA_ZOOM_SPEED = 0.1f;
    static constexpr float CAMERA_DISTANCE_MIN = 0.5f;
    static constexpr float CAMERA_DISTANCE_MAX = 100.0f;
    static constexpr float DEFAULT_CAMERA_SPEED = 5.0f;
};

} // namespace HybridCAD 