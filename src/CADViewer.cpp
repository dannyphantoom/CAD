#include "CADViewer.h"
#include <QtOpenGL/QOpenGLShader>
#include <QtCore/QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <cmath>

namespace HybridCAD {

CADViewer::CADViewer(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_cameraDistance(10.0f)
    , m_cameraRotationX(0.0f)
    , m_cameraRotationY(0.0f)
    , m_wireframeMode(false)
    , m_showGrid(true)
    , m_showAxes(true)
    , m_backgroundColor(64, 64, 64)
    , m_isDragging(false)
    , m_isRotating(false)
    , m_isPanning(false)
    , m_geometryManager(nullptr)
    , m_meshManager(nullptr)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // Initialize camera
    m_cameraPosition = QVector3D(0, 0, m_cameraDistance);
    m_cameraTarget = QVector3D(0, 0, 0);
    m_cameraUp = QVector3D(0, 1, 0);
    
    // Animation timer for smooth updates
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &CADViewer::animate);
    m_animationTimer->start(16); // ~60 FPS
}

CADViewer::~CADViewer()
{
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
    
    // Render grid
    if (m_showGrid) {
        renderGrid();
    }
    
    // Render axes
    if (m_showAxes) {
        renderAxes();
    }
    
    // Render objects
    renderObjects();
    
    // Render selection outline
    renderSelectionOutline();
}

void CADViewer::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    
    // Update projection matrix
    m_projectionMatrix.setToIdentity();
    float aspect = float(width) / float(height ? height : 1);
    m_projectionMatrix.perspective(45.0f, aspect, 0.1f, 1000.0f);
}

void CADViewer::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
    m_dragButton = event->button();
    
    if (event->button() == Qt::LeftButton) {
        // Check for object selection
        CADObjectPtr pickedObject = pickObject(event->pos());
        if (pickedObject) {
            selectObject(pickedObject);
        } else {
            deselectAll();
        }
        m_isRotating = true;
    } else if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
    } else if (event->button() == Qt::RightButton) {
        m_isRotating = true;
    }
    
    m_isDragging = true;
    update();
}

void CADViewer::mouseMoveEvent(QMouseEvent *event)
{
    QPoint delta = event->pos() - m_lastMousePos;
    
    if (m_isDragging) {
        if (m_isRotating && (m_dragButton == Qt::LeftButton || m_dragButton == Qt::RightButton)) {
            rotateCamera(delta.x() * CAMERA_ROTATION_SPEED, delta.y() * CAMERA_ROTATION_SPEED);
        } else if (m_isPanning && m_dragButton == Qt::MiddleButton) {
            panCamera(delta.x() * CAMERA_PAN_SPEED, delta.y() * CAMERA_PAN_SPEED);
        }
        update();
    }
    
    // Update coordinate display
    QVector3D worldPos = screenToWorld(event->pos());
    emit coordinatesChanged(worldPos);
    
    m_lastMousePos = event->pos();
}

void CADViewer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    m_isDragging = false;
    m_isRotating = false;
    m_isPanning = false;
}

void CADViewer::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() / 120.0f;
    zoomCamera(delta);
    update();
}

void CADViewer::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_W:
        m_wireframeMode = !m_wireframeMode;
        update();
        break;
    case Qt::Key_G:
        m_showGrid = !m_showGrid;
        update();
        break;
    case Qt::Key_Home:
        resetView();
        break;
    default:
        QOpenGLWidget::keyPressEvent(event);
    }
}

void CADViewer::animate()
{
    // Smooth camera updates or animations can go here
    // Currently just ensures smooth rendering
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
    glBegin(GL_LINES);
    float gridSize = m_viewSettings.gridSize;
    int gridDivisions = m_viewSettings.gridDivisions;
    float extent = gridSize * gridDivisions;
    
    // Horizontal lines
    for (int i = -gridDivisions; i <= gridDivisions; ++i) {
        float y = i * gridSize;
        glVertex3f(-extent, y, 0.0f);
        glVertex3f(extent, y, 0.0f);
    }
    
    // Vertical lines
    for (int i = -gridDivisions; i <= gridDivisions; ++i) {
        float x = i * gridSize;
        glVertex3f(x, -extent, 0.0f);
        glVertex3f(x, extent, 0.0f);
    }
    glEnd();
    
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
    float x = m_cameraDistance * std::cos(m_cameraRotationY) * std::sin(m_cameraRotationX);
    float y = m_cameraDistance * std::sin(m_cameraRotationY);
    float z = m_cameraDistance * std::cos(m_cameraRotationY) * std::cos(m_cameraRotationX);
    
    m_cameraPosition = QVector3D(x, y, z) + m_cameraTarget;
}

void CADViewer::panCamera(float deltaX, float deltaY)
{
    QVector3D right = QVector3D::crossProduct(m_cameraTarget - m_cameraPosition, m_cameraUp).normalized();
    QVector3D up = QVector3D::crossProduct(right, m_cameraTarget - m_cameraPosition).normalized();
    
    QVector3D translation = right * deltaX + up * deltaY;
    m_cameraTarget += translation;
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

QVector3D CADViewer::screenToWorld(const QPoint& screenPos, float depth) const
{
    // TODO: Implement screen to world coordinate conversion
    Q_UNUSED(screenPos)
    Q_UNUSED(depth)
    return QVector3D();
}

QPoint CADViewer::worldToScreen(const QVector3D& worldPos) const
{
    // TODO: Implement world to screen coordinate conversion
    Q_UNUSED(worldPos)
    return QPoint();
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

CADObjectPtr CADViewer::getSelectedObject() const
{
    return m_selectedObjects.empty() ? nullptr : m_selectedObjects[0];
}

} // namespace HybridCAD 