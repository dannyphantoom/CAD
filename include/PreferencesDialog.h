#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>

namespace HybridCAD {

class CADViewer;

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(CADViewer* cadViewer, QWidget *parent = nullptr);
    ~PreferencesDialog();

private slots:
    void onAccept();
    void onReject();
    void onMouseSensitivityChanged();
    void onCameraSpeedChanged();
    void onResetToDefaults();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    CADViewer* m_cadViewer;
    
    // UI elements
    QVBoxLayout* m_mainLayout;
    QTabWidget* m_tabWidget;
    
    // Camera/Mouse tab
    QWidget* m_cameraTab;
    QDoubleSpinBox* m_mouseSensitivitySpin;
    QSlider* m_mouseSensitivitySlider;
    QDoubleSpinBox* m_cameraSpeedSpin;
    QSlider* m_cameraSpeedSlider;
    
    // Buttons
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_resetButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    
    // Current values
    float m_mouseSensitivity;
    float m_cameraSpeed;
};

} // namespace HybridCAD 