#pragma once

#include <memory>
#include <vector>
#include <string>
#include "CADTypes.h"

#ifdef HAVE_OPENCASCADE
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#endif

namespace HybridCAD {

// Primitive geometry classes
class GeometryPrimitive : public CADObject {
public:
    GeometryPrimitive(const std::string& name = "Primitive") : CADObject(name) {}
    virtual ~GeometryPrimitive() = default;
    
    virtual void generateMesh() = 0;
    virtual Point3D getBoundingBoxMin() const = 0;
    virtual Point3D getBoundingBoxMax() const = 0;
    
    const std::vector<Point3D>& getVertices() const { return m_vertices; }
    const std::vector<Triangle>& getTriangles() const { return m_triangles; }

protected:
    std::vector<Point3D> m_vertices;
    std::vector<Triangle> m_triangles;
    bool m_meshGenerated = false;
};

class Box : public GeometryPrimitive {
public:
    Box(const Point3D& min = Point3D(-0.5, -0.5, -0.5), 
        const Point3D& max = Point3D(0.5, 0.5, 0.5));
    
    ObjectType getType() const override { return ObjectType::PRIMITIVE_BOX; }
    void render() const override;
    bool intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const override;
    void generateMesh() override;
    Point3D getBoundingBoxMin() const override { return m_min; }
    Point3D getBoundingBoxMax() const override { return m_max; }
    
    void setDimensions(const Point3D& min, const Point3D& max);
    Point3D getMin() const { return m_min; }
    Point3D getMax() const { return m_max; }

private:
    Point3D m_min, m_max;
};

class Cylinder : public GeometryPrimitive {
public:
    Cylinder(float radius = 0.5f, float height = 1.0f, int segments = 32);
    
    ObjectType getType() const override { return ObjectType::PRIMITIVE_CYLINDER; }
    void render() const override;
    bool intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const override;
    void generateMesh() override;
    Point3D getBoundingBoxMin() const override;
    Point3D getBoundingBoxMax() const override;
    
    void setParameters(float radius, float height, int segments);
    float getRadius() const { return m_radius; }
    float getHeight() const { return m_height; }
    int getSegments() const { return m_segments; }

private:
    float m_radius;
    float m_height;
    int m_segments;
};

class Sphere : public GeometryPrimitive {
public:
    Sphere(float radius = 0.5f, int segments = 32);
    
    ObjectType getType() const override { return ObjectType::PRIMITIVE_SPHERE; }
    void render() const override;
    bool intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const override;
    void generateMesh() override;
    Point3D getBoundingBoxMin() const override;
    Point3D getBoundingBoxMax() const override;
    
    void setParameters(float radius, int segments);
    float getRadius() const { return m_radius; }
    int getSegments() const { return m_segments; }

private:
    float m_radius;
    int m_segments;
};

class Cone : public GeometryPrimitive {
public:
    Cone(float bottomRadius = 0.5f, float topRadius = 0.0f, float height = 1.0f, int segments = 32);
    
    ObjectType getType() const override { return ObjectType::PRIMITIVE_CONE; }
    void render() const override;
    bool intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const override;
    void generateMesh() override;
    Point3D getBoundingBoxMin() const override;
    Point3D getBoundingBoxMax() const override;
    
    void setParameters(float bottomRadius, float topRadius, float height, int segments);
    float getBottomRadius() const { return m_bottomRadius; }
    float getTopRadius() const { return m_topRadius; }
    float getHeight() const { return m_height; }
    int getSegments() const { return m_segments; }

private:
    float m_bottomRadius;
    float m_topRadius;
    float m_height;
    int m_segments;
};

// Boolean operation result
class BooleanObject : public CADObject {
public:
    enum Operation { UNION, DIFFERENCE, INTERSECTION };
    
    BooleanObject(CADObjectPtr objectA, CADObjectPtr objectB, Operation op);
    
    ObjectType getType() const override;
    void render() const override;
    bool intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const override;
    
    Operation getOperation() const { return m_operation; }
    CADObjectPtr getObjectA() const { return m_objectA; }
    CADObjectPtr getObjectB() const { return m_objectB; }

private:
    CADObjectPtr m_objectA;
    CADObjectPtr m_objectB;
    Operation m_operation;
    CADObjectPtr m_result;
};

// Geometry manager class
class GeometryManager {
public:
    GeometryManager();
    ~GeometryManager();
    
    // Primitive creation
    std::shared_ptr<Box> createBox(const Point3D& min, const Point3D& max);
    std::shared_ptr<Cylinder> createCylinder(float radius, float height, int segments = 32);
    std::shared_ptr<Sphere> createSphere(float radius, int segments = 32);
    std::shared_ptr<Cone> createCone(float bottomRadius, float topRadius, float height, int segments = 32);
    
    // Boolean operations
    std::shared_ptr<BooleanObject> performUnion(CADObjectPtr objectA, CADObjectPtr objectB);
    std::shared_ptr<BooleanObject> performDifference(CADObjectPtr objectA, CADObjectPtr objectB);
    std::shared_ptr<BooleanObject> performIntersection(CADObjectPtr objectA, CADObjectPtr objectB);
    
    // Advanced operations (requires OpenCASCADE)
    CADObjectPtr extrudeProfile(const std::vector<Point3D>& profile, const Vector3D& direction, float distance);
    CADObjectPtr revolveProfile(const std::vector<Point3D>& profile, const Point3D& axisPoint, 
                               const Vector3D& axisDirection, float angle);
    
    // Utility functions
    void calculateBoundingBox(CADObjectPtr object, Point3D& min, Point3D& max);
    bool rayIntersects(const Point3D& rayOrigin, const Vector3D& rayDirection, 
                      CADObjectPtr object, float& distance);
    
    // Mesh generation
    void generateMeshForObject(CADObjectPtr object);
    
private:
    void initializeOpenCASCADE();
    
#ifdef HAVE_OPENCASCADE
    TopoDS_Shape convertToOpenCASCADE(CADObjectPtr object);
    CADObjectPtr convertFromOpenCASCADE(const TopoDS_Shape& shape);
#endif
    
    bool m_openCascadeInitialized;
};

} // namespace HybridCAD 