#include "CADViewer.h"
#include "GeometryManager.h"
#include "ToolManager.h"
#include <QtOpenGL/QOpenGLShader>
#include <QtCore/QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QEnterEvent>
#include <QSet>
#include <QStandardPaths>
#include <cmath>
#include <set>

namespace HybridCAD {

CADViewer::CADViewer(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_cameraDistance(10.0f)
    , m_cameraRotationX(0.0f)
    , m_cameraRotationY(0.0f)
    , m_cameraSpeed(DEFAULT_CAMERA_SPEED)
    , m_mouseSensitivity(DEFAULT_MOUSE_SENSITIVITY)
    , m_wireframeMode(false)
    , m_showGrid(true)
    , m_showAxes(true)
    , m_backgroundColor(64, 64, 64)
    , m_gridPlane(GridPlane::XY_PLANE)
    , m_gridSize(DEFAULT_GRID_SIZE)
    , m_snapToGrid(false)
    , m_showMultiPlaneGrid(false)
    , m_visibleGridPlanes{true, false, false}  // Only XY plane visible by default
    , m_currentSnapMode(SnapMode::NONE)
    , m_activeTool(ActiveTool::SELECT)
    , m_placementState(PlacementState::NONE)
    , m_shapeToPlace(ObjectType::PRIMITIVE_BOX)
    , m_isDraggingShape(false)
    , m_isSketchingActive(false)
    , m_extrusionDistance(1.0f)
    , m_eraserMode(false)
    , m_eraserShape(ObjectType::PRIMITIVE_BOX)
    , m_isDragging(false)
    , m_isRotating(false)
    , m_isPanning(false)
    , m_geometryManager(nullptr)
    , m_meshManager(nullptr)
    , m_navigationCube(nullptr)
    , m_settings(nullptr)
    , m_contextMenu(nullptr)
    , m_deleteAction(nullptr)
    , m_reshapeAction(nullptr)
    , m_padAction(nullptr)
    , m_moveAction(nullptr)
    , m_contextMenuObject(nullptr)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // Initialize camera
    m_cameraPosition = QVector3D(0, 0, m_cameraDistance);
    m_cameraTarget = QVector3D(0, 0, 0);
    m_cameraUp = QVector3D(0, 1, 0);
    
    // Setup navigation cube
    setupNavigationCube();
    
    // Initialize settings
    m_settings = new QSettings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/keybindings.ini", QSettings::IniFormat, this);
    
    // Load preferences
    QSettings preferences;
    preferences.beginGroup("Preferences");
    m_mouseSensitivity = preferences.value("mouseSensitivity", DEFAULT_MOUSE_SENSITIVITY).toFloat();
    m_cameraSpeed = preferences.value("cameraSpeed", DEFAULT_CAMERA_SPEED).toFloat();
    preferences.endGroup();
    
    // Setup default keybindings and load custom ones
    setupDefaultKeyBindings();
    loadKeyBindings();
    
    // Setup context menu
    setupContextMenu();
    
    // Initialize geometry manager
    m_geometryManager = new GeometryManager();
    
    // Animation timer for smooth updates
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &CADViewer::animate);
    m_animationTimer->start(16); // ~60 FPS
    
    // Keyboard input timer for continuous movement
    m_keyUpdateTimer = new QTimer(this);
    connect(m_keyUpdateTimer, &QTimer::timeout, this, &CADViewer::processKeyboardInput);
    m_keyUpdateTimer->start(16); // ~60 FPS for smooth movement
}

CADViewer::~CADViewer()
{
    saveKeyBindings();
    delete m_geometryManager;
    makeCurrent();
    // Cleanup OpenGL resources
    doneCurrent();
}

void CADViewer::initializeGL()
{
    initializeOpenGLFunctions();
    
    // Set clear color
    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), 
                 m_backgroundColor.blueF(), 1.0f);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Enable face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Setup shaders
    setupShaders();
    setupGeometry();
}

void CADViewer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    updateMatrices();
    
    // Set wireframe mode if enabled
    if (m_wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // Render grid on specified plane(s)
    if (m_showGrid) {
        if (m_showMultiPlaneGrid) {
            renderMultiPlaneGrid();
        } else {
            renderGrid();
        }
    }
    
    // Render axes
    if (m_showAxes) {
        renderAxes();
    }
    
    // Render objects
    renderObjects();
    
    // Render placement preview
    renderPlacementPreview();
    
    // Render sketch preview
    renderSketchPreview();
    
    // Render extrusion preview
    renderExtrusionPreview();
    
    // Render eraser preview
    renderEraserPreview();
    
    // Render size ruler during shape placement
    if (m_placementState == PlacementState::WAITING_FOR_SECOND_CLICK) {
        renderSizeRuler();
    }
}

void CADViewer::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    
    // Update projection matrix
    m_projectionMatrix.setToIdentity();
    float aspect = float(width) / float(height ? height : 1);
    m_projectionMatrix.perspective(45.0f, aspect, 0.1f, 1000.0f);
    
    // Reposition navigation cube
    if (m_navigationCube) {
        m_navigationCube->move(width - 90, 10);
    }
}

void CADViewer::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
    m_dragButton = event->button();

    if (event->button() == Qt::LeftButton) {
        switch (m_activeTool) {
            case ActiveTool::SELECT:
                {
                    CADObjectPtr pickedObject = pickObject(event->pos());
                    if (pickedObject) {
                        selectObject(pickedObject);
                    } else {
                        deselectAll();
                    }
                    m_isRotating = true;
                }
                break;
            case ActiveTool::PLACE_SHAPE:
                handleShapePlacementClick(event->pos());
                break;
            case ActiveTool::ERASER:
                handleEraserPlacementClick(event->pos());
                break;
            case ActiveTool::SKETCH_LINE:
            case ActiveTool::SKETCH_RECTANGLE:
            case ActiveTool::SKETCH_CIRCLE:
                handleSketchClick(event->pos());
                break;
            case ActiveTool::EXTRUDE_2D:
                if (m_extrusionObject) {
                    finishExtrusion();
                }
                break;
            default:
                m_isRotating = true;
                break;
        }
    } else if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
    } else if (event->button() == Qt::RightButton) {
        if (m_placementState != PlacementState::NONE || m_isSketchingActive) {
            cancelShapePlacement();
            cancelCurrentSketch();
        } else {
            CADObjectPtr pickedObject = pickObject(event->pos());
            if (pickedObject) {
                selectObject(pickedObject);
                m_contextMenuObject = pickedObject;
                showObjectContextMenu(event->globalPosition().toPoint());
            } else {
                deselectAll();
                m_isRotating = true;
            }
        }
    }
    update();
}

void CADViewer::mouseMoveEvent(QMouseEvent *event)
{
    QPoint delta = event->pos() - m_lastMousePos;
    float sensitivityFactor = m_mouseSensitivity * 0.5f;

    if ((event->buttons() & Qt::LeftButton && m_activeTool == ActiveTool::SELECT) || (event->buttons() & Qt::RightButton)) {
        rotateCamera(delta.x() * sensitivityFactor, delta.y() * sensitivityFactor);
    } else if (event->buttons() & Qt::MiddleButton) {
        panCamera(delta.x(), delta.y());
    }

    if (m_placementState == PlacementState::WAITING_FOR_SECOND_CLICK || m_isSketchingActive) {
        updatePlacementPreview(event->pos());
    }
    
    if (m_activeTool == ActiveTool::EXTRUDE_2D && m_extrusionObject) {
        updateExtrusionPreview(event->pos());
    }
    
    QVector3D worldPos = screenToWorld(event->pos());
    emit coordinatesChanged(worldPos);

    m_lastMousePos = event->pos();
    update();
}

void CADViewer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        m_isRotating = false;
    }
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
    }
    update();
}

void CADViewer::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() / 120.0f;
    zoomCamera(delta);
    update();
}

void CADViewer::keyPressEvent(QKeyEvent *event)
{
    // Add key to pressed keys set for continuous movement
    m_pressedKeys.insert(event->key());
    
    // Check for keybinding actions
    KeyAction action = getKeyActionFromEvent(event);
    if (action != static_cast<KeyAction>(-1)) {
        executeKeyAction(action);
        return;
    }
    
    // Handle other keys
    QOpenGLWidget::keyPressEvent(event);
}

void CADViewer::keyReleaseEvent(QKeyEvent *event)
{
    // Remove key from pressed keys set
    m_pressedKeys.remove(event->key());
    QOpenGLWidget::keyReleaseEvent(event);
}

void CADViewer::processKeyboardInput()
{
    // Process continuous WASD movement
    bool needsUpdate = false;
    float deltaTime = 0.016f; // 60 FPS
    
    for (int key : m_pressedKeys) {
        switch (key) {
        case Qt::Key_W:
            moveCamera(CameraMovement::FORWARD, deltaTime);
            needsUpdate = true;
            break;
        case Qt::Key_S:
            moveCamera(CameraMovement::BACKWARD, deltaTime);
            needsUpdate = true;
            break;
        case Qt::Key_A:
            moveCamera(CameraMovement::LEFT, deltaTime);
            needsUpdate = true;
            break;
        case Qt::Key_D:
            moveCamera(CameraMovement::RIGHT, deltaTime);
            needsUpdate = true;
            break;
        case Qt::Key_Q:
            moveCamera(CameraMovement::DOWN, deltaTime);
            needsUpdate = true;
            break;
        case Qt::Key_E:
            moveCamera(CameraMovement::UP, deltaTime);
            needsUpdate = true;
            break;
        }
    }
    
    if (needsUpdate) {
        update();
    }
}

void CADViewer::moveCamera(CameraMovement direction, float deltaTime)
{
    float velocity = m_cameraSpeed * deltaTime;
    
    // Calculate camera movement vectors
    QVector3D viewDirection = (m_cameraTarget - m_cameraPosition).normalized();
    QVector3D right = QVector3D::crossProduct(viewDirection, m_cameraUp).normalized();
    QVector3D up = QVector3D::crossProduct(right, viewDirection).normalized();
    
    QVector3D movement;
    switch (direction) {
    case CameraMovement::FORWARD:
        movement = viewDirection * velocity;
        break;
    case CameraMovement::BACKWARD:
        movement = -viewDirection * velocity;
        break;
    case CameraMovement::LEFT:
        movement = -right * velocity;
        break;
    case CameraMovement::RIGHT:
        movement = right * velocity;
        break;
    case CameraMovement::UP:
        movement = up * velocity;
        break;
    case CameraMovement::DOWN:
        movement = -up * velocity;
        break;
    }
    
    // Apply movement to both camera position and target
    m_cameraTarget += movement;
    m_cameraPosition += movement;
}

void CADViewer::animate()
{
    // Smooth camera updates or animations can go here
    // Currently just ensures smooth rendering
}

void CADViewer::setupDefaultKeyBindings()
{
    QMap<KeyAction, QKeySequence> defaults;
    defaults[KeyAction::TOGGLE_GRID] = QKeySequence(Qt::Key_G);
    defaults[KeyAction::TOGGLE_WIREFRAME] = QKeySequence(Qt::Key_Z);
    defaults[KeyAction::TOGGLE_AXES] = QKeySequence(Qt::Key_X);
    defaults[KeyAction::TOGGLE_GRID_XY] = QKeySequence(Qt::SHIFT | Qt::Key_1);
    defaults[KeyAction::TOGGLE_GRID_XZ] = QKeySequence(Qt::SHIFT | Qt::Key_2);
    defaults[KeyAction::TOGGLE_GRID_YZ] = QKeySequence(Qt::SHIFT | Qt::Key_3);
    defaults[KeyAction::TOGGLE_MULTI_PLANE_GRID] = QKeySequence(Qt::SHIFT | Qt::Key_G);
    defaults[KeyAction::RESET_VIEW] = QKeySequence(Qt::Key_Home);
    defaults[KeyAction::FRONT_VIEW] = QKeySequence(Qt::Key_1);
    defaults[KeyAction::BACK_VIEW] = QKeySequence(Qt::CTRL | Qt::Key_1);
    defaults[KeyAction::LEFT_VIEW] = QKeySequence(Qt::Key_3);
    defaults[KeyAction::RIGHT_VIEW] = QKeySequence(Qt::CTRL | Qt::Key_3);
    defaults[KeyAction::TOP_VIEW] = QKeySequence(Qt::Key_7);
    defaults[KeyAction::BOTTOM_VIEW] = QKeySequence(Qt::CTRL | Qt::Key_7);
    defaults[KeyAction::ISOMETRIC_VIEW] = QKeySequence(Qt::Key_9);
    defaults[KeyAction::DELETE_SELECTED] = QKeySequence(Qt::Key_Delete);
    defaults[KeyAction::SELECT_ALL] = QKeySequence(Qt::CTRL | Qt::Key_A);
    defaults[KeyAction::DESELECT_ALL] = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    defaults[KeyAction::PLACE_SHAPE] = QKeySequence(Qt::Key_P);
    defaults[KeyAction::SKETCH_LINE] = QKeySequence(Qt::Key_L);
    defaults[KeyAction::SKETCH_RECTANGLE] = QKeySequence(Qt::Key_R);
    defaults[KeyAction::SKETCH_CIRCLE] = QKeySequence(Qt::Key_C);
    defaults[KeyAction::CANCEL_CURRENT_ACTION] = QKeySequence(Qt::Key_Escape);
    m_keyBindings = defaults;
}

KeyAction CADViewer::getKeyActionFromEvent(QKeyEvent* event) const
{
    QKeySequence pressed(event->key() | event->modifiers());
    
    for (auto it = m_keyBindings.begin(); it != m_keyBindings.end(); ++it) {
        if (it.value() == pressed) {
            return it.key();
        }
    }
    
    return static_cast<KeyAction>(-1); // No action found
}

void CADViewer::executeKeyAction(KeyAction action)
{
    switch (action) {
    case KeyAction::TOGGLE_GRID:
        setGridVisible(!m_showGrid);
        emit gridToggled(m_showGrid);
        break;
    case KeyAction::TOGGLE_WIREFRAME:
        setWireframeMode(!m_wireframeMode);
        emit wireframeToggled(m_wireframeMode);
        break;
    case KeyAction::TOGGLE_AXES:
        setAxesVisible(!m_showAxes);
        emit axesToggled(m_showAxes);
        break;
    case KeyAction::TOGGLE_GRID_XY:
        toggleGridPlane(GridPlane::XY_PLANE);
        break;
    case KeyAction::TOGGLE_GRID_XZ:
        toggleGridPlane(GridPlane::XZ_PLANE);
        break;
    case KeyAction::TOGGLE_GRID_YZ:
        toggleGridPlane(GridPlane::YZ_PLANE);
        break;
    case KeyAction::TOGGLE_MULTI_PLANE_GRID:
        setMultiPlaneGridVisible(!m_showMultiPlaneGrid);
        break;
    case KeyAction::RESET_VIEW:
        resetView();
        break;
    case KeyAction::FRONT_VIEW:
        frontView();
        break;
    case KeyAction::BACK_VIEW:
        backView();
        break;
    case KeyAction::LEFT_VIEW:
        leftView();
        break;
    case KeyAction::RIGHT_VIEW:
        rightView();
        break;
    case KeyAction::TOP_VIEW:
        topView();
        break;
    case KeyAction::BOTTOM_VIEW:
        bottomView();
        break;
    case KeyAction::ISOMETRIC_VIEW:
        isometricView();
        break;
    case KeyAction::DELETE_SELECTED:
        deleteSelected();
        break;
    case KeyAction::SELECT_ALL:
        selectAll();
        break;
    case KeyAction::DESELECT_ALL:
        deselectAll();
        break;
    case KeyAction::PLACE_SHAPE:
        setActiveTool(ActiveTool::PLACE_SHAPE);
        startShapePlacement();
        break;
    case KeyAction::SKETCH_LINE:
        setActiveTool(ActiveTool::SKETCH_LINE);
        startLineSketch();
        break;
    case KeyAction::SKETCH_RECTANGLE:
        setActiveTool(ActiveTool::SKETCH_RECTANGLE);
        startRectangleSketch();
        break;
    case KeyAction::SKETCH_CIRCLE:
        setActiveTool(ActiveTool::SKETCH_CIRCLE);
        startCircleSketch();
        break;
    case KeyAction::CANCEL_CURRENT_ACTION:
        cancelShapePlacement();
        cancelCurrentSketch();
        setActiveTool(ActiveTool::SELECT);
        break;
    default:
        break;
    }
}

void CADViewer::setupShaders()
{
    // Basic vertex shader
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        out vec3 FragPos;
        out vec3 Normal;
        
        void main()
        {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";
    
    // Basic fragment shader
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec3 FragPos;
        in vec3 Normal;
        
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;
        uniform vec4 objectColor;
        
        void main()
        {
            // Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * lightColor;
            
            // Diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            
            // Specular
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * lightColor;
            
            vec3 result = (ambient + diffuse + specular) * objectColor.rgb;
            FragColor = vec4(result, objectColor.a);
        }
    )";
    
    m_shaderProgram = std::make_unique<QOpenGLShaderProgram>();
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    
    if (!m_shaderProgram->link()) {
        qWarning() << "Shader program linking failed:" << m_shaderProgram->log();
    }
    
    // Grid shader
    const char* gridVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main()
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";
    
    const char* gridFragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        uniform vec3 color;
        
        void main()
        {
            FragColor = vec4(color, 1.0);
        }
    )";
    
    m_gridShaderProgram = std::make_unique<QOpenGLShaderProgram>();
    m_gridShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, gridVertexShaderSource);
    m_gridShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, gridFragmentShaderSource);
    
    if (!m_gridShaderProgram->link()) {
        qWarning() << "Grid shader program linking failed:" << m_gridShaderProgram->log();
    }
    
    // Line shader (reuse grid shader for lines)
    m_lineShaderProgram = std::make_unique<QOpenGLShaderProgram>();
    m_lineShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, gridVertexShaderSource);
    m_lineShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, gridFragmentShaderSource);
    
    if (!m_lineShaderProgram->link()) {
        qWarning() << "Line shader program linking failed:" << m_lineShaderProgram->log();
    }
}

void CADViewer::setupGeometry()
{
    m_vao.create();
    m_vao.bind();
    
    m_vertexBuffer.create();
    m_indexBuffer.create();
    
    m_vao.release();
}

void CADViewer::updateMatrices()
{
    // Update view matrix
    updateCameraPosition();
    m_viewMatrix.setToIdentity();
    m_viewMatrix.lookAt(m_cameraPosition, m_cameraTarget, m_cameraUp);
    
    // Model matrix (identity for now)
    m_modelMatrix.setToIdentity();
}

void CADViewer::renderGrid()
{
    if (!m_gridShaderProgram) return;
    
    m_gridShaderProgram->bind();
    m_gridShaderProgram->setUniformValue("model", m_modelMatrix);
    m_gridShaderProgram->setUniformValue("view", m_viewMatrix);
    m_gridShaderProgram->setUniformValue("projection", m_projectionMatrix);
    m_gridShaderProgram->setUniformValue("color", QVector3D(0.3f, 0.3f, 0.3f));
    
    // Draw grid lines
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    
    float gridSize = m_gridSize;
    int gridDivisions = 20;
    float extent = gridSize * gridDivisions;
    
    // Draw based on current grid plane
    switch (m_gridPlane) {
    case GridPlane::XY_PLANE:
        // Horizontal lines (X direction)
        for (int i = -gridDivisions; i <= gridDivisions; ++i) {
            float y = i * gridSize;
            glVertex3f(-extent, y, 0.0f);
            glVertex3f(extent, y, 0.0f);
        }
        // Vertical lines (Y direction)
        for (int i = -gridDivisions; i <= gridDivisions; ++i) {
            float x = i * gridSize;
            glVertex3f(x, -extent, 0.0f);
            glVertex3f(x, extent, 0.0f);
        }
        break;
        
    case GridPlane::XZ_PLANE:
        // Lines in X direction
        for (int i = -gridDivisions; i <= gridDivisions; ++i) {
            float z = i * gridSize;
            glVertex3f(-extent, 0.0f, z);
            glVertex3f(extent, 0.0f, z);
        }
        // Lines in Z direction
        for (int i = -gridDivisions; i <= gridDivisions; ++i) {
            float x = i * gridSize;
            glVertex3f(x, 0.0f, -extent);
            glVertex3f(x, 0.0f, extent);
        }
        break;
        
    case GridPlane::YZ_PLANE:
        // Lines in Y direction
        for (int i = -gridDivisions; i <= gridDivisions; ++i) {
            float z = i * gridSize;
            glVertex3f(0.0f, -extent, z);
            glVertex3f(0.0f, extent, z);
        }
        // Lines in Z direction
        for (int i = -gridDivisions; i <= gridDivisions; ++i) {
            float y = i * gridSize;
            glVertex3f(0.0f, y, -extent);
            glVertex3f(0.0f, y, extent);
        }
        break;
    }
    
    glEnd();
    m_gridShaderProgram->release();
}

void CADViewer::renderMultiPlaneGrid()
{
    if (!m_gridShaderProgram) return;
    
    m_gridShaderProgram->bind();
    m_gridShaderProgram->setUniformValue("model", m_modelMatrix);
    m_gridShaderProgram->setUniformValue("view", m_viewMatrix);
    m_gridShaderProgram->setUniformValue("projection", m_projectionMatrix);
    
    float gridSize = m_gridSize;
    int gridDivisions = 20;
    float extent = gridSize * gridDivisions;
    
    glLineWidth(1.0f);
    
    // Render each visible grid plane with different alpha values
    for (int planeIndex = 0; planeIndex < 3; ++planeIndex) {
        if (!m_visibleGridPlanes[planeIndex]) continue;
        
        GridPlane plane = static_cast<GridPlane>(planeIndex);
        
        // Set color with reduced alpha for non-primary planes
        float alpha = (plane == m_gridPlane) ? 0.8f : 0.3f;
        QVector3D color;
        
        switch (plane) {
        case GridPlane::XY_PLANE:
            color = QVector3D(0.3f, 0.3f, 0.3f); // Gray
            break;
        case GridPlane::XZ_PLANE:
            color = QVector3D(0.3f, 0.2f, 0.2f); // Reddish
            break;
        case GridPlane::YZ_PLANE:
            color = QVector3D(0.2f, 0.3f, 0.2f); // Greenish
            break;
        }
        
        m_gridShaderProgram->setUniformValue("color", color);
        
        glBegin(GL_LINES);
        
        switch (plane) {
        case GridPlane::XY_PLANE:
            // Horizontal lines (X direction)
            for (int i = -gridDivisions; i <= gridDivisions; ++i) {
                float y = i * gridSize;
                glVertex3f(-extent, y, 0.0f);
                glVertex3f(extent, y, 0.0f);
            }
            // Vertical lines (Y direction)
            for (int i = -gridDivisions; i <= gridDivisions; ++i) {
                float x = i * gridSize;
                glVertex3f(x, -extent, 0.0f);
                glVertex3f(x, extent, 0.0f);
            }
            break;
            
        case GridPlane::XZ_PLANE:
            // Lines in X direction
            for (int i = -gridDivisions; i <= gridDivisions; ++i) {
                float z = i * gridSize;
                glVertex3f(-extent, 0.0f, z);
                glVertex3f(extent, 0.0f, z);
            }
            // Lines in Z direction
            for (int i = -gridDivisions; i <= gridDivisions; ++i) {
                float x = i * gridSize;
                glVertex3f(x, 0.0f, -extent);
                glVertex3f(x, 0.0f, extent);
            }
            break;
            
        case GridPlane::YZ_PLANE:
            // Lines in Y direction
            for (int i = -gridDivisions; i <= gridDivisions; ++i) {
                float z = i * gridSize;
                glVertex3f(0.0f, -extent, z);
                glVertex3f(0.0f, extent, z);
            }
            // Lines in Z direction
            for (int i = -gridDivisions; i <= gridDivisions; ++i) {
                float y = i * gridSize;
                glVertex3f(0.0f, y, -extent);
                glVertex3f(0.0f, y, extent);
            }
            break;
        }
        
        glEnd();
    }
    
    m_gridShaderProgram->release();
}

void CADViewer::renderAxes()
{
    if (!m_gridShaderProgram) return;
    
    m_gridShaderProgram->bind();
    m_gridShaderProgram->setUniformValue("model", m_modelMatrix);
    m_gridShaderProgram->setUniformValue("view", m_viewMatrix);
    m_gridShaderProgram->setUniformValue("projection", m_projectionMatrix);
    
    glLineWidth(3.0f);
    
    // X axis (red)
    m_gridShaderProgram->setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(2.0f, 0.0f, 0.0f);
    glEnd();
    
    // Y axis (green)
    m_gridShaderProgram->setUniformValue("color", QVector3D(0.0f, 1.0f, 0.0f));
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 2.0f, 0.0f);
    glEnd();
    
    // Z axis (blue)
    m_gridShaderProgram->setUniformValue("color", QVector3D(0.0f, 0.0f, 1.0f));
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 2.0f);
    glEnd();
    
    glLineWidth(1.0f);
    m_gridShaderProgram->release();
}

void CADViewer::renderObjects()
{
    if (!m_shaderProgram) return;
    
    m_shaderProgram->bind();
    m_shaderProgram->setUniformValue("model", m_modelMatrix);
    m_shaderProgram->setUniformValue("view", m_viewMatrix);
    m_shaderProgram->setUniformValue("projection", m_projectionMatrix);
    m_shaderProgram->setUniformValue("lightPos", QVector3D(5.0f, 5.0f, 5.0f));
    m_shaderProgram->setUniformValue("viewPos", m_cameraPosition);
    m_shaderProgram->setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
    
    // First pass: determine which objects should be transparent
    std::set<CADObjectPtr> transparentObjects;
    
    // Make objects transparent during active shape placement or eraser mode
    bool isPlacingShape = (m_placementState != PlacementState::NONE || m_eraserMode);
    
    // Check for objects that contain other objects (outer shapes should be transparent)
    for (const auto& outerObject : m_objects) {
        if (!outerObject || !outerObject->isVisible()) continue;
        
        for (const auto& innerObject : m_objects) {
            if (!innerObject || !innerObject->isVisible() || outerObject == innerObject) continue;
            
            if (objectContainsObject(outerObject, innerObject)) {
                transparentObjects.insert(outerObject);
            }
        }
    }
    
    // Render all objects
    for (const auto& object : m_objects) {
        if (object && object->isVisible()) {
            Material mat = object->getMaterial();
            QVector4D color(mat.diffuseColor.redF(), mat.diffuseColor.greenF(), mat.diffuseColor.blueF(), 1.0f);

            // Apply transparency rules:
            // 1. During shape placement/eraser mode, make existing objects transparent
            // 2. Make outer objects transparent when they contain inner objects
            if (isPlacingShape || transparentObjects.count(object) > 0) {
                color.setW(0.5f); // Make transparent
            } else {
                color.setW(1.0f - mat.transparency);
            }
            // Otherwise, objects remain fully opaque (alpha = 1.0f)
            
            m_shaderProgram->setUniformValue("objectColor", color);
            object->render();

            if (object->isSelected()) {
                renderSelectionOutline(object);
            }
        }
    }
    
    m_shaderProgram->release();
}

void CADViewer::renderSelectionOutline(CADObjectPtr object)
{
    if (!m_lineShaderProgram || !object) return;

    m_lineShaderProgram->bind();
    m_lineShaderProgram->setUniformValue("model", m_modelMatrix);
    m_lineShaderProgram->setUniformValue("view", m_viewMatrix);
    m_lineShaderProgram->setUniformValue("projection", m_projectionMatrix);
    m_lineShaderProgram->setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f)); // Red color for selection

    glLineWidth(5.0f); // Thicker line for glow effect
    glDepthMask(GL_FALSE); // Disable depth writes to ensure outline is always visible
    glDisable(GL_DEPTH_TEST); // Disable depth test

    object->render(); // Render the object's wireframe

    glEnable(GL_DEPTH_TEST); // Re-enable depth test
    glDepthMask(GL_TRUE); // Re-enable depth writes
    glLineWidth(1.0f);
    m_lineShaderProgram->release();
}

void CADViewer::updateCameraPosition()
{
    // Calculate camera position relative to target using spherical coordinates
    float x = m_cameraDistance * std::cos(m_cameraRotationY) * std::sin(m_cameraRotationX);
    float y = m_cameraDistance * std::sin(m_cameraRotationY);
    float z = m_cameraDistance * std::cos(m_cameraRotationY) * std::cos(m_cameraRotationX);
    
    m_cameraPosition = QVector3D(x, y, z) + m_cameraTarget;
}

void CADViewer::panCamera(float deltaX, float deltaY)
{
    // Calculate right and up vectors for proper panning
    QVector3D viewDirection = (m_cameraTarget - m_cameraPosition).normalized();
    QVector3D right = QVector3D::crossProduct(viewDirection, m_cameraUp).normalized();
    QVector3D up = QVector3D::crossProduct(right, viewDirection).normalized();
    
    // Scale pan speed based on camera distance for better control
    float scaleFactor = m_cameraDistance * 0.01f;
    QVector3D translation = (right * -deltaX + up * deltaY) * scaleFactor;
    
    // Move both camera position and target
    m_cameraTarget += translation;
    m_cameraPosition += translation;
}

void CADViewer::rotateCamera(float deltaX, float deltaY)
{
    m_cameraRotationX += deltaX * 0.01f;
    m_cameraRotationY += deltaY * 0.01f;
    
    // Clamp Y rotation
    m_cameraRotationY = std::max(-M_PI/2 + 0.1f, std::min(M_PI/2 - 0.1f, (double)m_cameraRotationY));
}

void CADViewer::zoomCamera(float delta)
{
    m_cameraDistance *= (1.0f - delta * CAMERA_ZOOM_SPEED);
    m_cameraDistance = std::max(CAMERA_DISTANCE_MIN, std::min(CAMERA_DISTANCE_MAX, m_cameraDistance));
}

CADObjectPtr CADViewer::pickObject(const QPoint& screenPos)
{
    QVector3D rayOrigin, rayDirection;
    
    QMatrix4x4 viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
    QMatrix4x4 invViewProjectionMatrix = viewProjectionMatrix.inverted();

    float x = (2.0f * screenPos.x()) / width() - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / height();

    QVector4D nearPoint(x, y, -1.0f, 1.0f);
    QVector4D farPoint(x, y, 1.0f, 1.0f);

    QVector4D nearWorld = invViewProjectionMatrix * nearPoint;
    QVector4D farWorld = invViewProjectionMatrix * farPoint;

    if (nearWorld.w() != 0.0f) nearWorld /= nearWorld.w();
    if (farWorld.w() != 0.0f) farWorld /= farWorld.w();

    rayOrigin = QVector3D(nearWorld);
    rayDirection = (QVector3D(farWorld) - rayOrigin).normalized();

    float minDistance = std::numeric_limits<float>::max();
    CADObjectPtr pickedObject = nullptr;

    for (const auto& object : m_objects) {
        float distance;
        if (rayIntersectsObject(rayOrigin, rayDirection, object, distance)) {
            if (distance < minDistance) {
                minDistance = distance;
                pickedObject = object;
            }
        }
    }
    return pickedObject;
}

bool CADViewer::rayIntersectsObject(const QVector3D& rayOrigin, const QVector3D& rayDirection, 
                                  CADObjectPtr object, float& distance)
{
    // For now, a simple bounding box intersection test
    // In a real CAD system, this would involve more complex geometry intersection tests
    Point3D min = object->getBoundingBoxMin();
    Point3D max = object->getBoundingBoxMax();

    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();

    // X-axis
    if (std::abs(rayDirection.x()) < 1e-6) {
        if (rayOrigin.x() < min.x || rayOrigin.x() > max.x) return false;
    } else {
        float t1 = (min.x - rayOrigin.x()) / rayDirection.x();
        float t2 = (max.x - rayOrigin.x()) / rayDirection.x();
        tMin = std::max(tMin, std::min(t1, t2));
        tMax = std::min(tMax, std::max(t1, t2));
    }

    // Y-axis
    if (std::abs(rayDirection.y()) < 1e-6) {
        if (rayOrigin.y() < min.y || rayOrigin.y() > max.y) return false;
    } else {
        float t1 = (min.y - rayOrigin.y()) / rayDirection.y();
        float t2 = (max.y - rayOrigin.y()) / rayDirection.y();
        tMin = std::max(tMin, std::min(t1, t2));
        tMax = std::min(tMax, std::max(t1, t2));
    }

    // Z-axis
    if (std::abs(rayDirection.z()) < 1e-6) {
        if (rayOrigin.z() < min.z || rayOrigin.z() > max.z) return false;
    } else {
        float t1 = (min.z - rayOrigin.z()) / rayDirection.z();
        float t2 = (max.z - rayOrigin.z()) / rayDirection.z();
        tMin = std::max(tMin, std::min(t1, t2));
        tMax = std::min(tMax, std::max(t1, t2));
    }

    if (tMin <= tMax && tMax > 0) {
        distance = tMin;
        return true;
    }
    return false;
}

QVector3D CADViewer::screenToWorld(const QPoint& screenPos, float depth)
{
    QMatrix4x4 viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
    QMatrix4x4 invViewProjectionMatrix = viewProjectionMatrix.inverted();

    float x = (2.0f * screenPos.x()) / width() - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / height();

    QVector4D nearPoint(x, y, -1.0f, 1.0f);
    QVector4D farPoint(x, y, 1.0f, 1.0f);

    QVector4D nearWorld = invViewProjectionMatrix * nearPoint;
    QVector4D farWorld = invViewProjectionMatrix * farPoint;

    if (nearWorld.w() != 0.0f) nearWorld /= nearWorld.w();
    if (farWorld.w() != 0.0f) farWorld /= farWorld.w();

    QVector3D rayOrigin(nearWorld);
    QVector3D rayDirection = (QVector3D(farWorld) - rayOrigin).normalized();

    QVector3D planeNormal = getGridPlaneNormal(m_gridPlane);
    float planeD = 0;
    float t = -(QVector3D::dotProduct(rayOrigin, planeNormal) + planeD) / QVector3D::dotProduct(rayDirection, planeNormal);

    return rayOrigin + t * rayDirection;
}

QPoint CADViewer::worldToScreen(const QVector3D& worldPos)
{
    // Transform world position to screen coordinates
    QMatrix4x4 mvp = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
    QVector4D worldPos4(worldPos, 1.0f);
    QVector4D clipPos = mvp * worldPos4;
    
    // Perspective divide
    if (clipPos.w() != 0.0f) {
        clipPos /= clipPos.w();
    }
    
    // Get viewport dimensions
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    // Convert from NDC to screen coordinates
    int screenX = static_cast<int>((clipPos.x() + 1.0f) * 0.5f * viewport[2] + viewport[0]);
    int screenY = static_cast<int>((1.0f - clipPos.y()) * 0.5f * viewport[3] + viewport[1]);
    
    return QPoint(screenX, screenY);
}

// Public interface methods
void CADViewer::resetView()
{
    m_cameraDistance = 10.0f;
    m_cameraRotationX = 0.0f;
    m_cameraRotationY = 0.0f;
    m_cameraTarget = QVector3D(0, 0, 0);
    update();
}

void CADViewer::frontView()
{
    m_cameraRotationX = 0.0f;
    m_cameraRotationY = 0.0f;
    update();
}

void CADViewer::backView()
{
    m_cameraRotationX = M_PI;
    m_cameraRotationY = 0.0f;
    update();
}

void CADViewer::leftView()
{
    m_cameraRotationX = -M_PI/2;
    m_cameraRotationY = 0.0f;
    update();
}

void CADViewer::rightView()
{
    m_cameraRotationX = M_PI/2;
    m_cameraRotationY = 0.0f;
    update();
}

void CADViewer::topView()
{
    m_cameraRotationX = 0.0f;
    m_cameraRotationY = M_PI/2;
    update();
}

void CADViewer::bottomView()
{
    m_cameraRotationX = 0.0f;
    m_cameraRotationY = -M_PI/2;
    update();
}

void CADViewer::isometricView()
{
    m_cameraRotationX = M_PI/4;
    m_cameraRotationY = M_PI/6;
    update();
}

void CADViewer::setWireframeMode(bool enabled)
{
    m_wireframeMode = enabled;
    update();
}

void CADViewer::setGridVisible(bool visible)
{
    m_showGrid = visible;
    update();
}

void CADViewer::setAxesVisible(bool visible)
{
    m_showAxes = visible;
    update();
}

void CADViewer::setBackgroundColor(const QColor& color)
{
    m_backgroundColor = color;
    makeCurrent();
    glClearColor(color.redF(), color.greenF(), color.blueF(), 1.0f);
    doneCurrent();
    update();
}

void CADViewer::addObject(CADObjectPtr object)
{
    if (object) {
        m_objects.push_back(object);
        update();
    }
}

void CADViewer::removeObject(CADObjectPtr object)
{
    auto it = std::find(m_objects.begin(), m_objects.end(), object);
    if (it != m_objects.end()) {
        m_objects.erase(it);
        update();
    }
}

void CADViewer::clearObjects()
{
    m_objects.clear();
    m_selectedObjects.clear();
    update();
}

void CADViewer::selectObject(CADObjectPtr object)
{
    if (object) {
        deselectAll();
        m_selectedObjects.push_back(object);
        object->setSelected(true);
        emit objectSelected(object);
        emit selectionChanged();
        update();
    }
}

void CADViewer::deselectAll()
{
    for (auto& object : m_selectedObjects) {
        if (object) {
            object->setSelected(false);
            emit objectDeselected(object);
        }
    }
    m_selectedObjects.clear();
    emit selectionChanged();
    update();
}

void CADViewer::selectAll()
{
    deselectAll();
    for (auto& object : m_objects) {
        if (object) {
            m_selectedObjects.push_back(object);
            object->setSelected(true);
            emit objectSelected(object);
        }
    }
    emit selectionChanged();
    update();
}

void CADViewer::deleteSelected()
{
    for (auto& object : m_selectedObjects) {
        removeObject(object);
    }
    m_selectedObjects.clear();
    emit selectionChanged();
    update();
}

// Sketching functionality
void CADViewer::startLineSketch()
{
    m_isSketchingActive = true;
    m_sketchPoints.clear();
    emit sketchStarted(ActiveTool::SKETCH_LINE);
}

void CADViewer::startRectangleSketch()
{
    m_isSketchingActive = true;
    m_sketchPoints.clear();
    emit sketchStarted(ActiveTool::SKETCH_RECTANGLE);
}

void CADViewer::startCircleSketch()
{
    m_isSketchingActive = true;
    m_sketchPoints.clear();
    emit sketchStarted(ActiveTool::SKETCH_CIRCLE);
}

void CADViewer::finishCurrentSketch()
{
    if (!m_isSketchingActive) return;
    
    CADObjectPtr newObject = nullptr;
    
    switch (m_activeTool) {
    case ActiveTool::SKETCH_LINE:
        if (m_sketchPoints.size() >= 2) {
            newObject = createLineFromPoints(m_sketchPoints[0], m_sketchPoints[1]);
        }
        break;
    case ActiveTool::SKETCH_RECTANGLE:
        if (m_sketchPoints.size() >= 2) {
            newObject = createRectangleFromPoints(m_sketchPoints[0], m_sketchPoints[1]);
        }
        break;
    case ActiveTool::SKETCH_CIRCLE:
        if (m_sketchPoints.size() >= 2) {
            newObject = createCircleFromPoints(m_sketchPoints[0], m_sketchPoints[1]);
        }
        break;
    default:
        break;
    }
    
    if (newObject) {
        addObject(newObject);
        emit sketchFinished(newObject);
    }
    
    cancelCurrentSketch();
}

void CADViewer::cancelCurrentSketch()
{
    m_isSketchingActive = false;
    m_sketchPoints.clear();
    update();
}

void CADViewer::handleSketchClick(const QPoint& screenPos)
{
    QVector3D worldPos = screenToWorld(screenPos);
    worldPos = applySnapping(worldPos, screenPos);
    
    switch (m_activeTool) {
    case ActiveTool::SKETCH_LINE:
        if (m_sketchPoints.size() == 0) {
            m_sketchPoints.push_back(worldPos);
        } else if (m_sketchPoints.size() == 1) {
            m_sketchPoints.push_back(worldPos);
            finishCurrentSketch();
        }
        break;
        
    case ActiveTool::SKETCH_RECTANGLE:
        if (m_sketchPoints.size() == 0) {
            m_sketchPoints.push_back(worldPos);
        } else if (m_sketchPoints.size() == 1) {
            m_sketchPoints.push_back(worldPos);
            finishCurrentSketch();
        }
        break;
        
    case ActiveTool::SKETCH_CIRCLE:
        if (m_sketchPoints.size() == 0) {
            m_sketchPoints.push_back(worldPos); // Center
        } else if (m_sketchPoints.size() == 1) {
            m_sketchPoints.push_back(worldPos); // Edge point
            finishCurrentSketch();
        }
        break;
        
    default:
        break;
    }
    
    update();
}

void CADViewer::updateSketchPreview(const QPoint& screenPos)
{
    if (!m_isSketchingActive || m_sketchPoints.empty()) return;
    
    QVector3D worldPos = screenToWorld(screenPos);
    worldPos = applySnapping(worldPos, screenPos);
    
    // Update preview point
    if (m_sketchPoints.size() == 1) {
        if (m_sketchPoints.size() == 1) {
            m_sketchPoints.resize(2);
        }
        m_sketchPoints[1] = worldPos;
    }
    
    update();
}

CADObjectPtr CADViewer::createLineFromPoints(const QVector3D& startPoint, const QVector3D& endPoint)
{
    // Create a thin box to represent the line
    float thickness = 0.01f; // Define a small thickness for the line
    
    // Calculate the center and dimensions of the bounding box for the line
    QVector3D center = (startPoint + endPoint) / 2.0f;
    QVector3D direction = (endPoint - startPoint).normalized();
    float length = (endPoint - startPoint).length();
    
    // Create a transformation matrix to align the box with the line direction
    QMatrix4x4 transformMatrix;
    transformMatrix.translate(center);
    
    // Calculate rotation to align with the line direction
    QVector3D defaultDirection(1.0f, 0.0f, 0.0f); // Assume line is created along X-axis initially
    QQuaternion rotation = QQuaternion::rotationTo(defaultDirection, direction);
    transformMatrix.rotate(rotation);
    
    // Create a box with length along X, and small thickness in Y and Z
    Point3D min(-length / 2.0f, -thickness / 2.0f, -thickness / 2.0f);
    Point3D max(length / 2.0f, thickness / 2.0f, thickness / 2.0f);
    
    auto lineBox = std::make_shared<Box>(min, max);
    
    // Apply the transformation to the lineBox (this would typically be handled by a scene graph or object transform)
    // For now, we'll just return the box and assume the viewer handles its placement.
    // In a more robust system, the Box object would have its own transform.
    
    // Set a distinct color for lines
    Material mat;
    mat.diffuseColor = QColor(255, 255, 0); // Yellow color for lines
    mat.specularColor = QColor(255, 255, 255);
    mat.shininess = 32.0f;
    lineBox->setMaterial(mat);

    return std::static_pointer_cast<CADObject>(lineBox);
}

CADObjectPtr CADViewer::createRectangleFromPoints(const QVector3D& startPoint, const QVector3D& endPoint)
{
    // TODO: Implement actual rectangle object creation
    Q_UNUSED(startPoint)
    Q_UNUSED(endPoint)
    return nullptr;
}

CADObjectPtr CADViewer::createCircleFromPoints(const QVector3D& centerPoint, const QVector3D& edgePoint)
{
    // TODO: Implement actual circle object creation
    Q_UNUSED(centerPoint)
    Q_UNUSED(edgePoint)
    return nullptr;
}

void CADViewer::renderSketchPreview()
{
    if (!m_isSketchingActive || m_sketchPoints.empty()) return;
    
    if (!m_lineShaderProgram) return;
    
    m_lineShaderProgram->bind();
    m_lineShaderProgram->setUniformValue("model", m_modelMatrix);
    m_lineShaderProgram->setUniformValue("view", m_viewMatrix);
    m_lineShaderProgram->setUniformValue("projection", m_projectionMatrix);
    m_lineShaderProgram->setUniformValue("color", QVector3D(1.0f, 1.0f, 0.0f)); // Yellow preview
    
    glLineWidth(2.0f);
    
    switch (m_activeTool) {
    case ActiveTool::SKETCH_LINE:
        if (m_sketchPoints.size() >= 2) {
            glBegin(GL_LINES);
            glVertex3f(m_sketchPoints[0].x(), m_sketchPoints[0].y(), m_sketchPoints[0].z());
            glVertex3f(m_sketchPoints[1].x(), m_sketchPoints[1].y(), m_sketchPoints[1].z());
            glEnd();
        }
        break;
        
    case ActiveTool::SKETCH_RECTANGLE:
        if (m_sketchPoints.size() >= 2) {
            QVector3D p1 = m_sketchPoints[0];
            QVector3D p2 = m_sketchPoints[1];
            glBegin(GL_LINE_LOOP);
            glVertex3f(p1.x(), p1.y(), p1.z());
            glVertex3f(p2.x(), p1.y(), p1.z());
            glVertex3f(p2.x(), p2.y(), p1.z());
            glVertex3f(p1.x(), p2.y(), p1.z());
            glEnd();
        }
        break;
        
    case ActiveTool::SKETCH_CIRCLE:
        if (m_sketchPoints.size() >= 2) {
            QVector3D center = m_sketchPoints[0];
            QVector3D edge = m_sketchPoints[1];
            float radius = (edge - center).length();
            
            glBegin(GL_LINE_LOOP);
            const int segments = 32;
            for (int i = 0; i < segments; ++i) {
                float angle = 2.0f * M_PI * i / segments;
                float x = center.x() + radius * cos(angle);
                float y = center.y() + radius * sin(angle);
                glVertex3f(x, y, center.z());
            }
            glEnd();
        }
        break;
        
    default:
        break;
    }
    
    glLineWidth(1.0f);
    m_lineShaderProgram->release();
}

void CADViewer::renderLine(const QVector3D& start, const QVector3D& end, float thickness)
{
    if (!m_lineShaderProgram) return;
    
    m_lineShaderProgram->bind();
    m_lineShaderProgram->setUniformValue("model", m_modelMatrix);
    m_lineShaderProgram->setUniformValue("view", m_viewMatrix);
    m_lineShaderProgram->setUniformValue("projection", m_projectionMatrix);
    m_lineShaderProgram->setUniformValue("color", QVector3D(1.0f, 1.0f, 1.0f));
    
    glLineWidth(thickness);
    glBegin(GL_LINES);
    glVertex3f(start.x(), start.y(), start.z());
    glVertex3f(end.x(), end.y(), end.z());
    glEnd();
    glLineWidth(1.0f);
    
    m_lineShaderProgram->release();
}

CADObjectPtr CADViewer::getSelectedObject() const
{
    return m_selectedObjects.empty() ? nullptr : m_selectedObjects[0];
}

// Navigation cube implementation
void CADViewer::setupNavigationCube()
{
    m_navigationCube = new NavigationCube(this);
    m_navigationCube->setFixedSize(80, 80);
    m_navigationCube->move(width() - 90, 10);
    m_navigationCube->show();
    
    connect(m_navigationCube, &NavigationCube::viewChanged, [this](const QString& viewName) {
        if (viewName == "Front") frontView();
        else if (viewName == "Back") backView();
        else if (viewName == "Left") leftView();
        else if (viewName == "Right") rightView();
        else if (viewName == "Top") topView();
        else if (viewName == "Bottom") bottomView();
    });
}

// Grid control methods
void CADViewer::setGridPlane(GridPlane plane)
{
    m_gridPlane = plane;
    update();
}

void CADViewer::setMultiPlaneGridVisible(bool visible)
{
    m_showMultiPlaneGrid = visible;
    update();
}

void CADViewer::toggleGridPlane(GridPlane plane)
{
    int index = static_cast<int>(plane);
    m_visibleGridPlanes[index] = !m_visibleGridPlanes[index];
    update();
}

bool CADViewer::isGridPlaneVisible(GridPlane plane) const
{
    int index = static_cast<int>(plane);
    return m_visibleGridPlanes[index];
}

void CADViewer::setGridSize(float size)
{
    m_gridSize = size;
    update();
}

// Snap functionality
void CADViewer::setSnapToGrid(bool enabled)
{
    m_snapToGrid = enabled;
}

void CADViewer::setSnapMode(SnapMode mode)
{
    m_currentSnapMode = mode;
}

QVector3D CADViewer::snapToGrid(const QVector3D& position) const
{
    if (!m_snapToGrid) return position;
    
    QVector3D snapped = position;
    snapped.setX(std::round(position.x() / m_gridSize) * m_gridSize);
    snapped.setY(std::round(position.y() / m_gridSize) * m_gridSize);
    snapped.setZ(std::round(position.z() / m_gridSize) * m_gridSize);
    
    return snapped;
}

// Tool management
void CADViewer::setActiveTool(ActiveTool tool)
{
    if (m_activeTool != tool) {
        // Cancel any ongoing operations
        cancelShapePlacement();
        
        m_activeTool = tool;
        
        // Auto-start shape placement when placing shapes or using eraser
        if (tool == ActiveTool::PLACE_SHAPE) {
            startShapePlacement();
            emit statusMessageChanged("Shape Creation Mode");
        } else if (tool == ActiveTool::ERASER) {
            setEraserMode(true);
            startShapePlacement(); // Eraser uses same placement workflow
            emit statusMessageChanged("Eraser Mode");
        } else {
            setEraserMode(false);
            emit statusMessageChanged("Navigation Mode");
        }
        
        update();
    }
}

void CADViewer::setShapeToPlace(ObjectType shapeType)
{
    m_shapeToPlace = shapeType;
}

void CADViewer::startShapePlacement()
{
    m_placementState = PlacementState::SETTING_START_POINT;
    emit shapePlacementStarted(m_shapeToPlace);
}

void CADViewer::cancelShapePlacement()
{
    m_placementState = PlacementState::NONE;
    // Preview object removed
    update();
}

// Shape placement workflow
void CADViewer::handleShapePlacementClick(const QPoint& screenPos)
{
    QVector3D worldPos = screenToWorld(screenPos);
    worldPos = applySnapping(worldPos, screenPos);

    if (m_placementState == PlacementState::SETTING_START_POINT) {
        m_placementStartPoint = worldPos;
        m_placementState = PlacementState::WAITING_FOR_SECOND_CLICK;
    } else if (m_placementState == PlacementState::WAITING_FOR_SECOND_CLICK) {
        m_placementEndPoint = worldPos;
        CADObjectPtr newObject = createShapeAtPoints(m_shapeToPlace, m_placementStartPoint, m_placementEndPoint);
        if (newObject) {
            if (m_activeTool == ActiveTool::ERASER) {
                // Perform boolean subtraction
            } else {
                addObject(newObject);
                emit shapePlacementFinished(newObject);
            }
        }
        cancelShapePlacement();
        setActiveTool(ActiveTool::SELECT);
    }
}

void CADViewer::updatePlacementPreview(const QPoint& screenPos)
{
    if (m_placementState == PlacementState::WAITING_FOR_SECOND_CLICK) {
        QVector3D worldPos = screenToWorld(screenPos);
        worldPos = applySnapping(worldPos, screenPos);
        m_placementEndPoint = worldPos;
        update();
    }
}

CADObjectPtr CADViewer::createShapeAtPoints(ObjectType shapeType, const QVector3D& startPoint, const QVector3D& endPoint)
{
    CADObjectPtr object;
    
    // Create the appropriate geometry primitive
    switch (shapeType) {
    case ObjectType::PRIMITIVE_BOX:
        {
            Point3D min(std::min(startPoint.x(), endPoint.x()), std::min(startPoint.y(), endPoint.y()), std::min(startPoint.z(), endPoint.z()));
            Point3D max(std::max(startPoint.x(), endPoint.x()), std::max(startPoint.y(), endPoint.y()), std::max(startPoint.z(), endPoint.z()));
            auto box = std::make_shared<Box>(min, max);
            object = std::static_pointer_cast<CADObject>(box);
        }
        break;
    case ObjectType::PRIMITIVE_CYLINDER:
        {
            QVector3D diff = endPoint - startPoint;
            float radius = std::sqrt(diff.x() * diff.x() + diff.y() * diff.y());
            float height = std::abs(diff.z());
            if (height < 0.01f) height = 1.0f;
            auto cylinder = std::make_shared<Cylinder>(radius, height);
            object = std::static_pointer_cast<CADObject>(cylinder);
        }
        break;
    case ObjectType::PRIMITIVE_SPHERE:
        {
            float radius = (endPoint - startPoint).length();
            auto sphere = std::make_shared<Sphere>(radius);
            sphere->setCenter(Point3D(startPoint.x(), startPoint.y(), startPoint.z()));
            object = std::static_pointer_cast<CADObject>(sphere);
        }
        break;
    case ObjectType::PRIMITIVE_CONE:
        {
            QVector3D diff = endPoint - startPoint;
            float radius = std::sqrt(diff.x() * diff.x() + diff.y() * diff.y());
            float height = std::abs(diff.z());
            if (height < 0.01f) height = 1.0f;
            auto cone = std::make_shared<Cone>(radius, 0.0f, height);
            object = std::static_pointer_cast<CADObject>(cone);
        }
        break;
    case ObjectType::PRIMITIVE_RECTANGLE:
        {
            float thickness = 0.01f;
            Point3D min, max;
            switch (m_gridPlane) {
            case GridPlane::XY_PLANE:
                min = Point3D(std::min(startPoint.x(), endPoint.x()), std::min(startPoint.y(), endPoint.y()), startPoint.z() - thickness/2);
                max = Point3D(std::max(startPoint.x(), endPoint.x()), std::max(startPoint.y(), endPoint.y()), startPoint.z() + thickness/2);
                break;
            case GridPlane::XZ_PLANE:
                min = Point3D(std::min(startPoint.x(), endPoint.x()), startPoint.y() - thickness/2, std::min(startPoint.z(), endPoint.z()));
                max = Point3D(std::max(startPoint.x(), endPoint.x()), startPoint.y() + thickness/2, std::max(startPoint.z(), endPoint.z()));
                break;
            case GridPlane::YZ_PLANE:
                min = Point3D(startPoint.x() - thickness/2, std::min(startPoint.y(), endPoint.y()), std::min(startPoint.z(), endPoint.z()));
                max = Point3D(startPoint.x() + thickness/2, std::max(startPoint.y(), endPoint.y()), std::max(startPoint.z(), endPoint.z()));
                break;
            }
            auto rectangle = std::make_shared<Box>(min, max);
            object = std::static_pointer_cast<CADObject>(rectangle);
        }
        break;
    case ObjectType::PRIMITIVE_CIRCLE:
        {
            QVector3D diff = endPoint - startPoint;
            float radius;
            switch (m_gridPlane) {
            case GridPlane::XY_PLANE:
                radius = std::sqrt(diff.x() * diff.x() + diff.y() * diff.y());
                break;
            case GridPlane::XZ_PLANE:
                radius = std::sqrt(diff.x() * diff.x() + diff.z() * diff.z());
                break;
            case GridPlane::YZ_PLANE:
                radius = std::sqrt(diff.y() * diff.y() + diff.z() * diff.z());
                break;
            }
            float height = 0.01f;
            auto circle = std::make_shared<Cylinder>(radius, height);
            // In a real implementation, we would create a proper 2D circle object
            // and orient it based on the grid plane. For now, we create a thin cylinder.
            object = std::static_pointer_cast<CADObject>(circle);
        }
        break;
    case ObjectType::PRIMITIVE_LINE:
        {
            object = createLineFromPoints(startPoint, endPoint);
        }
        break;
    default:
        return nullptr;
    }

    if (object) {
        Material mat;
        switch (shapeType) {
        case ObjectType::PRIMITIVE_BOX:
            mat.diffuseColor = QColor(100, 150, 255);
            break;
        case ObjectType::PRIMITIVE_CYLINDER:
            mat.diffuseColor = QColor(255, 100, 100);
            break;
        case ObjectType::PRIMITIVE_SPHERE:
            mat.diffuseColor = QColor(100, 255, 100);
            break;
        case ObjectType::PRIMITIVE_CONE:
            mat.diffuseColor = QColor(255, 255, 100);
            break;
        case ObjectType::PRIMITIVE_RECTANGLE:
            mat.diffuseColor = QColor(255, 150, 100);
            break;
        case ObjectType::PRIMITIVE_CIRCLE:
            mat.diffuseColor = QColor(200, 100, 255);
            break;
        case ObjectType::PRIMITIVE_LINE:
            mat.diffuseColor = QColor(255, 255, 255);
            break;
        default:
            mat.diffuseColor = QColor(128, 128, 128);
            break;
        }
        mat.specularColor = QColor(255, 255, 255);
        mat.shininess = 32.0f;
        object->setMaterial(mat);
    }
    
    return object;
}

// Extrusion functionality
void CADViewer::startExtrusionMode(CADObjectPtr object)
{
    if (object) {
        m_extrusionObject = object;
        m_activeTool = ActiveTool::EXTRUDE_2D;
        emit extrusionStarted(object);
        update();
    }
}

void CADViewer::setExtrusionDistance(float distance)
{
    m_extrusionDistance = distance;
    update();
}

void CADViewer::finishExtrusion()
{
    if (m_extrusionObject && m_extrusionDistance > 0.01f) {
        // Create a 3D extruded version of the 2D shape
        CADObjectPtr extrudedObject = nullptr;
        
        ObjectType type = m_extrusionObject->getType();
        if (type == ObjectType::PRIMITIVE_RECTANGLE) {
            // Convert 2D rectangle to 3D box
            Point3D min = m_extrusionObject->getBoundingBoxMin();
            Point3D max = m_extrusionObject->getBoundingBoxMax();
            
            // Extrude in the Z direction
            max.z = min.z + m_extrusionDistance;
            
            auto box = std::make_shared<Box>(min, max);
            extrudedObject = std::static_pointer_cast<CADObject>(box);
        } 
        else if (type == ObjectType::PRIMITIVE_CIRCLE) {
            // Convert 2D circle to 3D cylinder
            // For circles, we need to extract radius from bounding box
            Point3D min = m_extrusionObject->getBoundingBoxMin();
            Point3D max = m_extrusionObject->getBoundingBoxMax();
            float radius = (max.x - min.x) / 2.0f;
            
            auto cylinder = std::make_shared<Cylinder>(radius, m_extrusionDistance);
            extrudedObject = std::static_pointer_cast<CADObject>(cylinder);
        }
        
        if (extrudedObject) {
            // Remove the original 2D object and add the 3D extruded version
            removeObject(m_extrusionObject);
            addObject(extrudedObject);
            selectObject(extrudedObject);
            
            emit extrusionFinished(extrudedObject);
            emit statusMessageChanged("Shape successfully extruded to 3D");
        }
        
        m_extrusionObject.reset();
        m_activeTool = ActiveTool::SELECT;
        update();
    }
}

void CADViewer::handleExtrusionInteraction(const QPoint& screenPos)
{
    Q_UNUSED(screenPos)
    // Handle extrusion interaction based on mouse position
    // Implementation would depend on the specific extrusion UI
}

void CADViewer::updateExtrusionPreview(const QPoint& screenPos)
{
    if (m_extrusionObject && m_activeTool == ActiveTool::EXTRUDE_2D) {
        // Calculate extrusion distance based on mouse position
        QVector3D worldPos = screenToWorld(screenPos);
        
        // Get the object center for reference
        Point3D min = m_extrusionObject->getBoundingBoxMin();
        Point3D max = m_extrusionObject->getBoundingBoxMax();
        QVector3D center((min.x + max.x) / 2, (min.y + max.y) / 2, (min.z + max.z) / 2);
        
        // Calculate distance from center to mouse position
        float distance = (worldPos - center).length();
        
        // Limit the distance to reasonable bounds
        m_extrusionDistance = std::max(0.1f, std::min(10.0f, distance));
        
        update();
    }
}

// Eraser functionality
void CADViewer::setEraserMode(bool enabled)
{
    m_eraserMode = enabled;
    if (enabled) {
        m_activeTool = ActiveTool::ERASER;
    }
}

void CADViewer::setEraserShape(ObjectType shapeType)
{
    m_eraserShape = shapeType;
    // When in eraser mode, also set this as the shape to place for preview
    if (m_eraserMode) {
        m_shapeToPlace = shapeType;
    }
}

void CADViewer::handleEraserClick(const QPoint& screenPos)
{
    CADObjectPtr targetObject = pickObject(screenPos);
    if (targetObject) {
        // Create eraser shape at click position
        QVector3D worldPos = screenToWorld(screenPos);
        CADObjectPtr eraserObject = createShapeAtPoints(m_eraserShape, worldPos, worldPos + QVector3D(1, 1, 1));
        
        if (eraserObject) {
            performBooleanSubtraction(targetObject, eraserObject);
        }
    }
}

// New eraser placement method that works like shape creation
void CADViewer::handleEraserPlacementClick(const QPoint& screenPos)
{
    QVector3D worldPos = screenToWorld(screenPos);
    worldPos = applySnapping(worldPos, screenPos);

    if (m_placementState == PlacementState::SETTING_START_POINT) {
        m_placementStartPoint = worldPos;
        m_placementState = PlacementState::WAITING_FOR_SECOND_CLICK;
    } else if (m_placementState == PlacementState::WAITING_FOR_SECOND_CLICK) {
        m_placementEndPoint = worldPos;
        
        // Create eraser shape
        CADObjectPtr eraserObject = createShapeAtPoints(m_eraserShape, m_placementStartPoint, m_placementEndPoint);
        if (eraserObject) {
            // Find all objects that intersect with the eraser shape and perform boolean subtraction
            std::vector<CADObjectPtr> intersectedObjects;
            for (const auto& object : m_objects) {
                if (object && object.get() != eraserObject.get() && objectsIntersect(object, eraserObject)) {
                    intersectedObjects.push_back(object);
                }
            }
            
            // Perform boolean subtraction on all intersected objects
            for (const auto& targetObject : intersectedObjects) {
                performBooleanSubtraction(targetObject, eraserObject);
            }
        }
        
        cancelShapePlacement();
        setActiveTool(ActiveTool::SELECT);
    }
}

void CADViewer::performBooleanSubtraction(CADObjectPtr target, CADObjectPtr eraser)
{
    if (!target || !eraser) return;
    
    // Create a boolean difference object
    if (m_geometryManager) {
        auto booleanResult = m_geometryManager->performDifference(target, eraser);
        if (booleanResult) {
            // Replace the original object with the boolean result
            removeObject(target);
            addObject(std::static_pointer_cast<CADObject>(booleanResult));
        }
    }
}

bool CADViewer::objectsIntersect(CADObjectPtr obj1, CADObjectPtr obj2)
{
    if (!obj1 || !obj2) return false;
    
    // Simple bounding box intersection test
    Point3D min1 = obj1->getBoundingBoxMin();
    Point3D max1 = obj1->getBoundingBoxMax();
    Point3D min2 = obj2->getBoundingBoxMin();
    Point3D max2 = obj2->getBoundingBoxMax();
    
    return (min1.x <= max2.x && max1.x >= min2.x) &&
           (min1.y <= max2.y && max1.y >= min2.y) &&
           (min1.z <= max2.z && max1.z >= min2.z);
}

bool CADViewer::objectContainsObject(CADObjectPtr outer, CADObjectPtr inner)
{
    if (!outer || !inner) return false;
    
    Point3D min_outer = outer->getBoundingBoxMin();
    Point3D max_outer = outer->getBoundingBoxMax();
    Point3D min_inner = inner->getBoundingBoxMin();
    Point3D max_inner = inner->getBoundingBoxMax();
    
    return (min_inner.x >= min_outer.x && max_inner.x <= max_outer.x &&
            min_inner.y >= min_outer.y && max_inner.y <= max_outer.y &&
            min_inner.z >= min_outer.z && max_inner.z <= max_outer.z);
}

void CADViewer::renderPlacementPreview()
{
    if (m_placementState != PlacementState::WAITING_FOR_SECOND_CLICK) return;

    CADObjectPtr previewObject = createShapeAtPoints(m_shapeToPlace, m_placementStartPoint, m_placementEndPoint);
    if (!previewObject) return;

    // Make the preview object semi-transparent
    Material mat = previewObject->getMaterial();
    mat.transparency = 0.7f;
    previewObject->setMaterial(mat);

    // Render the preview
    m_shaderProgram->bind();
    m_shaderProgram->setUniformValue("model", m_modelMatrix);
    m_shaderProgram->setUniformValue("view", m_viewMatrix);
    m_shaderProgram->setUniformValue("projection", m_projectionMatrix);
    m_shaderProgram->setUniformValue("lightPos", QVector3D(5.0f, 5.0f, 5.0f));
    m_shaderProgram->setUniformValue("viewPos", m_cameraPosition);
    m_shaderProgram->setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
    
    QVector4D color(mat.diffuseColor.redF(), mat.diffuseColor.greenF(), mat.diffuseColor.blueF(), 1.0f - mat.transparency);
    m_shaderProgram->setUniformValue("objectColor", color);
    
    previewObject->render();
    
    m_shaderProgram->release();
}

void CADViewer::renderExtrusionPreview()
{
    if (!m_extrusionObject || m_activeTool != ActiveTool::EXTRUDE_2D) return;
    
    // Create a temporary extruded object for preview
    ObjectType type = m_extrusionObject->getType();
    CADObjectPtr previewObject = nullptr;
    
    if (type == ObjectType::PRIMITIVE_RECTANGLE) {
        Point3D min = m_extrusionObject->getBoundingBoxMin();
        Point3D max = m_extrusionObject->getBoundingBoxMax();
        max.z = min.z + m_extrusionDistance;
        auto box = std::make_shared<Box>(min, max);
        previewObject = std::static_pointer_cast<CADObject>(box);
    } else if (type == ObjectType::PRIMITIVE_CIRCLE) {
        Point3D min = m_extrusionObject->getBoundingBoxMin();
        Point3D max = m_extrusionObject->getBoundingBoxMax();
        float radius = (max.x - min.x) / 2.0f;
        auto cylinder = std::make_shared<Cylinder>(radius, m_extrusionDistance);
        previewObject = std::static_pointer_cast<CADObject>(cylinder);
    }
    
    if (previewObject) {
        Material mat;
        mat.diffuseColor = QColor(0, 255, 0, 100); // Green transparent for preview
        previewObject->setMaterial(mat);
        previewObject->render();
    }
}

void CADViewer::renderEraserPreview()
{
    if (m_activeTool != ActiveTool::ERASER || m_placementState != PlacementState::WAITING_FOR_SECOND_CLICK) return;
    
    CADObjectPtr previewObject = createShapeAtPoints(m_eraserShape, m_placementStartPoint, m_placementEndPoint);
    if (previewObject) {
        Material mat;
        mat.diffuseColor = QColor(255, 0, 0, 100); // Red transparent for eraser
        previewObject->setMaterial(mat);
        previewObject->render();
    }
}

void CADViewer::renderSizeRuler()
{
    // Render a line and text indicating the size of the shape being placed
}

QVector3D CADViewer::getGridPlaneNormal(GridPlane plane) const
{
    switch (plane) {
    case GridPlane::XY_PLANE: return QVector3D(0, 0, 1);
    case GridPlane::XZ_PLANE: return QVector3D(0, 1, 0);
    case GridPlane::YZ_PLANE: return QVector3D(1, 0, 0);
    }
    return QVector3D(0, 0, 1);
}

QVector3D CADViewer::projectToGridPlane(const QVector3D& point, GridPlane plane) const
{
    QVector3D normal = getGridPlaneNormal(plane);
    return point - QVector3D::dotProduct(point, normal) * normal;
}

void CADViewer::renderBox(const Point3D& min, const Point3D& max)
{
    // This function is now part of the Box class render method
}

void CADViewer::renderCylinder(float radius, float height, int segments)
{
    // This function is now part of the Cylinder class render method
}

void CADViewer::renderSphere(float radius, int segments)
{
    // This function is now part of the Sphere class render method
}

void CADViewer::renderCone(float bottomRadius, float topRadius, float height, int segments)
{
    // This function is now part of the Cone class render method
}

void CADViewer::showObjectContextMenu(const QPoint& pos)
{
    if (m_contextMenu) {
        m_contextMenu->exec(pos);
    }
}

void CADViewer::deleteSelectedObject()
{
    if (m_contextMenuObject) {
        removeObject(m_contextMenuObject);
        m_contextMenuObject = nullptr;
    }
}

void CADViewer::reshapeSelectedObject()
{
    if (m_contextMenuObject) {
        // For now, we will just log a message.
        // A real implementation would involve entering a reshape mode.
        qDebug() << "Reshape action triggered for object:" << QString::fromStdString(m_contextMenuObject->getName());
        emit statusMessageChanged("Reshape action is not yet implemented.");
    }
}

void CADViewer::padSelectedObject()
{
    if (m_contextMenuObject) {
        startExtrusionMode(m_contextMenuObject);
        emit statusMessageChanged("Extrusion mode enabled. Move mouse to set distance.");
    }
}

void CADViewer::moveSelectedObject()
{
    if (m_contextMenuObject) {
        // For now, we will just log a message.
        // A real implementation would involve a move tool or gizmo.
        qDebug() << "Move action triggered for object:" << QString::fromStdString(m_contextMenuObject->getName());
        emit statusMessageChanged("Move action is not yet implemented.");
    }
}

void CADViewer::setupContextMenu()
{
    m_contextMenu = new QMenu(this);
    
    m_deleteAction = new QAction("Delete", this);
    m_reshapeAction = new QAction("Reshape", this);
    m_padAction = new QAction("Pad", this);
    m_moveAction = new QAction("Move", this);
    
    m_contextMenu->addAction(m_deleteAction);
    m_contextMenu->addAction(m_reshapeAction);
    m_contextMenu->addAction(m_padAction);
    m_contextMenu->addAction(m_moveAction);
    
    connect(m_deleteAction, &QAction::triggered, this, &CADViewer::deleteSelectedObject);
    connect(m_reshapeAction, &QAction::triggered, this, &CADViewer::reshapeSelectedObject);
    connect(m_padAction, &QAction::triggered, this, &CADViewer::padSelectedObject);
    connect(m_moveAction, &QAction::triggered, this, &CADViewer::moveSelectedObject);
}

QVector3D CADViewer::applySnapping(const QVector3D& position, const QPoint& screenPos) const
{
    switch (m_currentSnapMode) {
    case SnapMode::GRID:
        return snapToGrid(position);
    case SnapMode::VERTEX:
        return snapToVertex(position);
    case SnapMode::EDGE:
        return snapToEdge(position, screenPos);
    case SnapMode::FACE:
        return snapToFace(position, screenPos);
    case SnapMode::CENTER:
        return snapToCenter(position);
    case SnapMode::MIDPOINT:
        return snapToMidpoint(position);
    case SnapMode::NONE:
    default:
        return position;
    }
}

QVector3D CADViewer::snapToVertex(const QVector3D& position) const
{
    QVector3D closestVertex = position;
    float minDistance = 0.5f; // Snap within a certain radius

    for (const auto& object : m_objects) {
        if (auto primitive = std::dynamic_pointer_cast<GeometryPrimitive>(object)) {
            if (!primitive->getVertices().empty()) {
                for (const auto& vertex : primitive->getVertices()) {
                    QVector3D qvertex = vertex.toQVector3D();
                    float distance = position.distanceToPoint(qvertex);
                    if (distance < minDistance) {
                        minDistance = distance;
                        closestVertex = qvertex;
                    }
                }
            }
        }
    }
    return closestVertex;
}

QVector3D CADViewer::snapToEdge(const QVector3D& position, const QPoint& screenPos) const
{
    // Placeholder implementation
    return position;
}

QVector3D CADViewer::snapToFace(const QVector3D& position, const QPoint& screenPos) const
{
    // Placeholder implementation
    return position;
}

QVector3D CADViewer::snapToCenter(const QVector3D& position) const
{
    QVector3D closestCenter = position;
    float minDistance = 0.5f; // Snap within a certain radius

    for (const auto& object : m_objects) {
        Point3D min = object->getBoundingBoxMin();
        Point3D max = object->getBoundingBoxMax();
        QVector3D center((min.x + max.x) / 2, (min.y + max.y) / 2, (min.z + max.z) / 2);
        
        float distance = position.distanceToPoint(center);
        if (distance < minDistance) {
            minDistance = distance;
            closestCenter = center;
        }
    }
    return closestCenter;
}

QVector3D CADViewer::snapToMidpoint(const QVector3D& position) const
{
    // Placeholder implementation
    return position;
}

QVector3D CADViewer::closestPointOnLine(const QVector3D& point, const QVector3D& lineStart, const QVector3D& lineEnd) const
{
    QVector3D lineDir = (lineEnd - lineStart).normalized();
    float t = QVector3D::dotProduct(point - lineStart, lineDir);
    return lineStart + t * lineDir;
}

void CADViewer::loadKeyBindings()
{
    m_settings->beginGroup("KeyBindings");
    for (auto it = m_keyBindings.begin(); it != m_keyBindings.end(); ++it) {
        QVariant value = m_settings->value(QString::number(static_cast<int>(it.key())));
        if (value.isValid()) {
            it.value() = QKeySequence(value.toString());
        }
    }
    m_settings->endGroup();
}

void CADViewer::saveKeyBindings()
{
    m_settings->beginGroup("KeyBindings");
    for (auto it = m_keyBindings.begin(); it != m_keyBindings.end(); ++it) {
        m_settings->setValue(QString::number(static_cast<int>(it.key())), it.value().toString());
    }
    m_settings->endGroup();
}

void CADViewer::setKeyBinding(KeyAction action, const QKeySequence& keySequence)
{
    m_keyBindings[action] = keySequence;
}

QKeySequence CADViewer::getKeyBinding(KeyAction action) const
{
    return m_keyBindings.value(action);
}

QMap<KeyAction, QKeySequence> CADViewer::getDefaultKeyBindings() const
{
    QMap<KeyAction, QKeySequence> defaults;
    defaults[KeyAction::TOGGLE_GRID] = QKeySequence(Qt::Key_G);
    defaults[KeyAction::TOGGLE_WIREFRAME] = QKeySequence(Qt::Key_Z);
    defaults[KeyAction::TOGGLE_AXES] = QKeySequence(Qt::Key_X);
    defaults[KeyAction::RESET_VIEW] = QKeySequence(Qt::Key_Home);
    defaults[KeyAction::FRONT_VIEW] = QKeySequence(Qt::Key_1);
    defaults[KeyAction::BACK_VIEW] = QKeySequence(Qt::CTRL | Qt::Key_1);
    defaults[KeyAction::LEFT_VIEW] = QKeySequence(Qt::Key_3);
    defaults[KeyAction::RIGHT_VIEW] = QKeySequence(Qt::CTRL | Qt::Key_3);
    defaults[KeyAction::TOP_VIEW] = QKeySequence(Qt::Key_7);
    defaults[KeyAction::BOTTOM_VIEW] = QKeySequence(Qt::CTRL | Qt::Key_7);
    defaults[KeyAction::ISOMETRIC_VIEW] = QKeySequence(Qt::Key_9);
    defaults[KeyAction::DELETE_SELECTED] = QKeySequence(Qt::Key_Delete);
    defaults[KeyAction::SELECT_ALL] = QKeySequence(Qt::CTRL | Qt::Key_A);
    defaults[KeyAction::DESELECT_ALL] = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    defaults[KeyAction::PLACE_SHAPE] = QKeySequence(Qt::Key_P);
    defaults[KeyAction::SKETCH_LINE] = QKeySequence(Qt::Key_L);
    defaults[KeyAction::SKETCH_RECTANGLE] = QKeySequence(Qt::Key_R);
    defaults[KeyAction::SKETCH_CIRCLE] = QKeySequence(Qt::Key_C);
    defaults[KeyAction::CANCEL_CURRENT_ACTION] = QKeySequence(Qt::Key_Escape);
    return defaults;
}

} // namespace HybridCAD