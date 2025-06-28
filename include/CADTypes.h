#pragma once

#include <vector>
#include <memory>
#include <string>
#include <QVector3D>
#include <QMatrix4x4>
#include <QColor>

namespace HybridCAD {

// Forward declarations
class CADObject;
class GeometryPrimitive;
class MeshObject;

// Common geometric types
struct Point3D {
    double x, y, z;
    Point3D(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    QVector3D toQVector3D() const { return QVector3D(x, y, z); }
};

struct Vector3D {
    double x, y, z;
    Vector3D(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    QVector3D toQVector3D() const { return QVector3D(x, y, z); }
};

struct Triangle {
    Point3D v0, v1, v2;
    Vector3D normal;
};

struct Face {
    std::vector<int> vertexIndices;
    Vector3D normal;
};

// CAD Object types
enum class ObjectType {
    PRIMITIVE_BOX,
    PRIMITIVE_CYLINDER,
    PRIMITIVE_SPHERE,
    PRIMITIVE_CONE,
    // 2D shapes
    PRIMITIVE_LINE, // New Line primitive
    PRIMITIVE_RECTANGLE,
    PRIMITIVE_CIRCLE,
    PRIMITIVE_POLYGON,
    SKETCH,
    EXTRUSION,
    REVOLUTION,
    BOOLEAN_UNION,
    BOOLEAN_DIFFERENCE,
    BOOLEAN_INTERSECTION,
    MESH,
    ASSEMBLY
};

// Mesh editing operations
enum class MeshOperation {
    EXTRUDE_FACE,
    INSET_FACE,
    SUBDIVIDE,
    SMOOTH,
    DECIMATE,
    BOOLEAN_MESH
};

// Material properties
struct Material {
    QColor diffuseColor;
    QColor specularColor;
    float shininess;
    float transparency;
    std::string name;
    
    Material() : 
        diffuseColor(128, 128, 128),
        specularColor(255, 255, 255),
        shininess(32.0f),
        transparency(0.0f),
        name("Default") {}
};

// Base class for all CAD objects
class CADObject {
public:
    CADObject(const std::string& name = "Object", CADObject* parent = nullptr) 
        : m_name(name), m_visible(true), m_selected(false), m_parent(parent) {}
    virtual ~CADObject() = default;

    CADObject* getParent() const { return m_parent; }
    void setParent(CADObject* parent) { m_parent = parent; }
    
    virtual ObjectType getType() const = 0;
    virtual void render() const = 0;
    virtual bool intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const = 0;
    virtual Point3D getBoundingBoxMin() const = 0;
    virtual Point3D getBoundingBoxMax() const = 0;
    
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    
    bool isSelected() const { return m_selected; }
    void setSelected(bool selected) { m_selected = selected; }
    
    const Material& getMaterial() const { return m_material; }
    void setMaterial(const Material& material) { m_material = material; }

protected:
    std::string m_name;
    bool m_visible;
    bool m_selected;
    Material m_material;
    CADObject* m_parent;
};

using CADObjectPtr = std::shared_ptr<CADObject>;
using CADObjectList = std::vector<CADObjectPtr>;

// Transformation matrix
struct Transform {
    QMatrix4x4 matrix;
    
    Transform() { matrix.setToIdentity(); }
    
    void translate(const Vector3D& translation) {
        matrix.translate(translation.toQVector3D());
    }
    
    void rotate(float angle, const Vector3D& axis) {
        matrix.rotate(angle, axis.toQVector3D());
    }
    
    void scale(const Vector3D& scaling) {
        matrix.scale(scaling.toQVector3D());
    }
};

// View settings
struct ViewSettings {
    bool showGrid = true;
    bool showAxes = true;
    bool showBoundingBoxes = false;
    bool wireframeMode = false;
    QColor backgroundColor = QColor(64, 64, 64);
    float gridSize = 1.0f;
    int gridDivisions = 10;
};

} // namespace HybridCAD 