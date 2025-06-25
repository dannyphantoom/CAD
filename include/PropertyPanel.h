#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QSlider>
#include <QColorDialog>
#include <QScrollArea>
#include <memory>

#include "CADTypes.h"

namespace HybridCAD {

class PropertyPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyPanel(QWidget *parent = nullptr);
    ~PropertyPanel();
    
    void setSelectedObject(CADObjectPtr object);
    void clearSelection();
    void updateProperties();

signals:
    void propertyChanged();
    void objectModified(CADObjectPtr object);

private slots:
    void onNameChanged();
    void onVisibilityChanged(bool visible);
    void onMaterialChanged();
    void onTransformChanged();
    void onGeometryParameterChanged();
    void onColorChanged();
    void onApplyChanges();
    void onResetChanges();

private:
    void setupUI();
    void createGeneralProperties();
    void createTransformProperties();
    void createMaterialProperties();
    void createGeometryProperties();
    void updateGeneralProperties();
    void updateTransformProperties();
    void updateMaterialProperties();
    void updateGeometryProperties();
    void clearGeometryProperties();
    
    // Property widgets
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_mainLayout;
    
    // General properties group
    QGroupBox* m_generalGroup;
    QLineEdit* m_nameEdit;
    QCheckBox* m_visibilityCheck;
    QComboBox* m_typeCombo;
    
    // Transform properties group
    QGroupBox* m_transformGroup;
    QDoubleSpinBox* m_posXSpin;
    QDoubleSpinBox* m_posYSpin;
    QDoubleSpinBox* m_posZSpin;
    QDoubleSpinBox* m_rotXSpin;
    QDoubleSpinBox* m_rotYSpin;
    QDoubleSpinBox* m_rotZSpin;
    QDoubleSpinBox* m_scaleXSpin;
    QDoubleSpinBox* m_scaleYSpin;
    QDoubleSpinBox* m_scaleZSpin;
    
    // Material properties group
    QGroupBox* m_materialGroup;
    QPushButton* m_diffuseColorButton;
    QPushButton* m_specularColorButton;
    QSlider* m_shininessSlider;
    QSlider* m_transparencySlider;
    QLineEdit* m_materialNameEdit;
    
    // Geometry properties group
    QGroupBox* m_geometryGroup;
    QWidget* m_geometryWidget;
    QVBoxLayout* m_geometryLayout;
    
    // Box-specific properties
    QDoubleSpinBox* m_boxMinXSpin;
    QDoubleSpinBox* m_boxMinYSpin;
    QDoubleSpinBox* m_boxMinZSpin;
    QDoubleSpinBox* m_boxMaxXSpin;
    QDoubleSpinBox* m_boxMaxYSpin;
    QDoubleSpinBox* m_boxMaxZSpin;
    
    // Cylinder-specific properties
    QDoubleSpinBox* m_cylinderRadiusSpin;
    QDoubleSpinBox* m_cylinderHeightSpin;
    QSpinBox* m_cylinderSegmentsSpin;
    
    // Sphere-specific properties
    QDoubleSpinBox* m_sphereRadiusSpin;
    QSpinBox* m_sphereSegmentsSpin;
    
    // Cone-specific properties
    QDoubleSpinBox* m_coneBottomRadiusSpin;
    QDoubleSpinBox* m_coneTopRadiusSpin;
    QDoubleSpinBox* m_coneHeightSpin;
    QSpinBox* m_coneSegmentsSpin;
    
    // Action buttons
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_applyButton;
    QPushButton* m_resetButton;
    
    // Current object
    CADObjectPtr m_currentObject;
    bool m_updating;
    
    // Helper methods
    void connectSignals();
    void disconnectSignals();
    QWidget* createSpinBoxRow(const QString& label, QDoubleSpinBox*& spinBox, 
                             double min = -1000.0, double max = 1000.0, double step = 0.1);
    QWidget* createIntSpinBoxRow(const QString& label, QSpinBox*& spinBox, 
                                int min = 1, int max = 1000, int step = 1);
    QWidget* createSliderRow(const QString& label, QSlider*& slider, 
                            int min = 0, int max = 100, int value = 50);
    QWidget* createColorButtonRow(const QString& label, QPushButton*& button);
    
    void setSpinBoxValue(QDoubleSpinBox* spinBox, double value);
    void setIntSpinBoxValue(QSpinBox* spinBox, int value);
};

} // namespace HybridCAD 