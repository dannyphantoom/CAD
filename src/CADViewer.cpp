#include "CADViewer.h"
#include "GeometryManager.h"
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

namespace HybridCAD {

CADViewer::CADViewer(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_cameraDistance(10.0f)
    , m_cameraRotationX(0.0f)
    , m_cameraRotationY(0.0f)
    , m_cameraSpeed(DEFAULT_CAMERA_SPEED)
    , m_wireframeMode(false)
    , m_showGrid(true)
    , m_showAxes(true)
    , m_backgroundColor(64, 64, 64)
    , m_gridPlane(GridPlane::XY_PLANE)
    , m_gridSize(DEFAULT_GRID_SIZE)
    , m_snapToGrid(false)
    , m_showMultiPlaneGrid(false)
    , m_visibleGridPlanes{true, false, false}  // Only XY plane visible by default
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
    
    // Setup default keybindings and load custom ones
    setupDefaultKeyBindings();
    loadKeyBindings();
    
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
    
    // Render selection outline
    renderSelectionOutline();
    
    // Render placement preview
    renderPlacementPreview();
    
    // Render sketch preview
    renderSketchPreview();
    
    // Render extrusion preview
    renderExtrusionPreview();
    
    // Render eraser preview
    renderEraserPreview();
    
    // Render size ruler during shape placement
    if (m_placementState == PlacementState::DRAGGING_TO_SIZE || 
        m_placementState == PlacementState::SETTING_END_POINT) {
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
        // Handle different tool modes
        switch (m_activeTool) {
        case ActiveTool::SELECT:
            {
                // Check for object selection
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
            
        case ActiveTool::SKETCH_LINE:
        case ActiveTool::SKETCH_RECTANGLE:
        case ActiveTool::SKETCH_CIRCLE:
            handleSketchClick(event->pos());
            break;
            
        case ActiveTool::EXTRUDE_2D:
            handleExtrusionInteraction(event->pos());
            break;
            
        case ActiveTool::ERASER:
            if (m_eraserMode) {
                handleEraserClick(event->pos());
            }
            break;
            
        default:
            m_isRotating = true;
            break;
        }
    } else if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_isDragging = true;
    } else if (event->button() == Qt::RightButton) {
        // Right click for context or rotation
        if (m_placementState == PlacementState::DRAGGING_TO_SIZE) {
            // Cancel current shape placement
            cancelShapePlacement();
        } else if (m_placementState != PlacementState::NONE || m_isSketchingActive) {
            cancelShapePlacement();
            cancelCurrentSketch();
        } else {
            // Enable rotation with right-click
            m_isRotating = true;
            m_isDragging = true;
        }
    }
    
    // For left click, set dragging based on tool
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
    }
    
    update();
}

void CADViewer::mouseMoveEvent(QMouseEvent *event)
{
    QPoint delta = event->pos() - m_lastMousePos;
    
    if (m_isDragging && (event->buttons() & Qt::MiddleButton)) {
        // Pan the camera
        panCamera(delta.x() * 0.01f, delta.y() * 0.01f);
    } else if (m_isRotating && (event->buttons() & Qt::LeftButton) && m_activeTool == ActiveTool::SELECT) {
        // Rotate the camera when not in placement mode
        rotateCamera(delta.x() * 0.01f, delta.y() * 0.01f);
    } else if (m_isPanning && (event->buttons() & Qt::RightButton)) {
        // Pan the camera
        panCamera(delta.x() * 0.01f, delta.y() * 0.01f);
    }
    
    // Handle shape placement preview updates during mouse movement
    if (m_placementState == PlacementState::DRAGGING_TO_SIZE) {
        updatePlacementPreview(event->pos());
    } else if (m_placementState == PlacementState::SETTING_END_POINT) {
        updatePlacementPreview(event->pos());
    } else if (m_isSketchingActive) {
        updateSketchPreview(event->pos());
    }
    
    // Emit world coordinates for status bar
    QVector3D worldPos = screenToWorld(event->pos());
    emit coordinatesChanged(worldPos);
    
    m_lastMousePos = event->pos();
    update();
}

void CADViewer::mouseReleaseEvent(QMouseEvent *event)
{
    // Handle end of shape dragging
    if (m_placementState == PlacementState::DRAGGING_TO_SIZE && m_isDraggingShape && event->button() == Qt::LeftButton) {
        QVector3D worldPos = screenToWorld(event->pos());
        if (m_snapToGrid) {
            worldPos = snapToGrid(worldPos);
        }
        
        m_placementEndPoint = worldPos;
        
        // Create the actual object
        CADObjectPtr newObject = createShapeAtPoints(m_shapeToPlace, m_placementStartPoint, m_placementEndPoint);
        if (newObject) {
            addObject(newObject);
            emit shapePlacementFinished(newObject);
        }
        
        // Reset for next placement
        m_placementState = PlacementState::SETTING_START_POINT;
        m_isDraggingShape = false;
        update();
        return;
    }
    
    m_isDragging = false;
    m_isRotating = false;
    m_isPanning = false;
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
    m_keyBindings[KeyAction::TOGGLE_GRID] = QKeySequence(Qt::Key_G);
    m_keyBindings[KeyAction::TOGGLE_WIREFRAME] = QKeySequence(Qt::Key_Z);
    m_keyBindings[KeyAction::TOGGLE_AXES] = QKeySequence(Qt::Key_X);
    m_keyBindings[KeyAction::TOGGLE_GRID_XY] = QKeySequence(Qt::SHIFT | Qt::Key_1);
    m_keyBindings[KeyAction::TOGGLE_GRID_XZ] = QKeySequence(Qt::SHIFT | Qt::Key_2);
    m_keyBindings[KeyAction::TOGGLE_GRID_YZ] = QKeySequence(Qt::SHIFT | Qt::Key_3);
    m_keyBindings[KeyAction::TOGGLE_MULTI_PLANE_GRID] = QKeySequence(Qt::SHIFT | Qt::Key_G);
    m_keyBindings[KeyAction::RESET_VIEW] = QKeySequence(Qt::Key_Home);
    m_keyBindings[KeyAction::FRONT_VIEW] = QKeySequence(Qt::Key_1);
    m_keyBindings[KeyAction::BACK_VIEW] = QKeySequence(Qt::CTRL | Qt::Key_1);
    m_keyBindings[KeyAction::LEFT_VIEW] = QKeySequence(Qt::Key_3);
    m_keyBindings[KeyAction::RIGHT_VIEW] = QKeySequence(Qt::CTRL | Qt::Key_3);
    m_keyBindings[KeyAction::TOP_VIEW] = QKeySequence(Qt::Key_7);
    m_keyBindings[KeyAction::BOTTOM_VIEW] = QKeySequence(Qt::CTRL | Qt::Key_7);
    m_keyBindings[KeyAction::ISOMETRIC_VIEW] = QKeySequence(Qt::Key_9);
    m_keyBindings[KeyAction::DELETE_SELECTED] = QKeySequence(Qt::Key_Delete);
    m_keyBindings[KeyAction::SELECT_ALL] = QKeySequence(Qt::CTRL | Qt::Key_A);
    m_keyBindings[KeyAction::DESELECT_ALL] = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    m_keyBindings[KeyAction::PLACE_SHAPE] = QKeySequence(Qt::Key_P);
    m_keyBindings[KeyAction::SKETCH_LINE] = QKeySequence(Qt::Key_L);
    m_keyBindings[KeyAction::SKETCH_RECTANGLE] = QKeySequence(Qt::Key_R);
    m_keyBindings[KeyAction::SKETCH_CIRCLE] = QKeySequence(Qt::Key_C);
    m_keyBindings[KeyAction::CANCEL_CURRENT_ACTION] = QKeySequence(Qt::Key_Escape);
}

void CADViewer::loadKeyBindings()
{
    m_settings->beginGroup("KeyBindings");
    for (auto it = m_keyBindings.begin(); it != m_keyBindings.end(); ++it) {
        QString key = QString::number(static_cast<int>(it.key()));
        QString defaultValue = it.value().toString();
        QString value = m_settings->value(key, defaultValue).toString();
        m_keyBindings[it.key()] = QKeySequence(value);
    }
    m_settings->endGroup();
}

void CADViewer::saveKeyBindings()
{
    if (!m_settings) return;
    
    m_settings->beginGroup("KeyBindings");
    for (auto it = m_keyBindings.begin(); it != m_keyBindings.end(); ++it) {
        QString key = QString::number(static_cast<int>(it.key()));
        m_settings->setValue(key, it.value().toString());
    }
    m_settings->endGroup();
    m_settings->sync();
}

void CADViewer::setKeyBinding(KeyAction action, const QKeySequence& keySequence)
{
    m_keyBindings[action] = keySequence;
}

QKeySequence CADViewer::getKeyBinding(KeyAction action) const
{
    return m_keyBindings.value(action, QKeySequence());
}

void CADViewer::resetKeyBindingsToDefault()
{
    setupDefaultKeyBindings();
    saveKeyBindings();
}

QMap<KeyAction, QKeySequence> CADViewer::getDefaultKeyBindings() const
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
    return defaults;
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
        uniform vec3 objectColor;
        
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
            
            vec3 result = (ambient + diffuse + specular) * objectColor;
            FragColor = vec4(result, 1.0);
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
    
    for (const auto& object : m_objects) {
        if (object && object->isVisible()) {
            // Set object color
            Material mat = object->getMaterial();
            QVector3D color(mat.diffuseColor.redF(), mat.diffuseColor.greenF(), mat.diffuseColor.blueF());
            m_shaderProgram->setUniformValue("objectColor", color);
            
            // Render the object
            object->render();
        }
    }
    
    m_shaderProgram->release();
}

void CADViewer::renderSelectionOutline()
{
    // TODO: Implement selection outline rendering
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
    QVector3D translation = (right * deltaX + up * deltaY) * scaleFactor;
    
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
    // TODO: Implement ray casting for object picking
    Q_UNUSED(screenPos)
    return nullptr;
}

QVector3D CADViewer::screenToWorld(const QPoint& screenPos, float depth)
{
    // Get viewport dimensions
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    // Convert screen coordinates to normalized device coordinates
    float x = (2.0f * screenPos.x()) / viewport[2] - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / viewport[3];
    float z = 2.0f * depth - 1.0f;
    
    // Create NDC coordinates
    QVector4D ndc(x, y, z, 1.0f);
    
    // Calculate inverse matrices
    QMatrix4x4 mvp = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
    QMatrix4x4 invMVP = mvp.inverted();
    
    // Transform to world coordinates
    QVector4D worldPos = invMVP * ndc;
    
    if (worldPos.w() != 0.0f) {
        worldPos /= worldPos.w();
    }
    
    QVector3D result(worldPos.x(), worldPos.y(), worldPos.z());
    
    // Only project to grid plane if we're in a placement mode that requires it
    if (m_placementState != PlacementState::NONE) {
        return projectToGridPlane(result, m_gridPlane);
    }
    
    return result;
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
    
    if (m_snapToGrid) {
        worldPos = snapToGrid(worldPos);
    }
    
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
    if (m_snapToGrid) {
        worldPos = snapToGrid(worldPos);
    }
    
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
    // TODO: Implement actual line object creation
    // For now, return a basic placeholder
    Q_UNUSED(startPoint)
    Q_UNUSED(endPoint)
    return nullptr;
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
        
        // Auto-start shape placement when placing shapes
        if (tool == ActiveTool::PLACE_SHAPE) {
            startShapePlacement();
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
    if (m_snapToGrid) {
        worldPos = snapToGrid(worldPos);
    }
    
    switch (m_placementState) {
    case PlacementState::NONE:
    case PlacementState::SETTING_START_POINT:
        // First click - start dragging
        m_placementStartPoint = worldPos;
        m_currentDragPoint = worldPos;
        m_placementState = PlacementState::DRAGGING_TO_SIZE;
        m_isDraggingShape = true;
        break;
        
    case PlacementState::DRAGGING_TO_SIZE:
        // This shouldn't happen during drag, handled in mouse release
        break;
        
    case PlacementState::SETTING_END_POINT:
        {
            // Legacy two-click mode (fallback)
            m_placementEndPoint = worldPos;
            m_placementState = PlacementState::PLACING;
            
            // Create the actual object
            CADObjectPtr newObject = createShapeAtPoints(m_shapeToPlace, m_placementStartPoint, m_placementEndPoint);
            if (newObject) {
                addObject(newObject);
                emit shapePlacementFinished(newObject);
            }
            
            // Reset for next placement
            m_placementState = PlacementState::SETTING_START_POINT;
            m_isDraggingShape = false;
        }
        break;
        
    default:
        break;
    }
    
    update();
}

void CADViewer::updatePlacementPreview(const QPoint& screenPos)
{
    if (m_placementState == PlacementState::SETTING_END_POINT) {
        QVector3D worldPos = screenToWorld(screenPos);
        if (m_snapToGrid) {
            worldPos = snapToGrid(worldPos);
        }
        m_placementEndPoint = worldPos;
    } else if (m_placementState == PlacementState::DRAGGING_TO_SIZE) {
        QVector3D worldPos = screenToWorld(screenPos);
        if (m_snapToGrid) {
            worldPos = snapToGrid(worldPos);
        }
        m_currentDragPoint = worldPos;
    }
    
    update();
}

CADObjectPtr CADViewer::createShapeAtPoints(ObjectType shapeType, const QVector3D& startPoint, const QVector3D& endPoint)
{
    // Calculate center and size from the two points
    QVector3D center = (startPoint + endPoint) * 0.5f;
    QVector3D size = QVector3D(std::abs(endPoint.x() - startPoint.x()),
                              std::abs(endPoint.y() - startPoint.y()),
                              std::abs(endPoint.z() - startPoint.z()));
    
    // Ensure minimum size
    if (size.length() < 0.1f) {
        size = QVector3D(1.0f, 1.0f, 1.0f);
    }
    
    CADObjectPtr object;
    
    // Create the appropriate geometry primitive
    switch (shapeType) {
    case ObjectType::PRIMITIVE_BOX:
        {
            Point3D min(center.x() - size.x()/2, center.y() - size.y()/2, center.z() - size.z()/2);
            Point3D max(center.x() + size.x()/2, center.y() + size.y()/2, center.z() + size.z()/2);
            auto box = std::make_shared<Box>(min, max);
            object = std::static_pointer_cast<CADObject>(box);
        }
        break;
    case ObjectType::PRIMITIVE_CYLINDER:
        {
            float radius = std::max(size.x(), size.y()) / 2.0f;
            float height = size.z();
            auto cylinder = std::make_shared<Cylinder>(radius, height);
            object = std::static_pointer_cast<CADObject>(cylinder);
        }
        break;
    case ObjectType::PRIMITIVE_SPHERE:
        {
            float radius = std::max({size.x(), size.y(), size.z()}) / 2.0f;
            auto sphere = std::make_shared<Sphere>(radius);
            object = std::static_pointer_cast<CADObject>(sphere);
        }
        break;
    case ObjectType::PRIMITIVE_CONE:
        {
            float radius = std::max(size.x(), size.y()) / 2.0f;
            float height = size.z();
            auto cone = std::make_shared<Cone>(radius, 0.0f, height);
            object = std::static_pointer_cast<CADObject>(cone);
        }
        break;
    case ObjectType::PRIMITIVE_RECTANGLE:
        {
            // Create a thin 2D rectangle on the current grid plane
            float thickness = 0.01f; // Very thin for 2D appearance
            Point3D min, max;
            
            switch (m_gridPlane) {
            case GridPlane::XY_PLANE:
                min = Point3D(center.x() - size.x()/2, center.y() - size.y()/2, center.z() - thickness/2);
                max = Point3D(center.x() + size.x()/2, center.y() + size.y()/2, center.z() + thickness/2);
                break;
            case GridPlane::XZ_PLANE:
                min = Point3D(center.x() - size.x()/2, center.y() - thickness/2, center.z() - size.z()/2);
                max = Point3D(center.x() + size.x()/2, center.y() + thickness/2, center.z() + size.z()/2);
                break;
            case GridPlane::YZ_PLANE:
                min = Point3D(center.x() - thickness/2, center.y() - size.y()/2, center.z() - size.z()/2);
                max = Point3D(center.x() + thickness/2, center.y() + size.y()/2, center.z() + size.z()/2);
                break;
            }
            
            auto rectangle = std::make_shared<Box>(min, max);
            object = std::static_pointer_cast<CADObject>(rectangle);
        }
        break;
    case ObjectType::PRIMITIVE_CIRCLE:
        {
            // Create a thin 2D circle on the current grid plane
            float radius = std::max(size.x(), size.y()) / 2.0f;
            float height = 0.01f; // Very thin for 2D appearance
            auto circle = std::make_shared<Cylinder>(radius, height);
            object = std::static_pointer_cast<CADObject>(circle);
        }
        break;
    case ObjectType::PRIMITIVE_LINE:
        {
            // Create a line representation using a thin cylinder
            float length = (endPoint - startPoint).length();
            float radius = 0.005f; // Very thin line
            auto line = std::make_shared<Cylinder>(radius, length);
            object = std::static_pointer_cast<CADObject>(line);
        }
        break;
    default:
        return nullptr;
    }
    
    if (object) {
        // Set a default material color based on shape type
        Material mat;
        switch (shapeType) {
        case ObjectType::PRIMITIVE_BOX:
            mat.diffuseColor = QColor(100, 150, 255); // Blue
            break;
        case ObjectType::PRIMITIVE_CYLINDER:
            mat.diffuseColor = QColor(255, 100, 100); // Red
            break;
        case ObjectType::PRIMITIVE_SPHERE:
            mat.diffuseColor = QColor(100, 255, 100); // Green
            break;
        case ObjectType::PRIMITIVE_CONE:
            mat.diffuseColor = QColor(255, 255, 100); // Yellow
            break;
        case ObjectType::PRIMITIVE_RECTANGLE:
            mat.diffuseColor = QColor(255, 150, 100); // Orange
            break;
        case ObjectType::PRIMITIVE_CIRCLE:
            mat.diffuseColor = QColor(150, 100, 255); // Purple
            break;
        case ObjectType::PRIMITIVE_LINE:
            mat.diffuseColor = QColor(255, 255, 255); // White
            break;
        default:
            mat.diffuseColor = QColor(128, 128, 128); // Gray
            break;
        }
        object->setMaterial(mat);
        object->setVisible(true);
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
    if (m_extrusionObject) {
        // Perform the actual extrusion operation
        emit extrusionFinished(m_extrusionObject);
        
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
    Q_UNUSED(screenPos)
    // Update extrusion preview based on mouse movement
    update();
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

void CADViewer::performBooleanSubtraction(CADObjectPtr target, CADObjectPtr eraser)
{
    Q_UNUSED(target)
    Q_UNUSED(eraser)
    // Implement boolean subtraction operation
    // This would require a boolean operations library like OpenCASCADE or CGAL
}

// Grid rendering methods
void CADViewer::renderGridPlane(GridPlane plane)
{
    // Enhanced grid rendering for different planes
    Q_UNUSED(plane)
    // Implementation would render grid on the specified plane
}

QVector3D CADViewer::getGridPlaneNormal(GridPlane plane) const
{
    switch (plane) {
    case GridPlane::XY_PLANE: return QVector3D(0, 0, 1);
    case GridPlane::XZ_PLANE: return QVector3D(0, 1, 0);
    case GridPlane::YZ_PLANE: return QVector3D(1, 0, 0);
    default: return QVector3D(0, 0, 1);
    }
}

QVector3D CADViewer::projectToGridPlane(const QVector3D& point, GridPlane plane) const
{
    QVector3D projected = point;
    
    switch (plane) {
    case GridPlane::XY_PLANE:
        projected.setZ(0);
        break;
    case GridPlane::XZ_PLANE:
        projected.setY(0);
        break;
    case GridPlane::YZ_PLANE:
        projected.setX(0);
        break;
    }
    
    return projected;
}

// Enhanced rendering methods
void CADViewer::renderPlacementPreview()
{
    if (m_placementState == PlacementState::SETTING_END_POINT || 
        m_placementState == PlacementState::DRAGGING_TO_SIZE) {
        
        if (!m_gridShaderProgram) return;
        
        QVector3D start = m_placementStartPoint;
        QVector3D end;
        
        if (m_placementState == PlacementState::DRAGGING_TO_SIZE) {
            end = m_currentDragPoint;
        } else {
            end = m_placementEndPoint;
        }
        
        // Calculate preview shape bounds
        QVector3D center = (start + end) * 0.5f;
        QVector3D size = QVector3D(std::abs(end.x() - start.x()),
                                  std::abs(end.y() - start.y()),
                                  std::abs(end.z() - start.z()));
        
        // Ensure minimum visible size
        if (size.length() < 0.1f) {
            size = QVector3D(0.5f, 0.5f, 0.5f);
        }
        
        m_gridShaderProgram->bind();
        m_gridShaderProgram->setUniformValue("model", m_modelMatrix);
        m_gridShaderProgram->setUniformValue("view", m_viewMatrix);
        m_gridShaderProgram->setUniformValue("projection", m_projectionMatrix);
        m_gridShaderProgram->setUniformValue("color", QVector3D(0.8f, 0.8f, 0.2f)); // Yellow preview
        
        glLineWidth(2.0f);
        
        // Draw wireframe preview based on shape type
        switch (m_shapeToPlace) {
        case ObjectType::PRIMITIVE_BOX:
            {
                QVector3D min = center - size * 0.5f;
                QVector3D max = center + size * 0.5f;
                
                // Draw box wireframe
                glBegin(GL_LINES);
                // Bottom face
                glVertex3f(min.x(), min.y(), min.z());
                glVertex3f(max.x(), min.y(), min.z());
                glVertex3f(max.x(), min.y(), min.z());
                glVertex3f(max.x(), max.y(), min.z());
                glVertex3f(max.x(), max.y(), min.z());
                glVertex3f(min.x(), max.y(), min.z());
                glVertex3f(min.x(), max.y(), min.z());
                glVertex3f(min.x(), min.y(), min.z());
                
                // Top face
                glVertex3f(min.x(), min.y(), max.z());
                glVertex3f(max.x(), min.y(), max.z());
                glVertex3f(max.x(), min.y(), max.z());
                glVertex3f(max.x(), max.y(), max.z());
                glVertex3f(max.x(), max.y(), max.z());
                glVertex3f(min.x(), max.y(), max.z());
                glVertex3f(min.x(), max.y(), max.z());
                glVertex3f(min.x(), min.y(), max.z());
                
                // Vertical edges
                glVertex3f(min.x(), min.y(), min.z());
                glVertex3f(min.x(), min.y(), max.z());
                glVertex3f(max.x(), min.y(), min.z());
                glVertex3f(max.x(), min.y(), max.z());
                glVertex3f(max.x(), max.y(), min.z());
                glVertex3f(max.x(), max.y(), max.z());
                glVertex3f(min.x(), max.y(), min.z());
                glVertex3f(min.x(), max.y(), max.z());
                glEnd();
            }
            break;
            
        case ObjectType::PRIMITIVE_RECTANGLE:
            {
                // Draw 2D rectangle wireframe on current grid plane
                QVector3D min = center - size * 0.5f;
                QVector3D max = center + size * 0.5f;
                
                glBegin(GL_LINE_LOOP);
                switch (m_gridPlane) {
                case GridPlane::XY_PLANE:
                    glVertex3f(min.x(), min.y(), center.z());
                    glVertex3f(max.x(), min.y(), center.z());
                    glVertex3f(max.x(), max.y(), center.z());
                    glVertex3f(min.x(), max.y(), center.z());
                    break;
                case GridPlane::XZ_PLANE:
                    glVertex3f(min.x(), center.y(), min.z());
                    glVertex3f(max.x(), center.y(), min.z());
                    glVertex3f(max.x(), center.y(), max.z());
                    glVertex3f(min.x(), center.y(), max.z());
                    break;
                case GridPlane::YZ_PLANE:
                    glVertex3f(center.x(), min.y(), min.z());
                    glVertex3f(center.x(), max.y(), min.z());
                    glVertex3f(center.x(), max.y(), max.z());
                    glVertex3f(center.x(), min.y(), max.z());
                    break;
                }
                glEnd();
            }
            break;
            
        case ObjectType::PRIMITIVE_CIRCLE:
            {
                // Draw 2D circle wireframe on current grid plane
                float radius = std::max(size.x(), size.y()) * 0.5f;
                int segments = 32;
                
                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < segments; ++i) {
                    float angle = 2.0f * M_PI * i / segments;
                    float cosAngle = cos(angle);
                    float sinAngle = sin(angle);
                    
                    switch (m_gridPlane) {
                    case GridPlane::XY_PLANE:
                        glVertex3f(center.x() + radius * cosAngle, center.y() + radius * sinAngle, center.z());
                        break;
                    case GridPlane::XZ_PLANE:
                        glVertex3f(center.x() + radius * cosAngle, center.y(), center.z() + radius * sinAngle);
                        break;
                    case GridPlane::YZ_PLANE:
                        glVertex3f(center.x(), center.y() + radius * cosAngle, center.z() + radius * sinAngle);
                        break;
                    }
                }
                glEnd();
            }
            break;
            
        case ObjectType::PRIMITIVE_LINE:
            {
                // Draw line from start to end
                glBegin(GL_LINES);
                glVertex3f(start.x(), start.y(), start.z());
                glVertex3f(end.x(), end.y(), end.z());
                glEnd();
            }
            break;
            
        case ObjectType::PRIMITIVE_CYLINDER:
        case ObjectType::PRIMITIVE_SPHERE:
        case ObjectType::PRIMITIVE_CONE:
            {
                // Draw simplified wireframe (circle for base)
                float radius = std::max(size.x(), size.y()) * 0.5f;
                int segments = 16;
                
                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < segments; ++i) {
                    float angle = 2.0f * M_PI * i / segments;
                    float x = center.x() + radius * cos(angle);
                    float y = center.y() + radius * sin(angle);
                    glVertex3f(x, y, center.z());
                }
                glEnd();
                
                // For cylinder/cone, draw top circle too
                if (m_shapeToPlace == ObjectType::PRIMITIVE_CYLINDER) {
                    glBegin(GL_LINE_LOOP);
                    for (int i = 0; i < segments; ++i) {
                        float angle = 2.0f * M_PI * i / segments;
                        float x = center.x() + radius * cos(angle);
                        float y = center.y() + radius * sin(angle);
                        glVertex3f(x, y, center.z() + size.z() * 0.5f);
                    }
                    glEnd();
                    
                    // Connect circles with vertical lines
                    glBegin(GL_LINES);
                    for (int i = 0; i < segments; i += 4) {
                        float angle = 2.0f * M_PI * i / segments;
                        float x = center.x() + radius * cos(angle);
                        float y = center.y() + radius * sin(angle);
                        glVertex3f(x, y, center.z() - size.z() * 0.5f);
                        glVertex3f(x, y, center.z() + size.z() * 0.5f);
                    }
                    glEnd();
                }
            }
            break;
            
        default:
            break;
        }
        
        glLineWidth(1.0f);
        m_gridShaderProgram->release();
    }
}

void CADViewer::renderExtrusionPreview()
{
    if (m_extrusionObject && m_activeTool == ActiveTool::EXTRUDE_2D) {
        // Render preview of the extrusion
    }
}

void CADViewer::renderEraserPreview()
{
    if (m_eraserMode) {
        // Render preview of the eraser shape
    }
}

void CADViewer::renderSizeRuler()
{
    if (!m_gridShaderProgram) return;
    
    QVector3D start = m_placementStartPoint;
    QVector3D end;
    
    if (m_placementState == PlacementState::DRAGGING_TO_SIZE) {
        end = m_currentDragPoint;
    } else {
        end = m_placementEndPoint;
    }
    
    // Calculate dimensions
    QVector3D size = end - start;
    float width = std::abs(size.x());
    float height = std::abs(size.y());
    float depth = std::abs(size.z());
    
    m_gridShaderProgram->bind();
    m_gridShaderProgram->setUniformValue("model", m_modelMatrix);
    m_gridShaderProgram->setUniformValue("view", m_viewMatrix);
    m_gridShaderProgram->setUniformValue("projection", m_projectionMatrix);
    m_gridShaderProgram->setUniformValue("color", QVector3D(1.0f, 1.0f, 0.0f)); // Yellow
    
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    
    // Draw dimension lines
    QVector3D center = (start + end) * 0.5f;
    
    // Width line (X direction)
    if (width > 0.01f) {
        float y_offset = (start.y() < end.y()) ? std::min(start.y(), end.y()) - 0.5f : std::max(start.y(), end.y()) + 0.5f;
        glVertex3f(start.x(), y_offset, center.z());
        glVertex3f(end.x(), y_offset, center.z());
        
        // Tick marks
        glVertex3f(start.x(), y_offset - 0.2f, center.z());
        glVertex3f(start.x(), y_offset + 0.2f, center.z());
        glVertex3f(end.x(), y_offset - 0.2f, center.z());
        glVertex3f(end.x(), y_offset + 0.2f, center.z());
    }
    
    // Height line (Y direction)
    if (height > 0.01f) {
        float x_offset = (start.x() < end.x()) ? std::min(start.x(), end.x()) - 0.5f : std::max(start.x(), end.x()) + 0.5f;
        glVertex3f(x_offset, start.y(), center.z());
        glVertex3f(x_offset, end.y(), center.z());
        
        // Tick marks
        glVertex3f(x_offset - 0.2f, start.y(), center.z());
        glVertex3f(x_offset + 0.2f, start.y(), center.z());
        glVertex3f(x_offset - 0.2f, end.y(), center.z());
        glVertex3f(x_offset + 0.2f, end.y(), center.z());
    }
    
    // Depth line (Z direction) - only if significant depth
    if (depth > 0.01f) {
        float x_offset = std::max(start.x(), end.x()) + 0.5f;
        float y_offset = std::max(start.y(), end.y()) + 0.5f;
        glVertex3f(x_offset, y_offset, start.z());
        glVertex3f(x_offset, y_offset, end.z());
        
        // Tick marks
        glVertex3f(x_offset - 0.1f, y_offset, start.z());
        glVertex3f(x_offset + 0.1f, y_offset, start.z());
        glVertex3f(x_offset - 0.1f, y_offset, end.z());
        glVertex3f(x_offset + 0.1f, y_offset, end.z());
    }
    
    glEnd();
    glLineWidth(1.0f);
    m_gridShaderProgram->release();
    
    // Render text labels using QPainter overlay
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QColor(255, 255, 0)); // Yellow text
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    
    // Width text
    if (width > 0.01f) {
        QVector3D labelPos = QVector3D(center.x(), (start.y() < end.y()) ? std::min(start.y(), end.y()) - 0.7f : std::max(start.y(), end.y()) + 0.7f, center.z());
        QPoint screenPos = worldToScreen(labelPos);
        if (screenPos.x() >= 0 && screenPos.y() >= 0) {
            QString widthText = QString::number(width, 'f', 2);
            painter.drawText(screenPos, widthText);
        }
    }
    
    // Height text
    if (height > 0.01f) {
        QVector3D labelPos = QVector3D((start.x() < end.x()) ? std::min(start.x(), end.x()) - 0.7f : std::max(start.x(), end.x()) + 0.7f, center.y(), center.z());
        QPoint screenPos = worldToScreen(labelPos);
        if (screenPos.x() >= 0 && screenPos.y() >= 0) {
            QString heightText = QString::number(height, 'f', 2);
            painter.drawText(screenPos, heightText);
        }
    }
    
    // Depth text
    if (depth > 0.01f) {
        QVector3D labelPos = QVector3D(std::max(start.x(), end.x()) + 0.7f, std::max(start.y(), end.y()) + 0.7f, center.z());
        QPoint screenPos = worldToScreen(labelPos);
        if (screenPos.x() >= 0 && screenPos.y() >= 0) {
            QString depthText = QString::number(depth, 'f', 2);
            painter.drawText(screenPos, depthText);
        }
    }
}

// NavigationCube implementation
NavigationCube::NavigationCube(QWidget *parent)
    : QWidget(parent), m_isHovered(false)
{
    setFixedSize(80, 80);
    setupFaces();
    setAttribute(Qt::WA_Hover, true);
}

NavigationCube::~NavigationCube() = default;

void NavigationCube::setupFaces()
{
    m_faceNames << "Front" << "Back" << "Left" << "Right" << "Top" << "Bottom";
    
    // Define face rectangles (simplified cube projection)
    int size = 80;
    int face = size / 3;
    
    m_faceRects["Front"] = QRect(face, face, face, face);
    m_faceRects["Back"] = QRect(0, 0, face, face);
    m_faceRects["Left"] = QRect(0, face, face, face);
    m_faceRects["Right"] = QRect(face * 2, face, face, face);
    m_faceRects["Top"] = QRect(face, 0, face, face);
    m_faceRects["Bottom"] = QRect(face, face * 2, face, face);
}

void NavigationCube::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw cube faces
    painter.setPen(QPen(Qt::gray, 1));
    
    for (auto it = m_faceRects.begin(); it != m_faceRects.end(); ++it) {
        const QString& faceName = it.key();
        const QRect& rect = it.value();
        
        QColor faceColor = (faceName == m_hoveredFace) ? QColor(100, 150, 255) : QColor(70, 70, 70);
        painter.fillRect(rect, faceColor);
        painter.drawRect(rect);
        
        // Draw face label
        painter.setPen(Qt::white);
        painter.drawText(rect, Qt::AlignCenter, faceName.left(1));
    }
}

void NavigationCube::mousePressEvent(QMouseEvent *event)
{
    QString face = getFaceFromPosition(event->pos());
    if (!face.isEmpty()) {
        emit viewChanged(face);
    }
}

void NavigationCube::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    m_isHovered = true;
    update();
}

void NavigationCube::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_isHovered = false;
    m_hoveredFace.clear();
    update();
}

QString NavigationCube::getFaceFromPosition(const QPoint& pos)
{
    for (auto it = m_faceRects.begin(); it != m_faceRects.end(); ++it) {
        if (it.value().contains(pos)) {
            m_hoveredFace = it.key();
            update();
            return it.key();
        }
    }
    return QString();
}

} // namespace HybridCAD 