#include "PropertyPanel.h"

namespace HybridCAD {

PropertyPanel::PropertyPanel(QWidget *parent)
    : QWidget(parent)
    , m_currentObject(nullptr)
    , m_updating(false)
{
    setupUI();
}

PropertyPanel::~PropertyPanel() {
}

void PropertyPanel::setSelectedObject(CADObjectPtr object) {
    if (m_currentObject == object) return;
    
    disconnectSignals();
    m_currentObject = object;
    updateProperties();
    connectSignals();
}

void PropertyPanel::clearSelection() {
    disconnectSignals();
    m_currentObject = nullptr;
    
    // Clear all controls
    m_nameEdit->clear();
    m_visibilityCheck->setChecked(false);
    m_typeCombo->setCurrentIndex(0);
    
    clearGeometryProperties();
}

void PropertyPanel::updateProperties() {
    if (!m_currentObject) {
        clearSelection();
        return;
    }
    
    m_updating = true;
    
    updateGeneralProperties();
    updateTransformProperties();
    updateMaterialProperties();
    updateGeometryProperties();
    
    m_updating = false;
}

void PropertyPanel::onNameChanged() {
    if (m_updating || !m_currentObject) return;
    
    m_currentObject->setName(m_nameEdit->text().toStdString());
    emit propertyChanged();
}

void PropertyPanel::onVisibilityChanged(bool visible) {
    if (m_updating || !m_currentObject) return;
    
    m_currentObject->setVisible(visible);
    emit propertyChanged();
}

void PropertyPanel::onMaterialChanged() {
    if (m_updating || !m_currentObject) return;
    
    // Update material properties based on UI controls
    Material material = m_currentObject->getMaterial();
    
    // Update the material (implementation would read from UI controls)
    m_currentObject->setMaterial(material);
    
    emit propertyChanged();
}

void PropertyPanel::onTransformChanged() {
    if (m_updating || !m_currentObject) return;
    
    // Transform changes would be handled here
    emit propertyChanged();
}

void PropertyPanel::onGeometryParameterChanged() {
    if (m_updating || !m_currentObject) return;
    
    // Update geometry-specific parameters
    emit propertyChanged();
}

void PropertyPanel::onColorChanged() {
    if (m_updating || !m_currentObject) return;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    QColor color = QColorDialog::getColor(Qt::white, this);
    if (color.isValid()) {
        Material material = m_currentObject->getMaterial();
        
        if (button == m_diffuseColorButton) {
            material.diffuseColor = color;
        } else if (button == m_specularColorButton) {
            material.specularColor = color;
        }
        
        m_currentObject->setMaterial(material);
        
        // Update button color
        QPalette palette = button->palette();
        palette.setColor(QPalette::Button, color);
        button->setPalette(palette);
        
        emit propertyChanged();
    }
}

void PropertyPanel::onApplyChanges() {
    if (m_currentObject) {
        emit objectModified(m_currentObject);
    }
}

void PropertyPanel::onResetChanges() {
    updateProperties();
}

void PropertyPanel::setupUI() {
    m_scrollArea = new QScrollArea(this);
    m_contentWidget = new QWidget();
    m_mainLayout = new QVBoxLayout(m_contentWidget);
    
    createGeneralProperties();
    createTransformProperties();
    createMaterialProperties();
    createGeometryProperties();
    
    // Add action buttons
    m_buttonLayout = new QHBoxLayout();
    m_applyButton = new QPushButton("Apply", this);
    m_resetButton = new QPushButton("Reset", this);
    
    m_buttonLayout->addWidget(m_applyButton);
    m_buttonLayout->addWidget(m_resetButton);
    m_buttonLayout->addStretch();
    
    m_mainLayout->addLayout(m_buttonLayout);
    m_mainLayout->addStretch();
    
    m_scrollArea->setWidget(m_contentWidget);
    m_scrollArea->setWidgetResizable(true);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_scrollArea);
    
    connectSignals();
}

void PropertyPanel::createGeneralProperties() {
    m_generalGroup = new QGroupBox("General", this);
    QVBoxLayout* layout = new QVBoxLayout(m_generalGroup);
    
    // Name
    layout->addWidget(new QLabel("Name:"));
    m_nameEdit = new QLineEdit(this);
    layout->addWidget(m_nameEdit);
    
    // Type
    layout->addWidget(new QLabel("Type:"));
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("Unknown");
    m_typeCombo->addItem("Box");
    m_typeCombo->addItem("Cylinder");
    m_typeCombo->addItem("Sphere");
    m_typeCombo->addItem("Cone");
    m_typeCombo->addItem("Mesh");
    m_typeCombo->addItem("Assembly");
    m_typeCombo->setEnabled(false); // Type is read-only
    layout->addWidget(m_typeCombo);
    
    // Visibility
    m_visibilityCheck = new QCheckBox("Visible", this);
    layout->addWidget(m_visibilityCheck);
    
    m_mainLayout->addWidget(m_generalGroup);
}

void PropertyPanel::createTransformProperties() {
    m_transformGroup = new QGroupBox("Transform", this);
    QVBoxLayout* layout = new QVBoxLayout(m_transformGroup);
    
    // Position
    layout->addWidget(new QLabel("Position:"));
    QWidget* posWidget = new QWidget();
    QHBoxLayout* posLayout = new QHBoxLayout(posWidget);
    
    m_posXSpin = new QDoubleSpinBox(this);
    m_posXSpin->setRange(-1000.0, 1000.0);
    m_posXSpin->setPrefix("X: ");
    posLayout->addWidget(m_posXSpin);
    
    m_posYSpin = new QDoubleSpinBox(this);
    m_posYSpin->setRange(-1000.0, 1000.0);
    m_posYSpin->setPrefix("Y: ");
    posLayout->addWidget(m_posYSpin);
    
    m_posZSpin = new QDoubleSpinBox(this);
    m_posZSpin->setRange(-1000.0, 1000.0);
    m_posZSpin->setPrefix("Z: ");
    posLayout->addWidget(m_posZSpin);
    
    layout->addWidget(posWidget);
    
    // Rotation
    layout->addWidget(new QLabel("Rotation:"));
    QWidget* rotWidget = new QWidget();
    QHBoxLayout* rotLayout = new QHBoxLayout(rotWidget);
    
    m_rotXSpin = new QDoubleSpinBox(this);
    m_rotXSpin->setRange(-360.0, 360.0);
    m_rotXSpin->setPrefix("X: ");
    m_rotXSpin->setSuffix("°");
    rotLayout->addWidget(m_rotXSpin);
    
    m_rotYSpin = new QDoubleSpinBox(this);
    m_rotYSpin->setRange(-360.0, 360.0);
    m_rotYSpin->setPrefix("Y: ");
    m_rotYSpin->setSuffix("°");
    rotLayout->addWidget(m_rotYSpin);
    
    m_rotZSpin = new QDoubleSpinBox(this);
    m_rotZSpin->setRange(-360.0, 360.0);
    m_rotZSpin->setPrefix("Z: ");
    m_rotZSpin->setSuffix("°");
    rotLayout->addWidget(m_rotZSpin);
    
    layout->addWidget(rotWidget);
    
    // Scale
    layout->addWidget(new QLabel("Scale:"));
    QWidget* scaleWidget = new QWidget();
    QHBoxLayout* scaleLayout = new QHBoxLayout(scaleWidget);
    
    m_scaleXSpin = new QDoubleSpinBox(this);
    m_scaleXSpin->setRange(0.01, 100.0);
    m_scaleXSpin->setValue(1.0);
    m_scaleXSpin->setPrefix("X: ");
    scaleLayout->addWidget(m_scaleXSpin);
    
    m_scaleYSpin = new QDoubleSpinBox(this);
    m_scaleYSpin->setRange(0.01, 100.0);
    m_scaleYSpin->setValue(1.0);
    m_scaleYSpin->setPrefix("Y: ");
    scaleLayout->addWidget(m_scaleYSpin);
    
    m_scaleZSpin = new QDoubleSpinBox(this);
    m_scaleZSpin->setRange(0.01, 100.0);
    m_scaleZSpin->setValue(1.0);
    m_scaleZSpin->setPrefix("Z: ");
    scaleLayout->addWidget(m_scaleZSpin);
    
    layout->addWidget(scaleWidget);
    
    m_mainLayout->addWidget(m_transformGroup);
}

void PropertyPanel::createMaterialProperties() {
    m_materialGroup = new QGroupBox("Material", this);
    QVBoxLayout* layout = new QVBoxLayout(m_materialGroup);
    
    // Material name
    layout->addWidget(new QLabel("Name:"));
    m_materialNameEdit = new QLineEdit(this);
    layout->addWidget(m_materialNameEdit);
    
    // Colors
    QWidget* diffuseWidget = createColorButtonRow("Diffuse Color:", m_diffuseColorButton);
    layout->addWidget(diffuseWidget);
    
    QWidget* specularWidget = createColorButtonRow("Specular Color:", m_specularColorButton);
    layout->addWidget(specularWidget);
    
    // Shininess
    QWidget* shininessWidget = createSliderRow("Shininess:", m_shininessSlider, 1, 128, 32);
    layout->addWidget(shininessWidget);
    
    // Transparency
    QWidget* transparencyWidget = createSliderRow("Transparency:", m_transparencySlider, 0, 100, 0);
    layout->addWidget(transparencyWidget);
    
    m_mainLayout->addWidget(m_materialGroup);
}

void PropertyPanel::createGeometryProperties() {
    m_geometryGroup = new QGroupBox("Geometry", this);
    m_geometryLayout = new QVBoxLayout(m_geometryGroup);
    
    m_geometryWidget = new QWidget(this);
    m_geometryLayout->addWidget(m_geometryWidget);
    
    // Box properties
    QWidget* boxWidget = new QWidget();
    QVBoxLayout* boxLayout = new QVBoxLayout(boxWidget);
    
    QWidget* minWidget = createSpinBoxRow("Min:", m_boxMinXSpin);
    boxLayout->addWidget(minWidget);
    QWidget* minYWidget = createSpinBoxRow("", m_boxMinYSpin);
    boxLayout->addWidget(minYWidget);
    QWidget* minZWidget = createSpinBoxRow("", m_boxMinZSpin);
    boxLayout->addWidget(minZWidget);
    
    QWidget* maxWidget = createSpinBoxRow("Max:", m_boxMaxXSpin);
    boxLayout->addWidget(maxWidget);
    QWidget* maxYWidget = createSpinBoxRow("", m_boxMaxYSpin);
    boxLayout->addWidget(maxYWidget);
    QWidget* maxZWidget = createSpinBoxRow("", m_boxMaxZSpin);
    boxLayout->addWidget(maxZWidget);
    
    // Cylinder properties
    QWidget* cylinderWidget = new QWidget();
    QVBoxLayout* cylinderLayout = new QVBoxLayout(cylinderWidget);
    
    QWidget* radiusWidget = createSpinBoxRow("Radius:", m_cylinderRadiusSpin, 0.01, 1000.0);
    cylinderLayout->addWidget(radiusWidget);
    QWidget* heightWidget = createSpinBoxRow("Height:", m_cylinderHeightSpin, 0.01, 1000.0);
    cylinderLayout->addWidget(heightWidget);
    QWidget* segmentsWidget = createIntSpinBoxRow("Segments:", m_cylinderSegmentsSpin, 3, 128);
    cylinderLayout->addWidget(segmentsWidget);
    
    // Initially hide all geometry widgets
    boxWidget->hide();
    cylinderWidget->hide();
    
    m_geometryLayout->addWidget(boxWidget);
    m_geometryLayout->addWidget(cylinderWidget);
    
    m_mainLayout->addWidget(m_geometryGroup);
}

void PropertyPanel::updateGeneralProperties() {
    if (!m_currentObject) return;
    
    m_nameEdit->setText(QString::fromStdString(m_currentObject->getName()));
    m_visibilityCheck->setChecked(m_currentObject->isVisible());
    
    // Set type combo based on object type
    ObjectType type = m_currentObject->getType();
    switch (type) {
        case ObjectType::PRIMITIVE_BOX: m_typeCombo->setCurrentIndex(1); break;
        case ObjectType::PRIMITIVE_CYLINDER: m_typeCombo->setCurrentIndex(2); break;
        case ObjectType::PRIMITIVE_SPHERE: m_typeCombo->setCurrentIndex(3); break;
        case ObjectType::PRIMITIVE_CONE: m_typeCombo->setCurrentIndex(4); break;
        case ObjectType::MESH: m_typeCombo->setCurrentIndex(5); break;
        case ObjectType::ASSEMBLY: m_typeCombo->setCurrentIndex(6); break;
        default: m_typeCombo->setCurrentIndex(0); break;
    }
}

void PropertyPanel::updateTransformProperties() {
    if (!m_currentObject) return;
    
    // For now, set default transform values
    setSpinBoxValue(m_posXSpin, 0.0);
    setSpinBoxValue(m_posYSpin, 0.0);
    setSpinBoxValue(m_posZSpin, 0.0);
    
    setSpinBoxValue(m_rotXSpin, 0.0);
    setSpinBoxValue(m_rotYSpin, 0.0);
    setSpinBoxValue(m_rotZSpin, 0.0);
    
    setSpinBoxValue(m_scaleXSpin, 1.0);
    setSpinBoxValue(m_scaleYSpin, 1.0);
    setSpinBoxValue(m_scaleZSpin, 1.0);
}

void PropertyPanel::updateMaterialProperties() {
    if (!m_currentObject) return;
    
    const Material& material = m_currentObject->getMaterial();
    
    m_materialNameEdit->setText(QString::fromStdString(material.name));
    m_shininessSlider->setValue(static_cast<int>(material.shininess));
    m_transparencySlider->setValue(static_cast<int>(material.transparency * 100));
    
    // Update color buttons
    QPalette diffusePalette = m_diffuseColorButton->palette();
    diffusePalette.setColor(QPalette::Button, material.diffuseColor);
    m_diffuseColorButton->setPalette(diffusePalette);
    
    QPalette specularPalette = m_specularColorButton->palette();
    specularPalette.setColor(QPalette::Button, material.specularColor);
    m_specularColorButton->setPalette(specularPalette);
}

void PropertyPanel::updateGeometryProperties() {
    clearGeometryProperties();
    
    if (!m_currentObject) return;
    
    // Show appropriate geometry controls based on object type
    ObjectType type = m_currentObject->getType();
    
    // This is a simplified implementation
    // In a real application, you would cast to specific types and update their parameters
}

void PropertyPanel::clearGeometryProperties() {
    // Hide all geometry-specific widgets
    // This would be implemented to hide/show appropriate widgets
}

void PropertyPanel::connectSignals() {
    if (!m_currentObject) return;
    
    connect(m_nameEdit, &QLineEdit::textChanged, this, &PropertyPanel::onNameChanged);
    connect(m_visibilityCheck, &QCheckBox::toggled, this, &PropertyPanel::onVisibilityChanged);
    connect(m_diffuseColorButton, &QPushButton::clicked, this, &PropertyPanel::onColorChanged);
    connect(m_specularColorButton, &QPushButton::clicked, this, &PropertyPanel::onColorChanged);
    connect(m_applyButton, &QPushButton::clicked, this, &PropertyPanel::onApplyChanges);
    connect(m_resetButton, &QPushButton::clicked, this, &PropertyPanel::onResetChanges);
    
    // Connect transform controls
    connect(m_posXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PropertyPanel::onTransformChanged);
    connect(m_posYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PropertyPanel::onTransformChanged);
    connect(m_posZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PropertyPanel::onTransformChanged);
}

void PropertyPanel::disconnectSignals() {
    // Disconnect all signals to prevent unwanted updates
    disconnect(m_nameEdit, nullptr, this, nullptr);
    disconnect(m_visibilityCheck, nullptr, this, nullptr);
    disconnect(m_diffuseColorButton, nullptr, this, nullptr);
    disconnect(m_specularColorButton, nullptr, this, nullptr);
    disconnect(m_applyButton, nullptr, this, nullptr);
    disconnect(m_resetButton, nullptr, this, nullptr);
    
    // Disconnect transform controls
    disconnect(m_posXSpin, nullptr, this, nullptr);
    disconnect(m_posYSpin, nullptr, this, nullptr);
    disconnect(m_posZSpin, nullptr, this, nullptr);
}

QWidget* PropertyPanel::createSpinBoxRow(const QString& label, QDoubleSpinBox*& spinBox, 
                                        double min, double max, double step) {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    
    if (!label.isEmpty()) {
        layout->addWidget(new QLabel(label));
    }
    
    spinBox = new QDoubleSpinBox();
    spinBox->setRange(min, max);
    spinBox->setSingleStep(step);
    layout->addWidget(spinBox);
    
    return widget;
}

QWidget* PropertyPanel::createIntSpinBoxRow(const QString& label, QSpinBox*& spinBox, 
                                          int min, int max, int step) {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    
    if (!label.isEmpty()) {
        layout->addWidget(new QLabel(label));
    }
    
    spinBox = new QSpinBox();
    spinBox->setRange(min, max);
    spinBox->setSingleStep(step);
    layout->addWidget(spinBox);
    
    return widget;
}

QWidget* PropertyPanel::createSliderRow(const QString& label, QSlider*& slider, 
                                      int min, int max, int value) {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    
    layout->addWidget(new QLabel(label));
    
    slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setValue(value);
    layout->addWidget(slider);
    
    return widget;
}

QWidget* PropertyPanel::createColorButtonRow(const QString& label, QPushButton*& button) {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    
    layout->addWidget(new QLabel(label));
    
    button = new QPushButton();
    button->setMaximumWidth(50);
    button->setMaximumHeight(25);
    layout->addWidget(button);
    layout->addStretch();
    
    return widget;
}

void PropertyPanel::setSpinBoxValue(QDoubleSpinBox* spinBox, double value) {
    if (spinBox) {
        spinBox->setValue(value);
    }
}

void PropertyPanel::setIntSpinBoxValue(QSpinBox* spinBox, int value) {
    if (spinBox) {
        spinBox->setValue(value);
    }
}

} // namespace HybridCAD 