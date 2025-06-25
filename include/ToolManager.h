#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QtCore/QTimer>
#include <memory>

#include "CADTypes.h"

namespace HybridCAD {

enum class ToolType {
    SELECT,
    MOVE,
    ROTATE,
    SCALE,
    EXTRUDE,
    SKETCH,
    MEASURE,
    SECTION,
    PLACE_SHAPE,
    EXTRUDE_2D,
    ERASER
};

enum class SnapMode {
    NONE,
    GRID,
    VERTEX,
    EDGE,
    FACE,
    CENTER,
    MIDPOINT
};

class ToolManager : public QWidget
{
    Q_OBJECT

public:
    explicit ToolManager(QWidget *parent = nullptr);
    ~ToolManager();
    
    void setActiveTool(ToolType tool);
    ToolType getActiveTool() const { return m_activeTool; }
    
    void setSnapMode(SnapMode mode);
    SnapMode getSnapMode() const { return m_snapMode; }
    
    void setGridSize(float size);
    float getGridSize() const { return m_gridSize; }
    
    void setAngleSnap(bool enabled);
    bool isAngleSnapEnabled() const { return m_angleSnap; }
    
    void setAngleSnapIncrement(float degrees);
    float getAngleSnapIncrement() const { return m_angleSnapIncrement; }
    
    // Grid plane control
    void setGridPlane(int plane); // 0=XY, 1=XZ, 2=YZ
    int getGridPlane() const { return m_gridPlane; }

signals:
    void toolChanged(ToolType tool);
    void snapModeChanged(SnapMode mode);
    void gridSizeChanged(float size);
    void angleSnapChanged(bool enabled);
    void angleSnapIncrementChanged(float degrees);
    void gridPlaneChanged(int plane);
    void parametersChanged();

private slots:
    void onToolButtonClicked();
    void onSnapModeChanged();
    void onGridSizeChanged();
    void onAngleSnapToggled(bool enabled);
    void onAngleSnapIncrementChanged();
    void onGridPlaneChanged();
    void onToolParametersChanged();

private:
    void setupUI();
    void createToolButtons();
    void createSnapControls();
    void createGridControls();
    void createAngleControls();
    void createToolParameters();
    void updateToolParameters();
    
    // UI components
    QVBoxLayout* m_mainLayout;
    
    // Tool selection
    QGroupBox* m_toolGroup;
    QVBoxLayout* m_toolLayout;
    QButtonGroup* m_toolButtonGroup;
    QToolButton* m_selectButton;
    QToolButton* m_moveButton;
    QToolButton* m_rotateButton;
    QToolButton* m_scaleButton;
    QToolButton* m_extrudeButton;
    QToolButton* m_sketchButton;
    QToolButton* m_measureButton;
    QToolButton* m_sectionButton;
    
    // Snap controls
    QGroupBox* m_snapGroup;
    QVBoxLayout* m_snapLayout;
    QButtonGroup* m_snapButtonGroup;
    QToolButton* m_snapNoneButton;
    QToolButton* m_snapGridButton;
    QToolButton* m_snapVertexButton;
    QToolButton* m_snapEdgeButton;
    QToolButton* m_snapFaceButton;
    QToolButton* m_snapCenterButton;
    QToolButton* m_snapMidpointButton;
    
    // Grid controls
    QGroupBox* m_gridGroup;
    QVBoxLayout* m_gridLayout;
    QLabel* m_gridSizeLabel;
    QDoubleSpinBox* m_gridSizeSpin;
    QLabel* m_gridPlaneLabel;
    QComboBox* m_gridPlaneCombo;
    
    // Angle controls
    QGroupBox* m_angleGroup;
    QVBoxLayout* m_angleLayout;
    QCheckBox* m_angleSnapCheck;
    QLabel* m_angleIncrementLabel;
    QDoubleSpinBox* m_angleIncrementSpin;
    
    // Tool parameters
    QGroupBox* m_parametersGroup;
    QVBoxLayout* m_parametersLayout;
    QWidget* m_parametersWidget;
    
    // Move tool parameters
    QWidget* m_moveParametersWidget;
    QDoubleSpinBox* m_moveStepSpin;
    QCheckBox* m_moveConstraintXCheck;
    QCheckBox* m_moveConstraintYCheck;
    QCheckBox* m_moveConstraintZCheck;
    
    // Rotate tool parameters
    QWidget* m_rotateParametersWidget;
    QDoubleSpinBox* m_rotateAngleSpin;
    QCheckBox* m_rotateConstraintXCheck;
    QCheckBox* m_rotateConstraintYCheck;
    QCheckBox* m_rotateConstraintZCheck;
    
    // Scale tool parameters
    QWidget* m_scaleParametersWidget;
    QDoubleSpinBox* m_scaleFactorSpin;
    QCheckBox* m_scaleUniformCheck;
    QCheckBox* m_scaleConstraintXCheck;
    QCheckBox* m_scaleConstraintYCheck;
    QCheckBox* m_scaleConstraintZCheck;
    
    // Extrude tool parameters
    QWidget* m_extrudeParametersWidget;
    QDoubleSpinBox* m_extrudeDistanceSpin;
    QCheckBox* m_extrudeBothDirectionsCheck;
    QSlider* m_extrudeTaperSlider;
    
    // Current state
    ToolType m_activeTool;
    SnapMode m_snapMode;
    float m_gridSize;
    bool m_angleSnap;
    float m_angleSnapIncrement;
    int m_gridPlane;
    
    // Helper methods
    void setToolButtonIcon(QToolButton* button, ToolType tool);
    void setSnapButtonIcon(QToolButton* button, SnapMode mode);
    QString getToolName(ToolType tool) const;
    QString getSnapModeName(SnapMode mode) const;
    void showParametersWidget(QWidget* widget);
    void hideAllParametersWidgets();
};

} // namespace HybridCAD 