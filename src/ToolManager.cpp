#include "ToolManager.h"
#include <QtWidgets/QCheckBox>

namespace HybridCAD {

ToolManager::ToolManager(QWidget *parent)
    : QWidget(parent)
    , m_activeTool(ToolType::SELECT)
    , m_snapMode(SnapMode::NONE)
    , m_gridSize(1.0f)
    , m_angleSnap(false)
    , m_angleSnapIncrement(15.0f)
    , m_gridPlane(0) // XY plane
{
    setupUI();
}

ToolManager::~ToolManager() {
}

void ToolManager::setActiveTool(ToolType tool) {
    if (m_activeTool != tool) {
        m_activeTool = tool;
        updateToolParameters();
        emit toolChanged(tool);
    }
}

void ToolManager::setSnapMode(SnapMode mode) {
    if (m_snapMode != mode) {
        m_snapMode = mode;
        emit snapModeChanged(mode);
    }
}

void ToolManager::setGridSize(float size) {
    if (m_gridSize != size) {
        m_gridSize = size;
        m_gridSizeSpin->setValue(size);
        emit gridSizeChanged(size);
    }
}

void ToolManager::setAngleSnap(bool enabled) {
    if (m_angleSnap != enabled) {
        m_angleSnap = enabled;
        m_angleSnapCheck->setChecked(enabled);
        emit angleSnapChanged(enabled);
    }
}

void ToolManager::setAngleSnapIncrement(float degrees) {
    if (m_angleSnapIncrement != degrees) {
        m_angleSnapIncrement = degrees;
        m_angleIncrementSpin->setValue(degrees);
        emit angleSnapIncrementChanged(degrees);
    }
}

void ToolManager::setGridPlane(int plane) {
    if (m_gridPlane != plane) {
        m_gridPlane = plane;
        m_gridPlaneCombo->setCurrentIndex(plane);
        emit gridPlaneChanged(plane);
    }
}

void ToolManager::onToolButtonClicked() {
    QToolButton* button = qobject_cast<QToolButton*>(sender());
    if (!button) return;
    
    ToolType tool = static_cast<ToolType>(m_toolButtonGroup->id(button));
    setActiveTool(tool);
}

void ToolManager::onSnapModeChanged() {
    QToolButton* button = qobject_cast<QToolButton*>(sender());
    if (!button) return;
    
    SnapMode mode = static_cast<SnapMode>(m_snapButtonGroup->id(button));
    setSnapMode(mode);
}

void ToolManager::onGridSizeChanged() {
    setGridSize(m_gridSizeSpin->value());
}

void ToolManager::onAngleSnapToggled(bool enabled) {
    setAngleSnap(enabled);
}

void ToolManager::onAngleSnapIncrementChanged() {
    setAngleSnapIncrement(m_angleIncrementSpin->value());
}

void ToolManager::onGridPlaneChanged() {
    setGridPlane(m_gridPlaneCombo->currentIndex());
}

void ToolManager::onToolParametersChanged() {
    emit parametersChanged();
}

void ToolManager::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    
    createToolButtons();
    createSnapControls();
    createGridControls();
    createAngleControls();
    createToolParameters();
    
    setLayout(m_mainLayout);
}

void ToolManager::createToolButtons() {
    m_toolGroup = new QGroupBox("Tools", this);
    m_toolLayout = new QVBoxLayout(m_toolGroup);
    m_toolButtonGroup = new QButtonGroup(this);
    
    // Create tool buttons
    m_selectButton = new QToolButton(this);
    m_selectButton->setText("Select");
    m_selectButton->setCheckable(true);
    m_selectButton->setChecked(true);
    m_toolButtonGroup->addButton(m_selectButton, static_cast<int>(ToolType::SELECT));
    
    m_moveButton = new QToolButton(this);
    m_moveButton->setText("Move");
    m_moveButton->setCheckable(true);
    m_toolButtonGroup->addButton(m_moveButton, static_cast<int>(ToolType::MOVE));
    
    m_rotateButton = new QToolButton(this);
    m_rotateButton->setText("Rotate");
    m_rotateButton->setCheckable(true);
    m_toolButtonGroup->addButton(m_rotateButton, static_cast<int>(ToolType::ROTATE));
    
    m_scaleButton = new QToolButton(this);
    m_scaleButton->setText("Scale");
    m_scaleButton->setCheckable(true);
    m_toolButtonGroup->addButton(m_scaleButton, static_cast<int>(ToolType::SCALE));
    
    m_extrudeButton = new QToolButton(this);
    m_extrudeButton->setText("Extrude");
    m_extrudeButton->setCheckable(true);
    m_toolButtonGroup->addButton(m_extrudeButton, static_cast<int>(ToolType::EXTRUDE));
    
    m_sketchButton = new QToolButton(this);
    m_sketchButton->setText("Sketch");
    m_sketchButton->setCheckable(true);
    m_toolButtonGroup->addButton(m_sketchButton, static_cast<int>(ToolType::SKETCH));
    
    m_measureButton = new QToolButton(this);
    m_measureButton->setText("Measure");
    m_measureButton->setCheckable(true);
    m_toolButtonGroup->addButton(m_measureButton, static_cast<int>(ToolType::MEASURE));
    
    m_sectionButton = new QToolButton(this);
    m_sectionButton->setText("Section");
    m_sectionButton->setCheckable(true);
    m_toolButtonGroup->addButton(m_sectionButton, static_cast<int>(ToolType::SECTION));
    
    // Add new tool buttons
    QToolButton* m_placeShapeButton = new QToolButton(this);
    m_placeShapeButton->setText("Place Shape");
    m_placeShapeButton->setCheckable(true);
    m_toolButtonGroup->addButton(m_placeShapeButton, static_cast<int>(ToolType::PLACE_SHAPE));
    
    QToolButton* m_extrude2DButton = new QToolButton(this);
    m_extrude2DButton->setText("Extrude 2D");
    m_extrude2DButton->setCheckable(true);
    m_toolButtonGroup->addButton(m_extrude2DButton, static_cast<int>(ToolType::EXTRUDE_2D));
    
    QToolButton* m_eraserButton = new QToolButton(this);
    m_eraserButton->setText("Eraser");
    m_eraserButton->setCheckable(true);
    m_toolButtonGroup->addButton(m_eraserButton, static_cast<int>(ToolType::ERASER));
    
    // Add buttons to layout
    m_toolLayout->addWidget(m_selectButton);
    m_toolLayout->addWidget(m_moveButton);
    m_toolLayout->addWidget(m_rotateButton);
    m_toolLayout->addWidget(m_scaleButton);
    m_toolLayout->addWidget(m_extrudeButton);
    m_toolLayout->addWidget(m_sketchButton);
    m_toolLayout->addWidget(m_measureButton);
    m_toolLayout->addWidget(m_sectionButton);
    m_toolLayout->addWidget(m_placeShapeButton);
    m_toolLayout->addWidget(m_extrude2DButton);
    m_toolLayout->addWidget(m_eraserButton);
    
    m_mainLayout->addWidget(m_toolGroup);
    
    // Connect signals
    connect(m_toolButtonGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &ToolManager::onToolButtonClicked);
}

void ToolManager::createSnapControls() {
    m_snapGroup = new QGroupBox("Snap", this);
    m_snapLayout = new QVBoxLayout(m_snapGroup);
    m_snapButtonGroup = new QButtonGroup(this);
    
    // Create snap buttons
    m_snapNoneButton = new QToolButton(this);
    m_snapNoneButton->setText("None");
    m_snapNoneButton->setCheckable(true);
    m_snapNoneButton->setChecked(true);
    m_snapButtonGroup->addButton(m_snapNoneButton, static_cast<int>(SnapMode::NONE));
    
    m_snapGridButton = new QToolButton(this);
    m_snapGridButton->setText("Grid");
    m_snapGridButton->setCheckable(true);
    m_snapButtonGroup->addButton(m_snapGridButton, static_cast<int>(SnapMode::GRID));
    
    m_snapVertexButton = new QToolButton(this);
    m_snapVertexButton->setText("Vertex");
    m_snapVertexButton->setCheckable(true);
    m_snapButtonGroup->addButton(m_snapVertexButton, static_cast<int>(SnapMode::VERTEX));
    
    m_snapEdgeButton = new QToolButton(this);
    m_snapEdgeButton->setText("Edge");
    m_snapEdgeButton->setCheckable(true);
    m_snapButtonGroup->addButton(m_snapEdgeButton, static_cast<int>(SnapMode::EDGE));
    
    m_snapFaceButton = new QToolButton(this);
    m_snapFaceButton->setText("Face");
    m_snapFaceButton->setCheckable(true);
    m_snapButtonGroup->addButton(m_snapFaceButton, static_cast<int>(SnapMode::FACE));
    
    m_snapCenterButton = new QToolButton(this);
    m_snapCenterButton->setText("Center");
    m_snapCenterButton->setCheckable(true);
    m_snapButtonGroup->addButton(m_snapCenterButton, static_cast<int>(SnapMode::CENTER));
    
    m_snapMidpointButton = new QToolButton(this);
    m_snapMidpointButton->setText("Midpoint");
    m_snapMidpointButton->setCheckable(true);
    m_snapButtonGroup->addButton(m_snapMidpointButton, static_cast<int>(SnapMode::MIDPOINT));
    
    // Add buttons to layout
    m_snapLayout->addWidget(m_snapNoneButton);
    m_snapLayout->addWidget(m_snapGridButton);
    m_snapLayout->addWidget(m_snapVertexButton);
    m_snapLayout->addWidget(m_snapEdgeButton);
    m_snapLayout->addWidget(m_snapFaceButton);
    m_snapLayout->addWidget(m_snapCenterButton);
    m_snapLayout->addWidget(m_snapMidpointButton);
    
    m_mainLayout->addWidget(m_snapGroup);
    
    // Connect signals
    connect(m_snapButtonGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &ToolManager::onSnapModeChanged);
}

void ToolManager::createGridControls() {
    m_gridGroup = new QGroupBox("Grid", this);
    m_gridLayout = new QVBoxLayout(m_gridGroup);
    
    m_gridSizeLabel = new QLabel("Grid Size:", this);
    m_gridSizeSpin = new QDoubleSpinBox(this);
    m_gridSizeSpin->setRange(0.1, 100.0);
    m_gridSizeSpin->setValue(m_gridSize);
    m_gridSizeSpin->setSingleStep(0.1);
    
    m_gridPlaneLabel = new QLabel("Grid Plane:", this);
    m_gridPlaneCombo = new QComboBox(this);
    m_gridPlaneCombo->addItem("XY Plane");
    m_gridPlaneCombo->addItem("XZ Plane");
    m_gridPlaneCombo->addItem("YZ Plane");
    m_gridPlaneCombo->setCurrentIndex(m_gridPlane);
    
    m_gridLayout->addWidget(m_gridSizeLabel);
    m_gridLayout->addWidget(m_gridSizeSpin);
    m_gridLayout->addWidget(m_gridPlaneLabel);
    m_gridLayout->addWidget(m_gridPlaneCombo);
    
    m_mainLayout->addWidget(m_gridGroup);
    
    // Connect signals
    connect(m_gridSizeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ToolManager::onGridSizeChanged);
    connect(m_gridPlaneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolManager::onGridPlaneChanged);
}

void ToolManager::createAngleControls() {
    m_angleGroup = new QGroupBox("Angle Snap", this);
    m_angleLayout = new QVBoxLayout(m_angleGroup);
    
    m_angleSnapCheck = new QCheckBox("Enable Angle Snap", this);
    m_angleSnapCheck->setChecked(m_angleSnap);
    
    m_angleIncrementLabel = new QLabel("Increment (degrees):", this);
    m_angleIncrementSpin = new QDoubleSpinBox(this);
    m_angleIncrementSpin->setRange(1.0, 180.0);
    m_angleIncrementSpin->setValue(m_angleSnapIncrement);
    m_angleIncrementSpin->setSingleStep(1.0);
    
    m_angleLayout->addWidget(m_angleSnapCheck);
    m_angleLayout->addWidget(m_angleIncrementLabel);
    m_angleLayout->addWidget(m_angleIncrementSpin);
    
    m_mainLayout->addWidget(m_angleGroup);
    
    // Connect signals
    connect(m_angleSnapCheck, &QCheckBox::toggled,
            this, &ToolManager::onAngleSnapToggled);
    connect(m_angleIncrementSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ToolManager::onAngleSnapIncrementChanged);
}

void ToolManager::createToolParameters() {
    m_parametersGroup = new QGroupBox("Tool Parameters", this);
    m_parametersLayout = new QVBoxLayout(m_parametersGroup);
    
    // Create parameter widgets for different tools
    
    // Move tool parameters
    m_moveParametersWidget = new QWidget(this);
    QVBoxLayout* moveLayout = new QVBoxLayout(m_moveParametersWidget);
    
    m_moveStepSpin = new QDoubleSpinBox(this);
    m_moveStepSpin->setRange(0.01, 100.0);
    m_moveStepSpin->setValue(1.0);
    moveLayout->addWidget(new QLabel("Step Size:"));
    moveLayout->addWidget(m_moveStepSpin);
    
    m_moveConstraintXCheck = new QCheckBox("Constrain X", this);
    m_moveConstraintYCheck = new QCheckBox("Constrain Y", this);
    m_moveConstraintZCheck = new QCheckBox("Constrain Z", this);
    moveLayout->addWidget(m_moveConstraintXCheck);
    moveLayout->addWidget(m_moveConstraintYCheck);
    moveLayout->addWidget(m_moveConstraintZCheck);
    
    // Rotate tool parameters
    m_rotateParametersWidget = new QWidget(this);
    QVBoxLayout* rotateLayout = new QVBoxLayout(m_rotateParametersWidget);
    
    m_rotateAngleSpin = new QDoubleSpinBox(this);
    m_rotateAngleSpin->setRange(-360.0, 360.0);
    m_rotateAngleSpin->setValue(90.0);
    rotateLayout->addWidget(new QLabel("Angle:"));
    rotateLayout->addWidget(m_rotateAngleSpin);
    
    m_rotateConstraintXCheck = new QCheckBox("X Axis", this);
    m_rotateConstraintYCheck = new QCheckBox("Y Axis", this);
    m_rotateConstraintZCheck = new QCheckBox("Z Axis", this);
    rotateLayout->addWidget(m_rotateConstraintXCheck);
    rotateLayout->addWidget(m_rotateConstraintYCheck);
    rotateLayout->addWidget(m_rotateConstraintZCheck);
    
    // Scale tool parameters
    m_scaleParametersWidget = new QWidget(this);
    QVBoxLayout* scaleLayout = new QVBoxLayout(m_scaleParametersWidget);
    
    m_scaleFactorSpin = new QDoubleSpinBox(this);
    m_scaleFactorSpin->setRange(0.1, 10.0);
    m_scaleFactorSpin->setValue(1.0);
    scaleLayout->addWidget(new QLabel("Scale Factor:"));
    scaleLayout->addWidget(m_scaleFactorSpin);
    
    m_scaleUniformCheck = new QCheckBox("Uniform Scale", this);
    m_scaleUniformCheck->setChecked(true);
    scaleLayout->addWidget(m_scaleUniformCheck);
    
    // Extrude tool parameters
    m_extrudeParametersWidget = new QWidget(this);
    QVBoxLayout* extrudeLayout = new QVBoxLayout(m_extrudeParametersWidget);
    
    m_extrudeDistanceSpin = new QDoubleSpinBox(this);
    m_extrudeDistanceSpin->setRange(-100.0, 100.0);
    m_extrudeDistanceSpin->setValue(1.0);
    extrudeLayout->addWidget(new QLabel("Distance:"));
    extrudeLayout->addWidget(m_extrudeDistanceSpin);
    
    m_extrudeBothDirectionsCheck = new QCheckBox("Both Directions", this);
    extrudeLayout->addWidget(m_extrudeBothDirectionsCheck);
    
    m_extrudeTaperSlider = new QSlider(Qt::Horizontal, this);
    m_extrudeTaperSlider->setRange(-45, 45);
    m_extrudeTaperSlider->setValue(0);
    extrudeLayout->addWidget(new QLabel("Taper Angle:"));
    extrudeLayout->addWidget(m_extrudeTaperSlider);
    
    // Hide all parameter widgets initially
    m_moveParametersWidget->hide();
    m_rotateParametersWidget->hide();
    m_scaleParametersWidget->hide();
    m_extrudeParametersWidget->hide();
    
    m_parametersLayout->addWidget(m_moveParametersWidget);
    m_parametersLayout->addWidget(m_rotateParametersWidget);
    m_parametersLayout->addWidget(m_scaleParametersWidget);
    m_parametersLayout->addWidget(m_extrudeParametersWidget);
    
    m_mainLayout->addWidget(m_parametersGroup);
    
    // Connect parameter change signals
    connect(m_moveStepSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ToolManager::onToolParametersChanged);
    connect(m_rotateAngleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ToolManager::onToolParametersChanged);
    connect(m_scaleFactorSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ToolManager::onToolParametersChanged);
    connect(m_extrudeDistanceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ToolManager::onToolParametersChanged);
}

void ToolManager::updateToolParameters() {
    hideAllParametersWidgets();
    
    switch (m_activeTool) {
        case ToolType::MOVE:
            showParametersWidget(m_moveParametersWidget);
            break;
        case ToolType::ROTATE:
            showParametersWidget(m_rotateParametersWidget);
            break;
        case ToolType::SCALE:
            showParametersWidget(m_scaleParametersWidget);
            break;
        case ToolType::EXTRUDE:
            showParametersWidget(m_extrudeParametersWidget);
            break;
        default:
            break;
    }
}

void ToolManager::setToolButtonIcon(QToolButton* button, ToolType tool) {
    // Placeholder for setting tool icons
}

void ToolManager::setSnapButtonIcon(QToolButton* button, SnapMode mode) {
    // Placeholder for setting snap icons
}

QString ToolManager::getToolName(ToolType tool) const {
    switch (tool) {
        case ToolType::SELECT: return "Select";
        case ToolType::MOVE: return "Move";
        case ToolType::ROTATE: return "Rotate";
        case ToolType::SCALE: return "Scale";
        case ToolType::EXTRUDE: return "Extrude";
        case ToolType::SKETCH: return "Sketch";
        case ToolType::MEASURE: return "Measure";
        case ToolType::SECTION: return "Section";
    }
    return "Unknown";
}

QString ToolManager::getSnapModeName(SnapMode mode) const {
    switch (mode) {
        case SnapMode::NONE: return "None";
        case SnapMode::GRID: return "Grid";
        case SnapMode::VERTEX: return "Vertex";
        case SnapMode::EDGE: return "Edge";
        case SnapMode::FACE: return "Face";
        case SnapMode::CENTER: return "Center";
        case SnapMode::MIDPOINT: return "Midpoint";
    }
    return "Unknown";
}

void ToolManager::showParametersWidget(QWidget* widget) {
    if (widget) {
        widget->show();
    }
}

void ToolManager::hideAllParametersWidgets() {
    m_moveParametersWidget->hide();
    m_rotateParametersWidget->hide();
    m_scaleParametersWidget->hide();
    m_extrudeParametersWidget->hide();
}

} // namespace HybridCAD 