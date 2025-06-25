#include "MeshManager.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>

namespace HybridCAD {

// MeshObject implementation
MeshObject::MeshObject(const std::string& name) 
    : CADObject(name), m_nextVertexId(0), m_nextEdgeId(0), m_nextFaceId(0) {
}

void MeshObject::render() const {
    if (!m_visible) return;
    
    glBegin(GL_TRIANGLES);
    for (const auto& face : m_faces) {
        if (face.vertices.size() >= 3) {
            for (size_t i = 1; i < face.vertices.size() - 1; ++i) {
                const auto& v0 = m_vertices[face.vertices[0]];
                const auto& v1 = m_vertices[face.vertices[i]];
                const auto& v2 = m_vertices[face.vertices[i + 1]];
                
                glNormal3f(face.normal.x, face.normal.y, face.normal.z);
                glVertex3f(v0.position.x, v0.position.y, v0.position.z);
                glVertex3f(v1.position.x, v1.position.y, v1.position.z);
                glVertex3f(v2.position.x, v2.position.y, v2.position.z);
            }
        }
    }
    glEnd();
}

bool MeshObject::intersects(const Point3D& rayOrigin, const Vector3D& rayDirection) const {
    // Basic mesh intersection test - check against bounding box
    Point3D min = getBoundingBoxMin();
    Point3D max = getBoundingBoxMax();
    
    return (rayOrigin.x >= min.x && rayOrigin.x <= max.x &&
            rayOrigin.y >= min.y && rayOrigin.y <= max.y &&
            rayOrigin.z >= min.z && rayOrigin.z <= max.z);
}

void MeshObject::createFromTriangles(const std::vector<Triangle>& triangles) {
    m_vertices.clear();
    m_edges.clear();
    m_faces.clear();
    
    int vertexId = 0;
    int faceId = 0;
    
    for (const auto& triangle : triangles) {
        std::vector<int> faceVertices;
        
        // Add vertices
        m_vertices.emplace_back(triangle.v0, vertexId++);
        faceVertices.push_back(vertexId - 1);
        
        m_vertices.emplace_back(triangle.v1, vertexId++);
        faceVertices.push_back(vertexId - 1);
        
        m_vertices.emplace_back(triangle.v2, vertexId++);
        faceVertices.push_back(vertexId - 1);
        
        // Add face
        MeshFace face(faceVertices, faceId++);
        face.normal = triangle.normal;
        m_faces.push_back(face);
    }
    
    buildTopology();
}

void MeshObject::createFromGeometry(const std::vector<Point3D>& vertices, const std::vector<Face>& faces) {
    m_vertices.clear();
    m_edges.clear();
    m_faces.clear();
    
    int vertexId = 0;
    for (const auto& vertex : vertices) {
        m_vertices.emplace_back(vertex, vertexId++);
    }
    
    int faceId = 0;
    for (const auto& face : faces) {
        MeshFace meshFace(face.vertexIndices, faceId++);
        meshFace.normal = face.normal;
        m_faces.push_back(meshFace);
    }
    
    buildTopology();
}

void MeshObject::selectVertex(int vertexId, bool addToSelection) {
    if (!addToSelection) {
        deselectAll();
    }
    
    m_selectedVertices.insert(vertexId);
    if (vertexId < m_vertices.size()) {
        m_vertices[vertexId].selected = true;
    }
}

void MeshObject::selectEdge(int edgeId, bool addToSelection) {
    if (!addToSelection) {
        deselectAll();
    }
    
    m_selectedEdges.insert(edgeId);
    if (edgeId < m_edges.size()) {
        m_edges[edgeId].selected = true;
    }
}

void MeshObject::selectFace(int faceId, bool addToSelection) {
    if (!addToSelection) {
        deselectAll();
    }
    
    m_selectedFaces.insert(faceId);
    if (faceId < m_faces.size()) {
        m_faces[faceId].selected = true;
    }
}

void MeshObject::deselectAll() {
    m_selectedVertices.clear();
    m_selectedEdges.clear();
    m_selectedFaces.clear();
    
    for (auto& vertex : m_vertices) {
        vertex.selected = false;
    }
    for (auto& edge : m_edges) {
        edge.selected = false;
    }
    for (auto& face : m_faces) {
        face.selected = false;
    }
}

bool MeshObject::isValid() const {
    // Basic validation - check that all face vertices exist
    for (const auto& face : m_faces) {
        for (int vertexId : face.vertices) {
            if (vertexId < 0 || vertexId >= m_vertices.size()) {
                return false;
            }
        }
    }
    return true;
}

void MeshObject::recalculateNormals() {
    // Calculate face normals
    for (auto& face : m_faces) {
        if (face.vertices.size() >= 3) {
            const auto& v0 = m_vertices[face.vertices[0]].position;
            const auto& v1 = m_vertices[face.vertices[1]].position;
            const auto& v2 = m_vertices[face.vertices[2]].position;
            
            Vector3D edge1(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
            Vector3D edge2(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
            
            // Cross product
            face.normal.x = edge1.y * edge2.z - edge1.z * edge2.y;
            face.normal.y = edge1.z * edge2.x - edge1.x * edge2.z;
            face.normal.z = edge1.x * edge2.y - edge1.y * edge2.x;
            
            // Normalize
            float length = sqrt(face.normal.x * face.normal.x + 
                              face.normal.y * face.normal.y + 
                              face.normal.z * face.normal.z);
            if (length > 0.0f) {
                face.normal.x /= length;
                face.normal.y /= length;
                face.normal.z /= length;
            }
        }
    }
    
    updateNormals();
}

void MeshObject::removeDuplicateVertices(float tolerance) {
    // Placeholder implementation
}

void MeshObject::removeUnusedVertices() {
    // Placeholder implementation
}

Point3D MeshObject::getBoundingBoxMin() const {
    if (m_vertices.empty()) {
        return Point3D(0, 0, 0);
    }
    
    Point3D min = m_vertices[0].position;
    for (const auto& vertex : m_vertices) {
        min.x = std::min(min.x, vertex.position.x);
        min.y = std::min(min.y, vertex.position.y);
        min.z = std::min(min.z, vertex.position.z);
    }
    return min;
}

Point3D MeshObject::getBoundingBoxMax() const {
    if (m_vertices.empty()) {
        return Point3D(0, 0, 0);
    }
    
    Point3D max = m_vertices[0].position;
    for (const auto& vertex : m_vertices) {
        max.x = std::max(max.x, vertex.position.x);
        max.y = std::max(max.y, vertex.position.y);
        max.z = std::max(max.z, vertex.position.z);
    }
    return max;
}

void MeshObject::buildTopology() {
    m_edges.clear();
    
    // Build edges from faces
    int edgeId = 0;
    for (const auto& face : m_faces) {
        for (size_t i = 0; i < face.vertices.size(); ++i) {
            int v1 = face.vertices[i];
            int v2 = face.vertices[(i + 1) % face.vertices.size()];
            
            // Check if edge already exists
            bool exists = false;
            for (auto& edge : m_edges) {
                if ((edge.vertex1 == v1 && edge.vertex2 == v2) ||
                    (edge.vertex1 == v2 && edge.vertex2 == v1)) {
                    edge.adjacentFaces.push_back(face.id);
                    exists = true;
                    break;
                }
            }
            
            if (!exists) {
                Edge edge(v1, v2, edgeId++);
                edge.adjacentFaces.push_back(face.id);
                m_edges.push_back(edge);
            }
        }
    }
}

void MeshObject::updateNormals() {
    // Calculate vertex normals from adjacent face normals
    for (auto& vertex : m_vertices) {
        vertex.normal = Vector3D(0, 0, 0);
        int faceCount = 0;
        
        for (const auto& face : m_faces) {
            for (int vertexId : face.vertices) {
                if (vertexId == vertex.id) {
                    vertex.normal.x += face.normal.x;
                    vertex.normal.y += face.normal.y;
                    vertex.normal.z += face.normal.z;
                    faceCount++;
                    break;
                }
            }
        }
        
        if (faceCount > 0) {
            vertex.normal.x /= faceCount;
            vertex.normal.y /= faceCount;
            vertex.normal.z /= faceCount;
            
            // Normalize
            float length = sqrt(vertex.normal.x * vertex.normal.x + 
                              vertex.normal.y * vertex.normal.y + 
                              vertex.normal.z * vertex.normal.z);
            if (length > 0.0f) {
                vertex.normal.x /= length;
                vertex.normal.y /= length;
                vertex.normal.z /= length;
            }
        }
    }
}

// MeshManager implementation
MeshManager::MeshManager() : m_selectionMode(SelectionMode::VERTEX), m_activeTool(MeshTool::SELECT) {
}

MeshManager::~MeshManager() {
}

std::shared_ptr<MeshObject> MeshManager::createMesh(const std::string& name) {
    return std::make_shared<MeshObject>(name);
}

std::shared_ptr<MeshObject> MeshManager::createPrimitiveMesh(ObjectType type, const std::string& name) {
    auto mesh = std::make_shared<MeshObject>(name);
    
    // Create basic primitive meshes
    switch (type) {
        case ObjectType::PRIMITIVE_BOX: {
            std::vector<Point3D> vertices = {
                Point3D(-0.5, -0.5, -0.5), Point3D(0.5, -0.5, -0.5),
                Point3D(0.5, 0.5, -0.5), Point3D(-0.5, 0.5, -0.5),
                Point3D(-0.5, -0.5, 0.5), Point3D(0.5, -0.5, 0.5),
                Point3D(0.5, 0.5, 0.5), Point3D(-0.5, 0.5, 0.5)
            };
            
            std::vector<Face> faces = {
                Face{{0, 1, 2, 3}}, Face{{4, 7, 6, 5}},
                Face{{0, 4, 5, 1}}, Face{{2, 6, 7, 3}},
                Face{{0, 3, 7, 4}}, Face{{1, 5, 6, 2}}
            };
            
            mesh->createFromGeometry(vertices, faces);
            break;
        }
        default:
            break;
    }
    
    return mesh;
}

std::shared_ptr<MeshObject> MeshManager::convertToMesh(CADObjectPtr cadObject) {
    if (!cadObject) return nullptr;
    
    auto mesh = std::make_shared<MeshObject>(cadObject->getName() + "_mesh");
    // Conversion logic would go here
    return mesh;
}

bool MeshManager::extrudeFaces(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& faceIds, 
                             const Vector3D& direction, float distance) {
    // Placeholder implementation
    return false;
}

bool MeshManager::insetFaces(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& faceIds, 
                           float insetAmount) {
    // Placeholder implementation
    return false;
}

bool MeshManager::subdivideEdges(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& edgeIds) {
    // Placeholder implementation
    return false;
}

bool MeshManager::subdivideSelected(std::shared_ptr<MeshObject> mesh) {
    // Placeholder implementation
    return false;
}

bool MeshManager::mergeVertices(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& vertexIds) {
    // Placeholder implementation
    return false;
}

bool MeshManager::dissolveVertices(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& vertexIds) {
    // Placeholder implementation
    return false;
}

bool MeshManager::extrudeEdges(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& edgeIds, 
                             const Vector3D& direction, float distance) {
    // Placeholder implementation
    return false;
}

bool MeshManager::bridgeEdgeLoops(std::shared_ptr<MeshObject> mesh, const std::unordered_set<int>& edgeIds1,
                                const std::unordered_set<int>& edgeIds2) {
    // Placeholder implementation
    return false;
}

bool MeshManager::knifeProject(std::shared_ptr<MeshObject> mesh, const Point3D& start, const Point3D& end) {
    // Placeholder implementation
    return false;
}

bool MeshManager::smoothMesh(std::shared_ptr<MeshObject> mesh, int iterations, float factor) {
    // Placeholder implementation
    return false;
}

bool MeshManager::decimateMesh(std::shared_ptr<MeshObject> mesh, float ratio) {
    // Placeholder implementation
    return false;
}

bool MeshManager::applySubdivisionSurface(std::shared_ptr<MeshObject> mesh, int levels) {
    // Placeholder implementation
    return false;
}

std::shared_ptr<MeshObject> MeshManager::booleanUnion(std::shared_ptr<MeshObject> meshA, 
                                                    std::shared_ptr<MeshObject> meshB) {
    // Placeholder implementation
    return nullptr;
}

std::shared_ptr<MeshObject> MeshManager::booleanDifference(std::shared_ptr<MeshObject> meshA, 
                                                         std::shared_ptr<MeshObject> meshB) {
    // Placeholder implementation
    return nullptr;
}

std::shared_ptr<MeshObject> MeshManager::booleanIntersection(std::shared_ptr<MeshObject> meshA, 
                                                           std::shared_ptr<MeshObject> meshB) {
    // Placeholder implementation
    return nullptr;
}

bool MeshManager::selectByRay(std::shared_ptr<MeshObject> mesh, const Point3D& rayOrigin, 
                            const Vector3D& rayDirection) {
    // Placeholder implementation
    return false;
}

void MeshManager::clearSelection(std::shared_ptr<MeshObject> mesh) {
    if (mesh) {
        mesh->deselectAll();
    }
}

void MeshManager::invertSelection(std::shared_ptr<MeshObject> mesh) {
    // Placeholder implementation
}

void MeshManager::selectAll(std::shared_ptr<MeshObject> mesh) {
    if (!mesh) return;
    
    switch (m_selectionMode) {
        case SelectionMode::VERTEX:
            for (int i = 0; i < mesh->getVertices().size(); ++i) {
                mesh->selectVertex(i, true);
            }
            break;
        case SelectionMode::EDGE:
            for (int i = 0; i < mesh->getEdges().size(); ++i) {
                mesh->selectEdge(i, true);
            }
            break;
        case SelectionMode::FACE:
            for (int i = 0; i < mesh->getFaces().size(); ++i) {
                mesh->selectFace(i, true);
            }
            break;
        default:
            break;
    }
}

bool MeshManager::importOBJ(const std::string& filename, std::shared_ptr<MeshObject> mesh) {
    // Placeholder implementation
    return false;
}

bool MeshManager::exportOBJ(const std::string& filename, std::shared_ptr<MeshObject> mesh) {
    // Placeholder implementation
    return false;
}

bool MeshManager::importSTL(const std::string& filename, std::shared_ptr<MeshObject> mesh) {
    // Placeholder implementation
    return false;
}

bool MeshManager::exportSTL(const std::string& filename, std::shared_ptr<MeshObject> mesh) {
    // Placeholder implementation
    return false;
}

void MeshManager::calculateFaceNormal(MeshObject* mesh, int faceId) {
    // Placeholder implementation
}

void MeshManager::calculateVertexNormal(MeshObject* mesh, int vertexId) {
    // Placeholder implementation
}

std::vector<int> MeshManager::getAdjacentVertices(const MeshObject* mesh, int vertexId) {
    // Placeholder implementation
    return {};
}

std::vector<int> MeshManager::getAdjacentFaces(const MeshObject* mesh, int vertexId) {
    // Placeholder implementation
    return {};
}

bool MeshManager::isEdgeManifold(const MeshObject* mesh, int edgeId) {
    // Placeholder implementation
    return false;
}

void MeshManager::catmullClarkSubdivision(MeshObject* mesh) {
    // Placeholder implementation
}

void MeshManager::loopSubdivision(MeshObject* mesh) {
    // Placeholder implementation
}

bool MeshManager::isMeshManifold(const MeshObject* mesh) {
    // Placeholder implementation
    return false;
}

void MeshManager::findBoundaryEdges(const MeshObject* mesh, std::vector<int>& boundaryEdges) {
    // Placeholder implementation
}

void MeshManager::findNonManifoldEdges(const MeshObject* mesh, std::vector<int>& nonManifoldEdges) {
    // Placeholder implementation
}

} // namespace HybridCAD 