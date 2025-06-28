#include "GeometryManager.h"
#include <cmath>
#include <GL/gl.h>

namespace HybridCAD {

// Box implementation
Box::Box(const Point3D& min, const Point3D& max) 
    : GeometryPrimitive("Box"), m_min(min), m_max(max) {
}

void Box::render() const {
    if (!m_visible) return;
    
    // Apply material color and transparency
    glColor4f(m_material.diffuseColor.redF(), 
              m_material.diffuseColor.greenF(), 
              m_material.diffuseColor.blueF(), 
              1.0f - m_material.transparency);
    
    glBegin(GL_QUADS);
    
    // Front face
    glVertex3f(m_min.x, m_min.y, m_max.z);
    glVertex3f(m_max.x, m_min.y, m_max.z);
    glVertex3f(m_max.x, m_max.y, m_max.z);
    glVertex3f(m_min.x, m_max.y, m_max.z);
    
    // Back face
    glVertex3f(m_min.x, m_min.y, m_min.z);
    glVertex3f(m_min.x, m_max.y, m_min.z);
    glVertex3f(m_max.x, m_max.y, m_min.z);
    glVertex3f(m_max.x, m_min.y, m_min.z);
    
    // Top face
    glVertex3f(m_min.x, m_max.y, m_min.z);
    glVertex3f(m_min.x, m_max.y, m_max.z);
    glVertex3f(m_max.x, m_max.y, m_max.z);
    glVertex3f(m_max.x, m_max.y, m_min.z);
    
    // Bottom face
    glVertex3f(m_min.x, m_min.y, m_min.z);
    glVertex3f(m_max.x, m_min.y, m_min.z);
    glVertex3f(m_max.x, m_min.y, m_max.z);
    glVertex3f(m_min.x, m_min.y, m_max.z);
    
    // Right face
    glVertex3f(m_max.x, m_min.y, m_min.z);
    glVertex3f(m_max.x, m_max.y, m_min.z);
    glVertex3f(m_max.x, m_max.y, m_max.z);
    glVertex3f(m_max.x, m_min.y, m_max.z);
    
    // Left face
    glVertex3f(m_min.x, m_min.y, m_min.z);
    glVertex3f(m_min.x, m_min.y, m_max.z);
    glVertex3f(m_min.x, m_max.y, m_max.z);
    glVertex3f(m_min.x, m_max.y, m_min.z);
    
    glEnd();
}

bool Box::intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const {
    // Basic box intersection test
    return (rayOrigin.x >= m_min.x && rayOrigin.x <= m_max.x &&
            rayOrigin.y >= m_min.y && rayOrigin.y <= m_max.y &&
            rayOrigin.z >= m_min.z && rayOrigin.z <= m_max.z);
}

void Box::generateMesh() {
    if (m_meshGenerated) return;
    
    m_vertices.clear();
    m_triangles.clear();
    
    // Generate vertices for a box
    m_vertices = {
        Point3D(m_min.x, m_min.y, m_min.z), // 0
        Point3D(m_max.x, m_min.y, m_min.z), // 1
        Point3D(m_max.x, m_max.y, m_min.z), // 2
        Point3D(m_min.x, m_max.y, m_min.z), // 3
        Point3D(m_min.x, m_min.y, m_max.z), // 4
        Point3D(m_max.x, m_min.y, m_max.z), // 5
        Point3D(m_max.x, m_max.y, m_max.z), // 6
        Point3D(m_min.x, m_max.y, m_max.z)  // 7
    };
    
    m_meshGenerated = true;
}

void Box::setDimensions(const Point3D& min, const Point3D& max) {
    m_min = min;
    m_max = max;
    m_meshGenerated = false;
}

// Cylinder implementation
Cylinder::Cylinder(float radius, float height, int segments) 
    : GeometryPrimitive("Cylinder"), m_radius(radius), m_height(height), m_segments(segments) {
}

void Cylinder::render() const {
    if (!m_visible) return;
    
    // Apply material color and transparency
    glColor4f(m_material.diffuseColor.redF(), 
              m_material.diffuseColor.greenF(), 
              m_material.diffuseColor.blueF(), 
              1.0f - m_material.transparency);
    
    // Basic cylinder rendering
    const float angleStep = 2.0f * M_PI / m_segments;
    
    glBegin(GL_QUADS);
    for (int i = 0; i < m_segments; ++i) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;
        
        float x1 = m_radius * cos(angle1);
        float z1 = m_radius * sin(angle1);
        float x2 = m_radius * cos(angle2);
        float z2 = m_radius * sin(angle2);
        
        // Side face
        glVertex3f(x1, -m_height/2, z1);
        glVertex3f(x2, -m_height/2, z2);
        glVertex3f(x2, m_height/2, z2);
        glVertex3f(x1, m_height/2, z1);
    }
    glEnd();
}

bool Cylinder::intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const {
    // Basic cylinder intersection test
    float dx = rayOrigin.x;
    float dz = rayOrigin.z;
    return (dx*dx + dz*dz <= m_radius*m_radius &&
            rayOrigin.y >= -m_height/2 && rayOrigin.y <= m_height/2);
}

void Cylinder::generateMesh() {
    if (m_meshGenerated) return;
    
    m_vertices.clear();
    m_triangles.clear();
    
    // Generate vertices for cylinder
    const float angleStep = 2.0f * M_PI / m_segments;
    
    for (int i = 0; i <= m_segments; ++i) {
        float angle = i * angleStep;
        float x = m_radius * cos(angle);
        float z = m_radius * sin(angle);
        
        m_vertices.push_back(Point3D(x, -m_height/2, z));
        m_vertices.push_back(Point3D(x, m_height/2, z));
    }
    
    m_meshGenerated = true;
}



void Cylinder::setParameters(float radius, float height, int segments) {
    m_radius = radius;
    m_height = height;
    m_segments = segments;
    m_meshGenerated = false;
}

// Sphere implementation
Sphere::Sphere(float radius, int segments) 
    : GeometryPrimitive("Sphere"), m_radius(radius), m_segments(segments), m_center(0, 0, 0) {
}

void Sphere::render() const {
    if (!m_visible) return;
    
    // Apply material color and transparency
    glColor4f(m_material.diffuseColor.redF(), 
              m_material.diffuseColor.greenF(), 
              m_material.diffuseColor.blueF(), 
              1.0f - m_material.transparency);
    
    // Basic sphere rendering using GL_QUAD_STRIP
    const float PI = 3.14159265359f;
    const int stacks = m_segments / 2;
    const int slices = m_segments;
    
    for (int i = 0; i < stacks; ++i) {
        float lat0 = PI * (-0.5f + (float)i / stacks);
        float z0 = sin(lat0);
        float zr0 = cos(lat0);
        
        float lat1 = PI * (-0.5f + (float)(i + 1) / stacks);
        float z1 = sin(lat1);
        float zr1 = cos(lat1);
        
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float lng = 2 * PI * (float)j / slices;
            float x = cos(lng);
            float y = sin(lng);
            
            glVertex3f(m_center.x + m_radius * x * zr0, m_center.y + m_radius * y * zr0, m_center.z + m_radius * z0);
            glVertex3f(m_center.x + m_radius * x * zr1, m_center.y + m_radius * y * zr1, m_center.z + m_radius * z1);
        }
        glEnd();
    }
}

bool Sphere::intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const {
    // Basic sphere intersection test relative to center
    float dx = rayOrigin.x - m_center.x;
    float dy = rayOrigin.y - m_center.y;
    float dz = rayOrigin.z - m_center.z;
    return (dx*dx + dy*dy + dz*dz <= m_radius*m_radius);
}

void Sphere::generateMesh() {
    if (m_meshGenerated) return;
    
    m_vertices.clear();
    m_triangles.clear();
    
    // Generate vertices for sphere
    const float PI = 3.14159265359f;
    const int stacks = m_segments / 2;
    const int slices = m_segments;
    
    for (int i = 0; i <= stacks; ++i) {
        float lat = PI * (-0.5f + (float)i / stacks);
        float z = sin(lat);
        float zr = cos(lat);
        
        for (int j = 0; j <= slices; ++j) {
            float lng = 2 * PI * (float)j / slices;
            float x = cos(lng);
            float y = sin(lng);
            
            m_vertices.push_back(Point3D(m_center.x + m_radius * x * zr, m_center.y + m_radius * y * zr, m_center.z + m_radius * z));
        }
    }
    
    m_meshGenerated = true;
}



void Sphere::setParameters(float radius, int segments) {
    m_radius = radius;
    m_segments = segments;
    m_meshGenerated = false;
}

// Cone implementation
Cone::Cone(float bottomRadius, float topRadius, float height, int segments) 
    : GeometryPrimitive("Cone"), m_bottomRadius(bottomRadius), m_topRadius(topRadius), 
      m_height(height), m_segments(segments) {
}

void Cone::render() const {
    if (!m_visible) return;
    
    // Apply material color and transparency
    glColor4f(m_material.diffuseColor.redF(), 
              m_material.diffuseColor.greenF(), 
              m_material.diffuseColor.blueF(), 
              1.0f - m_material.transparency);
    
    const float angleStep = 2.0f * M_PI / m_segments;
    
    glBegin(GL_QUADS);
    for (int i = 0; i < m_segments; ++i) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;
        
        float x1_bottom = m_bottomRadius * cos(angle1);
        float z1_bottom = m_bottomRadius * sin(angle1);
        float x2_bottom = m_bottomRadius * cos(angle2);
        float z2_bottom = m_bottomRadius * sin(angle2);
        
        float x1_top = m_topRadius * cos(angle1);
        float z1_top = m_topRadius * sin(angle1);
        float x2_top = m_topRadius * cos(angle2);
        float z2_top = m_topRadius * sin(angle2);
        
        // Side face
        glVertex3f(m_center.x + x1_bottom, m_center.y -m_height/2, m_center.z + z1_bottom);
        glVertex3f(m_center.x + x2_bottom, m_center.y -m_height/2, m_center.z + z2_bottom);
        glVertex3f(m_center.x + x2_top, m_center.y + m_height/2, m_center.z + z2_top);
        glVertex3f(m_center.x + x1_top, m_center.y + m_height/2, m_center.z + z1_top);
    }
    glEnd();
}

bool Cone::intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const {
    // Basic cone intersection test
    float maxRadius = std::max(m_bottomRadius, m_topRadius);
    float dx = rayOrigin.x - m_center.x;
    float dz = rayOrigin.z - m_center.z;
    return (dx*dx + dz*dz <= maxRadius*maxRadius &&
            rayOrigin.y >= m_center.y -m_height/2 && rayOrigin.y <= m_center.y + m_height/2);
}

void Cone::generateMesh() {
    if (m_meshGenerated) return;
    
    m_vertices.clear();
    m_triangles.clear();
    
    // Generate vertices for cone
    const float angleStep = 2.0f * M_PI / m_segments;
    
    for (int i = 0; i <= m_segments; ++i) {
        float angle = i * angleStep;
        float x_bottom = m_bottomRadius * cos(angle);
        float z_bottom = m_bottomRadius * sin(angle);
        float x_top = m_topRadius * cos(angle);
        float z_top = m_topRadius * sin(angle);
        
        m_vertices.push_back(Point3D(x_bottom, -m_height/2, z_bottom));
        m_vertices.push_back(Point3D(x_top, m_height/2, z_top));
    }
    
    m_meshGenerated = true;
}



void Cone::setParameters(float bottomRadius, float topRadius, float height, int segments) {
    m_bottomRadius = bottomRadius;
    m_topRadius = topRadius;
    m_height = height;
    m_segments = segments;
    m_meshGenerated = false;
}

// BooleanObject implementation
BooleanObject::BooleanObject(CADObjectPtr objectA, CADObjectPtr objectB, Operation op)
    : CADObject("Boolean"), m_objectA(objectA), m_objectB(objectB), m_operation(op) {
}

ObjectType BooleanObject::getType() const {
    switch (m_operation) {
        case UNION: return ObjectType::BOOLEAN_UNION;
        case DIFFERENCE: return ObjectType::BOOLEAN_DIFFERENCE;
        case INTERSECTION: return ObjectType::BOOLEAN_INTERSECTION;
    }
    return ObjectType::BOOLEAN_UNION;
}

void BooleanObject::render() const {
    if (!m_visible) return;
    
    // For now, just render both objects
    if (m_objectA) m_objectA->render();
    if (m_objectB && m_operation != DIFFERENCE) m_objectB->render();
}

bool BooleanObject::intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const {
    // Basic intersection test
    bool intersectsA = m_objectA ? m_objectA->intersects(rayOrigin, rayDirection) : false;
    bool intersectsB = m_objectB ? m_objectB->intersects(rayOrigin, rayDirection) : false;
    
    switch (m_operation) {
        case UNION: return intersectsA || intersectsB;
        case DIFFERENCE: return intersectsA && !intersectsB;
        case INTERSECTION: return intersectsA && intersectsB;
    }
    return false;
}

// GeometryManager implementation
GeometryManager::GeometryManager() : m_openCascadeInitialized(false) {
    initializeOpenCASCADE();
}

GeometryManager::~GeometryManager() {
}

std::shared_ptr<Box> GeometryManager::createBox(const Point3D& min, const Point3D& max) {
    return std::make_shared<Box>(min, max);
}

std::shared_ptr<Cylinder> GeometryManager::createCylinder(float radius, float height, int segments) {
    return std::make_shared<Cylinder>(radius, height, segments);
}

std::shared_ptr<Sphere> GeometryManager::createSphere(float radius, int segments) {
    return std::make_shared<Sphere>(radius, segments);
}

std::shared_ptr<Cone> GeometryManager::createCone(float bottomRadius, float topRadius, float height, int segments) {
    return std::make_shared<Cone>(bottomRadius, topRadius, height, segments);
}

std::shared_ptr<BooleanObject> GeometryManager::performUnion(CADObjectPtr objectA, CADObjectPtr objectB) {
    return std::make_shared<BooleanObject>(objectA, objectB, BooleanObject::UNION);
}

std::shared_ptr<BooleanObject> GeometryManager::performDifference(CADObjectPtr objectA, CADObjectPtr objectB) {
    return std::make_shared<BooleanObject>(objectA, objectB, BooleanObject::DIFFERENCE);
}

std::shared_ptr<BooleanObject> GeometryManager::performIntersection(CADObjectPtr objectA, CADObjectPtr objectB) {
    return std::make_shared<BooleanObject>(objectA, objectB, BooleanObject::INTERSECTION);
}

CADObjectPtr GeometryManager::extrudeProfile(const std::vector<Point3D>& profile, const Vector3D& direction, float distance) {
    // Placeholder implementation
    return nullptr;
}

CADObjectPtr GeometryManager::revolveProfile(const std::vector<Point3D>& profile, const Point3D& axisPoint, 
                                           const Vector3D& axisDirection, float angle) {
    // Placeholder implementation
    return nullptr;
}

void GeometryManager::calculateBoundingBox(CADObjectPtr object, Point3D& min, Point3D& max) {
    if (!object) return;
    
    // Basic bounding box calculation
    min = Point3D(-1, -1, -1);
    max = Point3D(1, 1, 1);
}

bool GeometryManager::rayIntersects(const Point3D& rayOrigin, const Vector3D& rayDirection, 
                                  CADObjectPtr object, float& distance) {
    if (!object) return false;
    
    bool intersects = object->intersects(rayOrigin, rayDirection);
    if (intersects) {
        distance = 1.0f; // Placeholder distance
    }
    return intersects;
}

void GeometryManager::generateMeshForObject(CADObjectPtr object) {
    // Placeholder implementation
}

void GeometryManager::initializeOpenCASCADE() {
    // Initialize OpenCASCADE if available
    m_openCascadeInitialized = false;
}

#ifdef HAVE_OPENCASCADE
TopoDS_Shape GeometryManager::convertToOpenCASCADE(CADObjectPtr object) {
    // Placeholder implementation
    return TopoDS_Shape();
}

CADObjectPtr GeometryManager::convertFromOpenCASCADE(const TopoDS_Shape& shape) {
    // Placeholder implementation
    return nullptr;
}
#endif

} // namespace HybridCAD 