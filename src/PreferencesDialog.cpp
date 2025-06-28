#include "PreferencesDialog.h"
#include "CADViewer.h"
#include <QSettings>

namespace HybridCAD {

PreferencesDialog::PreferencesDialog(CADViewer* cadViewer, QWidget *parent)
    : QDialog(parent)
    , m_cadViewer(cadViewer)
    , m_mouseSensitivity(1.0f)
    , m_cameraSpeed(5.0f)
{
    setWindowTitle("Preferences");
    setModal(true);
    resize(450, 300);
    
    setupUI();
    loadSettings();
}

PreferencesDialog::~PreferencesDialog() = default;

void PreferencesDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    // Camera/Mouse settings tab
    m_cameraTab = new QWidget();
    QGridLayout* cameraLayout = new QGridLayout(m_cameraTab);
    
    // Mouse sensitivity group
    QGroupBox* mouseGroup = new QGroupBox("Mouse Settings", this);
    QGridLayout* mouseLayout = new QGridLayout(mouseGroup);
    
    // Mouse sensitivity slider and spinbox
    mouseLayout->addWidget(new QLabel("Mouse Sensitivity:"), 0, 0);
    
    m_mouseSensitivitySlider = new QSlider(Qt::Horizontal, this);
    m_mouseSensitivitySlider->setRange(10, 500); // 0.1 to 5.0 (multiplied by 100)
    m_mouseSensitivitySlider->setValue(100); // Default 1.0
    mouseLayout->addWidget(m_mouseSensitivitySlider, 0, 1);
    
    m_mouseSensitivitySpin = new QDoubleSpinBox(this);
    m_mouseSensitivitySpin->setRange(0.1, 5.0);
    m_mouseSensitivitySpin->setSingleStep(0.1);
    m_mouseSensitivitySpin->setDecimals(1);
    m_mouseSensitivitySpin->setValue(1.0);
    mouseLayout->addWidget(m_mouseSensitivitySpin, 0, 2);
    
    cameraLayout->addWidget(mouseGroup, 0, 0, 1, 2);
    
    // Camera speed group
    QGroupBox* cameraGroup = new QGroupBox("Camera Settings", this);
    QGridLayout* cameraSpeedLayout = new QGridLayout(cameraGroup);
    
    // Camera speed slider and spinbox
    cameraSpeedLayout->addWidget(new QLabel("Camera Speed:"), 0, 0);
    
    m_cameraSpeedSlider = new QSlider(Qt::Horizontal, this);
    m_cameraSpeedSlider->setRange(10, 200); // 1.0 to 20.0 (multiplied by 10)
    m_cameraSpeedSlider->setValue(50); // Default 5.0
    cameraSpeedLayout->addWidget(m_cameraSpeedSlider, 0, 1);
    
    m_cameraSpeedSpin = new QDoubleSpinBox(this);
    m_cameraSpeedSpin->setRange(1.0, 20.0);
    m_cameraSpeedSpin->setSingleStep(0.5);
    m_cameraSpeedSpin->setDecimals(1);
    m_cameraSpeedSpin->setValue(5.0);
    cameraSpeedLayout->addWidget(m_cameraSpeedSpin, 0, 2);
    
    cameraLayout->addWidget(cameraGroup, 1, 0, 1, 2);
    
    // Add some spacing
    cameraLayout->setRowStretch(2, 1);
    
    m_tabWidget->addTab(m_cameraTab, "Camera & Mouse");
    m_mainLayout->addWidget(m_tabWidget);
    
    // Button layout
    m_buttonLayout = new QHBoxLayout();
    
    m_resetButton = new QPushButton("Reset to Defaults", this);
    m_okButton = new QPushButton("OK", this);
    m_cancelButton = new QPushButton("Cancel", this);
    
    m_buttonLayout->addWidget(m_resetButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_okButton);
    m_buttonLayout->addWidget(m_cancelButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_mouseSensitivitySlider, &QSlider::valueChanged, this, [this](int value) {
        float sensitivity = value / 100.0f;
        m_mouseSensitivitySpin->setValue(sensitivity);
        m_mouseSensitivity = sensitivity;
    });
    
    connect(m_mouseSensitivitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        m_mouseSensitivitySlider->setValue(static_cast<int>(value * 100));
        m_mouseSensitivity = static_cast<float>(value);
    });
    
    connect(m_cameraSpeedSlider, &QSlider::valueChanged, this, [this](int value) {
        float speed = value / 10.0f;
        m_cameraSpeedSpin->setValue(speed);
        m_cameraSpeed = speed;
    });
    
    connect(m_cameraSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        m_cameraSpeedSlider->setValue(static_cast<int>(value * 10));
        m_cameraSpeed = static_cast<float>(value);
    });
    
    connect(m_resetButton, &QPushButton::clicked, this, &PreferencesDialog::onResetToDefaults);
    connect(m_okButton, &QPushButton::clicked, this, &PreferencesDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &PreferencesDialog::onReject);
}

void PreferencesDialog::loadSettings()
{
    if (m_cadViewer) {
        m_mouseSensitivity = m_cadViewer->getMouseSensitivity();
        m_cameraSpeed = m_cadViewer->getCameraSpeed();
        
        m_mouseSensitivitySpin->setValue(m_mouseSensitivity);
        m_mouseSensitivitySlider->setValue(static_cast<int>(m_mouseSensitivity * 100));
        
        m_cameraSpeedSpin->setValue(m_cameraSpeed);
        m_cameraSpeedSlider->setValue(static_cast<int>(m_cameraSpeed * 10));
    }
}

void PreferencesDialog::saveSettings()
{
    if (m_cadViewer) {
        m_cadViewer->setMouseSensitivity(m_mouseSensitivity);
        m_cadViewer->setCameraSpeed(m_cameraSpeed);
    }
    
    // Save to QSettings for persistence
    QSettings settings;
    settings.beginGroup("Preferences");
    settings.setValue("mouseSensitivity", m_mouseSensitivity);
    settings.setValue("cameraSpeed", m_cameraSpeed);
    settings.endGroup();
    settings.sync();
}

void PreferencesDialog::onAccept()
{
    saveSettings();
    accept();
}

void PreferencesDialog::onReject()
{
    reject();
}

void PreferencesDialog::onMouseSensitivityChanged()
{
    // This is handled by lambda connections
}

void PreferencesDialog::onCameraSpeedChanged()
{
    // This is handled by lambda connections
}

void PreferencesDialog::onResetToDefaults()
{
    m_mouseSensitivity = 1.0f;
    m_cameraSpeed = 5.0f;
    
    m_mouseSensitivitySpin->setValue(m_mouseSensitivity);
    m_mouseSensitivitySlider->setValue(100);
    
    m_cameraSpeedSpin->setValue(m_cameraSpeed);
    m_cameraSpeedSlider->setValue(50);
}

} // namespace HybridCAD 